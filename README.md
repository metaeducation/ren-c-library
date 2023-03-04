## Library Extension

The LIBRARY! datatype in Rebol represented the idea of dynamically loadable
code.  It was used by the FFI to speak to an arbitrary DLL through C types,
but also used by the extension mechanism itself to deal with specially
constructed Rebol Extension DLLs--which can provide additional natives or
ports to the system.

There's a bit of an unusual aspect to having the Library code in an extension,
since it cannot itself be loaded dynamically.

The code was at one point tested in the continuous integration, with the
UUID extension by building with "UUID *" (* means dynamic) and this code: 

    - name: Test UUID Extension Built As DLL
      shell: r3built --fragment {0}  # See README: Ren-C Code As Step
      run: |
        print {== Hello From R3 DLL Test! ==}
        mod-uuid: load-extension join what-dir %build/libr3-uuid.so
        print {== UUID Extension Loaded ==}
        uuid: mod-uuid.generate
        assert [16 = length of uuid]
        print ["Succeeded:" uuid]

However it was removed because the feature isn't a high enough priority to be
in the forefront and make it something that can be worked on independently,
and not burden the rest of the system.  (e.g. the JavaScript build cannot
make use of the DLL handling).
