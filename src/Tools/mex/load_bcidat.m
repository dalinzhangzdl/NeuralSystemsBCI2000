[ signal, states, parameters ] = load_bcidat( varargin )
%LOAD_BCIDAT Load a BCI2000 data file into Matlab workspace variables.
%
%  [ signal, states, parameters ] = load_bcidat( 'filename1', 'filename2', ... )
%
%  loads signal, state, and parameter data from the files whose names are given
%  as function arguments.
%
%  Examples for loading multiple files:
%    files = dir( '*.dat' );
%    [ signal, states, parameters ] = load_bcidat( files.name );
%
%    files = struct( 'name', uigetfile( 'MultiSelect', 'on' ) );
%    [ signal, states, parameters ] = load_bcidat( files.name );
%
%
%  For multiple files, number of channels, states, and signal type must be
%  consistent.
%
%  Signal data will be raw (now calibrated), and will be represented by the 
%  smallest Matlab data type that accomodates them.
%
%  The 'states' output variable will be a Matlab struct with BCI2000 state
%  names as struct member names, and the number of state value entries matching
%  the first dimension of the 'signal' output variable.
%
%  The 'parameters' output variable will be a Matlab struct with BCI2000
%  parameter names as struct member names.
%  Individual parameters are represented as cell arrays of strings, and may
%  be converted into numeric matrices by Matlab's str2double function.
%  If multiple files are given, parameter values will match the first files'
%  parameters.
%
%  The load_bcidat function is part of the BCI2000 project 
%  (http://www.bciresearch.org).

%  This is a help file documenting the functionality contained in
%  load_bcimat.mex.
%  $Id$
%  $Log$
%  Revision 1.3  2006/02/07 13:23:32  mellinger
%  Improved documentation regarding multiple files.
%
%  Revision 1.2  2006/01/18 20:21:24  mellinger
%  Allowed for multiple input files.
%
%  Revision 1.1  2006/01/17 17:15:47  mellinger
%  Initial version.
%
error( 'There is no load_bcidat mex file for your platform available.' );
