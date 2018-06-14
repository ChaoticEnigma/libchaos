# LibChaos

LibChaos is a general-purpose C++ utilities library.

### Includes:

- Supported:
    - Data Structures:
        - Array             ("ZArray")
        - List              ("ZList")
        - Map               ("ZMap")
        - Set               ("ZSet")
        - Smart Pointer     ("ZPointer")
        - Stack             ("ZStack")
        - Queue             ("ZQueue")
    - Allocators            ("ZAllocator", "ZPoolAllocator", "ZWrapAllocator")
    - String Manipulation   ("ZString", "ZJSON")
    - File Manipulation     ("ZFile", "ZPath")
    - Threading             ("ZThread", "ZMutex", "ZCondition", "ZWorkQueue")
    - Hashing               ("ZHash")
    - Random Generation     ("ZRandom")
    - UUID Generation       ("ZUID")
    - Image Format Support:
        - Images            ("ZImage")
        - PNG Images        ("ZPNG")
        - BMP Images        ("ZBMP")
        - PPM Images        ("ZPPM")
        - WebP Images       ("ZWebP")
    - Program Arguments     ("ZOptions")
    - Configuration Files   ("ZSettings")
    - SQLite3 Database      ("ZDatabase")

- Experimental:
    - Networking            ("ZSocket", "ZAddress", "ZDatagramSocket", "ZStreamSocket", "ZConnection")

- WIP:
    - JPEG Images           ("ZJPEG")
    - Math                  ("ZNumber", "ZExpression", "ZFormula")
    - PDF                   ("ZPDF")
    - XML                   ("ZXML")

## Building LibChaos

    mkdir libchaos-build
    cd libchaos-build
    cmake ../libchaos
    make


## General LibChaos Notes
Z-prefixed classes are classes that should be used in user code.
Y-prefixed classes enforce interfaces on inheriting classes.
Other classes from LibChaos are not intended to be used outside the library code.

## Exception Policy
LibChaos makes _some_ use of exceptions. However, exceptions are only generated on misuse of
LibChaos classes and methods, e.g. calling a method on an object in an invalid state, which
should have been tested before the call. LibChaos makes heavy use of value-return methods
(return type is the value the method was called for). This pattern requires, on error,
either a way to leave the method without returing (exception or assert), or each returned type
to be able to indicate invalid state. Simple value types (strings, arrays, etc) shouldn't need
to carry this information. Exceptions are favored over asserts for this purpose, since they
allow the user to capture and get information about the error, e.g. a stack trace.

Methods that can legitimately fail when used correctly (files, networking, etc) will generally
return or set status codes, and set return values through references or pointers (as appropriate).
