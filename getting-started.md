---
title: Getting Started
layout: hclib
---

# Getting Started

## Installation

Refer the the [installation guide](installation.html) for instructions on how to install HClib and its dependences.

## Running HClib non-regression tests

````
    cd tests;
    ./runTests
````

# Writing programs using HClib

HClib defines a C-based interface in the `hclib.h` header file. 
Just use the include directive in your program to import the interface:

````
    #include "hclib.h"
````

# Linking with HClib

If you did set the `HCLIB_ROOT` and `OCR_ROOT` environment variable as described in the `INSTALL` file, you can define the following environment variables to simplify compiler command lines:

````
    export HClib_CFLAGS="-I${HCLIB_ROOT}/include"
    export HClib_LDFLAGS="-L${HCLIB_ROOT}/lib -L${OCR_ROOT}/lib -locr -lhclib"
````

Linking against OCR and HClib is then done easily:

````
    gcc -g ${HClib_LDFLAGS} ${HClib_CFLAGS} your_program.c
````
