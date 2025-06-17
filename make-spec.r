Rebol [
    name: Library
    notes: "See %extensions/README.md for the format and fields of this file"

    extended-types: [library!]
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
