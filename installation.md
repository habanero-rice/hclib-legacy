---
title: Getting Started
layout: hclib
---

# Installation Instructions

HClib is a library layer built on top of a supporting runtime. The reference HClib implementation is built on top of the Open Community Runtime (OCR) that must be installed separately.

## Installing Open Community Runtime (OCR)

Be careful to build OCR with ELS support enabled for one slot.

OCR is available from <https://01.org/projects/open-community-runtime>

1. Clone the OCR repository:
````
    git clone https://github.com/01org/ocr.git
    cd ocr
    git branch ocr-hclib --track origin/ocr-hclib
    git checkout ocr-hclib
````
2.  Build OCR with ELS and runtime API support:
````
    OCR_CONF_OPTS="--enable-els=1 --enable-rtapi --enable-ocrlib" ./install.sh 
````
By default, OCR is installed to the current folder, under `ocr-install/`

3.  Setup your environment:
````
    export OCR_ROOT=${PWD}/ocr-install
    export LD_LIBRARY_PATH=${OCR_ROOT}/lib:${LD_LIBRARY_PATH}
````

## Installing HClib

HClib relies on autotools as a build system. You can either invoke `configure` 
and `make` directly if familiar with, or use the provided installation script. 
Note that the installation script runs the whole tool chain and is overkill 
to recompile the library when updating code.

### Building from sources

1. Clone the HClib repository
````
    git clone https://github.com/habanero-rice/hclib.git
    cd hclib
````

2.  Build HClib

    ````
    ./install.sh
    ````

    By default, HClib is installed in the current folder, under `hclib-install/`

3.  Setup your environment:

    Defining the `HCLIB_ROOT` environment variable and updating the `LD_LIBRARY_PATH` is optional but comes handy when writing compiler command lines.

    ````
    export HCLIB_ROOT=$PWD/hclib-install
    export LD_LIBRARY_PATH=${OCR_ROOT}/lib:${HCLIB_ROOT}/lib:${LD_LIBRARY_PATH}
    export OCR_CONFIG=${OCR_ROOT}/config/default.cfg
    ````

    Note: `OCR_CONFIG` points to the OCR configuration file from which you can set the number of OCR workers.

You're done ! Check the [tutorial](tutorial.html) for an overview of HClib examples.
