
# Basic concepts

During linking, several common concepts are represented internally by common classes.

The basic unit of linking is a module.
This corresponds roughly to an object file.
A Module can contain data blocks, names and references to other names.
A sequence of data is contained in a Section.
Aside from their contents, Sections have names, binary flags and properties.
Sections can be zero-filled, or contain explicit data.

Any symbolic name is represented by a SymbolName.
A SymbolName can either be defined or undefined.
When a SymbolName is defined, it will have as value a Location, which is an offset inside a Section.
A Location may also be independent of a Section, such as constant values or absolute addresses, in which case a null pointer will replace the section.

Relocations are provided to represent data whose value depends on some other symbol.
Aside from its size and type, Relocations have a source, a target and a reference.
A source is a Location where the value to be replaced appears.
A target contains the SymbolName (or Location) whose final value should be placed there.
A reference is another SymbolName or Location that has to be subtracted from the target value.
References permit encoding self-relative relocations as well as x86 segment relative relocations (they roughly correspond to what Intel Object Module Format files call frames).

One of the main goals of a linker is to produce an output object.
This requires converting a sequence of Modules into a format usable to generate a binary file.
To simplify the handling of these objects, a new set of classes are introduced to represent the output structures.

Sections are grouped into Segments representing a segment of memory that can be addressed as a single memory block.
Segments can hold non-zero-filled as well as zero-filled Sections.
They can also have a name and flags.
These are not related to x86 segments, but are closer to the ELF concept of a segment.

Relocations usually get resolved to some value, typically the address of the relocation target.
Some output formats require knowledge of which segment the address belongs to.
To simplify handling this information, a resolved Relocation returns a Resolution object.
Aside from a value, Resolution objects also have a reference to the target as well as the reference Segments.
If the two are the same, they are replaced with nullptr.

Any file format is represented by a subclass of the Format class.
A Format class is typically used to read and write binary files, and to handle their contents in memory.
An InputFormat class is capable of converting a binary file already read into a single or multiple Modules.
An OutputFormat class is capable of converting an already linked Module into an output file.
Not all Format classes are InputFormat or OutputFormat, however all Format classes are in principle capable of reading or writing files, provided the feature is actually implemented.

# The linking process

First, the linker reads in the object files and generates an internal representation for them.
This internal representation is supposed to be thorough enough to be able to output a mostly identical binary.

Then the linker converts the internal representation into a common Module format.
Modules are independent of the input file format, and they contain sections, symbol definitions and relocation information.
Some input formats lack certain features (for example, ELF lacks any x86 segmentation support, and a.out lacks arbitrary sections).
These are extended by special symbol names, typically of the form $$DIRECTIVE$PARAMETERS.
See the section on *Special symbols* for more details.
When converted to the common Module format, these symbol names will be interpreted as the feature they are bridging.

Following this is the process of linking in modules and resolving internal references.
Any regular module is automatically included, whereas library modules only get included if their symbols are referenced in relocations by other, already included modules.
This is done recursively, until all referenced symbols appearing in any included module are resolved.
The modules are then concatenated into a single module.
Sections with the same name get merged, and any symbol definition or relocation gets displaced to its new offset values.
Finally, any common symbols are allocated space.

The next step is dependent on the output format.
First, the output format is provided a set of command line options, parsed by the linker.
A linker script is used to organize sections into logical segments.
These are not directly related to x86 segmentation, but rather to sequences of bytes that appear in a group in the file and the memory.
As part of this process, sections also get assigned a base address, and possibly extended to satisfy alignment requirements, among others.
To accomodate the requirements of x86 segmentation, a special script language was developed.
See *Linker script syntax* for more details.

At this point, the final task is up to the output format.
Relocations are resolved, or get stored in the file as external references.
Any header structures are layed out and the member fields of the file instance are populated.

Before writing the data to a file, value sanitization is executed, typically to update offset and count values.
The structure of the linker allows a programmer to directly generate a file of this file format, without using any linking capabilities.
This step allows the programmer to finalize any automatic values for consistency, and it is also invoked during the linking process.

Finally, the produced image is written to the output.
The name of the output file is either provided by the user, or as a fallback, by the output format, which can also determine the default file extension.
Some output formats will produce multiple files.
One of the more complicated examples is the Classic Macintosh output, which can generate one or more of a resource fork, an empty data fork, either an AppleSingle or AppleDouble file, and/or a MacBinary container.

# Special symbols

## x86 segmentation

To accomodate for x86 segmentation, a couple of new concepts need to be introduced.

On the 8086, any segmented address has two parts: a segment value and an offset value.
To obtain the actual linear address, one needs to multiply the segment value by 16 and add the offset value to it.
Groups of 16 bytes are referred to as paragraphs, and the segment value will also be called its paragraph value.
Because of this calculation, different segmented addresses can represent the same linear address.

When defining symbols, not only do they have an address, but also a preferred segment.
While the same symbol can usually be accessed with a non-preferred segment, this could lead to issues when the symbol overflows the 64 KiB segment boundary.
The linker will assign to each Section an x86 segment base address, and any symbol defined in this Section will have that x86 segment as its preferred segment.

By default, every symbol refers to its offset value, within its own segment.
Special symbol names are used to provide segmentation support for formats not supporting it by default.

* $$SEG$section_name refers to the paragraph value of the section section_name.

* $$SEGOF$symbol_name refers to the paragraph value of its preferred segment.

* $$SEGAT$symbol_name references the paragraph value at a specific symbol.
This lets the programmer reference intermediate segments.

* $$SECT$section_name and $$SECTSEG$section_name reference the starting address or paragraph value of a section.

* $$WRTSEG$symbol_name$section_name gives the offset of a symbol relative to the segment base of a separate section.

* $$SEGDIF$target_section_name$reference_section_name gives a paragraph distance between the paragraph values of two sections.

Many of these symbols can also be used as protected mode segment selector values.
Note however that the set of options is limited by what the output format supports.
A safe bet is to only use the $$SEGOF$ special symbol, since that will be available in segmented systems.

## Imported and exported names

TODO

## Other special symbols

TODO

# Linker script syntax

The linker script is partly inspired by the GNU linker script format.
However it has only a crucial subset of its features, extended and with support for x86 style segmentation.

Like for the GNU linker, a linker script is a collection of assignments, segment definitions and other directives.
A typical segment definition looks like as follows:

	".code" {
		all ".code";
	};

The first identifier defines the name of the segment.
This is followed by a predicate that collects specific sections from the input files.
In this case, all the sections whose name is ".code" is included, one after the other.
Predicates are much more powerful than simply checking for section names.
They can include logical operators and simple functions.
Sections can be checked for their properties, such as whether they are readable (`read`), writable (`write`), executable (`execute`) and zero filled (`zero`).
It is also possible to check that the section name ends with a specific string, using the `suffix ".bss"` syntax.
For example, the following code will include all non-zero data sections:

	".data" {
		all not execute and not zero;
	};

A segment may include multiple predicates, as well as actions.
At each statement, the position pointer is altered to accomodate the statement.
For example, if a section is included, the position pointer will be incremented by the size of the included section.
As an action, the `at` operator changes the current location of the linker pointer.
This can be used to place sections at specific offsets.

	".code" {
		at 256;
		all ".code";
	};

Though usually, a more complex expression would be provided, such as

	".image" {
		all ".code";
		at here + 256;
		all ".data";
	};

or even

	".image" {
		all ".code";
		at align(here, 256);
		all ".data";
	};

This one has a more convenient form of using the `align` directive:

	".image" {
		all ".code";
		align 256;
		all ".data";
	};

Note that these instructions also expand the size of the previous section, since segments have to be contiguous and are not allowed to have holes inside them.

Logical segments are not the same as x86 segments.
Instead, sections can be assigned to x86 segments, and placed at specific offsets.
To make linking easier, this is represented by the concept of a base pointer.
The base pointer refers to the offset 0 within the current x86 segment.
While the position pointer gets incremented at any addition of data, the base pointer by default does not change.
For any section included in the linking process, the current base pointer will determine where the x86 segment begins.
The `base` action can alter this.

For example, take a small memory model DOS MZ .EXE executable.
Such a file uses two x86 segments: a code segment and a data segment.
The code segment begins where the image starts, but the data segment typically starts inside the image, before the first data section.
To produce such a file, the following script could be provided:

	".image" {
		all ".code";
		align 16;
		base here;
		all ".data";
	};

Note that the position pointer needs to be aligned to a paragraph boundary before changing the base pointer, otherwise we would get an illegal x86 segment base.

As another example, DOS .COM files have only a single segment, but the first byte in the image is at offset 0x100 from the start of the default segment.
This could be represented as follows:

	".image" {
		base here - 0x100
		all ".code" or ".data";
	};

The convenience of using predicates to include multiple sections seems to diminish if we want all sections to start their own x86 segments.
However, syntacticall predicates can also be terminated by actions, which will take place before the section is attached:

	".data" {
		all read align 16 base here;
	}

This will concatenate all readable sections, but only after aligning them to paragraph boundaries, and setting their x86 segments to start at their first byte.

Not all the features of the linker script are listed here.
Most of it is coded in script/parser.yy and linker/segment_manager.cc.
However, other features included are functions on symbols, such as `base of "identifier"`, minimum and maximum functions, checking for custom section flags, and segment templates.
Segment templates replace the segment definition identifier with the `for` keyword and a predicate.
During execution, each section that fits the predicate (and has not yet been assigned to a segment) will create a corresponding segment with the same name (by default, the section is not assigned to it, this should be the first statement inside the segment definition).

As a final note, the quotation marks around identifiers can usually be avoided.
However, the linker script language contains many reserved words, and it is recommended to always put identifiers in quotation marks to avoid an accidental conflict.

