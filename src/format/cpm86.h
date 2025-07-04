#ifndef CPM86_H
#define CPM86_H

#include <array>
#include <map>
#include <set>
#include <vector>
#include "../common.h"
#include "../linker/module.h"
#include "../linker/options.h"
#include "../linker/segment.h"
#include "../linker/segment_manager.h"
#include "../linker/writer.h"
#include "../dumper/dumper.h"

namespace DigitalResearch
{
	/**
	 * @brief The native file format for 8086 based CP/M derived operating systems, including FlexOS 186/286
	 *
	 * The current implementation supports 3 memory model formats:
	 * - 8080 model (also called tiny model for MS-DOS programs)
	 * - Small model
	 * - Compact model
	 * - Preliminary provisions for FlexOS memory models
	 *
	 * The format was used on a multitude of Digital Research operating systems, from CP/M-86 and DOS Plus to Multiuser DOS, as well as FlexOS.
	 * It supports many special features, such as shared libraries and attachable residential system extensions.
	 * These are not currently implemented.
	 */
	class CPM86Format : public virtual Linker::SegmentManager
	{
	public:
		/* * * General members * * */

		/**
		 * @brief A representation of segment group within the executable
		 *
		 * CMD files begin with up to 8 9-byte group descriptors (with two special group descriptors following for FlexOS 286)
		 * that identify segment groups within the file. Each segment group has a type (code, data, stack, etc.), a size of stored
		 * data, a minimum and maximum memory requirement, as well an optional loading address, all of which are expressed in paragraphs
		 * (16-byte units). The UNIX text, data, bss segments are usually stored as a single code and data segment, with the data segment
		 * containing the bss as part of its minimum memory requirement.
		 *
		 * Aside from the typical segment groups, some system versions (particularly FlexOS 286) support certain specialized segment
		 * groups. Relocations are usually stored in the file at a 128-byte boundary, but their presence can be represented as a special
		 * segment group.
		 *
		 * Another specialized segment group allows executables to reference shared runtime libraries. The segment descriptor is stored
		 * at a special offset, and the segment group stores a sequence of library identifiers (name, version, flags), together with a
		 * number of relocations referencing the segment base of the shared library.
		 *
		 * It is also possible to include a segment group for faster loading, however this is not yet fully supported.
		 */
		class Descriptor
		{
		public:
			/** @brief Group type, stored as the first byte of the descriptor */
			enum group_type
			{
				/** @brief This group type is not the type of an actual group. Instead, it signals the end of the group descriptor table */
				Undefined,
				/** @brief A group containing executable instructions, the starting segment will be loaded into CS. Unlike SharedCode, this segment cannot be shared between different instances */
				Code = 1,
				/** @brief A group containing data, the starting segment will be loaded into DS */
				Data,
				/** @brief A group containing data, the starting semgnet will be loaded into ES */
				Extra,
				/** @brief A group describing the stack data. Unlike the other associated segments, SS does not point to it at the start of code execution */
				Stack,
				/** @brief A group containing data */
				Auxiliary1,
				/** @brief A group containing data */
				Auxiliary2,
				/** @brief A group containing data */
				Auxiliary3,
				/** @brief A group containing data. The group type was later repurposed to represent the relocations in a file. This can refer to either */
				Auxiliary4,
				/** @brief A group containing executable instructions, the starting segment will be loaded into CS. This segment can be shared between different instances */
				SharedCode,

				/** @brief (FlexOS 286 only) The group containing the relocations. Not required, since the offset to the relocations is also stored at offset 0x7D in the header, in 128-byte units */
				Fixups = Auxiliary4, /* FlexOS 286 only */
				/** @brief (FlexOS 286 only) The fast load group descriptor is stored at offset 0x51 in the header, and the group data appears before the other groups. Not implemented, no documentation */
				FastLoad = 0xFE,
				/** @brief (FlexOS 286 only) The shared runtime library group descriptor is stored at offset 0x48 in the header, and the group data appears before the other groups. Not tested */
				Libraries = 0xFF,

				/** @brief A group containing data, used when it is clear this is an auxiliary group */
				ActualAuxiliary4 = 0x100 | Auxiliary4,
				/** @brief (FlexOS 286 only) The group containing the relocations, used when it is clear this is a fixup group */
				ActualFixups = 0x200 | Fixups,
			};
			/** @brief The type of the group */
			group_type type = Undefined;
			/** @brief Size of the group, as stored on disk, in 16-byte paragraphs */
			uint16_t size_paras = 0;
			/** @brief Load segment address of the group, or 0 if it can be relocated. Not to be used outside of system drivers */
			uint16_t load_segment = 0;
			/** @brief Minimum required size of the group, when loaded into memory, in 16-byte paragraphs */
			uint16_t min_size_paras = 0;
			/** @brief Maximum required size of the group, when loaded into memory, in 16-byte paragraphs */
			uint16_t max_size_paras = 0;
			/**
			 * @brief Offset to image in file
			 *
			 * This value is not actually stored on disk. Instead, it can be determined from the lengths of the segments preceding
			 * it in the descriptor arrays. The first group is stored at offset 0x80, the next one at the paragraph boundary, and so on.
			 */
			uint32_t offset = 0;
			/** @brief The actual binary image of the group */
			std::shared_ptr<Linker::Image> image = nullptr;
			/** @brief Set to true if a supplementary 256 bytes of zeros are required. When generating image, it is easier to just insert 256 bytes of 0 instead of modifying the image. This is not required when storing a file loaded from disk */
			bool attach_zero_page = false;

			virtual void Clear();

			~Descriptor()
			{
				Clear();
			}

			/**
			 * @brief Returns the size of the segment group in 16-byte paragraphs
			 */
			virtual uint16_t GetSizeParas(const CPM86Format& module) const;

			void ReadDescriptor(Linker::Reader& rd);

			/**
			 * @brief Sets up any values before it can be written to file
			 */
			void Prepare(CPM86Format& module);

			void WriteDescriptor(Linker::Writer& wr, const CPM86Format& module) const;

			virtual void WriteData(Linker::Writer& wr, const CPM86Format& module) const;

			std::string GetDefaultName() const;

			virtual void ReadData(Linker::Reader& rd, const CPM86Format& module);
		};

		/**
		 * @brief Represents the location of a 16-bit words that needs to be relocated
		 */
		struct relocation_source
		{
			/** @brief The segment group number that contains the required relocation */
			number_t segment;
			/** @brief The offset to the word within the segment group */
			offset_t offset;

			relocation_source()
				: segment(0), offset(0)
			{
			}

			relocation_source(size_t segment, offset_t offset)
				: segment(segment), offset(offset)
			{
			}

			bool operator==(const relocation_source& other) const;
			bool operator<(const relocation_source& other) const;
		};

		typedef relocation_source relocation_target;

		/**
		 * @brief Represents a single relocation
		 */
		struct Relocation
		{
			uint8_t source = 0;
			uint16_t paragraph = 0;
			uint8_t offset = 0;
			uint8_t target = 0;

			Relocation()
			{
			}

			Relocation(uint8_t source, uint16_t paragraph, uint16_t offset, uint8_t target)
				: source(source), paragraph(paragraph), offset(offset), target(target)
			{
			}

			Relocation(relocation_source source, uint8_t target = 0)
				: source(source.segment), paragraph(source.offset >> 4), offset(source.offset & 0xF), target(target)
			{
			}

			operator bool() const;

			void Read(Linker::Reader& rd, CPM86Format& module, bool is_library = false);

			void Write(Linker::Writer& wr) const;

			relocation_source GetSource() const;
		};

		/**
		 * @brief Represents an attached RSX file (residential system extension)
		 *
		 * Any CMD file may contain a number of RSX attachments, either stored within the same file, or on disk
		 */
		struct rsx_record
		{
			/** @brief The host filename of the RSX file, used only for writing */
			std::string rsx_file_name;
			/** @brief The filename of the RSX file, 8-byte long */
			std::string name;
			/**
			 * @brief The offset to the attached RSX file, in 128-byte units
			 *
			 * The special value 0x0000 represents that the RSX file is stored separately and must be loaded dynamically.
			 * The special value 0xFFFF does not belong to an actual RSX file, but signals the end of an RSX record table.
			 */
			uint16_t offset_record = 0;
			static constexpr uint16_t RSX_TERMINATE = 0xFFFF;
			static constexpr uint16_t RSX_DYNAMIC = 0x0000;
			/**
			 * @brief A reference to the stored module, either parsed or a raw block of data
			 */
			std::shared_ptr<Linker::Image> contents;

			void Clear();

			void Read(Linker::Reader& rd);

			void ReadModule(Linker::Reader& rd);

			void Write(Linker::Writer& wr) const;

			void WriteModule(Linker::Writer& wr) const;

			offset_t GetFullFileSize() const;
			void SetOffset(offset_t new_offset);

			void Dump(Dumper::Dumper& dump) const;
		};

		/**
		 * @brief (FlexOS only) A shared runtime library identifier
		 *
		 * A library is identified by its name, version number and flags. This identifier is stored in the header of a library,
		 * and then used in executables to reference the library.
		 */
		struct library_id
		{
			/** @brief The name of the library, 8 characters long */
			std::string name;
			/** @brief The major version of the library */
			uint16_t major_version = 0;
			/** @brief The minor version of the library */
			uint16_t minor_version = 0;
			/** @brief System specific flags, undocumented */
			uint32_t flags = 0x11010000;

			library_id() { }

			library_id(std::string name, uint16_t major_version, uint16_t minor_version)
				: name(name), major_version(major_version), minor_version(minor_version)
			{
			}

			library_id(std::string name, uint16_t major_version, uint16_t minor_version, uint32_t flags)
				: name(name), major_version(major_version), minor_version(minor_version), flags(flags)
			{
			}

			library_id(const library_id& lib) = default;

			/** @brief Converts a string in the format "FILENAME.MAJ.MIN.FLAGS" to an identifier */
			static library_id ParseNameAndVersion(std::string name_and_version);

			bool operator ==(const library_id& other) const;

			void Write(Linker::Writer& wr) const;

			void Read(Linker::Reader& rd);
		};

		/**
		 * @brief (FlexOS 286 only) A shared runtime library entry in the shared runtime library group
		 *
		 * Executables that must be linked at runtime contain a sequence of library identifiers, together with a count
		 * of relocations to the library segment base.
		 */
		struct library : public library_id
		{
			/** @brief The set of relocations */
			std::vector<Relocation> relocations;
			/** @brief Relocation count */
			uint16_t relocation_count = 0;

			/** @brief (FASTLOAD only) First selector that references this library */
			uint16_t first_selector = 0;
			/** @brief (FASTLOAD only) Unknown */
			uint16_t unknown = 0;

			/** @brief Unordered relocations, only used during binary generation */
			std::set<relocation_source> relocation_targets;

			library()
			{
			}

			library(std::string name, uint16_t major_version, uint16_t minor_version)
				: library_id(name, major_version, minor_version)
			{
			}

			library(std::string name, uint16_t major_version, uint16_t minor_version, uint32_t flags)
				: library_id(name, major_version, minor_version, flags)
			{
			}

			library(const library_id& lib)
				: library_id(lib)
			{
			}

			void Write(Linker::Writer& wr) const;

			void WriteExtended(Linker::Writer& wr) const;

			void Read(Linker::Reader& rd);

			void ReadExtended(Linker::Reader& rd);
		};

		/** @brief A special descriptor to represent the group for imported shared runtime libraries */
		class LibraryDescriptor : public Descriptor
		{
		public:
			/** @brief The shared runtime libraries to be imported */
			std::vector<library> libraries;

			void Clear() override;

			/**
			 * @brief Sets up any values before it can be written to file
			 */
			void Prepare(CPM86Format& module);

#if 0
			/**
			 * @brief Support the newer POSTLINK format
			 */
			bool IsFastLoadFormat() const;
#endif

			library& FetchImportedLibrary(std::string name_dot_version);

			library& FetchImportedLibrary(library_id id);

			uint16_t GetSizeParas(const CPM86Format& module) const override;

			void WriteData(Linker::Writer& wr, const CPM86Format& module) const override;

			void ReadData(Linker::Reader& rd, const CPM86Format& module) override;
		};

		/** @brief (FlexOS 286 only) The fast loading group (unimplemented) */
		class FastLoadDescriptor : public Descriptor
		{
		public:
			/** @brief Maximum allowed LDT entries */
			uint16_t maximum_entries = 0;
			/** @brief First free entry in LDT after last filled entry */
			uint16_t first_free_entry = 0;
			/** @brief The index base */
			uint16_t index_base = 0;
			/** @brief First used index */
			uint16_t first_used_index = 0;

			struct ldt_descriptor
			{
				/** @brief Intel 286 descriptor limit */
				uint16_t limit = 0;
				/** @brief Intel 286 descriptor base address, 24-bit */
				uint32_t address = 0;
				/** @brief Group the segment belongs to */
				uint8_t group = 0;
				uint16_t reserved = 0;

				void Read(Linker::Reader& rd);

				void Write(Linker::Writer& wr) const;
			};

			std::vector<ldt_descriptor> ldt;

			void Clear() override;

			uint16_t GetSizeParas(const CPM86Format& module) const override;

			void WriteData(Linker::Writer& wr, const CPM86Format& module) const override;

			void ReadData(Linker::Reader& rd, const CPM86Format& module) override;
		};

		/**
		 * @brief A .cmd file may contain up to 8 descriptors that describer the segment groups
		 */
		std::array<Descriptor, 8> descriptors;
		/**
		 * @brief FlexOS 286 defines a shared runtime library group at offset 0x48
		 */
		LibraryDescriptor library_descriptor;
		/**
		 * @brief FlexOS 286 defines a fast load segment (unknown name) at offset 0x51
		 */
		FastLoadDescriptor fastload_descriptor;
		/**
		 * @brief FlexOS 286 library identifier (for libraries) at offset 0x60
		 */
		library_id lib_id;
		/**
		 * @brief The sequence of intramodule relocations
		 */
		std::vector<Relocation> relocations;
		/**
		 * @brief Offset of relocation records, stored in 128 byte units at offset 0x7D
		 */
		uint32_t relocations_offset = 0;
		/**
		 * @brief Represents a list of attached RSX modules
		 */
		std::array<rsx_record, 8> rsx_table;
		/**
		 * @brief The actual RSX table, stored in 128 byte units at offset 0x7B
		 */
		uint32_t rsx_table_offset = 0;
		/**
		 * @brief Execution flags, stored at offset 0x7F
		 */
		uint8_t flags = 0;
		/**
		 * @brief CP/M specific flags used by Concurrent DOS, stored at offset 0x7A
		 */
		uint8_t cpm_flags = 0;

		/**
		 * @brief The start of the image within the file, typically 0 except for embedded modules, usually for embedded RSX files
		 */
		uint32_t file_offset = 0;
		/** @brief The full size of the image */
		mutable offset_t file_size = offset_t(-1);

		enum
		{
			/** @brief Set when relocations are present, indicates that the executable is relocatable and needs fixing up before executing */
			FLAG_FIXUPS = 0x80,
			/** @brief Set when the software expects the system to allocate 8087 resources, but it can emulate the missing 8087 coprocessor insturctions if needed */
			FLAG_OPTIONAL_8087 = 0x40,
			/** @brief Set when the software only runs if an 8087 is present. The system will allocate 8087 resources */
			FLAG_REQUIRED_8087 = 0x20,
			/** @brief Set for residential system extensions (RSX files) */
			FLAG_RSX = 0x10,
			/** @brief Set when the program uses direct video access. Such programs cannot execute in the background */
			FLAG_DIRECT_VIDEO = 0x08,
			/** @brief Program uses record locking functions of MP/M-86 instead of those of Concurrent CP/M-86 or Concurrent DOS */
			FLAG_MPMLOCK = 0x04,
			/** @brief Set when the program cannot run in banked memory */
			FLAG_NO_BANKING = 0x02,
		};

		enum
		{
			CPM_FLAG_F1 = 0x01,
			CPM_FLAG_F2 = 0x02,
			CPM_FLAG_F3 = 0x03,
			CPM_FLAG_F4 = 0x04,
		};

		void Clear() override;

		/** @brief Describes the number and type of segment groups, as well as the target operating system */
		enum format_type
		{
			/**
			 * @brief Unspecified
			 */
			FORMAT_UNKNOWN,
			/**
			 * @brief CP/M-86 format, only a single non-shared code group is present, with optional relocations and attached RSX system extensions
			 */
			FORMAT_8080,
			/**
			 * @brief CP/M-86 format, only code (possibly shared) and a separate data group is present, also used for medium model
			 */
			FORMAT_SMALL,
			/**
			 * @brief CP/M-86 format, each non-executable section will have its own group, including stack and up to 4 auxiliary groups, also used for large model
			 */
			FORMAT_COMPACT,
			/**
			 * @brief FlexOS 286 format, running in protected mode, possible SRTL shared libraries (untested)
			 */
			FORMAT_FLEXOS,
			/**
			 * @brief FlexOS 286 fastload format as generated by postlink, with an LDT image in file (unfinished)
			 */
			FORMAT_FASTLOAD,
		};
		/** @brief Format of file to generate */
		format_type format;

		enum application_type
		{
			/** @brief Unspecified */
			APPL_UNKNOWN,
			/** @brief CP/M-86 .cmd executable file */
			APPL_CMD,
			/** @brief CP/M-86 .rsx resident system extension file */
			APPL_RSX,
			/** @brief MP/M-86 or Concurrent CP/M-86 .rsp resident system process file (unsupported) */
			APPL_RSP,
			/** @brief MP/M-86 .mpm system module file (unsupported) */
			APPL_MPM,
			/** @brief Concurrent CP/M-86 .con system module file (unsupported) */
			APPL_CON,
			/** @brief CP/M-86 .ovr overlay (unsupported) */
			APPL_OVR,
			/** @brief FlexOS 286 .286 executable file (limited support) */
			APPL_286,
			/** @brief FlexOS 286 .SRL shared runtime library (limited support) */
			APPL_SRL,
		};
		/** @brief Target application type */
		application_type application;

		CPM86Format(format_type format = FORMAT_UNKNOWN, application_type application = APPL_UNKNOWN)
			: format(format), application(application)
		{
		}

		~CPM86Format()
		{
			Clear();
		}

		uint16_t GetRelocationSizeParas() const;

		size_t CountValidGroups();

		number_t FindSegmentGroup(unsigned group) const;

		void CheckValidSegmentGroup(unsigned group);

		bool IsFastLoadFormat() const;
		bool IsSharedRunTimeLibrary() const;

		void ReadRelocations(Linker::Reader& rd);

		void WriteRelocations(Linker::Writer& wr) const;

		offset_t MeasureRelocations();

		void ReadFile(Linker::Reader& rd) override;

		offset_t ImageSize() const override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;

		offset_t GetFullFileSize() const;

		void Dump(Dumper::Dumper& dump) const override;

		void CalculateValues() override;

		/* * * Writer members * * */

		class CPM86OptionCollector : public Linker::OptionCollector
		{
		public:
			Linker::Option<bool> noreloc{"noreloc", "Suppress generating relocations"};
			Linker::Option<bool> sharedcode{"sharedcode", "Make the code segment shared (descriptor type 9)"};
			Linker::Option<std::optional<std::vector<std::string>>> rsx_file_names{"rsx", "List of filenames to append as Resident System Extensions (CP/M)"};
			Linker::Option<bool> fixupgroup{"fixupgroup", "Generates group descriptor for fixups (FlexOS)"};
			Linker::Option<std::optional<std::string>> srtl{"srtl", "Generate shared run-time library, parameter: <library name>.<major version>.<minor version>[.<optional flags as hexadecimal value>] (TODO: untested)"}; // TODO
			Linker::Option<std::string> x87{"8087", "Sets 8087 usage, valid values: off/on/auto", "off"};
			Linker::Option<bool> directvideo{"directvideo", "Direct video access"};
			Linker::Option<bool> mpmlock{"mpmlock", "MP/M-style record handling (Concurrent DOS)"};
			Linker::Option<bool> nobank{"nobank", "Cannot run in banked memory"};
			Linker::Option<bool> f1{"f1", "CP/M F1 flag (Concurrent DOS)"};
			Linker::Option<bool> f2{"f2", "CP/M F2 flag (Concurrent DOS)"};
			Linker::Option<bool> f3{"f3", "CP/M F3 flag (Concurrent DOS)"};
			Linker::Option<bool> f4{"f4", "CP/M F4 flag (Concurrent DOS)"};

			CPM86OptionCollector()
			{
				InitializeFields(noreloc, sharedcode, rsx_file_names, fixupgroup, srtl, x87, directvideo, mpmlock, nobank, f1, f2, f3, f4);
			}
		};

		/** @brief Flag indicating that the code group is shared, not possible in 8080 format */
		bool option_shared_code = false;
		/** @brief Flag to indicate that relocations must be suppressed */
		bool option_no_relocation = false;
		/** @brief On FlexOS, relocations can be part of its own group */
		bool option_generate_fixup_group = false;

		/** @brief Represents the memory model of the running executable, which is the way in which the segments are set up during execution */
		enum memory_model_t
		{
			MODEL_DEFAULT,
			/** @brief CS=DS=SS=ES, only possible in 8080 mode */
			MODEL_TINY,
			/** @brief DS=SS=ES, only possible in 8080 or small mode */
			MODEL_SMALL,
			/** @brief DS!=SS!=ES */
			MODEL_COMPACT,
		};
		/** @brief Memory model of generated code, determines how the offsets are calculated within a segment group */
		memory_model_t memory_model = MODEL_SMALL;

		bool FormatSupportsSegmentation() const override;

		bool FormatIs16bit() const override;

		bool FormatIsProtectedMode() const override;

		bool FormatSupportsLibraries() const override;

		unsigned FormatAdditionalSectionFlags(std::string section_name) const override;

		std::vector<std::shared_ptr<Linker::Segment>>& Segments();

		/** @brief Assignment from each segment to the segment group type number (1: code, 2: data, etc.) */
		std::map<std::shared_ptr<Linker::Segment>, Descriptor::group_type> segment_types;

		/** @brief Determine the segment group type number, must be called before accessing segment_types */
		void AssignSegmentTypes();

		unsigned GetSegmentNumber(std::shared_ptr<Linker::Segment> segment);

		static std::vector<Linker::OptionDescription<void>> MemoryModelNames;

		std::vector<Linker::OptionDescription<void>> GetMemoryModelNames() override;

		void SetModel(std::string model) override;

		std::shared_ptr<Linker::OptionCollector> GetOptions() override;

		void SetOptions(std::map<std::string, std::string>& options) override;

		std::unique_ptr<Script::List> GetScript(Linker::Module& module);

		void Link(Linker::Module& module);

		/** @brief Creates the LDT image for a fastload FlexOS binary */
		void BuildLDTImage(Linker::Module& module);

		void ProcessModule(Linker::Module& module) override;

		void GenerateFile(std::string filename, Linker::Module& module) override;

		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module, std::string filename) const override;
	};

}

#endif /* CPM86_H */
