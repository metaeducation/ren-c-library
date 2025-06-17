//
//  file: %library-windows.c
//  summary: "OS API function library called by REBOL interpreter"
//  project: "Rebol 3 Interpreter and Run-time (Ren-C branch)"
//  homepage: https://github.com/metaeducation/ren-c/
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

#include <stdlib.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN  // trim down the Win32 headers
#include <windows.h>
#undef IS_ERROR
#undef OUT  // %minwindef.h defines this, we have a better use for it
#undef VOID  // %winnt.h defines this, we have a better use for it


#include <process.h>
#include <assert.h>

#include "sys-core.h"
#include "tmp-mod-library.h"

#include "sys-library.h"

//
//  Trap_Open_File_Descriptor_For_Library: C
//
// Load a DLL library and return the handle to it, or else an error.
//
// 1. While often when communicating with the OS, the local path should be
//    fully resolved, the LoadLibraryW() function searches DLL directories by
//    default.  So if %foo is passed in, you don't want to prepend the
//    current dir to make it absolute, because it will *only* look there.
//
// 2. Though HMODULE isn't guaranteed to be a pointer, all Windows in the wild
//    implement it thusly, and it would break a lot more code than this if
//    that were to change.
//
Option(Error*) Trap_Open_File_Descriptor_For_Library(
    Sink(FileDescriptor) fd,
    const Element* path
){
    WCHAR* path_wide = rebSpellWide("file-to-local", path);  // no :FULL [1]

    HMODULE hmodule = LoadLibraryW(path_wide);
    rebFree(path_wide);

    if (hmodule) {
        *fd = hmodule;  // HMODULE not technically "guaranteed" as pointer [2]
        return SUCCESS;
    }

    return Error_OS(GetLastError());
}


//
//  Close_Library: C
//
// Free a DLL library opened earlier.
//
Option(Error*) Trap_Close_Library(Library* lib)
{
    assert(not Is_Library_Closed(lib));

    HMODULE hmodule = cast(HMODULE, Library_Fd(lib));
    if (FreeLibrary(hmodule))
        return SUCCESS;

    return Error_OS(GetLastError());
}


//
//  Trap_Find_Function_In_Library: C
//
// Get a DLL function address from its string name.
//
// !!! See notes about data pointers vs. function pointers in the definition
// of CFunction.  This is trying to stay on the right side of the spec, but
// OS APIs often are not standard C.  So this implementation is not guaranteed
// to work, just to suppress compiler warnings.  See:
//
//      http://stackoverflow.com/a/1096349/211160
//
Option(Error*) Trap_Find_Function_In_Library(
    Sink(CFunction*) cfunc,
    const Library* lib,
    const char* funcname
){
    HMODULE hmodule = cast(HMODULE, Library_Fd(lib));
    FARPROC fp = GetProcAddress(hmodule, funcname);

    if (fp != nullptr) {  // windows guarantees DLLs not at address 0
        *cfunc = f_cast(CFunction*, fp);
        return SUCCESS;
    }

    return Error_OS(GetLastError());
}
