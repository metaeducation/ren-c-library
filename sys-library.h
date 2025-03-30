//
//  File: %sys-library.h
//  Summary: "Definitions for LIBRARY! (DLL, .so, .dynlib)"
//  Project: "Rebol 3 Interpreter and Run-time (Ren-C branch)"
//  Homepage: https://github.com/metaeducation/ren-c/
//
//=////////////////////////////////////////////////////////////////////////=//
//
// Copyright 2014 Atronix Engineering, Inc.
// Copyright 2014-2025 Ren-C Open Source Contributors
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
// A library represents a loaded .DLL or .so file.  This contains native
// code, which can be executed through extensions.  The type is also used to
// load and execute non-Rebol-aware C code by the FFI extension.
//
// File descriptor in singular->link.fd
// Meta information in singular->misc.meta
//

typedef Array Library;

typedef void* FileDescriptor;

INLINE FileDescriptor Library_Fd(const Library *lib)
  { return lib->link.p; }

INLINE const Element* Library_File(const Library *lib) {
    return c_cast(Element*, Stub_Cell(lib));
}

INLINE bool Is_Library_Closed(const Library* lib)
  { return lib->link.p == nullptr; }

INLINE Library* Cell_Library(const Cell* v) {
    assert(Is_Library(v));
    return cast(Array*, CELL_NODE1(v));
}

#define Cell_Library_FD(v) \
    Library_Fd(Cell_Library(v))


// !!! Extensions not scanned for functions to make include files.  Review

extern Option(Error*) Trap_Open_File_Descriptor_For_Library(
    Sink(FileDescriptor),
    const Element* path
);

extern Option(Error*) Trap_Close_Library(Library* lib);

extern Option(Error*) Trap_Find_Function_In_Library(
    Sink(CFunction*) cfunc,
    const Library* lib,
    const char* funcname
);
