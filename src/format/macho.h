#ifndef MACHO_H
#define MACHO_H

#include <sstream>
#include <vector>
#include "../common.h"
#include "../dumper/dumper.h"
#include "../linker/module.h"
#include "../linker/segment_manager.h"
#include "../linker/reader.h"

// TODO: incomplete, untested

namespace MachO
{
	/**
	 * @brief Mach/NeXTSTEP/Mac OS X (macOS) file format
	 *
	 * Originally developed for the Mach kernel, it has been adopted by other UNIX systems based on the Mach kernel, including NeXTSTEP and macOS.
	 */
	class MachOFormat : public virtual Linker::SegmentManager
	{
	public:
		::EndianType endiantype = ::EndianType(0);
		size_t wordsize;
		enum cpu_type
		{
			CPU_VAX = 1,
			CPU_ROMP = 2,
			CPU_NS32032 = 4,
			CPU_NS32332 = 5,
			CPU_MC680x0 = 6,
			CPU_X86 = 7,
			CPU_MIPS = 8,
			CPU_NS32352 = 9,
			CPU_MC98000 = 10,
			CPU_HPPA = 11,
			CPU_ARM = 12,
			CPU_MC88000 = 13,
			CPU_SPARC = 14,
			CPU_I860 = 15,
			CPU_I860_LE = 16,
			CPU_ALPHA = 16,
			CPU_RS6000 = 17,
			CPU_POWERPC = 18,
		};
		cpu_type cpu = cpu_type(0);
		uint32_t cpu_subtype = 0;
		uint32_t file_type = 0;
		uint32_t commands_size = 0;
		uint32_t flags = 0;

		class LoadCommand
		{
		public:
			enum command_type : uint32_t
			{
				REQ_DYLD = 0x80000000,

				SEGMENT = 1,
				SYMTAB = 2,
				SYMSEG = 3, // obsolete
				THREAD = 4,
				UNIXTHREAD = 5,
				LOADFVMLIB = 6,
				IDFVMLIB = 7,
				IDENT = 8, // obsolete
				FVMFILE = 9, // internal
				PREPAGE = 10, // internal
				DYNSYMTAB = 11,
				LOAD_DYLIB = 12,
				ID_DYLIB = 13,
				LOAD_DYLINKER = 14,
				ID_DYLINKER = 15,
				PREBOUND_DYLIB = 16,
				ROUTINES = 17,
				SUB_FRAMEWORK = 18,
				SUB_UMBRELLA = 19,
				SUB_CLIENT = 20,
				SUB_LIBRARY = 21,
				TWOLEVEL_HINTS = 22,
				PREBIND_CKSUM = 23,
				_LOAD_WEAK_DYLIB = 24,
				SEGMENT_64 = 25,
				ROUTINES_64 = 26,
				UUID = 27,
				_RPATH = 28,
				CODE_SIGNATURE = 29,
				SEGMENT_SPLIT_INFO = 30,
				_REEXPORT_DYLIB = 31,
				LAZY_LOAD_DYLIB = 32,
				ENCRYPTION_INFO = 33,
				DYLD_INFO = 34,
				_LOAD_UPWARD_DYLIB = 35,
				VERSION_MIN_MACOSX = 36,
				VERSION_MIN_IPHONEOS = 37,
				FUNCTION_STARTS = 38,
				DYLD_ENVIRONMENT = 39,
				_MAIN = 40,
				DATA_IN_CODE = 41,
				SOURCE_VERSION = 42,
				DYLIB_CODE_SIGN_DRS = 43,
				ENCRYPTION_INFO_64 = 44,
				LINKER_OPTION = 45,
				LINKER_OPTIMIZATION_HINT = 46,
				VERSION_MIN_TVOS = 47,
				VERSION_MIN_WATCHOS = 48,
				NOTE = 49,
				BUILD_VERSION = 50,
				_DYLD_EXPORTS_TRIE = 51,
				_DYLD_CHAINED_FIXUPS = 52,
				_FILESET_ENTRY = 53,

				LOAD_WEAK_DYLIB = _LOAD_WEAK_DYLIB | REQ_DYLD,
				RPATH = _RPATH | REQ_DYLD,
				REEXPORT_DYLIB = _REEXPORT_DYLIB | REQ_DYLD,
				DYLD_INFO_ONLY = DYLD_INFO | REQ_DYLD,
				LOAD_UPWARD_DYLIB = _LOAD_UPWARD_DYLIB | REQ_DYLD,
				MAIN = _MAIN | REQ_DYLD,
				DYLD_EXPORTS_TRIE = _DYLD_EXPORTS_TRIE | REQ_DYLD,
				DYLD_CHAINED_FIXUPS = _DYLD_CHAINED_FIXUPS | REQ_DYLD,
				FILESET_ENTRY = _FILESET_ENTRY | REQ_DYLD,
			};
			command_type command;

			LoadCommand(command_type command)
				: command(command)
			{
			}

			virtual ~LoadCommand() = default;

			static std::unique_ptr<LoadCommand> Read(Linker::Reader& rd);

			virtual void ReadFile(Linker::Reader& rd);
			virtual void WriteFile(Linker::Writer& wr) const;

			virtual void Read(Linker::Reader& rd, offset_t size) = 0;
			virtual void Write(Linker::Writer& wr) const;
			virtual offset_t GetSize() const = 0;
		};
		std::vector<std::unique_ptr<LoadCommand>> load_commands;

		class GenericDataCommand : public LoadCommand
		{
		public:
			GenericDataCommand(command_type command)
				: LoadCommand(command)
			{
			}

			std::shared_ptr<Linker::Image> command_image;

			void Read(Linker::Reader& rd, offset_t size) override;
			void Write(Linker::Writer& wr) const override;
			offset_t GetSize() const override;
		};

		class Section
		{
		public:
			std::string name;
			std::string segment_name;
			uint64_t address = 0;
			uint64_t memory_size = 0;
			uint32_t offset = 0;
			uint32_t align_shift = 0;
			uint32_t relocation_offset = 0;
			uint32_t relocation_count = 0; // TODO
			uint32_t flags = 0;
			uint32_t reserved1 = 0;
			uint32_t reserved2 = 0;

			static Section Read(Linker::Reader& rd, int wordsize);
			void Write(Linker::Writer& rd, int wordsize) const;
		};

		class SegmentCommand : public LoadCommand
		{
		public:
			std::string name;
			uint64_t address = 0;
			uint64_t memory_size = 0;
			uint64_t offset = 0;
			uint64_t file_size = 0;
			uint32_t max_protection = 0;
			uint32_t init_protection = 0;
			uint32_t flags;
			std::vector<Section> sections;

			void ReadFile(Linker::Reader& rd) override;
			void WriteFile(Linker::Writer& wr) const override;

			void Read(Linker::Reader& rd, offset_t size) override;
			void Write(Linker::Writer& wr) const override;
			offset_t GetSize() const override;
		};

		void ReadFile(Linker::Reader& rd) override;
		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) override;
		void Dump(Dumper::Dumper& dump) override;
		/* TODO */
	};

	class FatMachOFormat : public virtual Linker::OutputFormat
	{
	public:
		struct Entry
		{
			MachOFormat::cpu_type cpu = MachOFormat::cpu_type(0);
			uint32_t cpu_subtype = 0;
			uint32_t offset = 0;
			uint32_t size = 0;
			uint32_t align = 0;
			std::shared_ptr<Linker::Image> image;

			static Entry Read(Linker::Reader& rd);
			void Write(Linker::Writer& wr) const;
		};
		std::vector<Entry> entries;

		offset_t ImageSize() override;
		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) override;

		void Dump(Dumper::Dumper& dump) override;
		void CalculateValues() override;
	};
}

#endif /* MACHO_H */
