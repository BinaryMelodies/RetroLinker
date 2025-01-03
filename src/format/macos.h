#ifndef MACOS_H
#define MACOS_H

#include <filesystem>
#include <optional>
#include <set>
#include <vector>
#include "../linker/linker.h"
#include "../linker/module.h"
#include "../linker/segment.h"
#include "../linker/writer.h"

/* Classic 68000 Mac OS file formats */

namespace Apple
{
	typedef char OSType[4];

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
		void ReadFile(Linker::Reader& rd) override;

		bool FormatSupportsResources() const override;

		enum format_type
		{
			SINGLE,
			DOUBLE,
		} type;
		unsigned version;
		/* Only relevant for version 1 */
		enum hfs_type
		{
			HFS_UNDEFINED,
			HFS_Macintosh,
			HFS_ProDOS,
			HFS_MSDOS,
			HFS_UNIX,
			HFS_VAX_VMS,
		} home_file_system;

		class Entry : public virtual Linker::Format
		{
		public:
			const uint32_t id;
		protected:
			Entry(uint32_t id)
				: id(id)
			{
			}
		public:
			virtual offset_t GetLength();
			void ReadFile(Linker::Reader& rd) override;
			void WriteFile(Linker::Writer& out) override;

			virtual void ProcessModule(Linker::Module& module);
			virtual void CalculateValues();
		};

	private:
		static const char TXT_undefined[16];
		static const char TXT_Macintosh[16];
		static const char TXT_ProDOS[16];
		static const char TXT_MS_DOS[16];
		static const char TXT_UNIX[16];
		static const char TXT_VAX_VMS[16];

	public:
		std::vector<Entry *> entries;

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

		AppleSingleDouble(format_type type, unsigned version, hfs_type home_file_system)
			: type(type), version(version), home_file_system(version == 1 ? home_file_system : HFS_UNDEFINED)
		{
			assert(type == SINGLE || type == DOUBLE);
		}

		AppleSingleDouble(format_type type, hfs_type home_file_system)
			: type(type), version(1), home_file_system(home_file_system)
		{
			assert(type == SINGLE || type == DOUBLE);
		}

		AppleSingleDouble(format_type type, unsigned version = 2)
			: type(type), version(version), home_file_system(HFS_UNDEFINED)
		{
			assert(type == SINGLE || type == DOUBLE);
		}

		/* TODO: a destructor might remove the entries of the other object as well */
		explicit AppleSingleDouble(AppleSingleDouble& other, format_type type)
			: type(type), version(other.version), home_file_system(other.home_file_system)
		{
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
		void SetModel(std::string model) override;
		void SetLinkScript(std::string script_file, std::map<std::string, std::string>& options) override;

		Entry * FindEntry(uint32_t id);

	protected:
		Entry * GetFileDatesInfo();
		Entry * GetMacintoshFileInfo();
		Entry * GetAUXFileInfo();
		Entry * GetProDOSFileInfo();
		Entry * GetMSDOSFileInfo();

		Entry * GetDataFork();
		Entry * GetResourceFork();
		Entry * GetFinderInfo();

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
		void WriteFile(Linker::Writer& wr) override;

		std::string PrefixFilename(std::string prefix, std::string filename);
		std::string PrefixFilename(std::string prefix, std::string filename, size_t limit);
		std::string ReplaceExtension(std::string filename, std::string extension, size_t limit);

		std::string GetUNIXDoubleFilename(std::string filename);
		std::string GetMacOSXDoubleFilename(std::string filename);
		std::string GetProDOSDoubleFilename(std::string filename);
		std::string GetMSDOSDoubleFilename(std::string filename);

		void GenerateFile(std::string filename, Linker::Module& module) override;

		std::string GetDefaultExtension(Linker::Module& module) override;
	};

	class DataFork : public AppleSingleDouble::Entry
	{
	public:
		DataFork()
			: Entry(AppleSingleDouble::ID_DataFork)
		{
		}
		/* TODO - this is a stub */
	};

	/**
	 * @brief A Macintosh resource fork
	 *
	 * Macintosh classic applications are stored as CODE resources inside a file's resource fork.
	 * When generating a binary image, the resource fork is usually bundled up in an AppleSingle or AppleDouble file.
	 *
	 * This format has been obsoleted in favor of the PEF format, used on PowerPC based Macintosh computers.
	 */
	class ResourceFork : public virtual AppleSingleDouble::Entry, public virtual Linker::OutputFormat, public Linker::LinkerManager
	{
	public:
		bool FormatSupportsResources() const override
		{
			return true;
		}

		using LinkerManager::SetLinkScript;

		enum memory_model_t
		{
			MODEL_DEFAULT,
			MODEL_TINY,
		} memory_model;


		void SetOptions(std::map<std::string, std::string>& options) override;

		void SetModel(std::string model) override;

		class Resource : public Linker::OutputFormat
		{
		public:
			void ReadFile(Linker::Reader& rd) override;
			void WriteFile(Linker::Writer& wr) override;

		protected:
			Resource(const char type[4], uint16_t id, uint8_t attributes = 0)
				: id(id), attributes(attributes), name_offset(0), data_offset(0)
			{
				memcpy(this->type, type, 4);
			}

			Resource(const char type[4], uint16_t id, std::string name, uint8_t attributes = 0)
				: id(id), name(name), attributes(attributes), name_offset(0), data_offset(0)
			{
				memcpy(this->type, type, 4);
			}

		public:
			char type[4];
			uint16_t id;

			std::optional<std::string> name;
			uint8_t attributes;

			/* calculated later */
			uint16_t name_offset;
			uint32_t data_offset;

			virtual offset_t GetLength() = 0;
		};

		class GenericResource : public Resource
		{
		public:
			GenericResource(const char type[4], uint16_t id)
				: Resource(type, id)
			{
			}

			Linker::Segment * resource;

			void ProcessModule(Linker::Module& module) override;

			void CalculateValues() override;

			offset_t GetLength() override;

			void WriteFile(Linker::Writer& wr) override;
		};

		class JumpTableCodeResource : public Resource
		{
		public:
			JumpTableCodeResource()
				: Resource("CODE", 0), above_a5(0), below_a5(0)
			{
			}

			struct Entry
			{
				uint16_t segment;
				uint32_t offset;
			};

			uint32_t above_a5;
			uint32_t below_a5;
			std::vector<Entry> near_entries;
			std::vector<Entry> far_entries;

			void ProcessModule(Linker::Module& module) override;

			void CalculateValues() override;

			offset_t GetLength() override;

			enum
			{
				MOVE_DATA_SP = 0x3F3C,
				LOADSEG = 0xA9F0,
			};

			void WriteFile(Linker::Writer& wr) override;
		};

		class CodeResource : public Resource
		{
		public:
			JumpTableCodeResource * jump_table;
			Linker::Segment * image;

			CodeResource(uint16_t id, JumpTableCodeResource * jump_table)
				: Resource("CODE", id), jump_table(jump_table)/*, image("code")*/, is_far(false), a5_address(0), base_address(0)
			{
			}

			bool is_far; /* TODO: test far segments thoroughly */
			uint32_t a5_address; /* TODO: meaning */
			uint32_t base_address; /* TODO: meaning */

//			std::vector<JumpTableCodeResource::Entry> near_entries;
//			std::vector<JumpTableCodeResource::Entry> far_entries;
			std::set<uint16_t> near_entries;
			std::set<uint32_t> far_entries;
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

			offset_t GetLength() override;

			uint32_t MeasureRelocations(std::set<uint32_t>& relocations);

			void WriteRelocations(Linker::Writer& wr, std::set<uint32_t>& relocations);

			void WriteFile(Linker::Writer& wr) override;
		};

		ResourceFork()
			: Entry(AppleSingleDouble::ID_ResourceFork), attributes(0), data_offset(0)/*,
				a5world(".bss")*/
		{
		}

		~ResourceFork()
		{
		}

		uint16_t attributes; /* TODO: parametrize */
		std::map<uint32_t, std::map<uint16_t, Resource *> > resources;

		/* these will be calculated */
		uint32_t data_offset, data_length, map_offset, map_length;
		uint16_t name_list_offset;
		std::map<uint32_t, uint16_t> reference_list_offsets;

		/* filled in during generation */
		JumpTableCodeResource * jump_table;
		std::vector<CodeResource *> codes;
		std::map<Linker::Segment *, CodeResource *> segments;
		Linker::Segment * a5world;

		void AddResource(Resource * resource);

		void OnNewSegment(Linker::Segment * segment) override;

		Script::List * GetScript(Linker::Module& module);

		void Link(Linker::Module& module);

		void ProcessModule(Linker::Module& module) override;

		void CalculateValues() override;

		offset_t GetLength() override;

		void WriteFile(Linker::Writer& wr) override;

		void GenerateFile(std::string filename, Linker::Module& module) override;

		std::string GetDefaultExtension(Linker::Module& module) override;
	};

	class RealName : public AppleSingleDouble::Entry
	{
	public:
		std::string name;

		RealName(std::string name)
			: Entry(AppleSingleDouble::ID_RealName), name(name)
		{
		}

		offset_t GetLength() override;

		void WriteFile(Linker::Writer& wr) override;
	};

	class Comment : public AppleSingleDouble::Entry
	{
	public:
		Comment()
			: Entry(AppleSingleDouble::ID_Comment)
		{
		}
		/* TODO - this is a stub */
	};

	class IconBW : public AppleSingleDouble::Entry
	{
	public:
		IconBW()
			: Entry(AppleSingleDouble::ID_IconBW)
		{
		}
		/* TODO - this is a stub */
	};

	class IconColor : public AppleSingleDouble::Entry
	{
	public:
		IconColor()
			: Entry(AppleSingleDouble::ID_IconColor)
		{
		}
		/* TODO - this is a stub */
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

		offset_t GetLength() override;

		void WriteFile(Linker::Writer& wr) override;
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

		offset_t GetLength() override;

		void WriteFile(Linker::Writer& wr) override;
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

		offset_t GetLength() override;

		void WriteFile(Linker::Writer& wr) override;
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

		offset_t GetLength() override;

		void WriteFile(Linker::Writer& wr) override;
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

		offset_t GetLength() override;

		void WriteFile(Linker::Writer& wr) override;
	};

	class FinderInfo : public AppleSingleDouble::Entry
	{
	public:
		struct Point
		{
			uint16_t x, y;
		};

		char Type[4];
		char Creator[4];
		uint16_t Flags;
		Point Location;

		FinderInfo()
			: Entry(AppleSingleDouble::ID_FinderInfo), Flags(0), Location({0,0})
		{
			memcpy(Type, "????", 4);
			memcpy(Creator, "????", 4);
		}

		offset_t GetLength() override;

		void WriteFile(Linker::Writer& wr) override;

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

		offset_t GetLength() override;

		void WriteFile(Linker::Writer& wr) override;
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

		offset_t GetLength() override;

		void WriteFile(Linker::Writer& wr) override;
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

		offset_t GetLength() override;

		void WriteFile(Linker::Writer& wr) override;
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
		} version, minimum_version;

		uint16_t secondary_header_size; /* TODO */
		uint16_t crc;

		std::string generated_file_name;

		MacBinary(version_t version = MACBIN3)
			: AppleSingleDouble(AppleSingleDouble::DOUBLE), version(version), minimum_version(version <= MACBIN2 ? version : MACBIN2), secondary_header_size(0)
		{
		}

		MacBinary(version_t version, version_t minimum_version)
			: AppleSingleDouble(AppleSingleDouble::DOUBLE), version(version), minimum_version(version < minimum_version ? version : minimum_version), secondary_header_size(0)
		{
		}

		explicit MacBinary(AppleSingleDouble& apple, version_t version, version_t minimum_version)
			: AppleSingleDouble(apple, AppleSingleDouble::DOUBLE), version(version), minimum_version(version < minimum_version ? version : minimum_version), secondary_header_size(0)
		{
		}

		/* CRC16-CCITT */
		static uint16_t crc_step[256];

		void CRC_Initialize();

		void CRC_Step(uint8_t byte = 0);

		void Skip(Linker::Writer& wr, size_t count);

		void WriteData(Linker::Writer& wr, size_t count, const void * data);

		void WriteData(Linker::Writer& wr, size_t count, std::string text);

		void WriteWord(Linker::Writer& wr, size_t bytes, uint64_t value);

		void WriteHeader(Linker::Writer& wr);

		void WriteFile(Linker::Writer& wr) override;

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
		} target;

		/* other files to produce */
		enum produce_format_t
		{
			PRODUCE_RESOURCE_FORK = 1 << 0, /* under .rsrc */
			PRODUCE_FINDER_INFO = 1 << 1, /* under .finf */
			PRODUCE_APPLE_DOUBLE = 1 << 2, /* with % prefix */
			PRODUCE_MAC_BINARY = 1 << 3, /* with .mbin extension */
		} produce;

		/* Typical combinations:
		 * - Executor: Generate a data fork and an AppleDouble with % prefix
		 * - Basilisk: Generate a data fork, a resource fork (under .rsrc) and a Finder Info file (under .finf)
		 * - Generate a MacBinary with .mbin extension
		 * - Generate an AppleSingle
		 */

		unsigned apple_single_double_version;
		/* Only relevant for version 1 */
		AppleSingleDouble::hfs_type home_file_system;

		MacBinary::version_t macbinary_version, macbinary_minimum_version;

		MacDriver(target_format_t target = TARGET_DATA_FORK)
			: target(target),
			produce(target == TARGET_NONE ? PRODUCE_MAC_BINARY
				: target == TARGET_DATA_FORK ? PRODUCE_APPLE_DOUBLE
				: produce_format_t(0)),
			apple_single_double_version(2),
			home_file_system(AppleSingleDouble::HFS_UNDEFINED),
			macbinary_version(MacBinary::MACBIN3),
			macbinary_minimum_version(MacBinary::MACBIN2)
		{
		}

		MacDriver(target_format_t target, int produce)
			: target(target), produce((produce_format_t)produce),
			apple_single_double_version(2),
			home_file_system(AppleSingleDouble::HFS_UNDEFINED),
			macbinary_version(MacBinary::MACBIN3),
			macbinary_minimum_version(MacBinary::MACBIN2)
		{
		}

		bool FormatSupportsResources() const override;

		bool AddSupplementaryOutputFormat(std::string subformat) override;

	private:
		AppleSingleDouble * container;

	public:
		void SetOptions(std::map<std::string, std::string>& options) override;

		void SetModel(std::string model) override;

		void SetLinkScript(std::string script_file, std::map<std::string, std::string>& options) override;

		void GenerateFile(std::string filename, Linker::Module& module) override;

		void ReadFile(Linker::Reader& rd) override;

		void WriteFile(Linker::Writer& wr) override;
	};
}

#endif /* MACOS_H */
