
#include <cstring>
#include <filesystem>
#include "apple.h"
#include "macos.h" // for MacintoshResourceFileFormat
#include "../dumper/dumper.h"
#include "../linker/buffer.h"
#include "../linker/image.h"
#include "../linker/reader.h"
#include "../linker/writer.h"

using namespace Apple;

/* AppleSingle/AppleDouble */

offset_t AppleSingleDouble::ImageSize() const
{
	return image_size;
}

AppleSingleDouble::hfs_type AppleSingleDouble::GetHomeFileSystem() const
{
	return home_file_system;
}

void AppleSingleDouble::SetHomeFileSystem(hfs_type type)
{
	switch(home_file_system = type)
	{
	default:
		Linker::Error << "Error: undefined home file system" << std::endl;
	case HFS_UNDEFINED:
		memcpy(home_file_system_string, TXT_undefined, 16);
		break;
	case HFS_Macintosh:
		memcpy(home_file_system_string, TXT_Macintosh, 16);
		break;
	case HFS_ProDOS:
		memcpy(home_file_system_string, TXT_ProDOS, 16);
		break;
	case HFS_MSDOS:
		memcpy(home_file_system_string, TXT_MS_DOS, 16);
		break;
	case HFS_UNIX:
		memcpy(home_file_system_string, TXT_UNIX, 16);
		break;
	case HFS_VAX_VMS:
		memcpy(home_file_system_string, TXT_VAX_VMS, 16);
		break;
	}
}

void AppleSingleDouble::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = ::BigEndian;
	type = format_type(rd.ReadUnsigned(4));
	if(type != SINGLE && type != DOUBLE)
	{
		Linker::Error << "Error: invalid AppleSingle/AppleDouble type" << std::hex << type << std::endl;
		type = format_type(0);
	}
	version = rd.ReadUnsigned(4) >> 16;
	rd.ReadData(16, home_file_system_string);
	if(memcmp(home_file_system_string, TXT_Macintosh, 16) == 0)
	{
		home_file_system = HFS_Macintosh;
	}
	else if(memcmp(home_file_system_string, TXT_ProDOS, 16) == 0)
	{
		home_file_system = HFS_ProDOS;
	}
	else if(memcmp(home_file_system_string, TXT_MS_DOS, 16) == 0)
	{
		home_file_system = HFS_ProDOS;
	}
	else if(memcmp(home_file_system_string, TXT_UNIX, 16) == 0)
	{
		home_file_system = HFS_UNIX;
	}
	else if(memcmp(home_file_system_string, TXT_VAX_VMS, 16) == 0)
	{
		home_file_system = HFS_VAX_VMS;
	}
	else
	{
		if(memcmp(home_file_system_string, TXT_undefined, 16) != 0)
		{
			Linker::Error << "Error: unidentified home file system" << std::endl;
		}
		home_file_system = HFS_UNDEFINED;
	}
	uint16_t entry_count = rd.ReadUnsigned(2);
	for(uint16_t i = 0; i < entry_count; i++)
	{
		entries.emplace_back(Entry::ReadEntry(rd, home_file_system));
	}
	image_size = rd.Tell();
	for(auto entry : entries)
	{
		rd.Seek(entry->file_offset);
		entry->ReadFile(rd);
		image_size = std::max(image_size, entry->file_offset + entry->image_size);
	}
}

// Entry

std::shared_ptr<AppleSingleDouble::Entry> AppleSingleDouble::Entry::ReadEntry(Linker::Reader& rd, hfs_type home_file_system)
{
	std::shared_ptr<Entry> entry;

	uint32_t id = rd.ReadUnsigned(4);
	switch(id)
	{
	case ID_DataFork:
		entry = std::make_shared<DataFork>();
		break;
	case ID_ResourceFork:
		entry = std::make_shared<ResourceFork>();
		break;
	case ID_RealName:
		entry = std::make_shared<RealName>();
		break;
	case ID_Comment:
		entry = std::make_shared<Comment>();
		break;
	case ID_IconBW:
		entry = std::make_shared<IconBW>();
		break;
	case ID_IconColor:
		entry = std::make_shared<IconColor>();
		break;
	case ID_FileInfo: /* v1 only */
		switch(home_file_system)
		{
		case HFS_Macintosh:
			entry = std::make_shared<FileInfo::Macintosh>();
			break;
		case HFS_ProDOS:
			entry = std::make_shared<FileInfo::ProDOS>();
			break;
		case HFS_MSDOS:
			entry = std::make_shared<FileInfo::MSDOS>();
			break;
		case HFS_UNIX:
			entry = std::make_shared<FileInfo::AUX>();
			break;
		default:
			entry = std::make_shared<UnknownEntry>(id);
			break;
		}
		break;
	case ID_FileDatesInfo: /* v2 only */
		entry = std::make_shared<FileDatesInfo>();
		break;
	case ID_FinderInfo:
		entry = std::make_shared<FinderInfo>();
		break;
	case ID_MacintoshFileInfo: /* v2 only */
		entry = std::make_shared<MacintoshFileInfo>();
		break;
	case ID_ProDOSFileInfo: /* v2 only */
		entry = std::make_shared<ProDOSFileInfo>();
		break;
	case ID_MSDOSFileInfo: /* v2 only */
		entry = std::make_shared<MSDOSFileInfo>();
		break;
	case ID_AFPShortName: /* v2 only */
		entry = std::make_shared<AFPShortName>();
		break;
	case ID_AFPFileInfo: /* v2 only */
		entry = std::make_shared<AFPFileInfo>();
		break;
	case ID_AFPDirectoryID: /* v2 only */
		entry = std::make_shared<AFPDirectoryID>();
		break;
	default:
		entry = std::make_shared<UnknownEntry>(id);
		break;
	}
	entry->file_offset = rd.ReadUnsigned(4);
	entry->image_size = rd.ReadUnsigned(4);
	return entry;
}

void AppleSingleDouble::Entry::DumpEntry(Dumper::Dumper& dump, unsigned index) const
{
	Dumper::Region entry_region("Entry", file_offset, image_size, 8);
	entry_region.InsertField(0, "Number", Dumper::DecDisplay::Make(), offset_t(index + 1));
	static const std::map<offset_t, std::string> id_descriptions =
	{
		{ ID_DataFork, "data fork" },
		{ ID_ResourceFork, "resource fork" },
		{ ID_RealName, "real name" },
		{ ID_Comment, "comment" },
		{ ID_IconBW, "icon, black and white" },
		{ ID_IconColor, "icon, color" },
		{ ID_FileInfo, "file info (version 1 only)" },
		{ ID_FileDatesInfo, "file dates info (version 2 only)" },
		{ ID_FinderInfo, "finder info" },
		{ ID_MacintoshFileInfo, "Macintosh file info (version 2 only)" },
		{ ID_ProDOSFileInfo, "ProDOS file info (version 2 only)" },
		{ ID_MSDOSFileInfo, "MS-DOS file info (version 2 only)" },
		{ ID_AFPShortName, "AFP short name (version 2 only)" },
		{ ID_AFPFileInfo, "AFP file info (version 2 only)" },
		{ ID_AFPDirectoryID, "AFP directory ID (version 2 only)" },
	};
	entry_region.AddField("Id", Dumper::ChoiceDisplay::Make(id_descriptions, Dumper::HexDisplay::Make(8)), offset_t(id));
	entry_region.Display(dump, Dumper::All);
}

void AppleSingleDouble::Entry::CalculateValues()
{
}

// UnknownEntry

offset_t AppleSingleDouble::UnknownEntry::ImageSize() const
{
	return image->ImageSize();
}

void AppleSingleDouble::UnknownEntry::ReadFile(Linker::Reader& rd)
{
	image = Linker::Buffer::ReadFromFile(rd, image_size);
}

offset_t AppleSingleDouble::UnknownEntry::WriteFile(Linker::Writer& out) const
{
	if(image != nullptr)
	{
		image->WriteFile(out);
		return image->ImageSize();
	}
	else
	{
		return 0;
	}
}

void AppleSingleDouble::UnknownEntry::Dump(Dumper::Dumper& dump) const
{
	Dumper::Block block("Block", file_offset, image->AsImage(), 0, 8);
	block.Display(dump, Dumper::Miscellaneous);
}

void AppleSingleDouble::UnknownEntry::CalculateValues()
{
	image_size = image != nullptr ? image->ImageSize() : 0;
}

const char AppleSingleDouble::TXT_undefined[16] = "";
const char AppleSingleDouble::TXT_Macintosh[16] = "Macintosh";
const char AppleSingleDouble::TXT_ProDOS[16] = "ProDOS";
const char AppleSingleDouble::TXT_MS_DOS[16] = "MS-DOS";
const char AppleSingleDouble::TXT_UNIX[16] = "Unix";
const char AppleSingleDouble::TXT_VAX_VMS[16] = "VAX VMS";

std::shared_ptr<const AppleSingleDouble::Entry> AppleSingleDouble::FindEntry(uint32_t id) const
{
	for(auto entry : entries)
	{
		if(entry->id == id)
			return entry;
	}
	return nullptr;
}

std::shared_ptr<AppleSingleDouble::Entry> AppleSingleDouble::FindEntry(uint32_t id)
{
	return std::const_pointer_cast<AppleSingleDouble::Entry>(const_cast<const AppleSingleDouble *>(this)->FindEntry(id));
}

void AppleSingleDouble::AppendEntry(std::shared_ptr<Entry> entry)
{
	for(auto entry_iter = entries.begin(); entry_iter != entries.end(); entry_iter ++)
	{
		if((*entry_iter)->id == entry->id)
		{
			entries.erase(entry_iter);
			break;
		}
	}

	entries.push_back(entry);
}

std::shared_ptr<AppleSingleDouble::Entry> AppleSingleDouble::GetFileDatesInfo()
{
	std::shared_ptr<Entry> entry;
	switch(version)
	{
	case 1:
		entry = FindEntry(ID_FileInfo);
		if(entry == nullptr)
		{
			switch(home_file_system)
			{
			case HFS_Macintosh:
				entry = std::make_shared<FileInfo::Macintosh>();
				break;
			case HFS_ProDOS:
				entry = std::make_shared<FileInfo::ProDOS>();
				break;
			case HFS_MSDOS:
				entry = std::make_shared<FileInfo::MSDOS>();
				break;
			case HFS_UNIX:
				entry = std::make_shared<FileInfo::AUX>();
				break;
			default:
				return nullptr;
			}
			entries.push_back(entry);
		}
		return entry;
	case 2:
		entry = FindEntry(ID_FileDatesInfo);
		if(entry == nullptr)
			entry = std::make_shared<FileDatesInfo>();
		return entry;
	default:
		Linker::FatalError("Internal error: invalid AppleSingle/AppleDouble version");
	}
}

std::shared_ptr<AppleSingleDouble::Entry> AppleSingleDouble::GetMacintoshFileInfo()
{
	std::shared_ptr<Entry> entry;
	switch(version)
	{
	case 1:
		if(home_file_system != HFS_Macintosh)
			return nullptr;
		else
			return GetFileDatesInfo();
	case 2:
		entry = FindEntry(ID_MacintoshFileInfo);
		if(entry == nullptr)
		{
			entry = std::make_shared<MacintoshFileInfo>();
			entries.push_back(entry);
		}
		return entry;
	default:
		Linker::FatalError("Internal error: invalid AppleSingle/AppleDouble version");
	}
}

std::shared_ptr<AppleSingleDouble::Entry> AppleSingleDouble::GetAUXFileInfo()
{
	switch(version)
	{
	case 1:
		if(home_file_system != HFS_UNIX)
			return nullptr;
		else
			return GetFileDatesInfo();
	case 2:
		return nullptr;
	default:
		Linker::FatalError("Internal error: invalid AppleSingle/AppleDouble version");
	}
}

std::shared_ptr<AppleSingleDouble::Entry> AppleSingleDouble::GetProDOSFileInfo()
{
	std::shared_ptr<Entry> entry;
	switch(version)
	{
	case 1:
		if(home_file_system != HFS_ProDOS)
			return nullptr;
		else
			return GetFileDatesInfo();
	case 2:
		entry = FindEntry(ID_ProDOSFileInfo);
		if(entry == nullptr)
		{
			entry = std::make_shared<ProDOSFileInfo>();
			entries.push_back(entry);
		}
		return entry;
	default:
		Linker::FatalError("Internal error: invalid AppleSingle/AppleDouble version");
	}
}

std::shared_ptr<AppleSingleDouble::Entry> AppleSingleDouble::GetMSDOSFileInfo()
{
	std::shared_ptr<Entry> entry;
	switch(version)
	{
	case 1:
		if(home_file_system != HFS_MSDOS)
			return nullptr;
		else
			return GetFileDatesInfo();
	case 2:
		entry = FindEntry(ID_MSDOSFileInfo);
		if(entry == nullptr)
		{
			entry = std::make_shared<MSDOSFileInfo>();
			entries.push_back(entry);
		}
		return entry;
	default:
		Linker::FatalError("Internal error: invalid AppleSingle/AppleDouble version");
	}
}

std::shared_ptr<AppleSingleDouble::Entry> AppleSingleDouble::GetDataFork()
{
	std::shared_ptr<Entry> entry;
	entry = FindEntry(ID_DataFork);
	if(entry == nullptr)
	{
		entry = std::make_shared<DataFork>();
		entries.push_back(entry);
	}
	return entry;
}

std::shared_ptr<AppleSingleDouble::Entry> AppleSingleDouble::GetResourceFork()
{
	std::shared_ptr<Entry> entry;
	entry = FindEntry(ID_ResourceFork);
	if(entry == nullptr)
	{
		entry = std::make_shared<ResourceFork>();
		entries.push_back(entry);
	}
	return entry;
}

std::shared_ptr<AppleSingleDouble::Entry> AppleSingleDouble::GetFinderInfo()
{
	std::shared_ptr<Entry> entry;
	entry = FindEntry(ID_FinderInfo);
	if(entry == nullptr)
	{
		entry = std::make_shared<FinderInfo>();
		entries.push_back(entry);
	}
	return entry;
}

std::shared_ptr<AppleSingleDouble::Entry> AppleSingleDouble::GetRealName()
{
	std::shared_ptr<Entry> entry;
	entry = FindEntry(ID_RealName);
	if(entry == nullptr)
	{
		entry = std::make_shared<RealName>();
		entries.push_back(entry);
	}
	return entry;
}

void AppleSingleDouble::SetCreationDate(uint32_t CreationDate)
{
	std::shared_ptr<Entry> entry;
	switch(version)
	{
	case 1:
		switch(home_file_system)
		{
		case HFS_Macintosh:
			entry = GetMacintoshFileInfo();
			if(entry != nullptr)
				std::dynamic_pointer_cast<FileInfo::Macintosh>(entry)->CreationDate = CreationDate;
			break;
		case HFS_UNIX:
			entry = GetAUXFileInfo();
			if(entry != nullptr)
				std::dynamic_pointer_cast<FileInfo::AUX>(entry)->CreationDate = CreationDate;
			break;
		case HFS_ProDOS:
			entry = GetProDOSFileInfo();
			if(entry != nullptr)
				std::dynamic_pointer_cast<FileInfo::ProDOS>(entry)->CreationDate = CreationDate;
			break;
		default:
			break;
		}
		break;
	case 2:
		entry = GetFileDatesInfo();
		if(entry != nullptr)
			std::dynamic_pointer_cast<FileDatesInfo>(entry)->CreationDate = CreationDate;
		break;
	}
}

uint32_t AppleSingleDouble::GetCreationDate()
{
	std::shared_ptr<Entry> entry;
	switch(version)
	{
	case 1:
		switch(home_file_system)
		{
		case HFS_Macintosh:
			entry = GetMacintoshFileInfo();
			if(entry != nullptr)
				return std::dynamic_pointer_cast<FileInfo::Macintosh>(entry)->CreationDate;
			break;
		case HFS_UNIX:
			entry = GetAUXFileInfo();
			if(entry != nullptr)
				return std::dynamic_pointer_cast<FileInfo::AUX>(entry)->CreationDate;
			break;
		case HFS_ProDOS:
			entry = GetProDOSFileInfo();
			if(entry != nullptr)
				return std::dynamic_pointer_cast<FileInfo::ProDOS>(entry)->CreationDate;
			break;
		default:
			break;
		}
		break;
	case 2:
		entry = GetFileDatesInfo();
		if(entry != nullptr)
			return std::dynamic_pointer_cast<FileDatesInfo>(entry)->CreationDate;
	}
	return 0;
}

uint32_t AppleSingleDouble::ReadCreationDate()
{
	switch(version)
	{
	case 1:
		switch(home_file_system)
		{
		case HFS_Macintosh:
		case HFS_UNIX:
		case HFS_ProDOS:
			if(FindEntry(ID_FileInfo) != nullptr)
			{
				return GetCreationDate();
			}
			break;
		default:
			break;
		}
		break;
	case 2:
		if(FindEntry(ID_FileDatesInfo) != nullptr)
		{
			return GetCreationDate();
		}
		break;
	}
	return 0;
}

void AppleSingleDouble::SetModificationDate(uint32_t ModificationDate)
{
	std::shared_ptr<Entry> entry;
	switch(version)
	{
	case 1:
		switch(home_file_system)
		{
		case HFS_Macintosh:
			entry = GetMacintoshFileInfo();
			if(entry != nullptr)
				std::dynamic_pointer_cast<FileInfo::Macintosh>(entry)->ModificationDate = ModificationDate;
			break;
		case HFS_UNIX:
			entry = GetAUXFileInfo();
			if(entry != nullptr)
				std::dynamic_pointer_cast<FileInfo::AUX>(entry)->ModificationDate = ModificationDate;
			break;
		case HFS_ProDOS:
			entry = GetProDOSFileInfo();
			if(entry != nullptr)
				std::dynamic_pointer_cast<FileInfo::ProDOS>(entry)->ModificationDate = ModificationDate;
			break;
		case HFS_MSDOS:
			entry = GetMSDOSFileInfo();
			if(entry != nullptr)
				std::dynamic_pointer_cast<FileInfo::MSDOS>(entry)->ModificationDate = ModificationDate;
			break;
		default:
			break;
		}
		break;
	case 2:
		entry = GetFileDatesInfo();
		if(entry != nullptr)
			std::dynamic_pointer_cast<FileDatesInfo>(entry)->ModificationDate = ModificationDate;
		break;
	}
}

uint32_t AppleSingleDouble::GetModificationDate()
{
	std::shared_ptr<Entry> entry;
	switch(version)
	{
	case 1:
		switch(home_file_system)
		{
		case HFS_Macintosh:
			entry = GetMacintoshFileInfo();
			if(entry != nullptr)
				return std::dynamic_pointer_cast<FileInfo::Macintosh>(entry)->ModificationDate;
			break;
		case HFS_UNIX:
			entry = GetAUXFileInfo();
			if(entry != nullptr)
				return std::dynamic_pointer_cast<FileInfo::AUX>(entry)->ModificationDate;
			break;
		case HFS_ProDOS:
			entry = GetProDOSFileInfo();
			if(entry != nullptr)
				return std::dynamic_pointer_cast<FileInfo::ProDOS>(entry)->ModificationDate;
			break;
		case HFS_MSDOS:
			entry = GetMSDOSFileInfo();
			if(entry != nullptr)
				return std::dynamic_pointer_cast<FileInfo::MSDOS>(entry)->ModificationDate;
			break;
		default:
			break;
		}
		break;
	case 2:
		entry = GetFileDatesInfo();
		if(entry != nullptr)
			return std::dynamic_pointer_cast<FileDatesInfo>(entry)->ModificationDate;
		break;
	}
	return 0;
}

uint32_t AppleSingleDouble::ReadModificationDate()
{
	switch(version)
	{
	case 1:
		switch(home_file_system)
		{
		case HFS_Macintosh:
		case HFS_UNIX:
		case HFS_ProDOS:
		case HFS_MSDOS:
			if(FindEntry(ID_FileInfo) != nullptr)
			{
				return GetModificationDate();
			}
			break;
		default:
			break;
		}
		break;
	case 2:
		if(FindEntry(ID_FileDatesInfo) != nullptr)
		{
			return GetModificationDate();
		}
		break;
	}
	return 0;
}

void AppleSingleDouble::SetBackupDate(uint32_t BackupDate)
{
	std::shared_ptr<Entry> entry;
	switch(version)
	{
	case 1:
		if(home_file_system == HFS_Macintosh)
		{
			entry = GetMacintoshFileInfo();
			if(entry != nullptr)
				std::dynamic_pointer_cast<FileInfo::Macintosh>(entry)->LastBackupDate = BackupDate;
		}
		break;
	case 2:
		entry = GetFileDatesInfo();
		if(entry != nullptr)
			std::dynamic_pointer_cast<FileDatesInfo>(entry)->BackupDate = BackupDate;
		break;
	}
}

void AppleSingleDouble::SetAccessDate(uint32_t AccessDate)
{
	std::shared_ptr<Entry> entry;
	switch(version)
	{
	case 1:
		if(home_file_system == HFS_UNIX)
		{
			entry = GetAUXFileInfo();
			if(entry != nullptr)
				std::dynamic_pointer_cast<FileInfo::AUX>(entry)->AccessDate = AccessDate;
		}
		break;
	case 2:
		entry = GetFileDatesInfo();
		if(entry != nullptr)
			std::dynamic_pointer_cast<FileDatesInfo>(entry)->AccessDate = AccessDate;
		break;
	}
}

void AppleSingleDouble::SetMacintoshAttributes(uint32_t Attributes)
{
	std::shared_ptr<Entry> entry = GetMacintoshFileInfo();
	if(entry == nullptr)
		return;
	switch(version)
	{
	case 1:
		std::dynamic_pointer_cast<FileInfo::Macintosh>(entry)->Attributes = Attributes;
		break;
	case 2:
		std::dynamic_pointer_cast<MacintoshFileInfo>(entry)->Attributes = Attributes;
		break;
	}
}

uint32_t AppleSingleDouble::GetMacintoshAttributes()
{
	std::shared_ptr<const Entry> entry = GetMacintoshFileInfo();
	if(entry == nullptr)
		return 0;
	switch(version)
	{
	case 1:
		return std::dynamic_pointer_cast<const FileInfo::Macintosh>(entry)->Attributes;
	case 2:
		return std::dynamic_pointer_cast<const MacintoshFileInfo>(entry)->Attributes;
	default:
		return 0;
	}
}

uint32_t AppleSingleDouble::ReadMacintoshAttributes()
{
	std::shared_ptr<Entry> entry;
	switch(version)
	{
	case 1:
		if(home_file_system == HFS_Macintosh
		&& FindEntry(ID_FileInfo) != nullptr)
		{
			return GetMacintoshAttributes();
		}
		break;
	case 2:
		if(FindEntry(ID_MacintoshFileInfo) != nullptr)
		{
			return GetMacintoshAttributes();
		}
		break;
	default:
		Linker::FatalError("Internal error: invalid AppleSingle/AppleDouble version");
	}

	return 0;
}

void AppleSingleDouble::SetProDOSAccess(uint16_t Access)
{
	std::shared_ptr<Entry> entry = GetProDOSFileInfo();
	if(entry == nullptr)
		return;
	switch(version)
	{
	case 1:
		std::dynamic_pointer_cast<FileInfo::ProDOS>(entry)->Access = Access;
		break;
	case 2:
		std::dynamic_pointer_cast<ProDOSFileInfo>(entry)->Access = Access;
		break;
	}
}

void AppleSingleDouble::SetProDOSFileType(uint16_t FileType)
{
	std::shared_ptr<Entry> entry = GetProDOSFileInfo();
	if(entry == nullptr)
		return;
	switch(version)
	{
	case 1:
		std::dynamic_pointer_cast<FileInfo::ProDOS>(entry)->FileType = FileType;
		break;
	case 2:
		std::dynamic_pointer_cast<ProDOSFileInfo>(entry)->FileType = FileType;
		break;
	}
}

void AppleSingleDouble::SetProDOSAUXType(uint32_t AUXType)
{
	std::shared_ptr<Entry> entry = GetProDOSFileInfo();
	if(entry == nullptr)
		return;
	switch(version)
	{
	case 1:
		std::dynamic_pointer_cast<FileInfo::ProDOS>(entry)->AUXType = AUXType;
		break;
	case 2:
		std::dynamic_pointer_cast<ProDOSFileInfo>(entry)->AUXType = AUXType;
		break;
	}
}

void AppleSingleDouble::SetMSDOSAttributes(uint16_t Attributes)
{
	std::shared_ptr<Entry> entry = GetMSDOSFileInfo();
	if(entry == nullptr)
		return;
	switch(version)
	{
	case 1:
		std::dynamic_pointer_cast<FileInfo::MSDOS>(entry)->Attributes = Attributes;
		break;
	case 2:
		std::dynamic_pointer_cast<MSDOSFileInfo>(entry)->Attributes = Attributes;
		break;
	}
}

void AppleSingleDouble::CalculateValues()
{
	unsigned entry_bitmap = 0;
	for(auto entry : entries)
	{
		if(entry->id < 32)
			entry_bitmap |= 1 << entry->id;
	}
	if(type == SINGLE && (entry_bitmap & (1 << ID_DataFork)) != 0)
	{
		GetDataFork();
	}

	offset_t current_offset = 26 + 12 * entries.size();
	for(auto entry : entries)
	{
		entry->file_offset = current_offset;
		entry->CalculateValues();
		entry->image_size = entry->ImageSize();
		current_offset = entry->file_offset + entry->image_size;
	}
	image_size = current_offset;
}

offset_t AppleSingleDouble::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::BigEndian;
	wr.WriteWord(4, type);
	wr.WriteWord(4, version << 16);
	wr.WriteData(16, home_file_system_string);
	wr.WriteWord(2, entries.size());
	for(auto entry : entries)
	{
		wr.WriteWord(4, entry->id);
		wr.WriteWord(4, entry->file_offset);
		wr.WriteWord(4, entry->image_size);
	}
	for(auto entry : entries)
	{
		entry->WriteFile(wr);
	}

	return ImageSize();
}

void AppleSingleDouble::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_macroman);

	dump.SetTitle("AppleSingle/AppleDouble format");
	Dumper::Region file_region("File", file_offset, ImageSize(), 8);
	file_region.Display(dump, Dumper::Header);

	unsigned i = 0;
	for(auto& entry : entries)
	{
		entry->DumpEntry(dump, i);
		i++;
	}

	for(auto& entry : entries)
	{
		entry->Dump(dump);
	}
}

std::string AppleSingleDouble::PrefixFilename(std::string prefix, std::string filename)
{
	std::filesystem::path path(filename);
	return (path.parent_path() / (prefix + path.filename().string())).string();
}

std::string AppleSingleDouble::PrefixFilename(std::string prefix, std::string filename, size_t limit)
{
	std::filesystem::path path(filename);
	filename = prefix + path.filename().string();
	if(filename.size() > 16)
		filename = filename.substr(0, limit);
	return (path.parent_path() / filename).string();
}

std::string AppleSingleDouble::ReplaceExtension(std::string filename, std::string extension, size_t limit)
{
	std::filesystem::path path(filename);
	std::string stem = path.stem();
	if(stem.size() > limit)
	{
		return (path.parent_path() / path.stem().string().substr(0, limit)).replace_extension(extension);
	}
	else
	{
		return path.replace_extension(extension);
	}
}

std::string AppleSingleDouble::GetUNIXDoubleFilename(std::string filename)
{
	return PrefixFilename("%", filename);
}

std::string AppleSingleDouble::GetMacOSXDoubleFilename(std::string filename)
{
	return PrefixFilename("._", filename);
}

std::string AppleSingleDouble::GetProDOSDoubleFilename(std::string filename)
{
	return PrefixFilename("R.", filename, 16);
}

std::string AppleSingleDouble::GetMSDOSDoubleFilename(std::string filename)
{
	return ReplaceExtension(filename, ".adf", 8);
}

// DataFork

offset_t DataFork::ImageSize() const
{
	return image ? image->ImageSize() : 0;
}

void DataFork::ReadFile(Linker::Reader& rd)
{
	if(auto format = std::dynamic_pointer_cast<Linker::Format>(image))
	{
		format->ReadFile(rd);
	}
	else
	{
		image = Linker::Buffer::ReadFromFile(rd, image_size);
	}
}

offset_t DataFork::WriteFile(Linker::Writer& out) const
{
	if(image != nullptr)
	{
		image->WriteFile(out);
		return image->ImageSize();
	}
	else
	{
		return 0;
	}
}

void DataFork::Dump(Dumper::Dumper& dump) const
{
	if(auto format = std::dynamic_pointer_cast<Linker::Format>(image))
	{
		Dumper::Region region("Data fork", file_offset, image->ImageSize(), 8);
		region.Display(dump, Dumper::Header);

		format->Dump(dump);
	}
	else
	{
		Dumper::Block block("Data fork", file_offset, image->AsImage(), 0, 8);
		block.Display(dump, Dumper::Image);
	}
}

void DataFork::CalculateValues()
{
	if(auto format = std::dynamic_pointer_cast<Linker::OutputFormat>(image))
	{
		format->CalculateValues();
	}

	image_size = image != nullptr ? image->ImageSize() : 0;
}

// ResourceFork

offset_t ResourceFork::ImageSize() const
{
	return image ? image->ImageSize() : 0;
}

void ResourceFork::ReadFile(Linker::Reader& rd)
{
	if(auto format = std::dynamic_pointer_cast<Linker::Format>(image))
	{
		format->ReadFile(rd);
	}
	else
	{
		// TODO: check file type
		//image = Linker::Buffer::ReadFromFile(rd, image_size);
		auto mac_rsrc = std::make_shared<MacintoshResourceFileFormat>();
		mac_rsrc->ReadFile(rd);
		image = mac_rsrc;
	}
}

offset_t ResourceFork::WriteFile(Linker::Writer& out) const
{
	if(image != nullptr)
	{
		image->WriteFile(out);
		return image->ImageSize();
	}
	else
	{
		return 0;
	}
}

void ResourceFork::Dump(Dumper::Dumper& dump) const
{
	if(auto format = std::dynamic_pointer_cast<Linker::Format>(image))
	{
		Dumper::Region region("Data fork", file_offset, image->ImageSize(), 8);
		region.Display(dump, Dumper::Header);

		format->Dump(dump);
	}
	else
	{
		Dumper::Block block("Data fork", file_offset, image->AsImage(), 0, 8);
		block.Display(dump, Dumper::Image);
	}
}

void ResourceFork::CalculateValues()
{
	if(auto format = std::dynamic_pointer_cast<Linker::OutputFormat>(image))
	{
		format->CalculateValues();
	}

	image_size = image != nullptr ? image->ImageSize() : 0;
}

// RealName

offset_t RealName::ImageSize() const
{
	return name.size();
}

void RealName::ReadFile(Linker::Reader& rd)
{
	// TODO
}

offset_t RealName::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::BigEndian;
	wr.WriteData(name.size(), name.c_str());

	return offset_t(-1);
}

void RealName::Dump(Dumper::Dumper& dump) const
{
	// TODO
}

// Comment

offset_t Comment::ImageSize() const
{
	return offset_t(-1); // TODO
}

void Comment::ReadFile(Linker::Reader& rd)
{
	// TODO
}

offset_t Comment::WriteFile(Linker::Writer& out) const
{
	// TODO
	return ImageSize();
}

void Comment::Dump(Dumper::Dumper& dump) const
{
	// TODO
}

// IconBW

offset_t IconBW::ImageSize() const
{
	return offset_t(-1); // TODO
}

void IconBW::ReadFile(Linker::Reader& rd)
{
	// TODO
}

offset_t IconBW::WriteFile(Linker::Writer& out) const
{
	// TODO
	return ImageSize();
}

void IconBW::Dump(Dumper::Dumper& dump) const
{
	// TODO
}

// IconColor

offset_t IconColor::ImageSize() const
{
	return offset_t(-1); // TODO
}

void IconColor::ReadFile(Linker::Reader& rd)
{
	// TODO
}

offset_t IconColor::WriteFile(Linker::Writer& out) const
{
	// TODO
	return ImageSize();
}

void IconColor::Dump(Dumper::Dumper& dump) const
{
	// TODO
}

// FileInfo::Macintosh

offset_t FileInfo::Macintosh::ImageSize() const
{
	return 16;
}

void FileInfo::Macintosh::ReadFile(Linker::Reader& rd)
{
	// TODO
}

offset_t FileInfo::Macintosh::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::BigEndian;
	wr.WriteWord(4, CreationDate);
	wr.WriteWord(4, ModificationDate);
	wr.WriteWord(4, LastBackupDate);
	wr.WriteWord(4, Attributes);

	return offset_t(-1);
}

void FileInfo::Macintosh::Dump(Dumper::Dumper& dump) const
{
	// TODO
}

// FileInfo::ProDOS

offset_t FileInfo::ProDOS::ImageSize() const
{
	return 16;
}

void FileInfo::ProDOS::ReadFile(Linker::Reader& rd)
{
	// TODO
}

offset_t FileInfo::ProDOS::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::BigEndian;
	wr.WriteWord(4, CreationDate);
	wr.WriteWord(4, ModificationDate);
	wr.WriteWord(2, Access);
	wr.WriteWord(2, FileType);
	wr.WriteWord(4, AUXType);

	return offset_t(-1);
}

void FileInfo::ProDOS::Dump(Dumper::Dumper& dump) const
{
	// TODO
}

// FileInfo::MSDOS

offset_t FileInfo::MSDOS::ImageSize() const
{
	return 6;
}

void FileInfo::MSDOS::ReadFile(Linker::Reader& rd)
{
	// TODO
}

offset_t FileInfo::MSDOS::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::BigEndian;
	wr.WriteWord(4, ModificationDate);
	wr.WriteWord(2, Attributes);

	return offset_t(-1);
}

void FileInfo::MSDOS::Dump(Dumper::Dumper& dump) const
{
	// TODO
}

// FileInfo::AUX

offset_t FileInfo::AUX::ImageSize() const
{
	return 12;
}

void FileInfo::AUX::ReadFile(Linker::Reader& rd)
{
	// TODO
}

offset_t FileInfo::AUX::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::BigEndian;
	wr.WriteWord(4, CreationDate);
	wr.WriteWord(4, AccessDate);
	wr.WriteWord(4, ModificationDate);

	return offset_t(-1);
}

void FileInfo::AUX::Dump(Dumper::Dumper& dump) const
{
	// TODO
}

// FileDatesInfo

offset_t FileDatesInfo::ImageSize() const
{
	return 16;
}

void FileDatesInfo::ReadFile(Linker::Reader& rd)
{
	// TODO
}

offset_t FileDatesInfo::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::BigEndian;
	wr.WriteWord(4, CreationDate);
	wr.WriteWord(4, ModificationDate);
	wr.WriteWord(4, BackupDate);
	wr.WriteWord(4, AccessDate);

	return offset_t(-1);
}

void FileDatesInfo::Dump(Dumper::Dumper& dump) const
{
	// TODO
}

// FinderInfo

offset_t FinderInfo::ImageSize() const
{
	return 32;
}

void FinderInfo::ReadFile(Linker::Reader& rd)
{
	// TODO
}

offset_t FinderInfo::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::BigEndian;
	wr.WriteData(4, Type);
	wr.WriteData(4, Creator);
	wr.WriteWord(2, Flags);
	wr.WriteWord(2, Location.x);
	wr.WriteWord(2, Location.y);
	wr.Skip(17);
	wr.WriteWord(1, 0);

	return offset_t(-1);
}

void FinderInfo::Dump(Dumper::Dumper& dump) const
{
	// TODO
}

void FinderInfo::SetTypeAndCreator(std::string type, std::string creator)
{
	if(type.size() < 4)
	{
		memcpy(Type, type.c_str(), type.size());
		memset(Type + type.size(), 0, 4 - type.size());
	}
	else
	{
		memcpy(Type, type.c_str(), 4);
	}

	if(creator.size() < 4)
	{
		memcpy(Creator, creator.c_str(), creator.size());
		memset(Type + creator.size(), 0, 4 - creator.size());
	}
	else
	{
		memcpy(Creator, creator.c_str(), 4);
	}
}

// MacintoshFileInfo

offset_t MacintoshFileInfo::ImageSize() const
{
	return 4;
}

void MacintoshFileInfo::ReadFile(Linker::Reader& rd)
{
	// TODO
}

offset_t MacintoshFileInfo::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::BigEndian;
	wr.WriteWord(4, Attributes);

	return offset_t(-1);
}

void MacintoshFileInfo::Dump(Dumper::Dumper& dump) const
{
	// TODO
}

// ProDOSFileInfo

offset_t ProDOSFileInfo::ImageSize() const
{
	return 8;
}

void ProDOSFileInfo::ReadFile(Linker::Reader& rd)
{
	// TODO
}

offset_t ProDOSFileInfo::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::BigEndian;
	wr.WriteWord(2, Access);
	wr.WriteWord(2, FileType);
	wr.WriteWord(4, AUXType);

	return offset_t(-1);
}

void ProDOSFileInfo::Dump(Dumper::Dumper& dump) const
{
	// TODO
}

// MSDOSFileInfo

offset_t MSDOSFileInfo::ImageSize() const
{
	return 2;
}

void MSDOSFileInfo::ReadFile(Linker::Reader& rd)
{
	// TODO
}

offset_t MSDOSFileInfo::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::BigEndian;
	wr.WriteWord(2, Attributes);

	return offset_t(-1);
}

void MSDOSFileInfo::Dump(Dumper::Dumper& dump) const
{
	// TODO
}

// AFPShortName

offset_t AFPShortName::ImageSize() const
{
	return offset_t(-1); // TODO
}

void AFPShortName::ReadFile(Linker::Reader& rd)
{
	// TODO
}

offset_t AFPShortName::WriteFile(Linker::Writer& out) const
{
	// TODO
	return ImageSize();
}

void AFPShortName::Dump(Dumper::Dumper& dump) const
{
	// TODO
}

// AFPFileInfo

offset_t AFPFileInfo::ImageSize() const
{
	return offset_t(-1); // TODO
}

void AFPFileInfo::ReadFile(Linker::Reader& rd)
{
	// TODO
}

offset_t AFPFileInfo::WriteFile(Linker::Writer& out) const
{
	// TODO
	return ImageSize();
}

void AFPFileInfo::Dump(Dumper::Dumper& dump) const
{
	// TODO
}

// AFPDirectoryID

offset_t AFPDirectoryID::ImageSize() const
{
	return offset_t(-1); // TODO
}

void AFPDirectoryID::ReadFile(Linker::Reader& rd)
{
	// TODO
}

offset_t AFPDirectoryID::WriteFile(Linker::Writer& out) const
{
	// TODO
	return ImageSize();
}

void AFPDirectoryID::Dump(Dumper::Dumper& dump) const
{
	// TODO
}

