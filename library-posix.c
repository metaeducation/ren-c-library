//
//  File: %library-posix.c
//  Summary: "POSIX Library-related functions"
//  Project: "Rebol 3 Interpreter and Run-time (Ren-C branch)"
//  Homepage: https://github.com/metaeducation/ren-c/
//
//=////////////////////////////////////////////////////////////////////////=//
//
// Copyright 2012 REBOL Technologies
// Copyright 2012-2025 Ren-C Open Source Contributors
// REBOL is a trademark of REBOL Technologies
//
// See README.md and CREDITS.md for more information.
//
// Licensed under the Lesser GPL, Version 3.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// https://www.gnu.org/licenses/lgpl-3.0.html
//
//=////////////////////////////////////////////////////////////////////////=//
//
// This is for support of the LIBRARY! type from the host on
// systems that support 'dlopen'.
//
//=//// NOTES /////////////////////////////////////////////////////////////=//
//
// A. While thread safety of dlerror() is implementation-defined :-/ it does
//    appear that all errors coming from dlerror(), dlopen(), dlsym() will
//    be communicated via dlerror().


#ifndef __cplusplus
    // See feature_test_macros(7)
    // This definition is redundant under C++
    #define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <poll.h>
#include <fcntl.h>  // Includes `O_XXX` constant definitions
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "sys-core.h"
#include "tmp-mod-library.h"

#include "sys-library.h"

#include <dlfcn.h>


//
//  Trap_Open_File_Descriptor_For_Library: C
//
// Load a DLL library and return the handle to it.
// If zero is returned, error indicates the reason.
//
// 1. Usually you want to fully resolve local paths before making OS calls.
//    But the dlopen() function searches library directories by default.
//    So if %foo is passed in, you don't want to prepend the current dir to
//    make it absolute, because it will *only* look there.
//
Option(Error*) Trap_Open_File_Descriptor_For_Library(
    Sink(FileDescriptor) fd_out,
    const Element* path
){
    char* path_utf8 = rebSpell("file-to-local", path);  // don't use :FULL

    void* handle = dlopen(path_utf8, RTLD_LAZY/*|RTLD_GLOBAL*/);
    rebFree(path_utf8);

    if (handle != nullptr) {
        *fd_out = handle;  // dlopen() returns void*, not a FILE*
        return SUCCESS;
    }

    const char* error_utf8 = dlerror();  // reports ALL dlopen() errors [A]
    return Error_User(error_utf8);
}


//
//  Trap_Close_Library: C
//
// Free a DLL library opened earlier.
//
Option(Error*) Trap_Close_Library(Library* lib)
{
    if (dlclose(Library_Fd(lib)) == 0)
        return SUCCESS;

    const char* error_utf8 = dlerror();  // reports ALL dlclose() errors [A]
    return Error_User(error_utf8);
}


//
//  Trap_Find_Function_In_Library: C
//
// Get a DLL function address from its string name.
//
// 1. dlsym() can validly return 0 if the address of a symbol is 0, or if
//    it is an "undefined weak symbol".  :-/
//
//    The way you check for errors from dlsym() is to see if it triggers a
//    non-null dlerror().  You have to clear any old dlerror() first.
//
// 2. There is no standardization of the error message returned by dlerror().
//    So failure could mean symbol not found, or something else.  Generally
//    speaking the only error we should get on a valid and opened library
//    would be symbol not found.
//
Option(Error*) Trap_Find_Function_In_Library(
    Sink(CFunction*) cfunc,
    const Library* lib,
    const char* funcname
){
    assert(not Is_Library_Closed(lib));

    dlerror();  // Clear any previous errors [1]

    void* symbol = dlsym(Library_Fd(lib), funcname);  // may be nullptr [1]

    const char* error_utf8 = dlerror();  // reports ALL dlsym() errors [A]

    if (error_utf8 == nullptr) {
        *cast(void**, cfunc) = symbol;
        return SUCCESS;
    }

    return Error_User(error_utf8);  // not necessarily "symbol not found" [2]
}
