#ifndef MACOS_H
#define MACOS_H

#include <filesystem>
#include <optional>
#include <set>
#include <vector>
#include "apple.h"
#include "../dumper/dumper.h"
#include "../linker/module.h"
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
	 * @brief MacBinary is an alternative format to AppleSingle for representing a Macintosh file on a non-Macintosh filesystem.
	 */
	class MacBinary : public Linker::Format
	{
	public:
		std::shared_ptr<AppleSingleDouble> apple_single;

		enum version_t
		{
			/* assigning values to the first two does not matter, because we don't generate the fields that hold them */
			MACBIN1,
			MACBIN1_GETINFO, /* extension */
			MACBIN2 = 0x11,
			MACBIN3 = 0x12,
		};
		version_t version, minimum_version;

		uint16_t secondary_header_size = 0; /* TODO */
		mutable uint16_t crc = 0;

		uint8_t attributes = 0;
		uint32_t creation = 0;
		uint32_t modification = 0;

		/* only used during parsing */
		uint32_t data_fork_length = 0;
		uint32_t resource_fork_length = 0;
		uint16_t comment_length = 0;

		std::string generated_file_name;

		MacBinary(version_t version = MACBIN3)
			: apple_single(std::make_shared<AppleSingleDouble>(AppleSingleDouble::DOUBLE)), version(version), minimum_version(version <= MACBIN2 ? version : MACBIN2)
		{
		}

		MacBinary(version_t version, version_t minimum_version)
			: apple_single(std::make_shared<AppleSingleDouble>(AppleSingleDouble::DOUBLE)), version(version), minimum_version(version < minimum_version ? version : minimum_version)
		{
		}

		explicit MacBinary(std::shared_ptr<AppleSingleDouble> apple, version_t version, version_t minimum_version)
			: apple_single(apple), version(version), minimum_version(version < minimum_version ? version : minimum_version)
		{
		}

		/* CRC16-CCITT */
		static uint16_t crc_step[256];

		void CRC_Initialize() const;

		void CRC_Step(uint8_t byte = 0) const;

		void Skip(Linker::Writer& wr, size_t count) const;

		void WriteData(Linker::Writer& wr, size_t count, const void * data) const;

		void WriteData(Linker::Writer& wr, size_t count, std::string text) const;

		void WriteWord(Linker::Writer& wr, size_t bytes, uint64_t value) const;

		void ReadHeader(Linker::Reader& rd);
		void WriteHeader(Linker::Writer& wr) const;

		void CalculateValues();

		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;

		void Dump(Dumper::Dumper& dump) const override;
	};

	class MacintoshOutput : public Linker::OutputFormat
	{
	public:
		/* format of "filename" */
		enum target_format_t
		{
			TARGET_NONE, /* do not generate main file */
			TARGET_DATA_FORK, /* main file is a data fork, typically empty */
			TARGET_RESOURCE_FORK, /* main file is a resource fork */
			TARGET_APPLE_SINGLE, /* main file is an AppleSingle */
			TARGET_APPLE_DOUBLE, /* main file is an AppleDouble */
			TARGET_MAC_BINARY, /* main file as a MacBinary */
		};
		target_format_t target;

		/* other files to produce */
		enum produce_format_t
		{
			PRODUCE_RESOURCE_FORK = 1 << 0, /* under .rsrc */
			PRODUCE_FINDER_INFO = 1 << 1, /* under .finf */
			PRODUCE_APPLE_DOUBLE = 1 << 2, /* with % prefix */
			PRODUCE_MAC_BINARY = 1 << 3, /* with .mbin extension */
		};
		produce_format_t produce;

		/* Typical combinations:
		 * - Executor: Generate a data fork and an AppleDouble with % prefix
		 * - Basilisk: Generate a data fork, a resource fork (under .rsrc) and a Finder Info file (under .finf)
		 * - Generate a MacBinary with .mbin extension
		 * - Generate an AppleSingle
		 */

		unsigned apple_single_double_version = 2;
		/* Only relevant for version 1 */
		AppleSingleDouble::hfs_type home_file_system = AppleSingleDouble::HFS_UNDEFINED;

		MacBinary::version_t macbinary_version = MacBinary::MACBIN3, macbinary_minimum_version = MacBinary::MACBIN2;

		MacintoshOutput(target_format_t target = TARGET_DATA_FORK)
			: target(target),
			produce(target == TARGET_NONE ? PRODUCE_MAC_BINARY
				: target == TARGET_DATA_FORK ? PRODUCE_APPLE_DOUBLE
				: produce_format_t(0))
		{
		}

		MacintoshOutput(target_format_t target, int produce)
			: target(target), produce(produce_format_t(produce))
		{
		}

		bool AddSupplementaryOutputFormat(std::string subformat) override;

	protected:
		/* format of information stored */
		enum container_format_t
		{
			CONTAINER_NONE,
			CONTAINER_APPLE_SINGLE,
			CONTAINER_MAC_BINARY,
		};
		container_format_t container = CONTAINER_NONE;

		/** Container for all the necessary additional information */
		std::shared_ptr<AppleSingleDouble> apple_single;
		/** Container for MacBinary */
		std::shared_ptr<MacBinary> mac_binary;

		/** @brief Called after the container is created */
		virtual void OnContainerCreated();
		/** @brief Called if there is no container allocated */
		virtual void OnCalculateValues();
		/** @brief Called if there is no container allocated */
		virtual void OnReadFile(Linker::Reader& rd);
		/** @brief Called if there is no container allocated */
		virtual offset_t OnWriteFile(Linker::Writer& wr) const;
		/** @brief Called if there is no container allocated */
		virtual void OnDump(Dumper::Dumper& dump) const;

	public:
		void GenerateFiles(std::string filename, std::shared_ptr<Contents> data_fork, std::shared_ptr<Contents> resource_fork);

		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;

		void Dump(Dumper::Dumper& dump) const override;
	};

	/**
	 * @brief This is not actually a file format, but an interface to permit generating multiple binary outputs for Macintosh executables.
	 *
	 * This class is needed because Macintosh executables require utilization of the resource fork, a part of the filesystem which is generally unavailable on other platforms.
	 * There are multiple ways to represent the resource fork on a non-Macintosh file system, including a separate file, an AppleSingle/AppleDouble container or a MacBinary file.
	 * This driver permits generation of one or more of these different formats for the same executable.
	 */
	class MacDriver : public MacintoshOutput
	{
	public:
		MacDriver(target_format_t target = TARGET_DATA_FORK)
			: MacintoshOutput(target)
		{
		}

		MacDriver(target_format_t target, int produce)
			: MacintoshOutput(target, produce)
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
