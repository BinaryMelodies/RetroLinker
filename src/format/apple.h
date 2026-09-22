#ifndef APPLE_H
#define APPLE_H

#include "../linker/format.h"

/* Structures common to multiple Apple products */

namespace Apple
{
	/* TODO: rework with Linker::Format */

	/**
	 * @brief AppleSingle & AppleDouble
	 *
	 * Permits storing certain metadata alongside the byte stream of a file, used for several Apple products
	 *
	 * On classic Macintosh systems, each file has two associated parts called forks: the data fork and the resource fork.
	 * Classic 68000 applications store all the executable data in the resource fork, which is usually not transferred to non-Macintosh platforms.
	 * The AppleSingle and AppleDouble formats provide a way to store both the data and resource fork, or the resource fork, as a separate file, alongside other metadata, which is essential when storing it on a non-Macintosh system.
	 *
	 * This container format was first invented for the A/UX Apple UNIX system.
	 * It has two versions, and version 2 is used most commonly.
	 * See also Apple::MacintoshResourceFileFormat.
	 */
	class AppleSingleDouble : public Linker::Format
	{
	public:
		offset_t ImageSize() const override;
		void ReadFile(Linker::Reader& rd) override;

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

			virtual void CalculateValues();
		};

		class GenericEntry : public Entry
		{
		public:
			std::shared_ptr<Linker::Contents> image;

			GenericEntry(uint32_t id)
				: Entry(id)
			{
			}

			GenericEntry(uint32_t id, std::shared_ptr<Linker::Contents> image)
				: Entry(id), image(image)
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

		std::shared_ptr<const Entry> FindEntry(uint32_t id) const;
		std::shared_ptr<Entry> FindEntry(uint32_t id);
		void AppendEntry(std::shared_ptr<Entry> entry);

		std::shared_ptr<Entry> GetFileDatesInfo();
		std::shared_ptr<Entry> GetMacintoshFileInfo();
		std::shared_ptr<Entry> GetAUXFileInfo();
		std::shared_ptr<Entry> GetProDOSFileInfo();
		std::shared_ptr<Entry> GetMSDOSFileInfo();

		std::shared_ptr<Entry> GetDataFork();
		std::shared_ptr<Entry> GetResourceFork();
		std::shared_ptr<Entry> GetFinderInfo();
		std::shared_ptr<Entry> GetRealName();

		void SetCreationDate(uint32_t CreationDate);
		void SetModificationDate(uint32_t ModificationDate);
		void SetBackupDate(uint32_t BackupDate);
		void SetAccessDate(uint32_t AccessDate);
		void SetMacintoshAttributes(uint32_t Attributes);
		void SetProDOSAccess(uint16_t Access);
		void SetProDOSFileType(uint16_t FileType);
		void SetProDOSAUXType(uint32_t AUXType);
		void SetMSDOSAttributes(uint16_t Attributes);

		/** @brief Retrieves creation date field and creates it if it does not exist */
		uint32_t GetCreationDate();
		/** @brief Retrieves modification date field and creates it if it does not exist */
		uint32_t GetModificationDate();
		/** @brief Retrieves Macintosh attributes field and creates it if it does not exist */
		uint32_t GetMacintoshAttributes();

		/** @brief Retrieves creation date field if it exists */
		uint32_t ReadCreationDate();
		/** @brief Retrieves modification date field if it exists */
		uint32_t ReadModificationDate();
		/** @brief Retrieves Macintosh attributes field if it exists */
		uint32_t ReadMacintoshAttributes();

		void CalculateValues();
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
	};

#if 0
	/** @brief Container for a resource fork
	 *
	 * Multiple resource file formats may be supported, for example Macintosh and GS/OS resources.
	 */
#endif

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

		void SetTypeAndCreator(std::string type, std::string creator);
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

	/**
	 * @brief This is not actually a file format, but an interface to permit generating multiple binary outputs for various Apple computers
	 */
	class OutputDriver : public Linker::OutputFormat
	{
	protected:
		/** @brief Represents the file type of the main file */
		enum target_format_t
		{
			/** @brief Do not generate main file */
			TARGET_NONE,
			/** @brief Main file is a data fork, typically empty */
			TARGET_DATA_FORK,
			/** @brief Main file is a resource fork */
			TARGET_RESOURCE_FORK,
			/** @brief Main file is an AppleSingle */
			TARGET_APPLE_SINGLE,
			/** @brief Main file is an AppleDouble */
			TARGET_APPLE_DOUBLE,
			/** @brief Main file as a MacBinary (Macintosh only) */
			TARGET_MAC_BINARY,
			// TODO: Binary II
		};

		/** @brief Represents what additional files should be generated */
		enum produce_format_t
		{
			/** @brief Places a GS/OS format resource file under the directory .rsrc */
			PRODUCE_RESOURCE_FORK = 1 << 0,
			/** @brief Places a Finder Information file under the directory .finf (Macintosh only) */
			PRODUCE_FINDER_INFO = 1 << 1,
			/** @brief Creates an AppleDouble binary with the '%' prefix */
			PRODUCE_APPLE_DOUBLE = 1 << 2,
			/** @brief Creates a MacBinary with the .mbin extension (Macintosh only) */
			PRODUCE_MAC_BINARY = 1 << 3,
			/** @brief NuLib2 attribute preservation string suffix, such as #06xxxx (Apple II, III, IIgs only)*/
			PRODUCE_NAPS_SUFFIX = 1 << 4,
		};

	public:
		/** @brief Format of "filename" */
		target_format_t target;

		/** @brief Bitset of other files to produce */
		produce_format_t produce;

// TODO: move?
		/* the ProDOS file type */
		enum file_type_t : uint8_t
		{
			FILE_TYPE_BIN = 0x06,
			FILE_TYPE_SOS = 0x0C,
			FILE_TYPE_S16 = 0xB3,
			FILE_TYPE_EXE = 0xB5,
			FILE_TYPE_SYS = 0xFF,
		};

		unsigned apple_single_double_version = 2;
		/* Only relevant for version 1 */
		Apple::AppleSingleDouble::hfs_type home_file_system = Apple::AppleSingleDouble::HFS_UNDEFINED;

		MacBinary::version_t macbinary_version = MacBinary::MACBIN3, macbinary_minimum_version = MacBinary::MACBIN2;

	protected:
		OutputDriver(target_format_t target = TARGET_DATA_FORK, produce_format_t produce = produce_format_t(0))
			: target(target), produce(produce_format_t(produce))
		{
		}

		virtual bool SupportedSupplementaryFormat(produce_format_t produce) = 0;
	public:
		bool AddSupplementaryOutputFormat(std::string subformat) override;

	protected:
		/** @brief Format of container stored in memory */
		enum container_format_t
		{
			/** @brief No container is stored */
			CONTAINER_NONE,
			/** @brief Use an AppleSingle container */
			CONTAINER_APPLE_SINGLE,
			/** @brief Use a MacBinary container as well as an AppleSingle container (Macintosh only) */
			CONTAINER_MAC_BINARY,
		};
		/** @brief The container type used to store metainformation while processing */
		container_format_t container = CONTAINER_NONE;

		/** @brief Container for all the necessary additional information */
		std::shared_ptr<Apple::AppleSingleDouble> apple_single;
		/** @brief Container for MacBinary (Macintosh only) */
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

	protected:
		/** @brief Tasked to create all the requested files */
		void GenerateFiles(std::string filename, std::shared_ptr<Contents> data_fork, std::shared_ptr<Contents> resource_fork, uint8_t file_type, uint16_t auxiliary_file_type);

	public:
		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;

		void Dump(Dumper::Dumper& dump) const override;
	};
}

#endif /* APPLE_H */
