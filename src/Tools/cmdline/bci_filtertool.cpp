////////////////////////////////////////////////////////////////////
// File:    bci_filtertool.cpp
// Date:    Jul 18, 2003
// Author:  juergen.mellinger@uni-tuebingen.de
// Description: A BCI2000 filter wrapper that reads a BCI2000
//          compliant binary stream from standard input, applies
//          a BCI2000 filter, and writes its output to the
//          standard output as a BCI2000 compliant binary stream.
////////////////////////////////////////////////////////////////////
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "bci_tool.h"
#include "shared/defines.h"
#include "shared/UParameter.h"
#include "shared/UState.h"
#include "shared/UGenericVisualization.h"
#include "shared/UGenericFilter.h"
#include "shared/MessageHandler.h"

#define FILTER_NAME "$FILTER$"

using namespace std;

string ToolInfo[] =
{
  "",
  "tool, framework version 0.1.0, compiled "__DATE__,
  "Process standard input with the \"" FILTER_NAME "\" BCI2000 filter.",
  "Reads a BCI2000 compliant binary stream from standard input, "
    "applies the " FILTER_NAME ", "
    "and writes its output to the standard "
    "output as a BCI2000 compliant binary stream.",
  "-o<file>, --operator<file>\tdirect visualization messages to <file>",
  "                            \tinstead of /dev/null",
  ""
};

class FilterWrapper : public MessageHandler
{
 public:
  FilterWrapper( ostream& arOut )
  : mrOut( arOut ), mpStatevector( NULL ), mpFilter( NULL ) {}
  ~FilterWrapper() { delete mpStatevector; delete mpFilter; }
  void RedirectOperator( string file ) { mOperator.open( file.c_str() ); }
  static const char* FilterName();

 private:
  ostream& mrOut;
  ofstream mOperator;
  GenericSignal mOutputSignal;
  PARAMLIST mParamlist;
  STATELIST mStatelist;
  STATEVECTOR* mpStatevector;
  GenericFilter* mpFilter;

  virtual bool HandlePARAM(       std::istream& );
  virtual bool HandleSTATE(       std::istream& );
  virtual bool HandleVisSignal(   std::istream& );
  virtual bool HandleSTATEVECTOR( std::istream& );
};

ToolResult
ToolInit()
{
  const char* pFilterName = FilterWrapper::FilterName();
  if( pFilterName == NULL )
    return genericError;
  for( int i = 0; ToolInfo[ i ] != ""; ++i )
  {
    size_t namePos;
    while( ( namePos = ToolInfo[ i ].find( FILTER_NAME ) ) != string::npos )
      ToolInfo[ i ].replace( namePos, string( FILTER_NAME ).length(), pFilterName );
  }
  return noError;
}

ToolResult
ToolMain( const OptionSet& arOptions, istream& arIn, ostream& arOut )
{
  FilterWrapper wrapper( arOut );
  if( arOptions.size() == 1 )
  {
    string operatorFile = arOptions.getopt( "-o|-O|--operator", "" );
    if( operatorFile == "" )
      return illegalInput;
    wrapper.RedirectOperator( operatorFile.c_str() );
  }
  while( arIn && arIn.peek() != EOF )
    wrapper.HandleMessage( arIn );
  if( !arIn )
    return illegalInput;
  return noError;
}

const char*
FilterWrapper::FilterName()
{
  const char* pName = NULL;
  PARAMLIST paramlist;
  STATELIST statelist;
  Environment::EnterConstructionPhase( &paramlist, &statelist, NULL, NULL );
  GenericFilter::InstantiateFilters();
  Environment::EnterNonaccessPhase();
  GenericFilter* pFilter = GenericFilter::GetFilter<GenericFilter>();
  if( pFilter == NULL )
    cerr << "Could not create filter instance.\n"
         << "Make sure there is a filter definition with a "
         << "\"RegisterFilter\" statement linked into the executable."
         << endl;
  else
    pName = typeid( *pFilter ).name();
  delete pFilter;
  return pName;
}

bool
FilterWrapper::HandlePARAM( istream& arIn )
{
  PARAM p;
  p.ReadBinary( arIn );
  if( arIn )
    mParamlist[ p.GetName() ] = p;
  return true;
}

bool
FilterWrapper::HandleSTATE( istream& arIn )
{
  if( mpStatevector != NULL )
  {
    if( mpFilter )
      mpFilter->StopRun();
    delete mpStatevector;
    mpStatevector = NULL;
  }
  STATE s;
  s.ReadBinary( arIn );
  if( arIn )
    mStatelist.AddState2List( &s );
  return true;
}

bool
FilterWrapper::HandleSTATEVECTOR( istream& arIn )
{
  if( mpStatevector == NULL )
  {
    mpStatevector = new STATEVECTOR( &mStatelist, true );
    if( mpFilter )
      mpFilter->StartRun();
  }
  bool wasRunning = mpStatevector->GetStateValue( "Running" );
  mpStatevector->ReadBinary( arIn );
  bool isRunning = mpStatevector->GetStateValue( "Running" );
  return true;
}

bool
FilterWrapper::HandleVisSignal( istream& arIn )
{
  VisSignal s;
  if( s.ReadBinary( arIn ) && s.GetSourceID() == 0 )
  {
    const GenericSignal& inputSignal = s;
    SignalProperties outputProperties;
    switch( Environment::GetPhase() )
    {
      case Environment::nonaccess:
        {
          PARAMLIST filterParams;
          STATELIST filterStates;
          Environment::EnterConstructionPhase( &filterParams, &filterStates, NULL, &mOperator );
          GenericFilter::InstantiateFilters();
          mpFilter = GenericFilter::GetFilter<GenericFilter>();
          if( __bcierr.flushes() > 0 )
            break;
          // Add the filter's parameters with their default values to the parameter
          // list as far as they are missing from the input stream.
          for( PARAMLIST::iterator i = filterParams.begin(); i != filterParams.end(); ++i )
            if( mParamlist.find( i->second.GetName() ) == mParamlist.end() )
              mParamlist[ i->second.GetName() ] = i->second;
          // If there are no states in the input stream, use the filter's states.
          if( mStatelist.GetNumStates() == 0 )
            mStatelist = filterStates;
        }
        /* no break */
      case Environment::construction:
        if( mpStatevector == NULL )
          mpStatevector = new STATEVECTOR( &mStatelist, true );
        for( int i = 0; i < mStatelist.GetNumStates(); ++i )
          PutMessage( mrOut, *mStatelist.GetStatePtr( i ) );
        Environment::EnterPreflightPhase( &mParamlist, &mStatelist, mpStatevector, &mOperator );
        mpFilter->Preflight( inputSignal, outputProperties );
        mOutputSignal.SetProperties( outputProperties );
        if( __bcierr.flushes() > 0 )
          break;
        /* no break */
      case Environment::preflight:
        Environment::EnterInitializationPhase( &mParamlist, &mStatelist, mpStatevector, &mOperator );
        mpFilter->Initialize();
        if( __bcierr.flushes() > 0 )
          break;
        /* no break */
      case Environment::initialization:
        // Write Parameters after initialization.
        for( PARAMLIST::const_iterator i = mParamlist.begin();
                                       i != mParamlist.end(); ++i )
          PutMessage( mrOut, i->second );
        Environment::EnterProcessingPhase( &mParamlist, &mStatelist, mpStatevector, &mOperator );
        /* no break */
      case Environment::processing:
        {
          bool wasRunning = mpStatevector->GetStateValue( "Running" );
          mpFilter->Process( &inputSignal, &mOutputSignal );
          if( __bcierr.flushes() > 0 )
            break;
          bool isRunning = mpStatevector->GetStateValue( "Running" );
          if( isRunning || wasRunning )
            if( mpStatevector->GetStateVectorLength() > 0 )
              PutMessage( mrOut, *mpStatevector );
          if( isRunning )
            PutMessage( mrOut, mOutputSignal );
        }
        break;
      default:
        bcierr << "Unknown Environment phase" << endl;
        arIn.setstate( ios::failbit );
    }
  }
  return arIn;
}

