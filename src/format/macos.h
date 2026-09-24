#ifndef MACOS_H
#define MACOS_H

#include <cstring>
#include <filesystem>
#include <optional>
#include <set>
#include <vector>
#include "apple.h"
#include "../dumper/dumper.h"
#include "../linker/module.h"
#include "../linker/options.h"
#include "../linker/segment.h"
#include "../linker/segment_manager.h"
#include "../linker/writer.h"

/* Classic 68000 Mac OS file formats */

namespace Apple
{
	typedef char OSType[4];

	static constexpr uint32_t OSTypeToUInt32(char type0, char type1, char type2, char type3)
	{
		return
			(uint32_t(uint8_t(type0)) << 24)
			| (uint32_t(uint8_t(type1)) << 16)
			| (uint32_t(uint8_t(type2)) << 8)
			| uint32_t(uint8_t(type3));
	}

	uint32_t OSTypeToUInt32(const OSType& type);
	void UInt32ToOSType(OSType& type, uint32_t value);

	/* TODO: rework with Linker::Format */

	/**
	 * @brief A Macintosh resource fork
	 *
	 * Macintosh classic applications are stored as CODE resources inside a file's resource fork.
	 * When generating a binary image, the resource fork is usually bundled up in an AppleSingle or AppleDouble file.
	 *
	 * This format has been obsoleted in favor of the PEF format, used on PowerPC based Macintosh computers.
	 */
	class MacintoshResourceFileFormat : public virtual Linker::SegmentManager
	{
	public:
		bool FormatSupportsResources() const override
		{
			return true;
		}

		enum memory_model_t
		{
			MODEL_DEFAULT,
			MODEL_TINY,
		};
		memory_model_t memory_model = MODEL_DEFAULT;

		void SetOptions(std::map<std::string, std::string>& options) override;

		static std::vector<Linker::OptionDescription<void>> MemoryModelNames;
		std::vector<Linker::OptionDescription<void>> GetMemoryModelNames() override;
		void SetModel(std::string model) override;

		class Resource : public Linker::Format
		{
		public:
			using Linker::Format::ReadFile;
			virtual void ReadFile(Linker::Reader& rd, offset_t length) = 0;
			void Dump(Dumper::Dumper& dump) const override;
			virtual int GetDisplayOptions() const;
			virtual void Dump(Dumper::Dumper& dump, offset_t file_offset) const;
			virtual void AddFields(Dumper::Dumper& dump, Dumper::Region& region, offset_t file_offset) const;
			virtual std::unique_ptr<Dumper::Region> CreateRegion(std::string name, offset_t offset, offset_t length, unsigned display_width) const;

		protected:
			Resource(const char type[4], uint16_t id, uint8_t attributes = 0)
				: id(id), attributes(attributes)
			{
				memcpy(this->type, type, 4);
			}

			Resource(const char type[4], uint16_t id, std::string name, uint8_t attributes = 0)
				: id(id), name(name), attributes(attributes)
			{
				memcpy(this->type, type, 4);
			}

		public:
			char type[4];
			uint16_t id;

			std::optional<std::string> name;
			uint8_t attributes;

			offset_t ImageSize() const override = 0;

			virtual void CalculateValues() = 0;
		};

		class GenericResource : public Resource
		{
		public:
			GenericResource(const char type[4], uint16_t id)
				: Resource(type, id)
			{
			}

			std::shared_ptr<Linker::Contents> image;

			void CalculateValues() override;

			offset_t ImageSize() const override;

			void ReadFile(Linker::Reader& rd) override;
			void ReadFile(Linker::Reader& rd, offset_t length) override;

			using Linker::Format::WriteFile;
			offset_t WriteFile(Linker::Writer& wr) const override;
			using Resource::Dump;
			std::unique_ptr<Dumper::Region> CreateRegion(std::string name, offset_t offset, offset_t length, unsigned display_width) const override;
		};

		class JumpTableCodeResource : public Resource
		{
		public:
			JumpTableCodeResource()
				: Resource("CODE", 0)
			{
			}

			struct Entry
			{
				uint16_t segment;
				uint32_t offset;
			};

			uint32_t above_a5 = 0;
			uint32_t below_a5 = 0;
			uint32_t jump_table_offset = 32;
			std::vector<Entry> near_entries;
			std::vector<Entry> far_entries;

			void CalculateValues() override;

			offset_t ImageSize() const override;

			enum
			{
				MOVE_DATA_SP = 0x3F3C,
				LOADSEG = 0xA9F0,
			};

			void ReadFile(Linker::Reader& rd) override;
			void ReadFile(Linker::Reader& rd, offset_t length) override;

			using Linker::Format::WriteFile;
			offset_t WriteFile(Linker::Writer& wr) const override;
			using Linker::Format::Dump;
			int GetDisplayOptions() const override;
			void Dump(Dumper::Dumper& dump, offset_t file_offset) const override;
			void AddFields(Dumper::Dumper& dump, Dumper::Region& region, offset_t file_offset) const override;
		};

		class CodeResource : public Resource
		{
		public:
			static constexpr uint32_t OSType = OSTypeToUInt32('C', 'O', 'D', 'E');

			std::shared_ptr<JumpTableCodeResource> jump_table;
			std::shared_ptr<Linker::Contents> image;
			uint32_t zero_fill = 0; // used for code generation

			CodeResource(uint16_t id, std::shared_ptr<JumpTableCodeResource> jump_table = nullptr)
				: Resource("CODE", id), jump_table(jump_table)/*, image("code")*/
			{
			}

			bool is_far = false; /* TODO: test far segments thoroughly */
			uint32_t a5_address = 0; /* TODO: meaning */
			uint32_t base_address = 0; /* TODO: meaning */

			// used for generation
			std::set<uint16_t> near_entries;
			uint16_t near_entry_count = 0;
			// used for generation
			std::set<uint32_t> far_entries;
			uint16_t far_entry_count = 0;
			std::set<uint32_t> a5_relocations;
			std::set<uint32_t> segment_relocations;

			/* filled in after calculation */
			uint32_t first_near_entry_offset;
			uint32_t first_far_entry_offset;
			uint32_t a5_relocation_offset;
			uint32_t segment_relocation_offset;
			uint32_t resource_size;

			void CalculateValues() override;

			offset_t ImageSize() const override;

			void ReadFile(Linker::Reader& rd) override;
			void ReadFile(Linker::Reader& rd, offset_t length) override;

			uint32_t MeasureRelocations(std::set<uint32_t>& relocations) const;

			void ReadRelocations(Linker::Reader& rd, std::set<uint32_t>& relocations) const;
			void WriteRelocations(Linker::Writer& wr, const std::set<uint32_t>& relocations) const;

			using Linker::Format::WriteFile;
			offset_t WriteFile(Linker::Writer& wr) const override;
			using Linker::Format::Dump;
			int GetDisplayOptions() const override;
			void Dump(Dumper::Dumper& dump, offset_t file_offset) const override;
			void AddFields(Dumper::Dumper& dump, Dumper::Region& region, offset_t file_offset) const override;
			std::unique_ptr<Dumper::Region> CreateRegion(std::string name, offset_t offset, offset_t length, unsigned display_width) const override;
		};

		MacintoshResourceFileFormat()
			/*: a5world(".bss")*/
		{
		}

		struct ResourceReference
		{
			uint16_t id = 0;
			uint16_t name_offset = 0xFFFF;
			std::optional<std::string> name;
			uint8_t attributes = 0;
			uint32_t data_offset = 0;
			std::shared_ptr<Resource> data;
		};

		struct ResourceType
		{
			OSType type { 0, 0, 0, 0 };
			uint32_t count = 0;
			uint16_t offset = 0;
			std::vector<ResourceReference> references;
		};

		static std::shared_ptr<Resource> ReadResource(Linker::Reader& rd, const ResourceType& type, const ResourceReference& reference);

		uint16_t attributes = 0; /* TODO: parametrize */
		/** @brief A list of all resource types, as stored in the file */
		std::vector<ResourceType> resource_types;
		/** @brief A list of all resource names, as stored in the file */
		std::vector<std::string> resource_names;

		/** @brief A convenient collection of resources */
		std::map<uint32_t, std::map<uint16_t, std::shared_ptr<Resource>>> resources;

		/* these will be calculated */
		uint32_t data_offset = 0, data_length = 0, map_offset = 0, map_length = 0;
		uint16_t resource_type_list_offset = 28;
		uint16_t name_list_offset = 0;

		/* filled in during generation */
		std::shared_ptr<JumpTableCodeResource> jump_table;
		std::vector<std::shared_ptr<CodeResource>> codes;
		std::map<std::shared_ptr<Linker::Segment>, std::shared_ptr<CodeResource>> segments;
		std::shared_ptr<Linker::Segment> a5world;

		void AddResource(std::shared_ptr<Resource> resource);

		void OnNewSegment(std::shared_ptr<Linker::Segment> segment) override;

		std::unique_ptr<Script::List> GetScript(Linker::Module& module);

		void Link(Linker::Module& module);

		void ProcessModule(Linker::Module& module) override;

		void CalculateValues() override;

		offset_t ImageSize() const override;

		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;

		void Dump(Dumper::Dumper& dump) const override;

		void GenerateFile(std::string filename, Linker::Module& module) override;

		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module) const override;
	};

	/**
	 * @brief This is not actually a file format, but an interface to permit generating multiple binary outputs for Macintosh executables.
	 *
	 * This class is needed because Macintosh executables require utilization of the resource fork, a part of the filesystem which is generally unavailable on other platforms.
	 * There are multiple ways to represent the resource fork on a non-Macintosh file system, including a separate file, an AppleSingle/AppleDouble container or a MacBinary file.
	 * This driver permits generation of one or more of these different formats for the same executable.
	 */
	class MacintoshOutputDriver : public OutputDriver
	{
	public:
		/** @brief Represents the file type of the main file */
		enum target_format_t
		{
			/** @brief Do not generate main file */
			TARGET_NONE = OutputDriver::TARGET_NONE,
			/** @brief Main file is a data fork, typically empty */
			TARGET_DATA_FORK = OutputDriver::TARGET_DATA_FORK,
			/** @brief Main file is a resource fork */
			TARGET_RESOURCE_FORK = OutputDriver::TARGET_RESOURCE_FORK,
			/** @brief Main file is an AppleSingle */
			TARGET_APPLE_SINGLE = OutputDriver::TARGET_APPLE_SINGLE,
			/** @brief Main file is an AppleDouble */
			TARGET_APPLE_DOUBLE = OutputDriver::TARGET_APPLE_DOUBLE,
			/** @brief Main file as a MacBinary */
			TARGET_MAC_BINARY = OutputDriver::TARGET_MAC_BINARY,
		};

		/** @brief Represents what additional files should be generated */
		enum produce_format_t
		{
			/** @brief Places a Macintosh format resource file under the directory .rsrc */
			PRODUCE_RESOURCE_FORK = OutputDriver::PRODUCE_RESOURCE_FORK,
			/** @brief Places a Finder Information file under the directory .finf */
			PRODUCE_FINDER_INFO = OutputDriver::PRODUCE_FINDER_INFO,
			/** @brief Creates an AppleDouble binary with the '%' prefix */
			PRODUCE_APPLE_DOUBLE = OutputDriver::PRODUCE_APPLE_DOUBLE,
			/** @brief Creates a MacBinary with the .mbin extension */
			PRODUCE_MAC_BINARY = OutputDriver::PRODUCE_MAC_BINARY,
		};

		/* Typical combinations:
		 * - Executor: Generate a data fork and an AppleDouble with % prefix
		 * - Basilisk: Generate a data fork, a resource fork (under .rsrc) and a Finder Info file (under .finf)
		 * - Generate a MacBinary with .mbin extension
		 * - Generate an AppleSingle
		 */

		MacintoshOutputDriver(target_format_t target = TARGET_DATA_FORK)
			: OutputDriver(OutputDriver::target_format_t(target),
				OutputDriver::produce_format_t(target == TARGET_NONE ? PRODUCE_MAC_BINARY
				: target == TARGET_DATA_FORK ? PRODUCE_APPLE_DOUBLE
				: produce_format_t(0)))
		{
		}

		MacintoshOutputDriver(target_format_t target, int produce)
			: OutputDriver(OutputDriver::target_format_t(target), OutputDriver::produce_format_t(produce))
		{
		}

	protected:
		bool SupportedSupplementaryFormat(OutputDriver::produce_format_t produce) override;

		/** @brief Tasked to create all the requested files */
		void GenerateFiles(std::string filename, std::shared_ptr<Contents> data_fork, std::shared_ptr<Contents> resource_fork);
	};

	/**
	 * @brief Interface to generate files required for the Classic 68K Mac OS runtime
	 */
	class Classic68KDriver : public MacintoshOutputDriver
	{
	public:
		Classic68KDriver(target_format_t target = TARGET_DATA_FORK)
			: MacintoshOutputDriver(target)
		{
		}

		Classic68KDriver(target_format_t target, int produce)
			: MacintoshOutputDriver(target, produce)
		{
		}

		bool FormatSupportsResources() const override;

	private:
		/** Direct access to the Mac OS resource fork */
		std::shared_ptr<MacintoshResourceFileFormat> resource_fork;
		std::shared_ptr<FinderInfo> finder_info;

		std::map<std::string, std::string> options;
		std::string model;
		std::string script_file;
		std::map<std::string, std::string> script_options;

	public:
		// TODO: move to OutputDriver
		void SetAppleSingleDoubleVersion(offset_t version);

		// TODO: extend OutputDriver::DriverOptionCollector
		class DriverOptionCollector : public Linker::OptionCollector
		{
		public:
			class MacBinaryVersionEnumerator : public Linker::Enumeration<MacBinary::version_t>
			{
			public:
				MacBinaryVersionEnumerator()
					: Enumeration(
						"1", MacBinary::MACBIN1,
						"GETINFO", MacBinary::MACBIN1_GETINFO,
						"2", MacBinary::MACBIN2,
						"3", MacBinary::MACBIN3)
				{
					descriptions = {
						{ MacBinary::MACBIN1, "Revision 1 (1985)" },
						{ MacBinary::MACBIN1_GETINFO, "Revision 1 (1985) with Get Info extension" },
						{ MacBinary::MACBIN2, "MacBinary II, Revision 2 (1987)" },
						{ MacBinary::MACBIN3, "MacBinary III, Revision 3 (1987)" },
					};
				}
			};

			Linker::Option<std::optional<offset_t>> asver{"asver", "Version of the AppleSingle/AppleDouble container (recognized values: 1, 2)"};
			Linker::Option<std::optional<offset_t>> adver{"adver", "Version of the AppleSingle/AppleDouble container (recognized values: 1, 2)"};
			Linker::Option<std::optional<Linker::ItemOf<MacBinaryVersionEnumerator>>> mbinver{"mbinver", "Version of the MacBinary container"};
			Linker::Option<std::optional<Linker::ItemOf<MacBinaryVersionEnumerator>>> minmbinver{"minmbinver", "Minimum required version for the MacBinary container"};

			DriverOptionCollector()
				// TODO: if MacintoshResourceFileFormat gets formats, call its constructor
			{
				InitializeFields(asver, adver, mbinver, minmbinver);
			}
		};

		std::shared_ptr<Linker::OptionCollector> GetOptions() override;
		void SetOptions(std::map<std::string, std::string>& options) override;

		std::vector<Linker::OptionDescription<void>> GetMemoryModelNames() override;
		void SetModel(std::string model) override;

		void SetLinkScript(std::string script_file, std::map<std::string, std::string>& options) override;

		void GenerateFile(std::string filename, Linker::Module& module) override;

	protected:
		void OnContainerCreated() override;
		void OnCalculateValues() override;
		void OnReadFile(Linker::Reader& rd) override;
		offset_t OnWriteFile(Linker::Writer& wr) const override;
		void OnDump(Dumper::Dumper& dump) const override;

	public:
		void ReadFile(Linker::Reader& rd) override;

		std::string GetDefaultExtension(Linker::Module& module) const override;
		std::string GetDefaultExtension(Linker::Module& module, std::string filename) const override;
	};
}

#endif /* MACOS_H */
