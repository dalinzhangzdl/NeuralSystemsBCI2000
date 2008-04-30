////////////////////////////////////////////////////////////////////////////////
// $Id$
// Author: schalk@wadsworth.org
// Description: BCI2000 Source Module for gUSBamp devices.
//
// (C) 2000-2008, BCI2000 Project
// http://www.bci2000.org
////////////////////////////////////////////////////////////////////////////////
#ifndef GUSBAMP_ADC_H
#define GUSBAMP_ADC_H

#include "GenericADC.h"
#include "GenericVisualization.h"

// Number of buffers for g.USBamp acquisition
//#define NUM_BUFS        1

#include <windows.h>
#include <vector>
#include <string>
#include <algorithm>
#include <Classes.hpp>
#include <SyncObjs.hpp>
#include "PrecisionTime.h"

class gUSBampADC : public GenericADC
{
 public:
			   gUSBampADC();
	virtual      ~gUSBampADC();

	virtual void Preflight( const SignalProperties&, SignalProperties& ) const;
	virtual void Initialize( const SignalProperties&, const SignalProperties& );
	virtual void Process( const GenericSignal&, GenericSignal& );
	virtual void Halt();

protected:
	OVERLAPPED     m_ov[20];
	HANDLE         m_hEvent[20];
	std::vector<std::string> m_DeviceIDs;
	std::vector<HANDLE>      m_hdev;
	//std::vector<BYTE *>      m_pBuffer[20];
    BYTE **mBuffer;
	std::vector<int>         m_buffersize;
	std::vector<int>         m_iBytesperScan;
	std::vector<int>         m_numchans;
	std::vector<float>       m_LSB; // how many microVolts is one A/D unit (=SourceChGain)
	int            DetectAutoMode() const;
	int            DetermineFilterNumber() const;
	int            DetermineNotchNumber() const;
	GenericVisualization mVis;
	int            m_numdevices;
	float          m_filterhighpass, m_filterlowpass, m_notchhighpass, m_notchlowpass;   // at the moment, only one filter setting for all channels and all devices
	int            m_filtermodelorder, m_filtertype, m_notchmodelorder, m_notchtype;
	std::string    mMasterDeviceID;  // device ID for the master device (exactly one device has to be master)
	int            m_timeoutms;
	int				mSampleBlockSize;
	bool           mFloatOutput;
	bool           m_digitalinput;
	bool 			 m_digitalOutput;
	int            m_acqmode;         // normal, calibrate, or impedance
	bool 			 m_digitalOut1;
	int            current_buffer;
    int mTotalChs;
	int NUM_BUFS, mBufferSize;
	float *mData;
	int *mDataInt;
	int mBufferReadPos, mBufferWritePos;

	class AcquireThread;
	TEvent *acquireEventRead;
	friend class AcquireThread;
	class AcquireThread : public TThread
	{
		public:
		AcquireThread( gUSBampADC * parent )
		: TThread( true ), amp( parent )
		{
			prevT = 0;
			for (int i = 0; i < 100; i++) tDiffs[i] = 0;
			curPos = 0;
            SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
		}
		private:
			virtual void __fastcall Execute();
			gUSBampADC *amp;
			int tDiffs[100], prevT;
			int curPos;
	} *mpAcquireThread;

};

#endif // GUSBAMP_ADC_H




