---
title: Getting Started
layout: hclib
---

# Requirements

- OCR (Open Community Runtime)

WARNING: HCLIB will NOT work if OCR is not specifically built for HCLIB support.

# OCR Install Instructions

HClib is a library layer built on top of a supporting runtime. The reference HClib implementation is built on top of the Open Community Runtime (OCR) that must be installed separately.

1.  Clone the HCLIB repository:
```
    git clone https://github.com/habanero-rice/hclib.git
```

    The folder where hclib is cloned is now referred to as 'HCLIB_SRC_ROOT'
```
    export HCLIB_SRC_ROOT=${PWD}/hclib.git
```

2.  Clone the OCR repository
```
    git clone https://xstack.exascale-tech.com/git/public/xstack.git
```

    The folder where xstack is cloned is now referred to as 'XSTACK_SRC_ROOT'
```
    export XSTACK_SRC_ROOT=${PWD}/xstack.git
```

3.  Define the 'OCR_SRC_ROOT' environment variable to point to the 'ocr/' folder
```
    export OCR_SRC_ROOT=${XSTACK_SRC_ROOT}/ocr
```

4.  Build OCR with support for HCLIB

    WARNING: HCLIB will NOT work if OCR is not specifically built for HCLIB support !

    HCLIB requires the following OCR setup:
    1) OCR ELS must be set to 1
    2) Legacy extension enabled
    3) Runtime extension enabled

    The HCLIB distribution provides a helper build script 'build-ocr.sh' 
    located under '${HCLIB_SRC_ROOT}/scripts/'. The script does some checks 
    and properly setup OCR for HCLIB support.
```
    ${HCLIB_SRC_ROOT}/scripts/ocr/build-ocr.sh ${OCR_SRC_ROOT}
```

    By default, OCR is installed under '${OCR_SRC_ROOT}/install/x86'

5.  Update your environment for OCR:
```
    export OCR_INSTALL=${OCR_SRC_ROOT}/install/x86
    export PATH=${OCR_INSTALL}/bin:${PATH}
    export LD_LIBRARY_PATH=${OCR_INSTALL}/lib:${LD_LIBRARY_PATH}
    export OCR_CONFIG=${OCR_INSTALL}/config/default.cfg
```

# HCLIB Install Instructions

HCLIB relies on autotools as a build system. You can either invoke `configure` 
and `make` directly if familiar with, or use the provided installation script. 
Note that the installation script runs the whole tool chain and is overkill 
to recompile the library when updating code.


## Installation with OCR backend

0.  Make sure OCR is specifically built and installed for HCLIB support

1.  Clone the HCLIB repository (if not done during OCR installation)
```
    git clone https://github.com/habanero-rice/hclib.git
```

    The folder where hclib is cloned is now referred to as 'HCLIB_SRC_ROOT'
```
    export HCLIB_SRC_ROOT=${PWD}/hclib.git
```

2.  Build HCLIB
```    
    cd ${HCLIB_SRC_ROOT}
    ./install.sh
```

    By default, HCLIB is installed under '${HCLIB_SRC_ROOT}/hclib-install/'

3.  Setup your environment:
Defining the 'HCLIB_ROOT' environment variable and updating the 'LD_LIBRARY_PATH'
is optional but comes handy when writing compiler command lines.
```
    export HCLIB_ROOT=$PWD/hclib-install

    export LD_LIBRARY_PATH=${HCLIB_ROOT}/lib:${LD_LIBRARY_PATH}

    export OCR_CONFIG=${OCR_INSTALL}/config/default.cfg
```

Note: OCR_CONFIG points to the OCR configuration file from which you can set the number of OCR workers.

You're done ! Check the [tutorial](tutorial.html) for an overview of HClib examples.

## Installation with additional phaser support

HClib can optionally link with a 'phaserLib' installation to enable utilization
of the phaser construct in HClib source files.

1.  Set the PHASERLIB_ROOT environment variable to point to the path for an installation 
of 'phaserLib'

```    
    export PHASERLIB_ROOT=/path/to/phaserLib/install
```

2.  Invoking HClib's install script with PHASERLIB_ROOT defined automatically makes 
HClib link with the phaserLib library.
```
    ./install.sh
```

If you're not relying on the install script, you need to invoke configure with 
the '--with-phaser=/path/to/phaserLib/install' option to specify the location of
a 'phaserLib' installation.