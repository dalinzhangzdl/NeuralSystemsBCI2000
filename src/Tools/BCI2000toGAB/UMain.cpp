//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include <stdio.h>
#include <vector>
#include <limits>

#include "UBCI2000DATA.h"
#include "UParameter.h"

#include "UMain.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "CGAUGES"
#pragma resource "*.dfm"

#define VERSION "0.5"

using namespace std;

TfMain *fMain;

// Target code conversions
static const short ud[] = { 11, 15 },
                   lr[] = { 13, 17 },
                   udlr[] = { 10, 12, 14, 16 };

template<typename From, typename To> float convert_num( const From& f, To& t )
{
  bool overflow = false;
  if( numeric_limits<To>::max() < f )
  {
    overflow = true;
    t = numeric_limits<To>::max();
  }
  else if( numeric_limits<To>::is_integer && f < numeric_limits<To>::min() )
  {
    overflow = true;
    t = numeric_limits<To>::min();
  }
  else if( !numeric_limits<To>::is_integer && f < -numeric_limits<To>::max() )
  {
    overflow = true;
    t = -numeric_limits<To>::max();
  }
  else
    t = To( f );
  float ratio = 0.0;
  if( overflow )
    ratio = float( f ) / ( float( t ) + numeric_limits<float>::epsilon() );
  return ratio;
}

void TfMain::UserMessage( const AnsiString& msg, TfMain::msgtypes type ) const
{
  bool actuallyShowIt = !mAutoMode;
  const char* msgtype = "Message";
  long msgflags = MB_OK;
  switch( type )
  {
    case Warning:
      msgflags |= MB_ICONWARNING;
      msgtype = "Warning";
      break;
    case Error:
      msgflags |= MB_ICONERROR;
      msgtype = "Error";
      actuallyShowIt = true;
      break;
  }
  if( actuallyShowIt )
    Application->MessageBox( msg.c_str(), msgtype, msgflags );
}

//---------------------------------------------------------------------------
__fastcall TfMain::TfMain(TComponent* Owner)
        : TForm(Owner),
          mpGabTargets( NULL ),
          mTargetMax( 0 ),
          mAutoMode( false )
{
  Caption = Caption + " " VERSION " ("__DATE__")";
  Constraints->MinHeight = Height;
  Constraints->MinWidth = Width;
  mSourceFiles->WordWrap = false;
  mSourceFiles->Text = "";
  mProgressLegend->Caption = "";
  eDestinationFile->Enabled = false;
  eDestinationFile->Text = "<auto>";
  
  if( Application->OnIdle == NULL )
    Application->OnIdle = DoStartupProcessing;
}
//---------------------------------------------------------------------------

void __fastcall TfMain::DoStartupProcessing( TObject*, bool& )
{
  Application->OnIdle = NULL;

  const char optionChar = '-';
  if( ParamCount() > 0 && ParamStr( 1 )[ 1 ] != optionChar )
  {
    mAutoMode = true;
    AutoscaleCheckbox->Checked = false;
    mSourceFiles->Lines->Clear();
    int i = 1;
    while( i <= ParamCount() && ParamStr( i )[ 1 ] != optionChar )
      mSourceFiles->Lines->Append( ParamStr( i++ ) );
    while( i <= ParamCount() && ParamStr( i )[ 1 ] == optionChar )
    {
      if( ParamStr( i ).Length() >= 2 )
      {
        switch( ParamStr( i )[ 2 ] )
        {
          case 'o':
          case 'O':
            if( ParamCount() > i && ParamStr( i + 1 )[ 1 ] != optionChar )
              eDestinationFile->Text = ParamStr( ++i );
            else
              eDestinationFile->Text = ParamStr( i ).c_str() + 2;
            break;
          case 'p':
          case 'P':
            ParameterFile->Enabled = true;
            if( ParamCount() > i && ParamStr( i + 1 )[ 1 ] != optionChar )
             ParameterFile->Text = ParamStr( ++i );
            else
              ParameterFile->Text = ParamStr( i ).c_str() + 2;
            break;
          case 'a':
          case 'A':
            AutoscaleCheckbox->Checked = true;
            break;
        }
      }
      ++i;
    }
    if( bConvert->Enabled )
      bConvert->Click();
    Application->Terminate();
  }
}

void __fastcall TfMain::CheckCalibrationFile( void )
{
  int i;
  int n_chans;
  PARAMLIST paramlist;

  if( ParameterFile->Enabled && ParameterFile->Text.Length() > 0 )
  {
    if( paramlist.LoadParameterList( ParameterFile->Text.c_str() ) )
    {
      n_chans= paramlist.GetParamPtr("SourceChOffset")->GetNumValues();
      mOffset.clear();
      mOffset.resize( n_chans, 0 );
      mGain.clear();
      mGain.resize( n_chans, 0 );
      for(i=0;i<n_chans;i++)
      {
        mOffset[i]=atoi(paramlist.GetParamPtr("SourceChOffset")->GetValue(i));
        mGain[i]=atof(paramlist.GetParamPtr("SourceChGain")->GetValue(i));
      }
    }
    else
      UserMessage( "Could not open parameter file \""
                   + ParameterFile->Text + "\".", Error );
  }

  BCI2000DATA data;
  data.Initialize( mSourceFiles->Lines->Strings[ 0 ].c_str(), 0 );

  const PARAM* param = paramlist.GetParamPtr( "NumberTargets" );
  if( param == NULL )
    param = data.GetParamListPtr()->GetParamPtr( "NumberTargets" );
  int numberTargets = ( param ? atoi( param->GetValue() ) : 0 );

  param = paramlist.GetParamPtr( "TargetOrientation" );
  if( param == NULL )
    param = data.GetParamListPtr()->GetParamPtr( "TargetOrientation" );
  int targetOrientation = ( param ? atoi( param->GetValue() ) : 0 );
  mTargetMax = 0;

  switch( targetOrientation )
  {
    case 0:
    case 3: // Both
      mpGabTargets = udlr;
      mTargetMax = sizeof( udlr ) / sizeof( *udlr );
      break;
    case 1: // Vertical
      mpGabTargets = ud;
      mTargetMax = sizeof( ud ) / sizeof( *ud );
      break;
    case 2: // Horizontal
      mpGabTargets = lr;
      mTargetMax = sizeof( lr ) / sizeof( *lr );
      break;
  }

  if( numberTargets > mTargetMax )
  {
    UserMessage( "Target type heuristics failed. "
                 "Target conversion is likely to be unusable.",
                 Warning );
    mpGabTargets = NULL;
  }
}

//-----------------------------------------------------------------------------

void __fastcall TfMain::bConvertClick(TObject *Sender)
{
 mOffset.clear();
 mGain.clear();

 FILE* fp = NULL;
 bool errorOccurred = mSourceFiles->Lines->Count < 1;

 {
   int  samplingRate = 0,
        numChannels = 0;
   for( int i = 0; i < mSourceFiles->Lines->Count; ++i )
   {
     BCI2000DATA reader;
     if( reader.Initialize( mSourceFiles->Lines->Strings[ i ].c_str(), 50000 )
          != BCI2000ERR_NOERR )
     {
        errorOccurred = true;
        UserMessage( "Error opening input file \""
                     + mSourceFiles->Lines->Strings[ i ] + "\"",
                     Error );
     }
     else
     {
        if( numChannels != 0 && reader.GetNumChannels() != numChannels )
        {
          UserMessage( "Inconsistent channel count in file \""
                       + mSourceFiles->Lines->Strings[ i ] + "\"",
                       Error );
          errorOccurred = true;
        }
        numChannels = reader.GetNumChannels();

        if( samplingRate != 0 && reader.GetSampleFrequency() != samplingRate )
        {
          UserMessage( "Inconsistent sampling rate in file \""
                       + mSourceFiles->Lines->Strings[ i ] + "\"",
                       Error );
          errorOccurred = true;
        }
        samplingRate = reader.GetSampleFrequency();

        if( ::SameFileName(
               ::ExtractShortPathName( mSourceFiles->Lines->Strings[ i ] ),
               ::ExtractShortPathName( eDestinationFile->Text ) ) )
        {
          UserMessage( "Input and output file are identical",
                       Error );
          errorOccurred = true;
        }
     }
   }
   if( errorOccurred )
     return;

   CheckCalibrationFile();

   // write the header
   fp = fopen(eDestinationFile->Text.c_str(), "wb");
   if (!fp)
      {
      UserMessage( "Error opening output file", Error );
      return;
      }

   gab_type val = numChannels;
   fwrite(&val, sizeof( val ), 1, fp);
   val = 0;
   fwrite(&val, sizeof( val ), 1, fp);
   val = samplingRate;
   fwrite(&val, sizeof( val ), 1, fp);
 }
 
 float maxRatio = 0.0;
 bool unexpectedTargetCode = false;

 Gauge->MinValue=0;
 Gauge->Progress=0;
 Gauge->MaxValue=Gauge->Width;

 int files = mSourceFiles->Lines->Count;
 float scalingFactor = 1.0 / 0.003;
 if( AutoscaleCheckbox->Checked )
 {
   mProgressLegend->Caption = "Scanning ...";
   mProgressLegend->Invalidate();
   float absMaxVal = 0.0;
   for( int file = 0; file < files; ++file )
   {
     BCI2000DATA data;
     data.Initialize( mSourceFiles->Lines->Strings[ file ].c_str(), 50000 );
     int samples = data.GetNumSamples(),
         channels = data.GetNumChannels();
     for( int sample = 0; sample < samples; ++sample )
     {
       if( sample % 1000 == 0 )
       {
         Gauge->Progress = ( file * Gauge->MaxValue ) / files
                          + ( sample * Gauge->MaxValue ) / ( samples * files );
         Application->ProcessMessages();
       }
       if( CalibFromFile() )
         for( int channel = 0; channel < channels; ++channel )
         {
           float absVal = fabs( mGain[channel] * ( data.ReadValue( channel, sample )
                                               - mOffset[channel] ) * scalingFactor );
           if( absVal > absMaxVal )
             absMaxVal = absVal;
         }
       else
         for( int channel = 0; channel < channels; ++channel )
         {
           float absVal = fabs( data.Value( channel, sample ) );
           if( absVal > absMaxVal )
             absMaxVal = absVal;
         }
     }
     Gauge->Progress = ( ( file + 1 ) * Gauge->MaxValue ) / files;
   }
   if( absMaxVal > 0.0 )
     scalingFactor = ( float( numeric_limits<short>::max() ) - 1e-10 ) / absMaxVal;
 }

 // go through each run
 for ( int file = 0; file < files; ++file )
  {
  mProgressLegend->Caption = "Converting ...";
  mProgressLegend->Invalidate();

  BCI2000DATA data;
  data.Initialize( mSourceFiles->Lines->Strings[ file ].c_str(), 50000 );
  int samples=data.GetNumSamples(),
      channels = data.GetNumChannels();

  // go through all samples in each run
  for ( int sample=0; sample<samples; sample++)
   {
   if (sample % 1000 == 0)
   {
     Gauge->Progress = ( file * Gauge->MaxValue ) / files
                      + ( sample * Gauge->MaxValue ) / ( samples * files );
     Application->ProcessMessages();
   }
   // go through each channel
   if( CalibFromFile() )
   {
     for (int channel=0; channel<channels; channel++)
     {
       float val = data.ReadValue(channel, sample);
       val= mGain[channel] * ( val - mOffset[channel] ) * scalingFactor;
       gab_type cur_value;
       float ratio = convert_num( val, cur_value );
       if( maxRatio < ratio )
         maxRatio = ratio;
       fwrite(&cur_value, sizeof( cur_value ), 1, fp);
     }
   }
   else
     for( int channel = 0; channel < channels; ++channel )
     {
       float val = data.Value( channel, sample ) * scalingFactor;
       gab_type cur_value;
       float ratio = convert_num( val, cur_value );
       if( maxRatio < ratio )
         maxRatio = ratio;
       fwrite( &cur_value, sizeof( cur_value ), 1, fp );
     }

   // write the state element
   data.ReadStateVector(sample);
   gab_type cur_state=ConvertState(&data);
   if( cur_state == -1 )
     unexpectedTargetCode = true;
   fwrite(&cur_state, sizeof( cur_state ), 1, fp);
   }
   // now, write one sample (all channels + state)
  // with a state value of 0 (i.e., Inter-Run-Interval)
  gab_type val = 0;
  for (int channel=0; channel<=channels; channel++)
     fwrite(&val, sizeof( val ), 1, fp);
  Gauge->Progress=( ( file + 1 ) * Gauge->MaxValue ) / files;
  }

 fclose(fp);

 if( maxRatio > 1.0 )
 {
   errorOccurred = true;
   UserMessage( "Numeric overflow occurred during conversion.\n"
                "Input data exceeded the numeric range by a ratio of "
                + FloatToStr( maxRatio ) + ".\n"
                "Conversion process finished.",
                Warning );
 }
 if( unexpectedTargetCode )
 {
   errorOccurred = true;
   UserMessage( "Unexpected target code found during conversion.\n"
                "Target conversion will be unusable.\n"
                "Conversion process finished.",
                Warning );
 }

 if( !errorOccurred )
   UserMessage( "Conversion process finished successfully",
                Message );
 Gauge->Progress=0;
 mProgressLegend->Caption = "";
 mProgressLegend->Invalidate();
}
//---------------------------------------------------------------------------


// convert one state of BCI2000 into the state of the old system
__int16 TfMain::ConvertState(BCI2000DATA *bci2000data)
{
const STATEVECTOR *statevector;
__int16     old_state;
short       ITI, targetcode, resultcode, feedback, restperiod;
static short cur_targetcode=-1;

 statevector=bci2000data->GetStateVectorPtr();
 ITI=statevector->GetStateValue("InterTrialInterval");
 targetcode=statevector->GetStateValue("TargetCode");
 resultcode=statevector->GetStateValue("ResultCode");
 feedback=statevector->GetStateValue("Feedback");
 restperiod= statevector->GetStateValue("RestPeriod");
 if (targetcode > 0) cur_targetcode=targetcode;

 // ITI
 if (ITI == 1)
    return(20);

 // there is an outcome
 if (resultcode > 0)
    {
    if (resultcode == cur_targetcode)
       return(18);
    else
       return(19);
    }

 // target on the screen
 if (targetcode > 0 )
 {
   if( targetcode > mTargetMax )
     return -1;
   else if( mpGabTargets != NULL )
     return mpGabTargets[ targetcode - 1 ];
   else
   {
      if (feedback == 0)                  // no cursor -> pre-trial pause
         return(2+targetcode-1);
      else
         return(10+targetcode-1);         // cursor on the screen
   }
 }

  if( restperiod == 1 )
        return( 24 );  

 // should actually never get here   
 return(1);
}


void __fastcall TfMain::bOpenFileClick(TObject *Sender)
{
 OpenDialog->Options << ofAllowMultiSelect;
 if( OpenDialog->Execute() )
 {
    TStringList* fileList = new TStringList;
    fileList->Assign( OpenDialog->Files );
    fileList->Sort();
    mSourceFiles->Lines->Assign( fileList );
    delete fileList;
 }
}
//---------------------------------------------------------------------------

void __fastcall TfMain::Button1Click(TObject *Sender)
{
  SaveDialog->InitialDir = ::ExtractFileDir( eDestinationFile->Text );
  if (SaveDialog->Execute())
  {
    eDestinationFile->Enabled = true;
    eDestinationFile->Text=SaveDialog->FileName;
  }
}
//---------------------------------------------------------------------------


void __fastcall TfMain::Button2Click(TObject *Sender)
{
  if (OpenParameter->Execute())
  {
    ParameterFile->Enabled = true;
    ParameterFile->Text=OpenParameter->FileName;
  }
}
//---------------------------------------------------------------------------

void __fastcall TfMain::mSourceFilesChange(TObject *Sender)
{
  TMemo* memo = static_cast<TMemo*>( Sender );
  bool haveFiles = ( memo->Text.Trim().Length() != 0 );
  bConvert->Enabled = haveFiles;
  if( haveFiles && !eDestinationFile->Enabled )
  {
    AnsiString destFileName = memo->Lines->Strings[ 0 ];
    eDestinationFile->Text = ::ChangeFileExt( destFileName, ".raw" );
  }
}
//---------------------------------------------------------------------------


