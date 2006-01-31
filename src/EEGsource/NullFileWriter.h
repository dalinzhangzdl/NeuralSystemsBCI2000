////////////////////////////////////////////////////////////////////////////////
//
// File:NullFileWriter.h
//
// Date: Sept 20, 2005
//
// Author: juergen.mellinger@uni-tuebingen.de
//
// Description: A dummy file writer.
//
////////////////////////////////////////////////////////////////////////////////
#ifndef NullFileWriterH
#define NullFileWriterH

#include "GenericFileWriter.h"

class NullFileWriter: public GenericFileWriter
{
 public:
          NullFileWriter();
  virtual ~NullFileWriter();
  virtual void Preflight( const SignalProperties& Input,
                                SignalProperties& Output ) const;
  virtual void Initialize2( const SignalProperties& Input,
                            const SignalProperties& Output );
  virtual void Write( const GenericSignal& Signal,
                      const STATEVECTOR&   Statevector );
};

#endif // NullFileWriterH