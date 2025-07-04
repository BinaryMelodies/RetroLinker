#ifndef MACOS_H
#define MACOS_H

#include <filesystem>
#include <optional>
#include <set>
#include <vector>
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
	 * @brief AppleSingle & AppleDouble
	 *
	 * On classic Macintosh systems, each file has two associated parts called forks: the data fork and the resource fork.
	 * Classic 68000 applications store all the executable data in the resource fork, which is usually not transferred to non-Macintosh platforms.
	 * The AppleSingle and AppleDouble formats provide a way to store both the data and resource fork, or the resource fork, as a separate file, alongside other metadata, which is essential when storing it on a non-Macintosh system.
	 *
	 * This container format was first invented for the A/UX Apple UNIX system.
	 * It has two versions, and version 2 is used most commonly.
	 * See also Apple::AppleSingleDouble::ResourceFork.
	 */
	class AppleSingleDouble : public Linker::OutputFormat
	{
	public:
		offset_t ImageSize() const override;
		void ReadFile(Linker::Reader& rd) override;

		bool FormatSupportsResources() const override;

		enum format_type
		{
			SINGLE = 0x00051600,
			DOUBLE = 0x00051607,
		};
		format_type type = DOUBLE;
		unsigned version = 2;

		/* Only relevant for version 1 */
		enum hfs_type
		{
			HFS_UNDEFINED,
			HFS_Macintosh,
			HFS_ProDOS,
			HFS_MSDOS,
			HFS_UNIX,
			HFS_VAX_VMS,
		};
	protected:
		hfs_type home_file_system;
	public:
		hfs_type GetHomeFileSystem() const;
		void SetHomeFileSystem(hfs_type type);

		char home_file_system_string[16] = "";

		class Entry : public virtual Linker::Format
		{
		public:
			const uint32_t id;
			offset_t file_offset = 0;
			offset_t image_size = 0;
		protected:
			Entry(uint32_t id)
				: id(id)
			{
			}
		public:
			offset_t ImageSize() const override = 0;
			static std::shared_ptr<Entry> ReadEntry(Linker::Reader& rd, hfs_type home_file_system);
			void ReadFile(Linker::Reader& rd) override = 0;
			using Linker::Format::WriteFile;
			offset_t WriteFile(Linker::Writer& out) const override = 0;
			void Dump(Dumper::Dumper& dump) const override = 0;

			void DumpEntry(Dumper::Dumper& dump, unsigned index) const;

			virtual void ProcessModule(Linker::Module& module);
			virtual void CalculateValues();
		};

		class UnknownEntry : public Entry
		{
		public:
			std::shared_ptr<Linker::Image> image;

			UnknownEntry(uint32_t id)
				: Entry(id)
			{
			}

			offset_t ImageSize() const override;

			void ReadFile(Linker::Reader& rd) override;

			using Linker::Format::WriteFile;
			offset_t WriteFile(Linker::Writer& out) const override;

			void Dump(Dumper::Dumper& dump) const override;

			void CalculateValues() override;
		};

	private:
		static const char TXT_undefined[16];
		static const char TXT_Macintosh[16];
		static const char TXT_ProDOS[16];
		static const char TXT_MS_DOS[16];
		static const char TXT_UNIX[16];
		static const char TXT_VAX_VMS[16];

	public:
		std::vector<std::shared_ptr<Entry>> entries;
		offset_t image_size = offset_t(-1);

		enum
		{
			ID_DataFork = 1,
			ID_ResourceFork,
			ID_RealName,
			ID_Comment,
			ID_IconBW,
			ID_IconColor,
			ID_FileInfo, /* v1 only */
			ID_FileDatesInfo, /* v2 only */
			ID_FinderInfo,
			ID_MacintoshFileInfo, /* v2 only */
			ID_ProDOSFileInfo, /* v2 only */
			ID_MSDOSFileInfo, /* v2 only */
			ID_AFPShortName, /* v2 only */
			ID_AFPFileInfo, /* v2 only */
			ID_AFPDirectoryID, /* v2 only */
		};

		explicit AppleSingleDouble()
		{
			SetHomeFileSystem(HFS_UNDEFINED);
		}

		AppleSingleDouble(format_type type, unsigned version, hfs_type home_file_system)
			: type(type), version(version)
		{
			assert(type == SINGLE || type == DOUBLE);
			SetHomeFileSystem(version == 1 ? home_file_system : HFS_UNDEFINED);
		}

		AppleSingleDouble(format_type type, hfs_type home_file_system)
			: type(type), version(1), home_file_system(home_file_system)
		{
			assert(type == SINGLE || type == DOUBLE);
			SetHomeFileSystem(home_file_system);
		}

		AppleSingleDouble(format_type type, unsigned version = 2)
			: type(type), version(version), home_file_system(HFS_UNDEFINED)
		{
			assert(type == SINGLE || type == DOUBLE);
			SetHomeFileSystem(HFS_UNDEFINED);
		}

		/* TODO: a destructor might remove the entries of the other object as well */
		explicit AppleSingleDouble(AppleSingleDouble& other, format_type type)
			: type(type), version(other.version), home_file_system(other.home_file_system)
		{
			SetHomeFileSystem(other.GetHomeFileSystem());
			if(type == SINGLE && other.type == DOUBLE)
			{
				GetDataFork();
			}
			for(auto entry : other.entries)
			{
				if(type == DOUBLE && entry->id == ID_DataFork)
					continue;
				entries.push_back(entry);
			}
		}

		explicit AppleSingleDouble(AppleSingleDouble& other)
			: type(other.type), version(other.version), home_file_system(other.home_file_system)
		{
			for(auto entry : other.entries)
			{
				entries.push_back(entry);
			}
		}

		void SetOptions(std::map<std::string, std::string>& options) override;
		std::vector<Linker::OptionDescription<void>> GetMemoryModelNames() override;
		void SetModel(std::string model) override;
		void SetLinkScript(std::string script_file, std::map<std::string, std::string>& options) override;

		std::shared_ptr<const Entry> FindEntry(uint32_t id) const;
		std::shared_ptr<Entry> FindEntry(uint32_t id);

	protected:
		std::shared_ptr<Entry> GetFileDatesInfo();
		std::shared_ptr<Entry> GetMacintoshFileInfo();
		std::shared_ptr<Entry> GetAUXFileInfo();
		std::shared_ptr<Entry> GetProDOSFileInfo();
		std::shared_ptr<Entry> GetMSDOSFileInfo();

		std::shared_ptr<Entry> GetDataFork();
		std::shared_ptr<Entry> GetResourceFork();
		std::shared_ptr<Entry> GetFinderInfo();

	public:
		void SetCreationDate(uint32_t CreationDate);
		void SetModificationDate(uint32_t ModificationDate);
		void SetBackupDate(uint32_t BackupDate);
		void SetAccessDate(uint32_t AccessDate);
		void SetMacintoshAttributes(uint32_t Attributes);
		void SetProDOSAccess(uint16_t Access);
		void SetProDOSFileType(uint16_t FileType);
		void SetProDOSAUXType(uint32_t AUXType);
		void SetMSDOSAttributes(uint16_t Attributes);

		uint32_t GetCreationDate();
		uint32_t GetModificationDate();
		uint32_t GetMacintoshAttributes();

		void ProcessModule(Linker::Module& module) override;
		void CalculateValues() override;
		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;

		std::string PrefixFilename(std::string prefix, std::string filename);
		std::string PrefixFilename(std::string prefix, std::string filename, size_t limit);
		std::string ReplaceExtension(std::string filename, std::string extension, size_t limit);

		std::string GetUNIXDoubleFilename(std::string filename);
		std::string GetMacOSXDoubleFilename(std::string filename);
		std::string GetProDOSDoubleFilename(std::string filename);
		std::string GetMSDOSDoubleFilename(std::string filename);

		void GenerateFile(std::string filename, Linker::Module& module) override;

		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module) const override;
	};

	class DataFork : public AppleSingleDouble::Entry
	{
	public:
		std::shared_ptr<Linker::Image> image;

		DataFork()
			: Entry(AppleSingleDouble::ID_DataFork)
		{
		}

		offset_t ImageSize() const override;

		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& out) const override;

		void Dump(Dumper::Dumper& dump) const override;

		void CalculateValues() override;
	};

	/**
	 * @brief A Macintosh resource fork
	 *
	 * Macintosh classic applications are stored as CODE resources inside a file's resource fork.
	 * When generating a binary image, the resource fork is usually bundled up in an AppleSingle or AppleDouble file.
	 *
	 * This format has been obsoleted in favor of the PEF format, used on PowerPC based Macintosh computers.
	 */
	class ResourceFork : public virtual AppleSingleDouble::Entry, public virtual Linker::SegmentManager
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

		class Resource : public Linker::OutputFormat
		{
		public:
			using Linker::Format::ReadFile;
			virtual void ReadFile(Linker::Reader& rd, offset_t length) = 0;
			void Dump(Dumper::Dumper& dump) const override;
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
		};

		class GenericResource : public Resource
		{
		public:
			GenericResource(const char type[4], uint16_t id)
				: Resource(type, id)
			{
			}

			std::shared_ptr<Linker::Image> image;

			void ProcessModule(Linker::Module& module) override;

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

			void ProcessModule(Linker::Module& module) override;

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
			void Dump(Dumper::Dumper& dump, offset_t file_offset) const override;
			void AddFields(Dumper::Dumper& dump, Dumper::Region& region, offset_t file_offset) const override;
		};

		class CodeResource : public Resource
		{
		public:
			static constexpr uint32_t OSType = OSTypeToUInt32('C', 'O', 'D', 'E');

			std::shared_ptr<JumpTableCodeResource> jump_table;
			std::shared_ptr<Linker::Image> image;
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

			void ProcessModule(Linker::Module& module) override;

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
			void Dump(Dumper::Dumper& dump, offset_t file_offset) const override;
			void AddFields(Dumper::Dumper& dump, Dumper::Region& region, offset_t file_offset) const override;
			std::unique_ptr<Dumper::Region> CreateRegion(std::string name, offset_t offset, offset_t length, unsigned display_width) const override;
		};

		ResourceFork()
			: Entry(AppleSingleDouble::ID_ResourceFork)/*, a5world(".bss")*/
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

	class RealName : public AppleSingleDouble::Entry
	{
	public:
		std::string name;

		RealName(std::string name = "")
			: Entry(AppleSingleDouble::ID_RealName), name(name)
		{
		}

		offset_t ImageSize() const override;

		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;

		void Dump(Dumper::Dumper& dump) const override;
	};

	class Comment : public AppleSingleDouble::Entry
	{
	public:
		Comment()
			: Entry(AppleSingleDouble::ID_Comment)
		{
		}
		/* TODO - this is a stub */

		offset_t ImageSize() const override;

		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& out) const override;

		void Dump(Dumper::Dumper& dump) const override;
	};

	class IconBW : public AppleSingleDouble::Entry
	{
	public:
		IconBW()
			: Entry(AppleSingleDouble::ID_IconBW)
		{
		}
		/* TODO - this is a stub */

		offset_t ImageSize() const override;

		void ReadFile(Linker::Reader& rd) override;
		using Linker::Format::WriteFile;

		offset_t WriteFile(Linker::Writer& out) const override;

		void Dump(Dumper::Dumper& dump) const override;
	};

	class IconColor : public AppleSingleDouble::Entry
	{
	public:
		IconColor()
			: Entry(AppleSingleDouble::ID_IconColor)
		{
		}
		/* TODO - this is a stub */

		offset_t ImageSize() const override;

		void ReadFile(Linker::Reader& rd) override;
		using Linker::Format::WriteFile;

		offset_t WriteFile(Linker::Writer& out) const override;

		void Dump(Dumper::Dumper& dump) const override;
	};

	/* Version 1 only */
	class FileInfo : public AppleSingleDouble::Entry
	{
	protected:
		FileInfo()
			: Entry(AppleSingleDouble::ID_FileInfo)
		{
		}

	public:
		class Macintosh;
		class ProDOS;
		class MSDOS;
		class AUX;
	};

	class FileInfo::Macintosh : public FileInfo
	{
	public:
		uint32_t CreationDate;
		uint32_t ModificationDate;
		uint32_t LastBackupDate;
		uint32_t Attributes;

		Macintosh(uint32_t CreationDate = 0,
				uint32_t ModificationDate = 0,
				uint32_t LastBackupDate = 0,
				uint32_t Attributes = 0)
			: CreationDate(CreationDate), ModificationDate(ModificationDate), LastBackupDate(LastBackupDate), Attributes(Attributes)
		{
		}

		offset_t ImageSize() const override;

		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;

		void Dump(Dumper::Dumper& dump) const override;
	};

	class FileInfo::ProDOS : public FileInfo
	{
	public:
		uint32_t CreationDate;
		uint32_t ModificationDate;
		uint16_t Access;
		uint16_t FileType;
		uint32_t AUXType;

		ProDOS(uint32_t CreationDate = 0,
				uint32_t ModificationDate = 0,
				uint16_t Access = 0,
				uint16_t FileType = 0,
				uint32_t AUXType = 0)
			: CreationDate(CreationDate), ModificationDate(ModificationDate), Access(Access), FileType(FileType), AUXType(AUXType)
		{
		}

		offset_t ImageSize() const override;

		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;

		void Dump(Dumper::Dumper& dump) const override;
	};

	class FileInfo::MSDOS : public FileInfo
	{
	public:
		uint32_t ModificationDate;
		uint16_t Attributes;

		MSDOS(uint32_t ModificationDate = 0,
				uint16_t Attributes = 0)
			: ModificationDate(ModificationDate), Attributes(Attributes)
		{
		}

		offset_t ImageSize() const override;

		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;

		void Dump(Dumper::Dumper& dump) const override;
	};

	class FileInfo::AUX : public FileInfo
	{
	public:
		uint32_t CreationDate;
		uint32_t AccessDate;
		uint32_t ModificationDate;

		AUX(uint32_t CreationDate = 0,
				uint32_t AccessDate = 0,
				uint32_t ModificationDate = 0)
			: CreationDate(CreationDate), AccessDate(AccessDate), ModificationDate(ModificationDate)
		{
		}

		offset_t ImageSize() const override;

		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;

		void Dump(Dumper::Dumper& dump) const override;
	};

	/* Version 2 only */
	class FileDatesInfo : public AppleSingleDouble::Entry
	{
	public:
		uint32_t CreationDate;
		uint32_t ModificationDate;
		uint32_t BackupDate;
		uint32_t AccessDate;

		FileDatesInfo(
				uint32_t CreationDate = 0,
				uint32_t ModificationDate = 0,
				uint32_t BackupDate = 0,
				uint32_t AccessDate = 0)
			: Entry(AppleSingleDouble::ID_FileDatesInfo),
				CreationDate(CreationDate), ModificationDate(ModificationDate), BackupDate(BackupDate), AccessDate(AccessDate)
		{
		}

		offset_t ImageSize() const override;

		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;

		void Dump(Dumper::Dumper& dump) const override;
	};

	class FinderInfo : public AppleSingleDouble::Entry
	{
	public:
		struct Point
		{
			uint16_t x, y;
		};

		char Type[4] = { '?', '?', '?', '?' };
		char Creator[4] = { '?', '?', '?', '?' };
		uint16_t Flags = 0;
		Point Location = { 0, 0 };

		FinderInfo()
			: Entry(AppleSingleDouble::ID_FinderInfo)
		{
		}

		offset_t ImageSize() const override;

		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;

		void Dump(Dumper::Dumper& dump) const override;

		void ProcessModule(Linker::Module& module) override;
	};

	/* Version 2 only */
	class MacintoshFileInfo : public AppleSingleDouble::Entry
	{
	public:
		uint32_t Attributes;
		MacintoshFileInfo(uint32_t Attributes = 0)
			: Entry(AppleSingleDouble::ID_MacintoshFileInfo), Attributes(Attributes)
		{
		}

		offset_t ImageSize() const override;

		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;

		void Dump(Dumper::Dumper& dump) const override;
	};

	/* Version 2 only */
	class ProDOSFileInfo : public AppleSingleDouble::Entry
	{
	public:
		uint16_t Access;
		uint16_t FileType;
		uint32_t AUXType;

		ProDOSFileInfo(uint16_t Access = 0,
				uint16_t FileType = 0,
				uint32_t AUXType = 0)
			: Entry(AppleSingleDouble::ID_ProDOSFileInfo), Access(Access), FileType(FileType), AUXType(AUXType)
		{
		}

		offset_t ImageSize() const override;

		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;

		void Dump(Dumper::Dumper& dump) const override;
	};

	/* Version 2 only */
	class MSDOSFileInfo : public AppleSingleDouble::Entry
	{
	public:
		uint16_t Attributes;

		MSDOSFileInfo(uint16_t Attributes = 0)
			: Entry(AppleSingleDouble::ID_MSDOSFileInfo), Attributes(Attributes)
		{
		}

		offset_t ImageSize() const override;

		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;

		void Dump(Dumper::Dumper& dump) const override;
	};

	/* Version 2 only */
	class AFPShortName : public AppleSingleDouble::Entry
	{
	public:
		AFPShortName()
			: Entry(AppleSingleDouble::ID_AFPShortName)
		{
		}
		/* TODO - this is a stub */

		offset_t ImageSize() const override;

		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;

		void Dump(Dumper::Dumper& dump) const override;
	};

	/* Version 2 only */
	class AFPFileInfo : public AppleSingleDouble::Entry
	{
	public:
		AFPFileInfo()
			: Entry(AppleSingleDouble::ID_AFPFileInfo)
		{
		}
		/* TODO - this is a stub */

		offset_t ImageSize() const override;

		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;

		void Dump(Dumper::Dumper& dump) const override;
	};

	/* Version 2 only */
	class AFPDirectoryID : public AppleSingleDouble::Entry
	{
	public:
		AFPDirectoryID()
			: Entry(AppleSingleDouble::ID_AFPDirectoryID)
		{
		}
		/* TODO - this is a stub */

		offset_t ImageSize() const override;

		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;

		void Dump(Dumper::Dumper& dump) const override;
	};

	/**
	 * @brief MacBinary is an alternative format to AppleSingle for representing a Macintosh file on a non-Macintosh filesystem.
	 */
	class MacBinary : public AppleSingleDouble
	{
	public:
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

		std::string generated_file_name;

		MacBinary(version_t version = MACBIN3)
			: AppleSingleDouble(AppleSingleDouble::DOUBLE), version(version), minimum_version(version <= MACBIN2 ? version : MACBIN2)
		{
		}

		MacBinary(version_t version, version_t minimum_version)
			: AppleSingleDouble(AppleSingleDouble::DOUBLE), version(version), minimum_version(version < minimum_version ? version : minimum_version)
		{
		}

		explicit MacBinary(AppleSingleDouble& apple, version_t version, version_t minimum_version)
			: AppleSingleDouble(apple, AppleSingleDouble::DOUBLE), version(version), minimum_version(version < minimum_version ? version : minimum_version)
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

		void WriteHeader(Linker::Writer& wr) const;

		void CalculateValues() override;

		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;

		void Dump(Dumper::Dumper& dump) const override;

		void GenerateFile(std::string filename, Linker::Module& module) override;
	};

	/**
	 * @brief This is not actually a file format, but an interface to permit generating multiple binary outputs for Macintosh executables.
	 *
	 * This class is needed because Macintosh executables require utilization of the resource fork, a part of the filesystem which is generally unavailable on other platforms.
	 * There are multiple ways to represent the resource fork on a non-Macintosh file system, including a separate file, an AppleSingle/AppleDouble container or a MacBinary file.
	 * This driver permits generation of one or more of these different formats for the same executable.
	 */
	class MacDriver : public Linker::OutputFormat
	{
	public:
		/* format of "filename" */
		enum target_format_t
		{
			TARGET_NONE, /* do not generate main file */
			TARGET_DATA_FORK, /* main file is a data fork, typically empty */
			TARGET_RESOURCE_FORK, /* main file is a resource fork */
			TARGET_APPLE_SINGLE, /* main file is an AppleSingle */
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

		MacDriver(target_format_t target = TARGET_DATA_FORK)
			: target(target),
			produce(target == TARGET_NONE ? PRODUCE_MAC_BINARY
				: target == TARGET_DATA_FORK ? PRODUCE_APPLE_DOUBLE
				: produce_format_t(0))
		{
		}

		MacDriver(target_format_t target, int produce)
			: target(target), produce(produce_format_t(produce))
		{
		}

		bool FormatSupportsResources() const override;

		bool AddSupplementaryOutputFormat(std::string subformat) override;

	private:
		std::variant<
			std::shared_ptr<ResourceFork>,
			std::shared_ptr<AppleSingleDouble>
			//std::shared_ptr<MacBinary> // stored as AppleSingleDouble
		> container;

		std::shared_ptr<const ResourceFork> GetResourceFork() const;
		std::shared_ptr<ResourceFork> GetResourceFork();
		std::shared_ptr<const AppleSingleDouble> GetAppleSingleDouble() const;
		std::shared_ptr<AppleSingleDouble> GetAppleSingleDouble();
		std::shared_ptr<const MacBinary> GetMacBinary() const;
		std::shared_ptr<MacBinary> GetMacBinary();

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

		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;

		void Dump(Dumper::Dumper& dump) const override;
	};
}

#endif /* MACOS_H */
