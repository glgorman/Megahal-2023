# Megahal-2023
Updated version of the classic AI.  For Copyright and License information see
the file, megahal_license.txt.

Changes Made to Original Megahal: June 2023

1.  Changed file type from c to cpp.  The application is called from a short "main.cpp"
    source file, which currently is being used as a Windows compatability helper, but
    which will eventually be revised when mulit-threading, and multi-model support
    is added.
2.  Replaced strcpy and other deprected library calls with strcpy_s style variants.
3.  Converted DICTIONARY, MODEL, NODE, SWAP, and TREE structs to classes.
4.  Moved most, if not all relevant global functions that operate on stucts to classes.
5.  Created a MEGAHAL and an "intrinsics" namespace, and organized the relevant functions
    into those namespaces.  Also moved most of the global variables into namespaces. 
6.  Introduced a "symbol_table" class, and did some refactoring of the dictioary class,
    to reduce redunancy and bloat, while also making the DICTIONARY class more STL
    friendly.
7.  Created an OPCODE class that works with the COMMAND processing facility that is
    currently a part of the DICTIONARY class.  Ckeaned up the command processing
    code a bit.
8.  Added my own experimental "calculator_test.cpp", which is a "soft FPU" which
    attempts to implement at least some IEEE-754 capabilities, written entirely
    in C++, thus creating a "real" number class, which by using operator overloads,
    etc., tries to be compataible with the normal C-style float type.  This is
    now being used by megahal for some of the math operations, in anticpation for
    eventual porting to Parallax Propeller P2, Arduino, or FPGA based platforms.
   

