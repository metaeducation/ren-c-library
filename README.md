## Library Extension

The LIBRARY! datatype is used to interact with things like .DLL or .so files,
to locate dynamically loadable code.

    >> winsock: make library! %/C/Windows/System32/wsock32.dll
    == &[library! %/C/Windows/System32/wsock32.dll]

    >> pick winsock "gethostbyname"
    == &[handle! []]

    >> pick winsock "gethostbynickname"
    ** Error: Couldn't find "gethostbynickname"
          in &[library! %/C/Windows/System32/wsock32.dll]

    >> try pick winsock "gethostbynickname"
    == ~null~  ; anti

    >> close winsock
    == &[library! {closed} %/C/Windows/System32/wsock32.dll]

It's the foundation of being able to load extensions dynamically.  For
instance, if you chose to build the UUID extension as dynamically loadable,
LIBRARY! provides the foundation for LOAD-EXTENSION in that case:

    mod-uuid: load-extension join what-dir %build/libr3-uuid.so
    uuid: mod-uuid.generate
    assert [16 = length of uuid]


## Library Extension can't itself be a DLL-based extension

There's a bit of an unusual aspect to having the Library code in an extension,
since it cannot itself be loaded dynamically.

Hence, it's only useful if you choose the "build into executable" option for
the extension in make.


## Supports POSIX and Windows, WebAssembly support TBD

POSIX version uses dlopen(), and the Windows version uses GetProcAddress().

WebAssembly has something called "side modules", and it's strongly desired
to be able to support LIBRARY! using these:

  https://emscripten.org/docs/compiling/Dynamic-Linking.html
