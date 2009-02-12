////////////////////////////////////////////////////////////////////////////////
// $Id$
// Description: A finite impulse response (FIR) filter for temporal filtering.
//
// (C) 2000-2009, BCI2000 Project
// http://www.bci2000.org
////////////////////////////////////////////////////////////////////////////////
#include "PCHIncludes.h"
#pragma hdrstop

#include "FIRFilter.h"
#include <algorithm>

using namespace std;

RegisterFilter( FIRFilter, 2.C );

FIRFilter::FIRFilter()
: mFIRIntegration( none )
{
  BEGIN_PARAMETER_DEFINITIONS
    "Filtering int FIRIntegration= 1 0 0 3 "
      "// FIR Integration: 0 none, 1 mean, 2 rms, 3 max (enumeration)",
    "Filtering matrix FIRCoefficients= 4 4 "
      " 1 0 0 0"
      " 1 0 0 0"
      " 1 0 0 0"
      " 1 0 0 0"
        " 1 % % // FIR Filter Coefficients (rows correspond to channels; empty matrix for off)",
  END_PARAMETER_DEFINITIONS
}


FIRFilter::~FIRFilter()
{
}


void
FIRFilter::Preflight( const SignalProperties& Input,
                            SignalProperties& Output ) const
{
  PreflightCondition( Parameter( "FIRCoefficients" )->NumRows() <= Input.Channels() );

  // Output signal dimensions.
  Output = Input;
  if( Parameter( "FIRCoefficients" )->NumValues() > 0 )
  {
    switch( int( Parameter( "FIRIntegration" ) ) )
    {
      case none:
        break;

      case mean:
      case rms:
      case max:
      { // Reduce output to a single element per block.
        Output.SetElements( 1 );
        // Adapt element unit to obtain correct time axis.
        float gain = Output.ElementUnit().Gain();
        Output.ElementUnit().SetGain( gain * Input.Elements() );
      } break;

      default:
        bcierr << "Unknown value of FIRIntegration parameter" << endl;
    }
  }
}


void
FIRFilter::Initialize( const SignalProperties& Input, const SignalProperties& Output )
{
  mFilter.clear();
  mBuffer.clear();

  mFIRIntegration = Parameter( "FIRIntegration" );

  ParamRef FIRCoefficients = Parameter( "FIRCoefficients" );
  int numChannels = FIRCoefficients->NumRows(),
      filterLength = FIRCoefficients->NumColumns();
  mFilter.resize( numChannels, DataVector( 0.0, filterLength ) );
  for( int channel = 0; channel < numChannels; ++channel )
    for( int sample = 0; sample < filterLength; ++sample )
      mFilter[channel][sample] = FIRCoefficients( channel, sample );

  int bufferLength = filterLength + Input.Elements() - 1;
  mBuffer.resize( numChannels, DataVector( 0.0, bufferLength ) );
}


void
FIRFilter::Process( const GenericSignal& Input, GenericSignal& Output )
{
  if( mBuffer.empty() )
    Output = Input;
  else for( size_t channel = 0; channel < mBuffer.size(); ++channel )
  {
    int bufferLength = mBuffer[channel].size(),
        filterLength = mFilter[channel].size(),
        inputLength = Input.Elements();
    // Move buffer content towards the buffer's begin.
    for( int sample = 0; sample < bufferLength - inputLength; ++sample )
      mBuffer[channel][sample] = mBuffer[channel][sample + inputLength];
    // Copy current input into the buffer's end.
    for( int sample = 0; sample < inputLength; ++sample )
      mBuffer[channel][bufferLength - inputLength + sample] = Input( channel, sample );
    // Compute buffer's convolution with coefficient vector.
    DataVector result( 0.0, inputLength );
    for( int sample = 0; sample < inputLength; ++sample )
      result[sample] = inner_product( &mFilter[channel][0], &mFilter[channel][filterLength-1], &mBuffer[channel][sample], 0 );
    // Compute output.
    switch( mFIRIntegration )
    {
      case none:
        for( int sample = 0; sample < inputLength; ++sample )
          Output( channel, sample ) = result[sample];
        break;

      case mean:
        Output( channel, 0 ) = result.sum() / inputLength;
        break;

      case rms:
        result *= result;
        Output( channel, 0 ) = ::sqrt( result.sum() / inputLength );
        break;

      case max:
        Output( channel, 0 ) = result.max();
        break;

      default:
        bcierr << "Unknown value of FIRIntegration parameter" << endl;
    }
  }
}

