//
//  file: %t-library.c
//  summary: "External Library Support"
//  section: datatypes
//  project: "Rebol 3 Interpreter and Run-time (Ren-C branch)"
//  homepage: https://github.com/metaeducation/ren-c/
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

#include "sys-core.h"
#include "tmp-mod-library.h"

#include "sys-library.h"


// 1. The original library code didn't save the filename.  But with immutable
//    FILE! it's easy enough to do.
//
IMPLEMENT_GENERIC(MAKE, Is_Library)
{
    INCLUDE_PARAMS_OF_MAKE;
    UNUSED(ARG(TYPE));

    if (not Is_File(ARG(DEF)))
        return FAIL(PARAM(DEF));

    Element* file = Element_ARG(DEF);

    FileDescriptor fd;
    Option(Error*) e = Trap_Open_File_Descriptor_For_Library(&fd, file);
    if (e)
        return RAISE(unwrap e);

    Library* library = cast(Library*, Prep_Stub(
        FLAG_FLAVOR(CELLS)
            | NODE_FLAG_MANAGED
            | (not STUB_FLAG_LINK_NODE_NEEDS_MARK)  // width, integer
            | (not STUB_FLAG_MISC_NODE_NEEDS_MARK)  // height, integer
            | (not STUB_FLAG_INFO_NODE_NEEDS_MARK),  // info, not used ATM
        Alloc_Stub()
    ));
    Copy_Cell(Force_Erase_Cell(Stub_Cell(library)), file);  // save it [1]

    library->link.p = fd;  // seen as shared by all instances
    Corrupt_Unused_Field(library->misc.corrupt);

    Reset_Extended_Cell_Header_Noquote(
        OUT,
        EXTRA_HEART_LIBRARY,
        (not CELL_FLAG_DONT_MARK_NODE1)  // library stub needs mark
            | CELL_FLAG_DONT_MARK_NODE2  // second slot not used ATM
    );

    CELL_NODE1(OUT) = library;

    return OUT;
}


IMPLEMENT_GENERIC(MOLDIFY, Is_Library)
{
    INCLUDE_PARAMS_OF_MOLDIFY;

    Element* v = Element_ARG(ELEMENT);
    assert(Is_Library(v));

    Molder* mo = Cell_Handle_Pointer(Molder, ARG(MOLDER));
    bool form = Bool_ARG(FORM);

    UNUSED(form);

    Library* lib = Cell_Library(v);
    Append_Ascii(mo->string, "#[library! ");
    if (Is_Library_Closed(lib))
        Append_Ascii(mo->string, "{closed} ");
    Mold_Or_Form_Element(mo, Library_File(lib), false);
    Append_Ascii(mo->string, "]");

    return NOTHING;
}


// 1. On POSIX in particular, there's no standardization of error codes for
//    *why* a dlsym() lookup failed.  You get a string back that you'd have
//    to parse.  The messages can vary:
//
//       "undefined symbol: <symbol_name>"
//       "symbol <symbol_name> not found"
//       "invalid handle"
//       "symbol version mismatch"
//       "corrupted shared object"
//       "permission denied"
//       "shared object not found"
//       "relocation error"
//
//    On Windows, it's possible to get ERROR_PROC_NOT_FOUND specifically, but
//    we're calling an abstraction layer.  If we just give back the OS error
//    message it will say:
//
//       "The specified procedure could not be found.^M^/"
//
//    Use a heuristic to try and guess if we should decide it's a symbol
//    not found error, and give a useful message.
//
// 2. What we really want is to RAISE if the function is not found, but FAIL
//    if it's another unexpected condition.  But we only have a heuristic
//    (at least on POSIX).
//
IMPLEMENT_GENERIC(PICK, Is_Library)
{
    INCLUDE_PARAMS_OF_PICK;

    Element* library = Element_ARG(LOCATION);
    Library* lib = Cell_Library(library);

    Element* picker = Element_ARG(PICKER);

    if (not Is_Text(picker) and not Is_Word(picker))
        return FAIL(PARAM(PICKER));

    const char* funcname = cs_cast(Cell_Utf8_At(picker));

    CFunction* cfunc;
    Option(Error*) e = Trap_Find_Function_In_Library(&cfunc, lib, funcname);
    if (not e)
        return Init_Handle_Cfunc(OUT, cfunc);

    Element* error = Init_Error(SPARE, unwrap e);

    return rebDelegate("any [",  // use heuristic for better error handling [1]
        "find pick", error, "'message -{could not be found}-",
        "all [",
            "find pick", error, "'message -{not found}-",
            "not find pick", error, "'message -{shared}-",
        "]",
    "] then [raise [",  // RAISE allows defuse as NULL with TRY
        "-{Couldn't find}- mold", picker, "-{in}- mold", library,
    "]] else [",
        "fail", error,  // is heuristic good enough to fail if not matching? [2]
    "]");
}


IMPLEMENT_GENERIC(OPEN_Q, Is_Library)
{
    INCLUDE_PARAMS_OF_OPEN_Q;

    Element* library = Element_ARG(ELEMENT);
    Library* lib = Cell_Library(library);

    return rebLogic(Library_Fd(lib) != nullptr);
}


IMPLEMENT_GENERIC(CLOSE, Is_Library)
{
    INCLUDE_PARAMS_OF_CLOSE;

    Element* library = Element_ARG(PORT);  // !!! generic arg name is "port"?
    Library* lib = Cell_Library(library);

    if (Library_Fd(lib) == nullptr)
        return rebDelegate("raise [",  // RAISE allows defuse with TRY
            "-{CLOSE called on already closed library:}- mold", library,
        "]");

    Option(Error*) e = Trap_Close_Library(lib);
    Cell_Library(library)->link.p = nullptr;
    if (e)
        return FAIL(unwrap e);  // unexpected failure: FAIL, don't RAISE

    return COPY(library);
}


extern RebolApiTable g_librebol;

//
//  export run-library-collator: native [
//
//  "Execute DLL function that takes RebolApiTable* and returns RebolValue*"
//
//      return: [~null~ any-value?]
//      library [library!]
//      linkname [text!]
//  ]
//
DECLARE_NATIVE(RUN_LIBRARY_COLLATOR)
//
// !!! This code used to check for loading an already loaded extension.  It
// looked in an "extensions list", but now that the extensions are modules
// really this should just be the same as looking in the modules list.  Such
// code should be in usermode (very awkward in C).  Only unusual C bit was:
//
//     /*
//      * Found existing extension, decrease reference added by MAKE_library
//      */
//     OS_CLOSE_LIBRARY(Cell_Library_FD(lib));
//
// 1. We pass the collation entry point the table of API functions.  This is
//    how DLLs know the addresses of functions in the EXE that they can call.
//    If the extension is built in with the executable, it uses a shortcut
//    and just calls the API_rebXXX() functions directly...so it does not use
//    this table.
{
    INCLUDE_PARAMS_OF_RUN_LIBRARY_COLLATOR;

    CFunction* cfunc;
    Option(Error*) e = Trap_Find_Function_In_Library(
        &cfunc,
        Cell_Library(ARG(LIBRARY)),
        cs_cast(String_Head(Cell_String(ARG(LINKNAME))))
    );
    if (e)
        return RAISE(unwrap e);  // RAISE allows defuse with TRY

    ExtensionCollator* collator = cast(ExtensionCollator*, cfunc);

    return (*collator)(&g_librebol);  // pass collation entry point the API [1]
}


//
//  startup*: native [
//
//  "Startup LIBRARY! Extension"
//
//      return: [~]
//  ]
//
DECLARE_NATIVE(STARTUP_P)
{
    INCLUDE_PARAMS_OF_STARTUP_P;

    return NOTHING;
}


//
//  shutdown*: native [
//
//  "Shutdown LIBRARY! Extension"
//
//      return: [~]
//  ]
//
DECLARE_NATIVE(SHUTDOWN_P)
{
    INCLUDE_PARAMS_OF_SHUTDOWN_P;

    return NOTHING;
}
