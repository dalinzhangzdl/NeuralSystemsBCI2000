//   $Id$
//  
//   This file is part of the BCPy2000 foundation, a set of modules for
//   the BCI2000 <http://bci2000.org/> that allow communication with a
//   Python framework built on top. It is distributed together with the
//   BCPy2000 framework.
// 
//   Copyright (C) 2007-8  Thomas Schreiner, Jeremy Hill, 
//                         Christian Puzicha, Jason Farquhar
//   
//   bcpy2000@bci2000.org
//   
//   The BCPy2000 foundation is free software: you can redistribute it
//   and/or modify it under the terms of the GNU Lesser General Public
//   License as published by the Free Software Foundation, either
//   version 3 of the License, or (at your option) any later version.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU Lesser General Public License for more details.
//
//   You should have received a copy of the GNU Lesser General Public License
//   along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// Description: Allows Python functions to be loaded dynamically from a DLL at
// run-time (no linking against import libraries). Each of the *macros* that
// we need from the Python and NumPy APIs is wrapped into multiple functions,
// according to the different supported Python versions,  and only the versions
// corresponding to the loaded DLL are made available. For the subset of the
// Python and NumPy APIs that we need, only very minimal changes were needed to
// the client code, and the client code can still be compiled without the wrapper.

////////////////////////////////////////////////////////////////
#ifndef PYTHON_API_NAMESPACE  //////// Define and load functions
////////////////////////////////////////////////////////////////

namespace PyAPI24 {void Macros2Functions(void);};
namespace PyAPI25 {void Macros2Functions(void);};

#ifdef _WIN32
#include "BCIError.h"
#include <windows.h>
#else
#include <iostream>
#define bcierr               std::cerr
#include <dlfcn.h>
#define HINSTANCE            void*
#define LoadLibrary(a)       dlopen(a, RTLD_LAZY)
#define GetProcAddress(a,b)  dlsym((a),(b))
#endif

#define  PYTHON_LINK_HEADER_MODE   2 // normal header stuff, plus actual
#include "PythonWrapper.h"           // instantiation of function pointers


#include <string>

int LoadPythonLinks(const char *dllname)
{
	static char trynames[32] = "python25\0python24\0\0";
	HINSTANCE dll = NULL;
	if(dllname && *dllname) {
		dll = ::LoadLibrary(dllname);
		if(!dll) bcierr << "failed to load dynamic library \"" << dllname << "\"" << std::endl;
	}
	else {
		dllname = trynames;
		while(*dllname) {
			dll = ::LoadLibrary(dllname);
			if(dll) break;
			while(*dllname) dllname++;
			dllname++;
		}
		if(!dll) bcierr << "Failed to find a dynamic library for python24 or python25. Is python installed?" << std::endl;
	}
	bool all_loaded = (dll != 0);

#define  PYTHON_LINK_HEADER_MODE   3
#include "PythonWrapper.h"

	std::string lowername = "";
	for(const char *s = dllname; s && *s; s++) lowername += tolower(*s);
	if(dll) {
		if      (lowername.find("python24") !=std::string::npos) PyAPI24::Macros2Functions();
		else if (lowername.find("python2.4")!=std::string::npos) PyAPI24::Macros2Functions();
		else if (lowername.find("python25") !=std::string::npos) PyAPI25::Macros2Functions();
		else if (lowername.find("python2.5")!=std::string::npos) PyAPI25::Macros2Functions();
		else {
			all_loaded = false;
			bcierr << "failed to recognize version from dll name \"" << dllname << "\"" << std::endl;
		}
	}
	return !all_loaded;
}



// PyList_Check() and friends are not, as they appear, functions, but macros.
// We've worked painfully around *that* in principle, but *these* particular
// macros depend on static data objects, so we can't use the dynamic loading
// strategy at all. We'll have to settle for these much slower alternatives.
int PyList_Check(PyObject* a)
{
	PyObject* py_template = PyList_New(0);
	PyObject* py_class = PyObject_GetAttrString(py_template, "__class__");
	// probably there's single API call for getting class too, but the API
	// doc is impossible to navigate unless you already know the name of
	// what you're looking for...
	int result = PyObject_IsInstance(a, py_class);
	Py_DECREF(py_class);
	Py_DECREF(py_template);
	return result;
}
int PyString_Check(PyObject* a)
{
	PyObject* py_template = PyString_FromString("");
	PyObject* py_class = PyObject_GetAttrString(py_template, "__class__");
	int result = PyObject_IsInstance(a, py_class);
	Py_DECREF(py_class);
	Py_DECREF(py_template);
	return result;
}

////////////////////////////////////////////////////////////////
#else //////////////// wrap (possibly-version-dependent) macros

// This second half of the file is the only place where version-
// dependent Python headers have been included.

#define PyObject PyObject
#define PyThreadState PyThreadState
#define PyArrayObject PyArrayObject
#define PYTHON_LINK_HEADER_MODE 0 // 
#include "PythonWrapper.h"
namespace PYTHON_API_NAMESPACE { ///////////////////////////////
////////////////////////////////////////////////////////////////

void    PyWrapMacro_Py_DECREF(PyObject* a) {Py_DECREF(a);}
void    PyWrapMacro_PyList_SET_ITEM(PyObject* op,int i,PyObject* v) {PyList_SET_ITEM(op, i, v);}
double* PyWrapMacro_PyArray_DATA(PyArrayObject* a)    {return (double*)PyArray_DATA(a);}
int*    PyWrapMacro_PyArray_DIMS(PyArrayObject* a)    {return    (int*)PyArray_DIMS(a);}
int*    PyWrapMacro_PyArray_STRIDES(PyArrayObject* a) {return    (int*)PyArray_STRIDES(a);}

 ////////////////////////////////////////////////////////////////
void Macros2Functions(void)
{
	Py_DECREF = PyWrapMacro_Py_DECREF;
	PyList_SET_ITEM = PyWrapMacro_PyList_SET_ITEM;
	PyArray_DATA = PyWrapMacro_PyArray_DATA;
	PyArray_DIMS = PyWrapMacro_PyArray_DIMS;
	PyArray_STRIDES = PyWrapMacro_PyArray_STRIDES;
}
////////////////////////////////////////////////////////////////
} // end of namespace PYTHON_API_NAMESPACE /////////////////////
#endif /////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
