/*
 * @(#)engine.h    generated by: makeheader 4.21  Mon Aug 16 03:43:59 2004
 *
 *		built from:	../../src/include/copyright.h
 *				../../src/include/pragma_interface.h
 *				./engapi.cpp
 *				./engapiv4.cpp
 *				./engapiv5.cpp
 *				./fengapi.cpp
 *				./fengapiv5.cpp
 */

#ifndef engine_h
#define engine_h


/*
 * Copyright 1984-2003 The MathWorks, Inc.
 * All Rights Reserved.
 */



/* Copyright 2003 The MathWorks, Inc. */

/*
 * Prevent g++ from making copies of vtable and typeinfo data
 * in every compilation unit.  By allowing for only one, we can
 * save space and prevent some situations where the linker fails
 * to coalesce them properly into a single entry.
 *
 * References:
 *    http://gcc.gnu.org/onlinedocs/gcc/Vague-Linkage.html#Vague%20Linkage
 *    http://gcc.gnu.org/onlinedocs/gcc/C---Interface.html
 */

#ifdef __cplusplus
#  ifdef GLNX86
#    pragma interface
#  endif
#endif



#include "matrix.h"     /* mx Routines used in module */


typedef struct engine Engine;	/* Incomplete definition for Engine */

#ifdef __cplusplus
extern "C" {
#endif
/*
 * Execute matlab statement
 */
extern int engEvalString(
	Engine	*ep,		/* engine pointer */
	const char *string	/* string for matlab t execute */
	);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
/*
 * Start matlab process for single use.
 * Not currently supported on UNIX.
 */
extern Engine *engOpenSingleUse(
			 const char *startcmd, /* exec command string used to start matlab */
			 void *reserved, /* reserved for future use, must be NULL */
			 int *retstatus /* return status */
);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
/*
 * SetVisible, do nothing since this function is only for NT 
 */ 
extern int engSetVisible( 
		  Engine *ep,        /* engine pointer */ 
		  bool newVal 
		  );
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* 
 * GetVisible, do nothing since this function is only for NT 
 */ 
extern int engGetVisible( 
		  Engine *ep,        /* engine pointer */ 
		  bool* bVal 
		  );
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* 
 * Start matlab process
 */
extern Engine *engOpen(
	const char *startcmd /* exec command string used to start matlab */
	);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
/*
 * Close down matlab server
 */
extern int engClose(
	Engine	*ep         /* engine pointer */
	);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
/*
 * Get a variable with the specified name from MATLAB's workspace
 */
extern mxArray *engGetVariable(
	Engine	*ep,		/* engine pointer */
	const char *name	/* name of variable to get */
	);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
/*
 * Put a variable into MATLAB's workspace with the specified name
 */
extern int engPutVariable(
		   Engine	*ep,        /* engine pointer */
		   const char *var_name,
		   const mxArray *ap   /* array pointer */
		   );
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
/*
 * register a buffer to hold matlab text output
 */
extern int engOutputBuffer(
	Engine	*ep,		/* engine pointer */
	char	*buffer,	/* character array to hold output */
	int     buflen		/* buffer array length */
	);
#ifdef __cplusplus
}
#endif


#define engOpenV4()      cannot_call_engOpenV4


#define engGetFull()     engGetFull_is_obsolete
#define engPutFull()     engPutFull_is_obsolete
#define engGetMatrix()   engGetMatrix_is_obsolete
#define engPutMatrix()   engPutMatrix_is_obsolete


#if defined(V5_COMPAT)
#define engPutArray(ep, ap)   engPutVariable(ep, mxGetName(ap), ap)
#define engGetArray(ep, name) engGetVariable(ep, name)
#else
#define engPutArray() engPutArray_is_obsolete
#define engGetArray() engGetArray_is_obsolete
#endif /* defined(V5_COMPAT) */

#endif /* engine_h */
