# RetroLinker, a linker for multiple 8-bit, 16-bit and 32-bit executable formats

This program grew out of a desire to be able to process and generate old object and executable file formats.
The initial targets were 16-bit 8086 operating systems, in particular CP/M-86, MS-DOS, Windows and ELKS (16-bit port of Linux).
It was also a goal to be able to parse Intel OMF object files as well as as86 object files, to provide a drop-in replacement for the WATCOM and dev86 compilers.
Not all of these goals have been achieved yet.

The project is in an alpha stage, however it works sufficiently well for the author's every day purposes.
It is released in its current state as-is, in the hopes that others might find it useful for their own work.
The most up to date documentation is found in the [generated doxygen documents](https://binarymelodies.github.io/RetroLinker/).

# Acknowledgements

The project is heavily inspired by the dev86 project, the vasm assembler, the Watcom compiler and the Retro68 project.
Multiple sources were consulted in the research and documentation for the linker, for details see [the descriptions for the old executable formats](https://github.com/BinaryMelodies/OldExecutableFormats).

# Requirements

To compile the linker and dumper, an up-to-date C++ compiler is necessary, as well as Flex and GNU Bison.
To generate the assembly tests, a Python interpreter is required, as well as cross compilers for the ia16, i386, m68k, pdp11, arm and z8k backends.

# Usage

The linker is a very basic standalone linker that can link several object files into an executable of several formats.
For a detailed list of formats, see the doxygen documentation, or mainpage.dox.
However, among others, the following targets are supported:

* CP/M-80
* MP/M-80 (`.prl` executables)
* CP/M-86
* CP/M-68K
* CP/M-8000
* MS-DOS
* Multitasking MS-DOS (`NE` executables)
* Windows (16-bit only)
* OS/2 (16-bit and 32-bit)
* ELKS binaries (16-bit Linux port to the 8086)
* multiple DOS extenders
* Atari TOS and GEMDOS
* AmigaOS (68000 only)
* Classic Mac OS (68000 only, no CFM support)
* Human68k

The linker can only read ELF, COFF and a.out object files.
To make up for missing features, the linker is extended to support segmentation (for 8086), resources (Mac OS), etc.
This information needs to be mapped into the object file.
For details, see the example files under the directory tests.

# Other features

The linker has a separate mode called the dumper, it reads in some binary file formats and displays their contents on the screen.
It can be invoked by providing the command line flag `--dump`.
It is not yet feature complete, and a fallback tool dump.py is provided.

A set of tests are provided under the tests folder.
Since the same project has to be compiled for multiple architectures, the standard make utility was felt to be insufficient.
A new tool called bake was developed in Python, where recipes can be defined for compilation, and then a list of targets specified.

Some preliminary unit tests have also been developed.

