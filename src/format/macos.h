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
#include "../runtime/resource.h"

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
	class MacintoshResourceFileFormat : public virtual Linker::SegmentManager, public Runtime::ResourceManager, protected Runtime::ResourceIterable
	{
	public:
		bool FormatSupportsResources() const override
		{
			return true;
		}

		enum memory_model_t
		{
			/** @brief Each non-zero section becomes its own CODE segment, except .code/.text/.data/.rodata becomes its own section */
			MODEL_DEFAULT,
			/** @brief Create a single CODE segment containing all executable, non-zero filled and zero filled data, as well as .a5world and any resources */
			MODEL_TINY,
		};
		memory_model_t memory_model = MODEL_DEFAULT;

		class MacintoshOptionCollector : public Linker::OptionCollector
		{
		public:
			Linker::Option<bool> _32{"32", "Make 32-bit clean"};
			Linker::Option<bool> far{"far", "Far code and data (32-bit Everything) [not implemented]"}; // TODO: not implemented

			MacintoshOptionCollector()
			{
				InitializeFields(_32, far);
			}
		};

		std::shared_ptr<Linker::OptionCollector> GetOptions() override;
		void SetOptions(std::map<std::string, std::string>& options) override;

		static std::vector<Linker::OptionDescription<void>> MemoryModelNames;
		std::vector<Linker::OptionDescription<void>> GetMemoryModelNames() override;
		void SetModel(std::string model) override;

		/**
		 * @brief A single resource in the resource file
		 */
		class Resource : public Linker::Format
		{
		public:
			using Linker::Format::ReadFile;
			virtual void ReadFile(const std::shared_ptr<Linker::Reader>& rd, offset_t length) = 0;
			void Dump(Dumper::Dumper& dump) const override;
			virtual int GetDisplayOptions() const;
			virtual void Dump(Dumper::Dumper& dump, offset_t file_offset) const;
			virtual void AddFields(Dumper::Dumper& dump, Dumper::Region& region, offset_t file_offset) const;
			virtual std::unique_ptr<Dumper::Region> CreateRegion(std::string name, offset_t offset, offset_t length, unsigned display_width) const;
			/** @brief Apply some changes to the format after parsing */
			virtual void EffectFormat(MacintoshResourceFileFormat& format, const std::shared_ptr<Linker::Reader>& rd);

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

		/**
		 * @brief Stores a resource with no specific format, as as sequence of raw bytes
		 */
		class GenericResource : public Resource
		{
		public:
			GenericResource(const char type[4], uint16_t id)
				: Resource(type, id)
			{
			}

			/** @brief The raw contents of the resource */
			std::shared_ptr<Linker::Contents> image;

			void CalculateValues() override;

			offset_t ImageSize() const override;

			void ReadFile(const std::shared_ptr<Linker::Reader>& rd) override;
			void ReadFile(const std::shared_ptr<Linker::Reader>& rd, offset_t length) override;

			using Linker::Format::WriteFile;
			offset_t WriteFile(const std::shared_ptr<Linker::Writer>& wr) const override;
			using Resource::Dump;
			std::unique_ptr<Dumper::Region> CreateRegion(std::string name, offset_t offset, offset_t length, unsigned display_width) const override;
		};

		/** @brief Represents the information stored in a jump table */
		class JumpTable
		{
		public:
			struct Entry
			{
				uint16_t segment;
				uint32_t offset;
				/** @brief Section number: only in PEF flattened shared libraries */
				std::optional<uint32_t> section;
			};

			struct TransitionVector
			{
				std::optional<uint32_t> seg_loader_section;
				uint32_t seg_loader_offset;
				std::optional<uint32_t> a5_section;
				uint32_t a5_offset;
				uint16_t segment;
				std::optional<uint32_t> section;
				uint32_t offset;
			};

			uint32_t above_a5 = 0;
			uint32_t below_a5 = 0;
			uint32_t jump_table_size = 0;
			uint32_t jump_table_offset = 32; // seems to be fixed for Macintosh programs
			/** @brief Entries with a 16-bit segment offset, used for near model jump tables, for near model format entries in far model jump tables and classic format entries in CFM-68K jump tables */
			std::vector<Entry> legacy_entries;
			/** @brief Entries with a 32-bit segment offset, used for far model format entries in far model jump tables and regular format entries in CFM-68K jump tables */
			std::vector<Entry> extended_entries;
			std::vector<TransitionVector> transition_vectors;
		};

		/** @brief Stores a 68K jump table, found in 'CODE'0 resources */
		class JumpTableCodeResource : public Resource
		{
		public:
			std::shared_ptr<JumpTable> jump_table;

			JumpTableCodeResource(std::shared_ptr<JumpTable> jump_table)
				: Resource("CODE", 0), jump_table(jump_table)
			{
			}

			JumpTableCodeResource()
				: JumpTableCodeResource(std::make_shared<JumpTable>())
			{
			}

			enum JumpTableFormat
			{
				/** @brief Near model jump table, with only near model format entries */
				Near,
				/** @brief Far model jump table, with a single near model format entries and several far model format entries */
				Far,
				/** @brief CFM-68K jump table as stored in the PEF container, with a single classic format entries and several regular format entries */
				CFM_68K,
				/** @brief CFM-68K startup jump table as stored in the 'CODE'0 resource, with a single classic format entry */
				CFM_68K_StartUp,
				/** @brief CFM-68K shared library jump table as stored in the PEF container, with a single classic format entry
				 *
				 * All jump table entries and transition vectors are stored in a flattened format
				 */
				CFM_68K_Library,
				/** @brief CFM-68K shared library jump table as stored in the PEF container, with a single classic format entry
				 *
				 * Jump table entries are stored in their segmented format, transition vectors are stored in an 8-byte unflattened format
				 */
				CFM_68K_Unflattened_Library,
			};
			JumpTableFormat jump_table_format = Near;

			void CalculateValues() override;

			offset_t JumpTableSize() const;

			offset_t ImageSize() const override;

			enum
			{
				MOVE_DATA_SP = 0x3F3C,
				LOADSEG = 0xA9F0,
				JMP_ABS_L = 0x4EF9,

				FAR_VERSION_MARK = 0xFFFF,
				CFM_68K_VERSION_MARK = 0xFFFD,
			};

			void ReadFile(const std::shared_ptr<Linker::Reader>& rd) override;
			void ReadFile(const std::shared_ptr<Linker::Reader>& rd, offset_t length) override;

			/** @brief Describes how a jump table should be handled */
			enum jump_table_parse_mode
			{
				/** @brief No information, try to make the best out of it (untested) */
				FromUnknown,
				/** @brief The jump table is found inside a 'CODE' resource */
				FromResource,
				/** @brief The jump table is found inside a PEF container data section */
				FromPEFContainer,
			};

			/** @brief Read full jump table
			 *
			 * When the jump table is found in a resource, this will also determine what format the jump table is in.
			 * For PEF containers, the format of the jump table must be assigned prior to calling this function.
			 *
			 * @param rd The reader
			 * @param length The expected length of the jump table
			 * @param mode Whether to parse it as a jump table found inside a 'CODE' resource or a PEF container data section
			 * @param section_relocations For PEF container data, also check if there are section relocations at the specified offsets
			 */
			void ReadJumpTable(const std::shared_ptr<Linker::Reader>& rd, offset_t length, jump_table_parse_mode mode, std::map<offset_t, offset_t> section_relocations = {});

			/** @brief Generate the section relocations in a PEF container data section */
			void CollectRelocations(std::map<offset_t, offset_t>& section_relocations); // TODO: untested

			using Linker::Format::WriteFile;
			offset_t WriteFile(const std::shared_ptr<Linker::Writer>& wr) const override;
			using Linker::Format::Dump;
			int GetDisplayOptions() const override;
			void Dump(Dumper::Dumper& dump, offset_t file_offset) const override;
			void AddFields(Dumper::Dumper& dump, Dumper::Region& region, offset_t file_offset) const override;
		};

		/** @brief Stores as 68K code segment, found in 'CODE' resources */
		class CodeResource : public Resource
		{
		public:
			static constexpr uint32_t OSType = OSTypeToUInt32('C', 'O', 'D', 'E');

			static constexpr uint32_t NearSegmentHeaderSize = 0x4;
			static constexpr uint32_t FarSegmentHeaderSize = 0x28;

			std::shared_ptr<JumpTableCodeResource> jump_table;
			std::shared_ptr<Linker::Contents> image;
			uint32_t zero_fill = 0; // used for code generation

			CodeResource(uint16_t id, std::shared_ptr<JumpTableCodeResource> jump_table = nullptr)
				: Resource("CODE", id), jump_table(jump_table)/*, image("code")*/
			{
			}

			enum HeaderFormat
			{
				/** @brief Near model segment, segment starts with a 4-byte header */
				Near,
				/** @brief Far model segment, segment starts with a 40-byte header and potentially followed by segment based and A5 based relocations */
				Far,
				/** @brief CFM-68K segment, segment starts with a 40-byte header */
				CFM_68K,
			};
			HeaderFormat header_format = Near;
			uint32_t a5_address = 0; /* TODO: meaning */
			uint32_t base_address = 0;

			// used for generation
			std::set<uint16_t> near_entries;
			uint16_t near_entry_count = 0;
			// used for generation
			std::set<uint32_t> far_entries;
			uint16_t far_entry_count = 0;
			std::set<uint32_t> a5_relocations;
			std::set<uint32_t> segment_relocations;
			uint32_t transition_vector_count = 0;

			/* filled in after calculation */
			uint32_t first_near_entry_offset;
			uint32_t first_far_entry_offset;
			uint32_t first_transition_vector_offset;
			uint32_t a5_relocation_offset;
			uint32_t segment_relocation_offset;
			uint32_t resource_size;

			/**
			 * @brief For near segments, stores all entries into the jump table
			 *
			 * @param jump_table Pointer to jump table resource
			 * @param entry_relocations Collection of relocations that need fixing up
			 * @param jump_table_offset Latest offset into the jump table where the entries must start, gets updated
			 * @param starting_entry If this segment contains a dedicated entry point, places it as the first entry
			 */
			void StoreNearSegmentEntries(
				std::shared_ptr<JumpTableCodeResource> jump_table,
				std::map<uint32_t, std::vector<Linker::Relocation>> * entry_relocations,
				uint32_t& jump_table_offset,
				std::optional<uint32_t> starting_entry = {});

			/**
			 * @brief For far segments, stores all near entries into the jump table
			 *
			 * @param jump_table Pointer to jump table resource
			 * @param entry_relocations Collection of relocations that need fixing up
			 * @param jump_table_offset Latest offset into the jump table where the entries must start, gets updated
			 * @param starting_entry If this segment contains a dedicated entry point, places it as the first entry
			 */
			void StoreFarSegmentNearEntries(
				std::shared_ptr<JumpTableCodeResource> jump_table,
				std::map<uint32_t, std::vector<Linker::Relocation>> * entry_relocations,
				uint32_t& jump_table_offset,
				std::optional<uint32_t> starting_entry = {});

			/**
			 * @brief For far segments, stores all far entries into the jump table
			 *
			 * @param jump_table Pointer to jump table resource
			 * @param entry_relocations Collection of relocations that need fixing up
			 * @param jump_table_offset Latest offset into the jump table where the entries must start, gets updated
			 */
			void StoreFarSegmentFarEntries(
				std::shared_ptr<JumpTableCodeResource> jump_table,
				std::map<uint32_t, std::vector<Linker::Relocation>> * entry_relocations,
				uint32_t& jump_table_offset);

			void CalculateValues() override;

			offset_t ImageSize() const override;

			void ReadFile(const std::shared_ptr<Linker::Reader>& rd) override;
			void ReadFile(const std::shared_ptr<Linker::Reader>& rd, offset_t length) override;

			uint32_t MeasureRelocations(std::set<uint32_t>& relocations) const;

			void ReadRelocations(const std::shared_ptr<Linker::Reader>& rd, std::set<uint32_t>& relocations) const;
			void WriteRelocations(const std::shared_ptr<Linker::Writer>& wr, const std::set<uint32_t>& relocations) const;

			using Linker::Format::WriteFile;
			offset_t WriteFile(const std::shared_ptr<Linker::Writer>& wr) const override;
			using Linker::Format::Dump;
			int GetDisplayOptions() const override;
			void Dump(Dumper::Dumper& dump, offset_t file_offset) const override;
			void AddFields(Dumper::Dumper& dump, Dumper::Region& region, offset_t file_offset) const override;
		};

		/** @brief Stores a 'SIZE'-1 resource, containing information related to program behavior, such as being able to run in 32-bit clean mode */
		class SizeResource : public Resource
		{
		public:
			static constexpr uint32_t OSType = OSTypeToUInt32('S', 'I', 'Z', 'E');
			static constexpr uint32_t ExpectedLength = 10;

			enum target_type
			{
				Classic24,
				Classic32,
				AUX32,
				Carbon,
			};

			/** @deprecated */
			static constexpr uint16_t SaveScreen = 0x8000;
			static constexpr uint16_t AcceptSuspendResumeEvents = 0x4000;
			/** @deprecated */
			static constexpr uint16_t DisableOptionSwitch = 0x2000;
			static constexpr uint16_t CanBackground = 0x1000;
			static constexpr uint16_t MultiFinderAware = 0x0800;
			static constexpr uint16_t OnlyBackground = 0x0400;
			static constexpr uint16_t GetFrontClicks = 0x0200;
			static constexpr uint16_t AcceptChildDiedEvents = 0x0100;
			static constexpr uint16_t Is32BitCompatible = 0x0080;
			static constexpr uint16_t IsHighLevelEventAware = 0x0040;
			static constexpr uint16_t LocalAndRemoteHLEvents = 0x0020;
			static constexpr uint16_t IsStationaryAware = 0x0010;
			static constexpr uint16_t UseTextEditServices = 0x0008;
			static constexpr uint16_t DisplayManagerAware = 0x0004;

			uint16_t flags = 0;
			uint32_t preferred_memory = 0;
			uint32_t minimum_memory = 0;

			static constexpr uint16_t GetDefaultFlags(target_type target)
			{
				switch(target)
				{
				default:
				case Classic24:
					return 0;
				case Classic32:
					return Is32BitCompatible;
				case AUX32:
					// based on A/UX tools
					return AcceptSuspendResumeEvents | MultiFinderAware | GetFrontClicks | Is32BitCompatible;
				case Carbon:
					// based on libretro flags
					return AcceptSuspendResumeEvents | CanBackground | MultiFinderAware | IsHighLevelEventAware;
				}
			}

			static constexpr uint32_t GetDefaultMemory(target_type target)
			{
				switch(target)
				{
				case Classic24:
				case Classic32:
					return 100 * 1024;
				case AUX32:
					// based on A/UX tools
					return 32 * 1024;
				default:
				case Carbon:
					// according to Wolfgang Thaller (wdefshell.r)
					return 500 * 1024;
				}
			}

			SizeResource()
				: Resource("SIZE", 0xFFFF)
			{
			}

			SizeResource(target_type target)
				: Resource("SIZE", 0xFFFF),
				flags(GetDefaultFlags(target)),
				preferred_memory(GetDefaultMemory(target)),
				minimum_memory(GetDefaultMemory(target))
			{
			}

			void CalculateValues() override;

			offset_t ImageSize() const override;

			void ReadFile(const std::shared_ptr<Linker::Reader>& rd) override;
			void ReadFile(const std::shared_ptr<Linker::Reader>& rd, offset_t length) override;

			using Linker::Format::WriteFile;
			offset_t WriteFile(const std::shared_ptr<Linker::Writer>& wr) const override;
			int GetDisplayOptions() const override;
			//using Linker::Format::Dump;
			//void Dump(Dumper::Dumper& dump, offset_t file_offset) const override;
			void AddFields(Dumper::Dumper& dump, Dumper::Region& region, offset_t file_offset) const override;
			//std::unique_ptr<Dumper::Region> CreateRegion(std::string name, offset_t offset, offset_t length, unsigned display_width) const override;
		};

		/** @brief Stores a 'cfrg'0 resource, used to store PowerPC and CFM-68K code fragments */
		class CodeFragmentResource : public Resource
		{
		public:
			static constexpr uint32_t OSType = OSTypeToUInt32('c', 'f', 'r', 'g');

			class Member
			{
			public:
				enum architecture_type : uint32_t
				{
					PowerPC = OSTypeToUInt32('p', 'w', 'p', 'c'),
					Motorola68K = OSTypeToUInt32('m', '6', '8', 'k'),
					//Compiled // not an actual architecture but a compiler macro
				};
				architecture_type architecture;
				enum update_level_type : uint8_t
				{
					FullLib = 0,
					UpdateLib = 1,
				};
				update_level_type update_level = FullLib;
				uint32_t current_version = 0;
				uint32_t old_def_version = 0;
				static constexpr uint32_t DefaultStackSize = 0;
				uint32_t app_stack_size = 0;
				static constexpr int16_t NoAppSubFolder = 0;
				int16_t app_subdir_id = 0;
				enum usage_type : uint8_t
				{
					ImportLibrary = 0,
					Application = 1,
					DropInAddition = 2,
					StubLibrary = 3,
					WeakStubLibrary = 4,
				};
				usage_type usage;
				enum where_type : uint8_t
				{
					Memory = 0,
					DataFork = 1,
					Resource = 2,
					NamedFragment = 3, // reserved
					CFBundle = 4,
					CFBundleInt = 5,
				};
				where_type where = DataFork;
				static constexpr uint32_t RSEG = OSTypeToUInt32('r', 's', 'e', 'g');
				uint32_t offset = 0;
				static constexpr uint32_t CFragGoesToEOF = 0;
				uint32_t length = 0;
				uint32_t space_id = 0;
				uint16_t extension_count = 0;
				uint16_t member_size = 0;
				std::string name;

				Member(architecture_type architecture)
					: architecture(architecture)
				{
				}

				offset_t ImageSize() const;
				static std::shared_ptr<Member> ReadFile(const std::shared_ptr<Linker::Reader>& rd);
				offset_t WriteFile(const std::shared_ptr<Linker::Writer>& wr) const;

				static std::shared_ptr<Member> CreateEmpty()
				{
					return std::make_shared<Member>(architecture_type(0));
				}

				/** @brief Initialize with default settings */
				static std::shared_ptr<Member> CreateApplication(architecture_type architecture, std::string name)
				{
					std::shared_ptr<Member> member = std::make_shared<Member>(architecture);
					member->update_level = FullLib;
					member->current_version = 0;
					member->old_def_version = 0;
					member->app_stack_size = DefaultStackSize;
					member->app_subdir_id = NoAppSubFolder;
					member->usage = Application;
					if(architecture == Motorola68K)
					{
						member->where = Resource;
						member->offset = RSEG;
						member->length = 0;
					}
					else
					{
						member->where = DataFork;
						member->offset = 0;
						member->length = CFragGoesToEOF;
					}
					member->name = name;
					if(member->name.size() > 15)
					{
						member->name = member->name.substr(0, 15);
					}
					return member;
				}

				/** @brief Initialize with default settings */
				static std::shared_ptr<Member> CreateLibrary(architecture_type architecture, std::string name, uint32_t current_version, uint32_t old_def_version)
				{
					std::shared_ptr<Member> member = std::make_shared<Member>(architecture);
					member->update_level = FullLib;
					member->current_version = current_version;
					member->old_def_version = old_def_version;
					member->app_stack_size = DefaultStackSize;
					member->app_subdir_id = NoAppSubFolder;
					member->usage = ImportLibrary;
					member->where = DataFork;
					member->offset = 0;
					member->length = CFragGoesToEOF;
					member->name = name;
					if(member->name.size() > 15)
					{
						member->name = member->name.substr(0, 15);
					}
					return member;
				}
			};

			uint16_t version = 1;
			std::vector<std::shared_ptr<Member>> members;

			CodeFragmentResource()
				: Resource("cfrg", 0x0000)
			{
			}

			void CalculateValues() override;

			offset_t ImageSize() const override;

			void ReadFile(const std::shared_ptr<Linker::Reader>& rd) override;
			void ReadFile(const std::shared_ptr<Linker::Reader>& rd, offset_t length) override;

			using Linker::Format::WriteFile;
			offset_t WriteFile(const std::shared_ptr<Linker::Writer>& wr) const override;
			int GetDisplayOptions() const override;
			using Linker::Format::Dump;
			void Dump(Dumper::Dumper& dump, offset_t file_offset) const override;
			void AddFields(Dumper::Dumper& dump, Dumper::Region& region, offset_t file_offset) const override;
			//std::unique_ptr<Dumper::Region> CreateRegion(std::string name, offset_t offset, offset_t length, unsigned display_width) const override;

			void EffectFormat(MacintoshResourceFileFormat& format, const std::shared_ptr<Linker::Reader>& rd) override;
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

		static std::shared_ptr<Resource> ReadResource(const std::shared_ptr<Linker::Reader>& rd, const ResourceType& type, const ResourceReference& reference);

		uint16_t attributes = 0; /* TODO: parametrize */
		/* command line parameter */
		bool use_far_segments = false;
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
		std::shared_ptr<SizeResource> size_resource = nullptr;

		void AddResource(std::shared_ptr<Resource> resource);
		std::shared_ptr<Resource> FindResource(const OSType& type, uint16_t id);
		ResourceReference * FindResourceReference(const OSType& type, uint16_t id);

		void OnNewSegment(std::shared_ptr<Linker::Segment> segment) override;

		std::unique_ptr<Script::List> GetScript(Linker::Module& module);

		void Link(Linker::Module& module);

	protected:
		/** @brief Checks if this relocation belongs to an A5-relative addressing mode */
		bool CheckA5Relative(Linker::Relocation& rel);

	public:
		void ProcessModule(Linker::Module& module) override;

		void CalculateValues() override;

		offset_t ImageSize() const override;

		void ReadFile(const std::shared_ptr<Linker::Reader>& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(const std::shared_ptr<Linker::Writer>& wr) const override;

		void Dump(Dumper::Dumper& dump) const override;

		void GenerateFile(std::string filename, Linker::Module& module) override;

		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module) const override;


		/** @brief Interface for the resource manager */
		class ResourceInformation : public Runtime::Resource
		{
		public:
			ResourceType resource_type;
			ResourceReference resource_reference;

			ResourceInformation(ResourceType resource_type, ResourceReference resource_reference)
				: resource_type(resource_type), resource_reference(resource_reference)
			{
			}

			Runtime::Value GetProperties() const override;
		};

		class ResourceIterator : public Runtime::ResourceIterator
		{
		public:
			std::vector<ResourceType>& resource_types;
			mutable std::vector<ResourceType>::iterator resource_types_iterator;
			mutable std::vector<ResourceReference>::iterator resource_references_iterator;

			ResourceIterator(
				std::vector<ResourceType>& resource_types,
				std::vector<ResourceType>::iterator resource_types_iterator,
				std::vector<ResourceReference>::iterator resource_references_iterator)
			: resource_types(resource_types), resource_types_iterator(resource_types_iterator), resource_references_iterator(resource_references_iterator)
			{
			}

			void Step() const override;
			std::shared_ptr<const Runtime::Resource> Get() const override;
			bool SameAs(const Runtime::ResourceIterator * other) const override;
			void Erase() override;
		};

		Runtime::ResourceIterable& Iterate() override;
		std::shared_ptr<Runtime::ResourceIterator> BeginIteration() override;
		std::shared_ptr<Runtime::ResourceIterator> EndOfIteration() override;
		std::shared_ptr<Runtime::ResourceIterator> FindResource(const Runtime::Value& value) override;
		bool UpdateResource(std::shared_ptr<Runtime::Resource> resource) override;
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

		/** Direct access to the Mac OS resource fork */
		std::shared_ptr<MacintoshResourceFileFormat> resource_fork;

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
		std::shared_ptr<FinderInfo> finder_info;

		std::map<std::string, std::string> options;
		std::string model;
		std::string script_file;
		std::map<std::string, std::string> script_options;

	public:
		// TODO: extend OutputDriver::DriverOptionCollector
		class DriverOptionCollector : public MacintoshResourceFileFormat::MacintoshOptionCollector
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
				: MacintoshOptionCollector()
				// TODO: if OutputDriver::DriverOptionCollector is created, call its too constructor
			{
				InitializeFields(asver, adver, mbinver, minmbinver);
			}
		};

		std::shared_ptr<Linker::OptionCollector> GetOptions() override;
		void SetOptions(std::map<std::string, std::string>& options) override;

		std::vector<Linker::OptionDescription<void>> GetMemoryModelNames() override;
		void SetModel(std::string model) override;

		void SetLinkScript(std::string script_file, std::map<std::string, std::string>& options) override;

		void CalculateValues() override;
		void GenerateFile(std::string filename, Linker::Module& module) override;

	protected:
		void OnContainerCreated() override;
		void OnReadFile(const std::shared_ptr<Linker::Reader>& rd) override;
		offset_t OnWriteFile(const std::shared_ptr<Linker::Writer>& wr) const override;
		void OnDump(Dumper::Dumper& dump) const override;

	public:
		void ReadFile(const std::shared_ptr<Linker::Reader>& rd) override;

		std::string GetDefaultExtension(Linker::Module& module) const override;
		std::string GetDefaultExtension(Linker::Module& module, std::string filename) const override;
	};
}

#endif /* MACOS_H */
