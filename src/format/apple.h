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

		class UnknownEntry : public Entry
		{
		public:
			std::shared_ptr<Linker::Contents> image;

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

	class DataFork : public AppleSingleDouble::Entry
	{
	public:
		std::shared_ptr<Linker::Contents> image;

		DataFork()
			: Entry(AppleSingleDouble::ID_DataFork)
		{
		}

		DataFork(std::shared_ptr<Linker::Contents> image)
			: Entry(AppleSingleDouble::ID_DataFork), image(image)
		{
		}

		offset_t ImageSize() const override;

		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& out) const override;

		void Dump(Dumper::Dumper& dump) const override;

		void CalculateValues() override;
	};

	/** @brief Container for a resource fork
	 *
	 * Multiple resource file formats may be supported, for example Macintosh and GS/OS resources.
	 */
	class ResourceFork : public AppleSingleDouble::Entry
	{
	public:
		/** @brief The actual resource image, for example Apple::MacintoshResourceFileFormat */
		std::shared_ptr<Linker::Contents> image;

		ResourceFork()
			: Entry(AppleSingleDouble::ID_ResourceFork)
		{
		}

		ResourceFork(std::shared_ptr<Linker::Contents> image)
			: Entry(AppleSingleDouble::ID_ResourceFork), image(image)
		{
		}

		offset_t ImageSize() const override;

		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& out) const override;

		void Dump(Dumper::Dumper& dump) const override;

		void CalculateValues() override;
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
}

#endif /* APPLE_H */
