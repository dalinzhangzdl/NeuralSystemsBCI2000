////////////////////////////////////////////////////////////////////////////////
// $Id$
// Authors: juergen.mellinger@uni-tuebingen.de
// Description: Script object type for the script interpreter.
//
// $BEGIN_BCI2000_LICENSE$
//
// This file is part of BCI2000, a platform for real-time bio-signal research.
// [ Copyright (C) 2000-2012: BCI2000 team and many external contributors ]
//
// BCI2000 is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// BCI2000 is distributed in the hope that it will be useful, but
//                         WITHOUT ANY WARRANTY
// - without even the implied warranty of MERCHANTABILITY or FITNESS FOR
// A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// this program.  If not, see <http://www.gnu.org/licenses/>.
//
// $END_BCI2000_LICENSE$
////////////////////////////////////////////////////////////////////////////////
#ifndef SCRIPT_TYPE_H
#define SCRIPT_TYPE_H

#include "ObjectType.h"

namespace Interpreter {

class ScriptType : public ObjectType
{
 protected:
  virtual const char* Name() const { return "Script"; }
  virtual const MethodEntry* MethodTable() const { return sMethodTable; }

 public:
  static bool Set( ScriptInterpreter& );
  static bool Clear( ScriptInterpreter& );
  static bool Execute( ScriptInterpreter& );

 private:
  static void SetScript( ScriptInterpreter&, const std::string&, const std::string& );

  static const MethodEntry sMethodTable[];
  static ScriptType sInstance;
};

} // namespace

#endif // SCRIPT_TYPE_H
