REBOL [
    Name: Library
    Notes: "See %extensions/README.md for the format and fields of this file"
]

use-librebol: 'no

includes: []

sources: %mod-library.c

depends: compose [
    (switch platform-config.os-base [
        'Windows [
            spread [
                [%library-windows.c]
            ]
        ]
    ] else [
        spread [
            [%library-posix.c]
        ]
    ])
]
