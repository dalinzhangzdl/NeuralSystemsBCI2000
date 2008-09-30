#   $Id$
#   
#   This file is part of the BCPy2000 framework, a Python framework for
#   implementing modules that run on top of the BCI2000 <http://bci2000.org/>
#   platform, for the purpose of realtime biosignal processing.
# 
#   Copyright (C) 2007-8  Thomas Schreiner, Jeremy Hill
#                         Christian Puzicha, Jason Farquhar
#   
#   bcpy2000@bci2000.org
#   
#   The BCPy2000 framework is free software: you can redistribute it
#   and/or modify it under the terms of the GNU General Public License
#   as published by the Free Software Foundation, either version 3 of
#   the License, or (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
__all__ = ['bcistream']

import os
import struct
import numpy

class DatFileError(Exception): pass

class bcistream(object):
	
	def __init__(self, filename):
		self.filename        = filename
		self.headerlen       = 0
		self.stateveclen     = 0
		self.nchan           = 0
		self.bytesperchannel = 0
		self.bytesperframe   = 0
		self.framefmt        = ''
		self.unpacksig       = ''
		self.unpackstates    = ''
		self.paramdefs       = {}
		self.statedefs       = {}
		self.samplingfreq_hz = 0
		self.gains           = None
		self.offsets         = None
		self.params          = {}
		
		self.file = open(self.filename, 'r')
		self.readHeader()
		self.file.close()
				
		self.bytesperframe = self.nchan * self.bytesperchannel + self.stateveclen

		self.gains = self.params.get('SourceChGain')
		if self.gains != None:
			self.gains = numpy.array(map(float,self.gains))
			self.gains.shape = (self.nchan,1)
		self.offsets = self.params.get('SourceChOffset')
		if self.offsets != None:
			self.offsets = numpy.array(map(float,self.offsets))
			self.offsets.shape = (self.nchan,1)
		
		for k,v in self.statedefs.items():
			a = numpy.zeros((self.stateveclen,1), dtype=numpy.uint16)
			startbyte = int(v['bytePos'])
			startbit  = int(v['bitPos'])
			nbits     = int(v['length'])
			nbytes    = (startbit+nbits)/8 
			if (startbit+nbits)%8: nbytes += 1
			extrabits = nbytes*8 - nbits - startbit;
			startmask = 255 & (255 << startbit)
			endmask   = 255 & (255 >> extrabits)
			div       = (1 << startbit);
			v['slice'] = slice(startbyte, startbyte+nbytes)
			v['mask'] = numpy.array([255]*nbytes, dtype=numpy.uint8)
			v['mask'][0]  &= startmask
			v['mask'][-1] &= endmask
			v['mask'].shape = (nbytes,1)
			v['mult'] = numpy.asmatrix(256.0 ** numpy.arange(nbytes, dtype=numpy.float64) / float(div))
			self.statedefs[k] = v

		self.open()
		
	def open(self):
		if self.file.closed:
			self.file = open(self.filename, 'rb')
		self.file.seek(self.headerlen)
	
	def close(self):
		if not self.file.closed:
			self.file.close()
		
	def __str__(self):
		nsamp = self.samples()
		s = ["<%s.%s instance at 0x%08X>" % (self.__class__.__module__,self.__class__.__name__,id(self))]
		s.append('file ' + self.filename)
		s.append('recorded ' + self.params['StorageTime'])
		s.append('%d samples @ %gHz = %s' % (nsamp, self.samplingfreq_hz, self.sample2time(nsamp),) )
		s.append('%d channels, total %.3g MB' % (self.nchan, self.datasize()/1024.0**2,) )
		if not self.file.closed:
			s.append('open for reading at sample %d  (%s)' % (self.tell(), self.sample2time(self.tell()),) )			
		return '\n    '.join(s)
		
	def __repr__(self):
		return self.__str__()
	
	def channels(self):
		return self.nchan

	def samplingrate(self):
		return self.samplingfreq_hz

	def datasize(self):
		return os.stat(self.filename)[6] - self.headerlen
		
	def samples(self):
		return self.datasize() / self.bytesperframe

		
	def readHeader(self):
		line = self.file.readline().split()
		k = map(lambda x:x.rstrip('='), line[::2])
		v = line[1::2]
		self.headline = dict(zip(k,v))
		self.headerlen = int(self.headline['HeaderLen'])
		self.nchan = int(self.headline['SourceCh'])
		self.stateveclen = int(self.headline['StatevectorLen'])
		fmtstr = self.headline.get('DataFormat','int16')
		fmt = {'int16':'h', 'int32':'l', 'float32':'f'}.get(fmtstr)
		if fmt == None: raise DatFileError, 'unrecognized DataFormat "%s"' % fmtstr
		self.bytesperchannel = struct.calcsize(fmt)
		self.framefmt     = fmt * self.nchan + 'B' * self.stateveclen
		self.unpacksig    = fmt * self.nchan + 'x' * self.stateveclen
		self.unpackstates = 'x' * self.bytesperchannel * self.nchan + 'B' * self.stateveclen
		
		line = self.file.readline()
		if line.strip() != '[ State Vector Definition ]':
			raise DatFileError, 'failed to find state vector definition section where expected'
		while True:
			line = self.file.readline()
			if len(line) == 0 or line[0] == '[': break
			self.initState(line)
		
		if line.strip() != '[ Parameter Definition ]':
			raise DatFileError, 'failed to find parameter definition section where expected'
			
		while True:
			line = self.file.readline()
			if self.file.tell() >= self.headerlen: break
			self.initParam(line)

		self.samplingfreq_hz = float(str(self.params['SamplingRate']).rstrip('Hz'))

		
	def initState(self, state):
		state = state.split()
		self.statedefs[state[0]] = {
			'length'  : int(state[1]),
			'startVal': int(state[2]),
			'bytePos' : int(state[3]),
			'bitPos'  : int(state[4])
		}
			
	def initParam(self, param):
		param = param.split('//', 1)
		comment = ''
		if len(param) > 1:
			comment = param[1]
			
		param = map(unescape, param[0].split())
		category = param.pop(0).split(':')
		category += [''] * (3-len(category))
		datatype = param.pop(0)
		name = param.pop(0).rstrip('=')
		self.paramdefs[name] = rec = {
			'name':name, 'comment':comment, 'category':category, 'type':datatype,
			'defaultVal':'', 'minVal':'', 'maxVal':'',
		}

		scaled = None
		if datatype in ('int', 'float'):
			datatype = {'float':float, 'int':int}.get(datatype)
			val = param[0]
			unscaled,units,scaled = decode_units(val, datatype)
			rec.update({
				'val'        : unscaled,
				'units'      : units,
			})

		elif 'list' in datatype:
			listtype = datatype.replace('list','')
			listtype = {'float':float, 'int':int, '':str}.get(listtype, listtype)
			if isinstance(listtype,str):
				raise DatFileError, 'Unknown list type "%s"' % listtype			
			numel = int(param.pop(0))
			val = param[:numel]
			if listtype==str:
				unscaled = val
				units = [''] * len(val)
			else:
				val = map(lambda x:decode_units(x,listtype), val)
				unscaled,units,scaled = map(lambda x:x[0],val),map(lambda x:x[1],val),map(lambda x:x[2],val)
			rec.update({
				'valtype'    : listtype,
				'len'        : numel,
				'val'        : unscaled,
				'units'      : units,
			})
		
		elif datatype in ('string', 'variant'):
			val = param.pop(0)
			rec.update({
				'val'        : val,
			})
			
		elif datatype == 'matrix':
			nrows,rowlabels = parsedim(param)
			ncols,collabels = parsedim(param)
			val = []
			for i in range(nrows):
				val.append([])
				for j in range(ncols):
					val[-1].append(param.pop(0))
			rec.update({
				'val'        : val,
				'shape'      : (nrows,ncols),
				'dimlabels'  : (rowlabels,collabels),
			})
					
		else:
			print "unsupported parameter type",datatype
			rec.update({
				'val'        : param,
			})

		param.reverse()
		if len(param): rec['maxVal'] = param.pop(0)
		if len(param): rec['minVal'] = param.pop(0)
		if len(param): rec['defaultVal'] = param.pop(0)

		if scaled == None:
			self.params[name] = rec['val']
		else:
			self.params[name] = scaled
			
	
	def read(self, nsamp=1):
		if nsamp==-1:
			nsamp = self.samples() - self.tell()
		if nsamp=='all':
			self.rewind(); nsamp = self.samples()
		if isinstance(nsamp, str):
			nsamp = self.time2sample(nsamp)
		raw = self.file.read(self.bytesperframe*nsamp)
		nsamp = len(raw) / self.bytesperframe
		sig = numpy.zeros((self.nchan,nsamp),dtype=numpy.float32)
		fmt = '<' + self.unpacksig * nsamp
		sig.T.flat = struct.unpack(fmt, raw)
		rawstates = numpy.zeros((self.stateveclen,nsamp), dtype=numpy.uint8)
		fmt = '<' + self.unpackstates * nsamp
		rawstates.T.flat = struct.unpack(fmt, raw)
		if self.gains != None:
			sig = sig * self.gains
		if self.offsets != None:
			sig = sig + self.offsets
		sig = numpy.asmatrix(sig)
		return sig,rawstates
	
	def decode(self, nsamp=1, states='all'):
		sig,rawstates = self.read(nsamp)
		states,statenames = {},states
		if statenames == 'all':
			statenames = self.statedefs.keys()
		for statename in statenames:
			sd = self.statedefs[statename]
			states[statename] = numpy.array(sd['mult']*numpy.asmatrix(rawstates[sd['slice'],:] & sd['mask']), dtype=numpy.int32)
		return sig,states
				
	def tell(self):
		if self.file.closed: raise IOError, 'dat file is closed'
		return (self.file.tell() - self.headerlen) / self.bytesperframe
		
	def seek(self, value, wrt='bof'):
		if self.file.closed: raise IOError, 'dat file is closed'
		if isinstance(value, str):
			value = self.time2sample(value)
		
		if wrt in ('bof',-1):
			wrt = 0
		elif wrt in ('eof',+1):
			wrt = self.samples()
		elif wrt in ('cof',0):
			wrt = self.tell()
		else:
			raise IOError, 'unknown origin "%s"' % str(wrt)
		
		value = min(self.samples(), max(0, value + wrt))
		self.file.seek(value * self.bytesperframe + self.headerlen)
				
	def rewind(self):
		self.file.seek(self.headerlen)
		
	def time2sample(self, value):
		t = value.split(':')
		if len(t) > 3:
			raise DatFileError, 'too many colons in timestamp "%s"' % value
		t.reverse()
		t = map(float,t) + [0]*(3-len(t))
		t = t[0] + 60.0 * t[1] + 3600.0 * t[2]
		return int(round(t * self.samplingfreq_hz))
		
	def sample2time(self, value):
		msecs = round(1000.0 * float(value) / self.samplingfreq_hz)
		secs,msecs = divmod(int(msecs), 1000)
		mins,secs  = divmod(int(secs), 60)
		hours,mins  = divmod(int(mins), 60)
		return '%02d:%02d:%02d.%03d' % (hours,mins,secs,msecs)

	def plotstates(self, states): # TODO:  choose which states to plot
		labels = states.keys()
		v = numpy.matrix(numpy.concatenate(states.values(), axis=0), dtype=numpy.float32)
		ntraces,nsamp = v.shape
		#v = v - numpy.min(v,1)
		sc = numpy.max(v,1)
		sc[numpy.where(sc==0.0)] = 1.0
		v = v / sc
		offsets = numpy.asmatrix(numpy.arange(1.0,ntraces+1.0))
		v = v.T * -0.7 + offsets
		t = numpy.matrix(range(nsamp), dtype=numpy.float32).T / self.samplingfreq_hz

		pylab = load_pylab()
		pylab.cla()
		ax = pylab.gca()
		h = pylab.plot(t, v)
		ax.set_xlim(0,nsamp/self.samplingfreq_hz)
		ax.set_yticks(offsets.A.flatten())
		ax.set_yticklabels(labels)
		ax.set_ylim(ntraces+1,0)
		ax.grid(True)
		pylab.draw()
		return h
	
	def plotsig(self, sig, fac=3.0): # TODO: plot subsets of channels which don't necessarily correspond to ChannelNames param		
		ntraces,nsamp = sig.shape
		labels = self.params.get('ChannelNames', '')
		if len(labels)==0: labels = map(str, range(1,ntraces+1))

		v = sig.T
		v = v - numpy.median(v)
		offsets = numpy.asmatrix(numpy.arange(-1.0,ntraces+1.0))
		offsets = offsets * max(v.A.std(axis=0)) * fac
		v = v + offsets[:,1:-1]
		
		t = numpy.matrix(range(nsamp), dtype=numpy.float32).T / self.samplingfreq_hz

		pylab = load_pylab()
		pylab.cla()
		ax = pylab.gca()
		h = pylab.plot(t, v)
		ax.set_xlim(0,nsamp/self.samplingfreq_hz)
		ax.set_yticks(offsets.A.flatten()[1:-1])
		ax.set_yticklabels(labels)
		ax.set_ylim(offsets.A.flatten()[-1],offsets.A.flatten()[0])
		ax.grid(True)
		pylab.draw()
		return h
		
def unescape(s):
	if s in ['%', '%0', '%00']:
		return ''
	out = ''
	s = list(s)
	while len(s):
		c = s.pop(0)
		if c == '%':
			out += chr(int(''.join(s[:2]),16))
			s = s[2:]
		else:
			out += c
	return out
	
def parsedim(param):
	extent = param.pop(0)
	labels = []
	if extent == '{':
		while True:
			p = param.pop(0)
			if p == '}': break
			labels.append(p)
		extent = len(labels)
	else:
		extent = int(extent)
		labels = map(str, range(1,extent+1))			
	return extent,labels

def decode_units(s, datatype=float):
	units = ''
	while len(s) and not s[-1] in '0123456789.':
		units = s[-1] + units
		s = s[:-1]
	try: unscaled = datatype(s)
	except: unscaled = float(s)
	scaled = unscaled * {
		  'hz':1, 'khz':1000, 'mhz':1000000,
		 'muv':1,  'mv':1000,   'v':1000000,
		'msec':1, 'sec':1000, 'min':60000,
		  'ms':1,   's':1000,
	}.get(units.lower(), 1)
	return unscaled,units,scaled

def load_pylab():
	try:
		import matplotlib,sys
		if not 'matplotlib.backends' in sys.modules: matplotlib.interactive(True)
		import pylab
		return pylab
	except:
		print __name__, "module failed to import pylab: plotting methods will not work"

