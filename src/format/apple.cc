
#include <cstring>
#include <filesystem>
#include "apple.h"
#include "macos.h" // for MacintoshResourceFileFormat
#include "gsos.h" // for GSOSResourceFileFormat
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

void AppleSingleDouble::ReadFile(const std::shared_ptr<Linker::Reader>& rd)
{
	rd->endiantype = ::BigEndian;
	type = format_type(rd->ReadUnsigned(4));
	if(type != SINGLE && type != DOUBLE)
	{
		Linker::Error << "Error: invalid AppleSingle/AppleDouble type" << std::hex << type << std::endl;
		type = format_type(0);
	}
	version = rd->ReadUnsigned(4) >> 16;
	rd->ReadData(16, home_file_system_string);
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
	uint16_t entry_count = rd->ReadUnsigned(2);
	for(uint16_t i = 0; i < entry_count; i++)
	{
		entries.emplace_back(Entry::ReadEntry(rd, home_file_system));
	}
	image_size = rd->Tell();
	for(auto entry : entries)
	{
		rd->Seek(entry->file_offset);
		entry->ReadFile(rd);
		image_size = std::max(image_size, entry->file_offset + entry->image_size);
	}
}

// Entry

std::shared_ptr<AppleSingleDouble::Entry> AppleSingleDouble::Entry::ReadEntry(const std::shared_ptr<Linker::Reader>& rd, hfs_type home_file_system)
{
	std::shared_ptr<Entry> entry;

	uint32_t id = rd->ReadUnsigned(4);
	switch(id)
	{
	case ID_DataFork:
		entry = std::make_shared<GenericEntry>(ID_DataFork);
		break;
	case ID_ResourceFork:
		entry = std::make_shared<GenericEntry>(ID_ResourceFork);
		break;
	case ID_RealName:
		entry = std::make_shared<RealName>();
		break;
	case ID_Comment:
		entry = std::make_shared<GenericEntry>(ID_Comment);
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
			entry = std::make_shared<GenericEntry>(id);
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
		entry = std::make_shared<GenericEntry>(id);
		break;
	}
	entry->file_offset = rd->ReadUnsigned(4);
	entry->image_size = rd->ReadUnsigned(4);
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

// GenericEntry

offset_t AppleSingleDouble::GenericEntry::ImageSize() const
{
	return image ? image->ImageSize() : 0;
}

void AppleSingleDouble::GenericEntry::ReadFile(const std::shared_ptr<Linker::Reader>& rd)
{
	if(auto format = std::dynamic_pointer_cast<Linker::Format>(image))
	{
		format->ReadFile(rd);
	}
	else if(id == ID_ResourceFork)
	{
		uint32_t version = rd->ReadUnsigned(4, EndianType::BigEndian);
		rd->Skip(-4);
		if(version >= 128)
		{
			auto mac_rsrc = std::make_shared<MacintoshResourceFileFormat>();
			mac_rsrc->ReadFile(rd);
			image = mac_rsrc;
		}
		else
		{
			auto gsos_rsrc = std::make_shared<GSOSResourceFileFormat>();
			gsos_rsrc->ReadFile(rd);
			image = gsos_rsrc;
		}
	}
	else
	{
		image = Linker::Buffer::ReadFromFile(rd, image_size);
	}
}

offset_t AppleSingleDouble::GenericEntry::WriteFile(const std::shared_ptr<Linker::Writer>& out) const
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

void AppleSingleDouble::GenericEntry::Dump(Dumper::Dumper& dump) const
{
	std::string region_name;
	int display_option;
	switch(id)
	{
	case ID_DataFork:
		region_name = "Data fork";
		display_option = Dumper::Image;
		break;
	case ID_ResourceFork:
		region_name = "Resource fork";
		display_option = Dumper::Image;
		break;
	case ID_Comment:
		region_name = "Comment";
		display_option = Dumper::Miscellaneous;
		break;
	default:
		region_name = "Unidentified block";
		display_option = Dumper::Miscellaneous;
		break;
	}

	if(auto format = std::dynamic_pointer_cast<Linker::Format>(image))
	{
		Dumper::Region region(region_name, file_offset, image->ImageSize(), 8);
		region.Display(dump, Dumper::Header);

		format->Dump(dump);
	}
	else
	{
		Dumper::Block block(region_name, file_offset, image->AsImage(), 0, 8);
		block.Display(dump, display_option);
	}
}

void AppleSingleDouble::GenericEntry::CalculateValues()
{
	if(auto format = std::dynamic_pointer_cast<Linker::OutputFormat>(image))
	{
		format->CalculateValues();
	}

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
		entry = std::make_shared<GenericEntry>(ID_DataFork);
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
		entry = std::make_shared<GenericEntry>(ID_ResourceFork);
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

void AppleSingleDouble::SetCreationDate(AppleSingleDouble::Timestamp CreationDate)
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

AppleSingleDouble::Timestamp AppleSingleDouble::GetCreationDate()
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
	return { };
}

std::optional<AppleSingleDouble::Timestamp> AppleSingleDouble::ReadCreationDate()
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
	return { };
}

void AppleSingleDouble::SetModificationDate(AppleSingleDouble::Timestamp ModificationDate)
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

AppleSingleDouble::Timestamp AppleSingleDouble::GetModificationDate()
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
	return { };
}

std::optional<AppleSingleDouble::Timestamp> AppleSingleDouble::ReadModificationDate()
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
	return { };
}

void AppleSingleDouble::SetBackupDate(AppleSingleDouble::Timestamp BackupDate)
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

void AppleSingleDouble::SetAccessDate(AppleSingleDouble::Timestamp AccessDate)
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

void AppleSingleDouble::SetProDOSAuxiliaryType(uint32_t AuxiliaryType)
{
	std::shared_ptr<Entry> entry = GetProDOSFileInfo();
	if(entry == nullptr)
		return;
	switch(version)
	{
	case 1:
		std::dynamic_pointer_cast<FileInfo::ProDOS>(entry)->AuxiliaryType = AuxiliaryType;
		break;
	case 2:
		std::dynamic_pointer_cast<ProDOSFileInfo>(entry)->AuxiliaryType = AuxiliaryType;
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

offset_t AppleSingleDouble::WriteFile(const std::shared_ptr<Linker::Writer>& wr) const
{
	wr->endiantype = ::BigEndian;
	wr->WriteWord(4, type);
	wr->WriteWord(4, version << 16);
	wr->WriteData(16, home_file_system_string);
	wr->WriteWord(2, entries.size());
	for(auto entry : entries)
	{
		wr->WriteWord(4, entry->id);
		wr->WriteWord(4, entry->file_offset);
		wr->WriteWord(4, entry->image_size);
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

// RealName

offset_t RealName::ImageSize() const
{
	return name.size();
}

void RealName::ReadFile(const std::shared_ptr<Linker::Reader>& rd)
{
	// TODO
}

offset_t RealName::WriteFile(const std::shared_ptr<Linker::Writer>& wr) const
{
	wr->endiantype = ::BigEndian;
	wr->WriteData(name.size(), name.c_str());

	return offset_t(-1);
}

void RealName::Dump(Dumper::Dumper& dump) const
{
	// TODO
}

// IconBW

offset_t IconBW::ImageSize() const
{
	return offset_t(-1); // TODO
}

void IconBW::ReadFile(const std::shared_ptr<Linker::Reader>& rd)
{
	// TODO
}

offset_t IconBW::WriteFile(const std::shared_ptr<Linker::Writer>& out) const
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

void IconColor::ReadFile(const std::shared_ptr<Linker::Reader>& rd)
{
	// TODO
}

offset_t IconColor::WriteFile(const std::shared_ptr<Linker::Writer>& out) const
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

void FileInfo::Macintosh::ReadFile(const std::shared_ptr<Linker::Reader>& rd)
{
	rd->endiantype = ::BigEndian;
	CreationDate = rd->ReadTimestamp<AppleSingleDouble::clock>();
	ModificationDate = rd->ReadTimestamp<AppleSingleDouble::clock>();
	LastBackupDate = rd->ReadTimestamp<AppleSingleDouble::clock>();
	Attributes = rd->ReadUnsigned(4);
}

offset_t FileInfo::Macintosh::WriteFile(const std::shared_ptr<Linker::Writer>& wr) const
{
	wr->endiantype = ::BigEndian;
	wr->WriteTimestamp(CreationDate);
	wr->WriteTimestamp(ModificationDate);
	wr->WriteTimestamp(LastBackupDate);
	wr->WriteWord(4, Attributes);

	return offset_t(-1);
}

void FileInfo::Macintosh::Dump(Dumper::Dumper& dump) const
{
	Dumper::Region region("File info", file_offset, ImageSize(), 8);
	region.AddField("Home File System", Dumper::StringDisplay::Make("'"), std::string(AppleSingleDouble::TXT_Macintosh, 16));
	FileDatesInfo::DumpFields(region, CreationDate, ModificationDate, LastBackupDate, {});
	MacintoshFileInfo::DumpFields(region, Attributes);
	region.Display(dump, Dumper::Header);
}

// FileInfo::ProDOS

offset_t FileInfo::ProDOS::ImageSize() const
{
	return 16;
}

void FileInfo::ProDOS::ReadFile(const std::shared_ptr<Linker::Reader>& rd)
{
	rd->endiantype = ::BigEndian;
	CreationDate = rd->ReadTimestamp<AppleSingleDouble::clock>();
	ModificationDate = rd->ReadTimestamp<AppleSingleDouble::clock>();
	Access = rd->ReadUnsigned(2);
	FileType = rd->ReadUnsigned(2);
	AuxiliaryType = rd->ReadUnsigned(4);
}

offset_t FileInfo::ProDOS::WriteFile(const std::shared_ptr<Linker::Writer>& wr) const
{
	wr->endiantype = ::BigEndian;
	wr->WriteTimestamp(CreationDate);
	wr->WriteTimestamp(ModificationDate);
	wr->WriteWord(2, Access);
	wr->WriteWord(2, FileType);
	wr->WriteWord(4, AuxiliaryType);

	return offset_t(-1);
}

void FileInfo::ProDOS::Dump(Dumper::Dumper& dump) const
{
	Dumper::Region region("File info", file_offset, ImageSize(), 8);
	region.AddField("Home File System", Dumper::StringDisplay::Make("'"), std::string(AppleSingleDouble::TXT_ProDOS, 16));
	FileDatesInfo::DumpFields(region, CreationDate, ModificationDate, {}, {});
	ProDOSFileInfo::DumpFields(region, Access, FileType, AuxiliaryType);
	region.Display(dump, Dumper::Header);
}

// FileInfo::MSDOS

offset_t FileInfo::MSDOS::ImageSize() const
{
	return 6;
}

void FileInfo::MSDOS::ReadFile(const std::shared_ptr<Linker::Reader>& rd)
{
	rd->endiantype = ::BigEndian;
	ModificationDate = rd->ReadTimestamp<AppleSingleDouble::clock>();
	Attributes = rd->ReadUnsigned(2);
}

offset_t FileInfo::MSDOS::WriteFile(const std::shared_ptr<Linker::Writer>& wr) const
{
	wr->endiantype = ::BigEndian;
	wr->WriteTimestamp(ModificationDate);
	wr->WriteWord(2, Attributes);

	return offset_t(-1);
}

void FileInfo::MSDOS::Dump(Dumper::Dumper& dump) const
{
	Dumper::Region region("File info", file_offset, ImageSize(), 8);
	region.AddField("Home File System", Dumper::StringDisplay::Make("'"), std::string(AppleSingleDouble::TXT_MS_DOS, 16));
	FileDatesInfo::DumpFields(region, {}, ModificationDate, {}, {});
	MSDOSFileInfo::DumpFields(region, Attributes);
	region.Display(dump, Dumper::Header);
}

// FileInfo::AUX

offset_t FileInfo::AUX::ImageSize() const
{
	return 12;
}

void FileInfo::AUX::ReadFile(const std::shared_ptr<Linker::Reader>& rd)
{
}

offset_t FileInfo::AUX::WriteFile(const std::shared_ptr<Linker::Writer>& wr) const
{
	wr->endiantype = ::BigEndian;
	wr->WriteTimestamp(CreationDate);
	wr->WriteTimestamp(AccessDate);
	wr->WriteTimestamp(ModificationDate);

	return offset_t(-1);
}

void FileInfo::AUX::Dump(Dumper::Dumper& dump) const
{
	Dumper::Region region("File info", file_offset, ImageSize(), 8);
	region.AddField("Home File System", Dumper::StringDisplay::Make("'"), std::string(AppleSingleDouble::TXT_UNIX, 16));
	FileDatesInfo::DumpFields(region, CreationDate, ModificationDate, {}, AccessDate);
	region.Display(dump, Dumper::Header);
}

// FileDatesInfo

offset_t FileDatesInfo::ImageSize() const
{
	return 16;
}

void FileDatesInfo::ReadFile(const std::shared_ptr<Linker::Reader>& rd)
{
	// TODO
}

offset_t FileDatesInfo::WriteFile(const std::shared_ptr<Linker::Writer>& wr) const
{
	wr->endiantype = ::BigEndian;
	wr->WriteTimestamp(CreationDate);
	wr->WriteTimestamp(ModificationDate);
	wr->WriteTimestamp(BackupDate);
	wr->WriteTimestamp(AccessDate);

	return offset_t(-1);
}

void FileDatesInfo::Dump(Dumper::Dumper& dump) const
{
	Dumper::Region region("File Dates Info", file_offset, ImageSize(), 8);
	DumpFields(region, CreationDate, ModificationDate, BackupDate, AccessDate);
	region.Display(dump, Dumper::Header);
}

void FileDatesInfo::DumpFields(Dumper::Region& region, std::optional<AppleSingleDouble::Timestamp> CreationDate, std::optional<AppleSingleDouble::Timestamp> ModificationDate, std::optional<AppleSingleDouble::Timestamp> BackupDate, std::optional<AppleSingleDouble::Timestamp> AccessDate)
{
	if(CreationDate)
	{
		region.AddField("Creation date", Dumper::TimestampDisplay<AppleSingleDouble::clock>::Make(), *CreationDate);
	}

	if(ModificationDate)
	{
		region.AddField("Modification date", Dumper::TimestampDisplay<AppleSingleDouble::clock>::Make(), *ModificationDate);
	}

	if(BackupDate)
	{
		region.AddField("Backup date", Dumper::TimestampDisplay<AppleSingleDouble::clock>::Make(), *BackupDate);
	}

	if(AccessDate)
	{
		region.AddField("Access date", Dumper::TimestampDisplay<AppleSingleDouble::clock>::Make(), *AccessDate);
	}
}

// FinderInfo

offset_t FinderInfo::ImageSize() const
{
	return 32;
}

void FinderInfo::ReadFile(const std::shared_ptr<Linker::Reader>& rd)
{
	rd->endiantype = ::BigEndian;
	rd->ReadData(4, Type);
	rd->ReadData(4, Creator);
	Flags = rd->ReadSigned(2);
	Location.x = rd->ReadSigned(2);
	Location.y = rd->ReadSigned(2);
	Folder = rd->ReadSigned(2);
	// extended file information
	IconID = rd->ReadSigned(2);
	rd->Skip(6);
	Script = rd->ReadSigned(1);
	rd->Skip(1);
	CommentID = rd->ReadSigned(2);
	HomeDirectoryID = rd->ReadSigned(4);
}

offset_t FinderInfo::WriteFile(const std::shared_ptr<Linker::Writer>& wr) const
{
	wr->endiantype = ::BigEndian;
	wr->WriteData(4, Type);
	wr->WriteData(4, Creator);
	wr->WriteWord(2, Flags);
	wr->WriteWord(2, Location.x);
	wr->WriteWord(2, Location.y);
	wr->WriteWord(2, Folder);
	// extended file information
	wr->WriteWord(2, IconID);
	wr->Skip(6);
	wr->WriteWord(1, Script);
	wr->Skip(1);
	wr->WriteWord(2, CommentID);
	wr->WriteWord(4, HomeDirectoryID);

	return offset_t(-1);
}

void FinderInfo::Dump(Dumper::Dumper& dump) const
{
	Dumper::Region region("Finder Info", file_offset, ImageSize(), 8);
	region.AddField("Type", Dumper::StringDisplay::Make("'"), std::string(Type, 4));
	region.AddField("Creator", Dumper::StringDisplay::Make("'"), std::string(Creator, 4));
	// TODO
	region.Display(dump, Dumper::Header);
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

void MacintoshFileInfo::ReadFile(const std::shared_ptr<Linker::Reader>& rd)
{
	rd->endiantype = ::BigEndian;
	Attributes = rd->ReadUnsigned(4);
}

offset_t MacintoshFileInfo::WriteFile(const std::shared_ptr<Linker::Writer>& wr) const
{
	wr->endiantype = ::BigEndian;
	wr->WriteWord(4, Attributes);

	return offset_t(-1);
}

void MacintoshFileInfo::Dump(Dumper::Dumper& dump) const
{
	Dumper::Region region("Macintosh file info", file_offset, ImageSize(), 8);
	DumpFields(region, Attributes);
	region.Display(dump, Dumper::Header);
}

void MacintoshFileInfo::DumpFields(Dumper::Region& region, uint32_t Attributes)
{
	region.AddField("Attributes",
		Dumper::BitFieldDisplay::Make(8)
			->AddBitField(0, 1, Dumper::ChoiceDisplay::Make("Locked"), false)
			->AddBitField(1, 1, Dumper::ChoiceDisplay::Make("Protected"), false),
		offset_t(Attributes));
}

// ProDOSFileInfo

offset_t ProDOSFileInfo::ImageSize() const
{
	return 8;
}

void ProDOSFileInfo::ReadFile(const std::shared_ptr<Linker::Reader>& rd)
{
	rd->endiantype = ::BigEndian;
	Access = rd->ReadUnsigned(2);
	FileType = rd->ReadUnsigned(2);
	AuxiliaryType = rd->ReadUnsigned(4);
}

offset_t ProDOSFileInfo::WriteFile(const std::shared_ptr<Linker::Writer>& wr) const
{
	wr->endiantype = ::BigEndian;
	wr->WriteWord(2, Access);
	wr->WriteWord(2, FileType);
	wr->WriteWord(4, AuxiliaryType);

	return offset_t(-1);
}

void ProDOSFileInfo::Dump(Dumper::Dumper& dump) const
{
	Dumper::Region region("ProDOS file info", file_offset, ImageSize(), 8);
	DumpFields(region, Access, FileType, AuxiliaryType);
	region.Display(dump, Dumper::Header);
}

void ProDOSFileInfo::DumpFields(Dumper::Region& region, uint16_t Access, uint16_t FileType, uint32_t AuxiliaryType)
{
	region.AddField("Access", Dumper::HexDisplay::Make(4), offset_t(Access)); // TODO: should be a bitmap

	static const std::map<offset_t, std::string> file_types =
	{
		// based on Jon Relay's Apple II Info Archives
		{ 0x00, "UNK (Unknown)" },
		{ 0x01, "BAD (Bad Block)" },
		{ 0x02, "PCD (Pascal Code)" },
		{ 0x03, "PTX (Pascal Text)" },
		{ 0x04, "TXT (ASCII Text) [Apple DOS: 0x00 'T']" },
		{ 0x05, "PDA (Pascal Data)" },
		{ 0x06, "BIN (Binary File) [Apple DOS: 0x04 'B']" },
		{ 0x07, "FNT (Apple /// Font)" },
		{ 0x08, "FOT (HiRes/Double HiRes Graphics)" },
		{ 0x09, "BA3 (Apple /// BASIC Program)" },
		{ 0x0A, "DA3 (Apple /// BASIC Data)" },
		{ 0x0B, "WPF (Generic Word Processing)" },
		{ 0x0C, "SOS (SOS System File)" },
		{ 0x0F, "DIR (ProDOS Directory)" },
		{ 0x10, "RPD (RPS Data)" },
		{ 0x11, "RPI (RPS Index)" },
		{ 0x12, "AFP (AppleFile Discard)" },
		{ 0x13, "AFM (AppleFile Model)" },
		{ 0x14, "AFR (AppleFile Report)" },
		{ 0x15, "SCL (Screen Library)" },
		{ 0x16, "PFS (PFS Document)" },

		{ 0x19, "ADB (AppleWorks Database)" },
		{ 0x1A, "AWP (AppleWorks Word Processing)" },
		{ 0x1B, "ASP (AppleWorks Spreadsheet)" },

		{ 0x20, "TDM (Desktop Manager File)" },
		{ 0x21, "IPS (Instant Pascal Source)" },
		{ 0x22, "UPV (UCSD Pascal Volume)" },

		{ 0x29, "3SD (SOS Directory)" },
		{ 0x2A, "8SC (Source Code)" },
		{ 0x2B, "8OB (Object Code)" },
		{ 0x2C, "8IC (Interpreted Code)" },
		{ 0x2D, "8LD (Language Data)" },
		{ 0x2E, "P8C (ProDOS 8 Code Module)" },

		{ 0x41, "OCR (Optical Character Recognition)" },
		{ 0x42, "FTD (File Type Definition)" },

		{ 0x50, "GWP (Apple IIgs Word Processing)" },
		{ 0x51, "GSS (Apple IIgs Spreadsheet)" },
		{ 0x52, "GDB (Apple IIgs Database)" },
		{ 0x53, "DRW (Object Oriented Graphics)" },
		{ 0x54, "GDP (Apple IIgs Desktop Publishing)" },
		{ 0x55, "HMD (HyperMedia)" },
		{ 0x56, "EDU (Educational Program Data)" },
		{ 0x57, "STN (Stationary)" },
		{ 0x58, "HLP (Help File)" },
		{ 0x59, "COM (Communcations)" },
		{ 0x5A, "CFG (Configuration)" },
		{ 0x5B, "ANM (Animation)" },
		{ 0x5C, "MUM (Multimedia)" },
		{ 0x5D, "ENT (Entertainment)" },
		{ 0x5E, "DVU (Development Utility)" },

		{ 0x60, "PRE (PC Pre-Boot)" }, // repeated

		{ 0x66, "NCF (ProDOS File Navigator Command File)" },

		{ 0x6B, "BIO (PC BIOS)" },

		{ 0x6D, "DVR (PC Driver)" },
		{ 0x6E, "PRE (PC Pre-Boot)" }, // repeated
		{ 0x6F, "HDV (PC Hard Disk Image)" },

		{ 0x80, "GES (GEOS System File)" },
		{ 0x81, "GEA (GEOS Desk Accessory)" },
		{ 0x82, "GEO (GEOS Application)" },
		{ 0x83, "GED (GEOS Document)" },
		{ 0x84, "GEF (GEOS Font)" },
		{ 0x85, "GEP (GEOS Printer Driver)" },
		{ 0x86, "GEI (GEOS Input Driver)" },
		{ 0x87, "GEX (GEOS Auxiliary Driver)" },

		{ 0x89, "GEV (GEOS Swap File)" },

		{ 0x8B, "GEC (GEOS Clock Driver)" },
		{ 0x8C, "GEK (GEOS Interface Card Driver)" },
		{ 0x8D, "GEW (GEOS Formatting Data)" },

		{ 0xA0, "WP  (WordPerfect)" },

		{ 0xAB, "GSB (Apple IIgs BASIC Program)" },
		{ 0xAC, "TDF (Apple IIgs BASIC TDF)" },
		{ 0xAD, "BDF (Apple IIgs BASIC Data)" },

		{ 0xB0, "SRC (Apple IIgs Source Code)" },
		{ 0xB1, "OBJ (Apple IIgs Object Code)" },
		{ 0xB2, "LIB (Apple IIgs Library)" },
		{ 0xB3, "S16 (Apple IIgs Application Program)" },
		{ 0xB4, "RTL (Apple IIgs Runtime Library)" },
		{ 0xB5, "EXE (Apple IIgs Shell Script)" },
		{ 0xB6, "PIF (Apple IIgs Permanent INIT)" },
		{ 0xB7, "TIF (Apple IIgs Temporary INIT)" },
		{ 0xB8, "NDA (Apple IIgs New Desk Accessory)" },
		{ 0xB9, "CDA (Apple IIgs Classic Desk Accessory)" },
		{ 0xBA, "TOL (Apple IIgs Tool)" },
		{ 0xBB, "DRV (Apple IIgs Device Driver)" },
		{ 0xBC, "LDF (Apple IIgs Generic Load File)" },
		{ 0xBD, "FST (Apple IIgs File System Translator)" },

		{ 0xBF, "DOC (Apple IIgs Document)" },

		{ 0xC0, "PNT (Apple IIgs Packed Super HiRes)" },
		{ 0xC1, "PIC (Apple IIgs Super HiRes)" },
		{ 0xC2, "ANI (PaintWorks Animation)" },
		{ 0xC3, "PAL (PaintWorks Palette)" },

		{ 0xC5, "OOG (Object-Oriented Graphics)" },
		{ 0xC6, "SCR (Script)" },
		{ 0xC7, "CDV (Apple IIgs Control Panel)" },
		{ 0xC8, "FON (Apple IIgs Font)" },
		{ 0xC9, "FND (Apple IIgs Finder Data)" },
		{ 0xCA, "ICN (Apple IIgs Icon File)" },

		{ 0xD5, "MUS (Music)" },
		{ 0xD6, "INS (Instrument)" },
		{ 0xD7, "MID (MIDI)" },
		{ 0xD8, "SND (Apple IIgs Audio)" },

		{ 0xDB, "DBM (DB Master Document)" },

		{ 0xE0, "LBR (Archive)" },

		{ 0xE2, "ATK (AppleTalk Data)" },

		{ 0xEE, "R16 (EDASM 816 Relocatable Code)" },
		{ 0xEF, "PAR (Pascal Area)" },
		{ 0xF0, "CMD (ProDOS Command File)" },
		{ 0xF1, "OVL (User Defined 1)" },
		{ 0xF2, "UD2 (User Defined 2) [Apple DOS: 0x08 'S']" },
		{ 0xF3, "UD3 (User Defined 3)[Apple DOS: 0x20 'AA']" },
		{ 0xF4, "UD4 (User Defined 4)[Apple DOS: 0x40 'BB']" },
		{ 0xF5, "UD5 (User Defined 5)" },
		{ 0xF6, "UD6 (User Defined 6)" },
		{ 0xF7, "UD7 (User Defined 7)" },
		{ 0xF8, "UD8 (User Defined 8)" },
		{ 0xF9, "P16 (ProDOS-16 System File)" },
		{ 0xFA, "INT (Integer BASIC Program) [Apple DOS: 0x01 'I']" },
		{ 0xFB, "IVR (Integer BASIC Variables)" },
		{ 0xFC, "BAS (Applesoft BASIC Program) [Apple DOS: 0x02 'A']" },
		{ 0xFD, "VAR (Applesoft BASIC Variables)" },
		{ 0xFE, "REL (EDASM Relocatable Code) [Apple DOS: 0x10 'R']" },
		{ 0xFF, "SYS (ProDOS-8 System File)" },
	};

	static const std::map<offset_t, std::string> TXT_file_types =
	{
		{ 0x0000, "Sequential" },
	};

	static const std::map<offset_t, std::string> _8IC_file_types =
	{
		{ 0x8003, "Apex Program File" },
	};

	static const std::map<offset_t, std::string> GWP_file_types =
	{
		{ 0x5445, "Teach" },
		{ 0x8001, "DeluxeWrite" },
		{ 0x8010, "AppleWorks GS" },
	};

	static const std::map<offset_t, std::string> GSS_file_types =
	{
		{ 0x8010, "AppleWorks GS" },
	};

	static const std::map<offset_t, std::string> GDB_file_types =
	{
		{ 0x8010, "AppleWorks GS" },
		{ 0x8011, "AppleWorks GS Template" },
		{ 0x8013, "GSAS" },
	};

	static const std::map<offset_t, std::string> DRW_file_types =
	{
		{ 0x8010, "AppleWorks GS" },
	};

	static const std::map<offset_t, std::string> GDP_file_types =
	{
		{ 0x8002, "GraphicWriter" },
		{ 0x8010, "AppleWorks GS" },
	};

	static const std::map<offset_t, std::string> HMD_file_types =
	{
		{ 0x0001, "HyperCard GS" },
		{ 0x8001, "Tutor-Tech" },
		{ 0x8002, "HyperStudio" },
		{ 0x8003, "Nexus" },
	};

	static const std::map<offset_t, std::string> COM_file_types =
	{
		{ 0x8010, "AppleWorks GS" },
	};

	static const std::map<offset_t, std::string> LDF_file_types =
	{
		{ 0x4001, "Nifty List Module" },
		{ 0x4002, "Super Info Module" },
		{ 0x4004, "Twilight Module" },
		{ 0x4083, "Marinetti Link Layer Module" },
	};

	static const std::map<offset_t, std::string> PNT_file_types =
	{
		{ 0x0001, "Packed Super HiRes" },
		{ 0x0002, "Apple Preferred Format" },
		{ 0x0003, "Packed QuickDraw II PICT" },
	};

	static const std::map<offset_t, std::string> PIC_file_types =
	{
		{ 0x0001, "QuickDraw PICT" },
		{ 0x0002, "Super HiRes 3200" },
	};

	static const std::map<offset_t, std::string> FON_file_types =
	{
		{ 0x0000, "QuickDraw Bitmap Font" },
		{ 0x0001, "Pointless TrueType Font" },
	};

	static const std::map<offset_t, std::string> SND_file_types =
	{
		{ 0x0000, "AIFF" },
		{ 0x0001, "AIFF-C" },
		{ 0x0002, "ASIF Instrument" },
		{ 0x0003, "Sound Resource" },
		{ 0x0004, "MIDI Synth Wave" },
		{ 0x8001, "HyperStudio Sound" },
	};

	static const std::map<offset_t, std::string> LBR_file_types =
	{
		{ 0x0000, "ALU" },
		{ 0x0001, "AppleSingle" },
		{ 0x0002, "AppleDouble Header" },
		{ 0x0003, "AppleDouble Data" },
		{ 0x8000, "Binary II" },
		{ 0x8001, "AppleLink ACU" },
		{ 0x8002, "ShrinkIt" },
	};

	static const std::map<offset_t, std::string> ATK_file_types =
	{
		{ 0xFFFF, "EasyMount Alias" },
	};

	std::string auxiliary_type_name = "Auxiliary type";
	const std::map<offset_t, std::string> * auxiliary_file_types = nullptr;
	switch(FileType)
	{
	case 0x04: // TXT
		auxiliary_type_name = "Record length";
		auxiliary_file_types = &TXT_file_types;
		break;
	case 0x06: // BIN
	case 0xFC: // BAS
		auxiliary_type_name = "Load address";
		break;
	case 0x2C: // 8IC
		auxiliary_file_types = &_8IC_file_types;
		break;
	case 0x50: // GWP
		auxiliary_file_types = &GWP_file_types;
		break;
	case 0x51: // GSS
		auxiliary_file_types = &GSS_file_types;
		break;
	case 0x52: // GDB
		auxiliary_file_types = &GDB_file_types;
		break;
	case 0x53: // DRW
		auxiliary_file_types = &DRW_file_types;
		break;
	case 0x54: // GDP
		auxiliary_file_types = &GDP_file_types;
		break;
	case 0x55: // HMD
		auxiliary_file_types = &HMD_file_types;
		break;
	case 0x59: // COM
		auxiliary_file_types = &COM_file_types;
		break;
	case 0xBC: // LDF
		auxiliary_file_types = &LDF_file_types;
		break;
	case 0xC0: // PNT
		auxiliary_file_types = &PNT_file_types;
		break;
	case 0xC1: // PIC
		auxiliary_file_types = &PIC_file_types;
		break;
	case 0xC8: // FON
		auxiliary_file_types = &FON_file_types;
		break;
	case 0xD8: // SND
		auxiliary_file_types = &SND_file_types;
		break;
	case 0xE0: // LBR
		auxiliary_file_types = &LBR_file_types;
		break;
	case 0xE2: // ATK
		auxiliary_file_types = &ATK_file_types;
		break;
	}

	region.AddField("File type", Dumper::ChoiceDisplay::Make(file_types, Dumper::HexDisplay::Make(2)), offset_t(FileType));
	if(auxiliary_file_types)
	{
		region.AddField(auxiliary_type_name, Dumper::ChoiceDisplay::Make(file_types, Dumper::HexDisplay::Make(4)), offset_t(AuxiliaryType));
	}
	else
	{
		region.AddField(auxiliary_type_name, Dumper::HexDisplay::Make(4), offset_t(AuxiliaryType));
	}
}

// MSDOSFileInfo

offset_t MSDOSFileInfo::ImageSize() const
{
	return 2;
}

void MSDOSFileInfo::ReadFile(const std::shared_ptr<Linker::Reader>& rd)
{
	rd->endiantype = ::BigEndian;
	Attributes = rd->ReadUnsigned(2);
}

offset_t MSDOSFileInfo::WriteFile(const std::shared_ptr<Linker::Writer>& wr) const
{
	wr->endiantype = ::BigEndian;
	wr->WriteWord(2, Attributes);

	return offset_t(-1);
}

void MSDOSFileInfo::Dump(Dumper::Dumper& dump) const
{
	Dumper::Region region("MS-DOS file info", file_offset, ImageSize(), 8);
	DumpFields(region, Attributes);
	region.Display(dump, Dumper::Header);
}

void MSDOSFileInfo::DumpFields(Dumper::Region& region, uint16_t Attributes)
{
	region.AddField("Attributes", Dumper::HexDisplay::Make(8), offset_t(Attributes)); // TODO: bit field
}

// AFPShortName

offset_t AFPShortName::ImageSize() const
{
	return offset_t(-1); // TODO
}

void AFPShortName::ReadFile(const std::shared_ptr<Linker::Reader>& rd)
{
	// TODO
}

offset_t AFPShortName::WriteFile(const std::shared_ptr<Linker::Writer>& out) const
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

void AFPFileInfo::ReadFile(const std::shared_ptr<Linker::Reader>& rd)
{
	// TODO
}

offset_t AFPFileInfo::WriteFile(const std::shared_ptr<Linker::Writer>& out) const
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

void AFPDirectoryID::ReadFile(const std::shared_ptr<Linker::Reader>& rd)
{
	// TODO
}

offset_t AFPDirectoryID::WriteFile(const std::shared_ptr<Linker::Writer>& out) const
{
	// TODO
	return ImageSize();
}

void AFPDirectoryID::Dump(Dumper::Dumper& dump) const
{
	// TODO
}

// MacBinary

uint16_t MacBinary::crc_step[256];

void MacBinary::CRC_Initialize() const
{
	crc = 0; // 0x1021;
	for(int byte = 0; byte < 256; byte++)
	{
		uint16_t value = byte << 8;
		for(int shift = 0; shift < 8; shift++)
		{
			if((value & 0x8000))
			{
				value = (value << 1) ^ 0x1021;
			}
			else
			{
				value <<= 1;
			}
		}
		crc_step[byte] = value;
	}
}

void MacBinary::CRC_Step(uint8_t byte) const
{
	crc = (crc << 8) ^ crc_step[(crc >> 8) ^ byte];
}

void MacBinary::Skip(const std::shared_ptr<Linker::Writer>& wr, size_t count) const
{
	for(size_t i = 0; i < count; i++)
	{
		CRC_Step(0);
	}
	wr->Skip(count);
}

void MacBinary::WriteData(const std::shared_ptr<Linker::Writer>& wr, size_t count, const void * data) const
{
	for(size_t i = 0; i < count; i++)
	{
		CRC_Step(static_cast<const char *>(data)[i]);
	}
	wr->WriteData(count, data);
}

void MacBinary::WriteData(const std::shared_ptr<Linker::Writer>& wr, size_t count, std::string text) const
{
	for(size_t i = 0; i < count; i++)
	{
		CRC_Step(i < text.size() ? text[i] : 0);
	}
	wr->WriteData(count, text);
}

void MacBinary::WriteWord(const std::shared_ptr<Linker::Writer>& wr, size_t bytes, uint64_t value) const
{
	std::vector<uint8_t> data(bytes);
	::WriteWord(bytes, bytes, data.data(), value, EndianType::BigEndian);
	WriteData(wr, bytes, data.data());
}

void MacBinary::ReadHeader(const std::shared_ptr<Linker::Reader>& rd)
{
	version = MACBIN1;

	rd->endiantype = ::BigEndian;
	rd->Skip(1);
	if(apple_single == nullptr)
	{
		apple_single = std::make_shared<AppleSingleDouble>();
	}
	uint8_t name_size = rd->ReadUnsigned(1);
	if(name_size > 63)
	{
		Linker::Warning << "Warning: Invalid name size found, truncating: " << name_size << std::endl;
		name_size = 63;
	}
	auto real_name = std::dynamic_pointer_cast<RealName>(apple_single->GetRealName());
	real_name->name = rd->ReadData(name_size);
	rd->Skip(63 - name_size);
	auto finder_info = std::dynamic_pointer_cast<FinderInfo>(apple_single->GetFinderInfo());
	rd->ReadData(4, finder_info->Type);
	rd->ReadData(4, finder_info->Creator);
	finder_info->Flags = rd->ReadUnsigned(1) << 8;
	rd->Skip(1);
	finder_info->Location.x = rd->ReadUnsigned(2);
	finder_info->Location.y = rd->ReadUnsigned(2);
	rd->Skip(2); // TODO: window/folder info?
	attributes = rd->ReadUnsigned(1);
	rd->Skip(1);
	data_fork_length = rd->ReadUnsigned(4);
	resource_fork_length = rd->ReadUnsigned(4);
	creation = rd->ReadTimestamp<Macintosh_clock>(); // TODO: maybe these 2 could be stored in a file field?
	modification = rd->ReadTimestamp<Macintosh_clock>();
	// Get Info extension
	comment_length = rd->ReadUnsigned(2);
	if(comment_length != 0 && version < MACBIN1_GETINFO)
	{
		version = MACBIN1_GETINFO;
	}
	// MacBinary II
	uint8_t flags_low_byte = rd->ReadUnsigned(1);
	finder_info->Flags |= flags_low_byte;
	if(flags_low_byte != 0 && version < MACBIN2)
	{
		version = MACBIN2;
	}
	// MacBinary III
	auto signature = rd->ReadData(4);
	if(signature == "mBIN")
	{
		// TODO: script of file and extended Finder flags
	}
	rd->Skip(14); // TODO:
	secondary_header_size = rd->ReadUnsigned(2);
	uint8_t actual_version = rd->ReadUnsigned(1);
	if(actual_version != 0)
	{
		version = version_t(actual_version);
	}
	minimum_version = version_t(rd->ReadUnsigned(1));
	crc = rd->ReadUnsigned(2);
}

void MacBinary::WriteHeader(const std::shared_ptr<Linker::Writer>& wr) const
{
	CRC_Initialize();
	WriteWord(wr, 1, 0);
	if(auto entry = apple_single->FindEntry(AppleSingleDouble::ID_RealName))
	{
		const std::string& name = std::dynamic_pointer_cast<const RealName>(entry)->name;
		WriteWord(wr, 1, name.size() > 63 ? 63 : name.size());
		WriteData(wr, 63, name);
	}
	else
	{
		WriteWord(wr, 1, generated_file_name.size() > 63 ? 63 : generated_file_name.size());
		WriteData(wr, 63, generated_file_name);
	}
	std::shared_ptr<const FinderInfo> finder_info = nullptr;
	if(auto entry = apple_single->FindEntry(AppleSingleDouble::ID_FinderInfo))
	{
		finder_info = std::dynamic_pointer_cast<const FinderInfo>(entry);
		WriteData(wr, 4, finder_info->Type);
		WriteData(wr, 4, finder_info->Creator);
		WriteWord(wr, 1, finder_info->Flags >> 8);
		WriteWord(wr, 1, 0);
		WriteWord(wr, 2, finder_info->Location.y);
		WriteWord(wr, 2, finder_info->Location.x);
		WriteWord(wr, 2, 0); /* window/folder info */
	}
	else
	{
		WriteData(wr, 16, "");
	}
	WriteWord(wr, 1, attributes);
	WriteWord(wr, 1, 0);
	if(auto entry = apple_single->FindEntry(AppleSingleDouble::ID_DataFork))
	{
		WriteWord(wr, 4, entry->ImageSize());
	}
	else
	{
		WriteWord(wr, 4, 0);
	}
	if(auto entry = apple_single->FindEntry(AppleSingleDouble::ID_ResourceFork))
	{
		WriteWord(wr, 4, entry->ImageSize());
	}
	else
	{
		WriteWord(wr, 4, 0);
	}
	WriteTimestamp(wr, creation);
	WriteTimestamp(wr, modification);
	if(version < MACBIN1_GETINFO)
	{
		return;
	}
	if(auto entry = apple_single->FindEntry(AppleSingleDouble::ID_Comment))
	{
		WriteWord(wr, 2, entry->ImageSize());
	}
	else
	{
		WriteWord(wr, 2, 0);
	}
	if(version < MACBIN2)
	{
		return;
	}
	if(finder_info != nullptr)
	{
		WriteWord(wr, 1, finder_info->Flags & 0xFF);
	}
	else
	{
		WriteWord(wr, 1, 0);
	}
	if(version >= MACBIN3)
	{
		WriteData(wr, 4, "mBIN");
		WriteWord(wr, 1, 0); /* script of file */
		WriteWord(wr, 1, 0); /* extended Finder flags */
		Skip(wr, 8);
	}
	else
	{
		Skip(wr, 14);
	}
	WriteWord(wr, 4, 0); /* unpacked file size */
	WriteWord(wr, 2, secondary_header_size);
	WriteWord(wr, 1, version);
	WriteWord(wr, 1, minimum_version);
	wr->WriteWord(2, crc);
}

void MacBinary::CalculateValues()
{
	attributes = apple_single->ReadMacintoshAttributes();

	auto date_option = apple_single->ReadCreationDate();
	if(date_option)
	{
		creation = Macintosh_clock::convert(*date_option);
	}
	else
	{
		creation = {};
	}

	date_option = apple_single->ReadModificationDate();
	if(date_option)
	{
		modification = Macintosh_clock::convert(*date_option);
	}
	else
	{
		modification = {};
	}

	apple_single->CalculateValues();
}

void MacBinary::ReadFile(const std::shared_ptr<Linker::Reader>& rd)
{
	ReadHeader(rd);
	rd->Seek(::AlignTo(0x80 + secondary_header_size, 0x80));
	/* secondary header */
	if(data_fork_length != 0)
	{
		auto data_fork = dynamic_pointer_cast<AppleSingleDouble::GenericEntry>(apple_single->GetDataFork());
		// TODO: check format
		auto image = Linker::Buffer::ReadFromFile(rd, data_fork_length);
		data_fork->image = image;
		rd->Seek(::AlignTo(rd->Tell(), 0x80));
	}
	if(resource_fork_length != 0)
	{
		auto resource_fork = dynamic_pointer_cast<AppleSingleDouble::GenericEntry>(apple_single->GetResourceFork());
		// TODO: check format
		auto mac_rsrc = std::make_shared<MacintoshResourceFileFormat>();
		mac_rsrc->ReadFile(rd);
		resource_fork->image = mac_rsrc;
		rd->Seek(::AlignTo(rd->Tell(), 0x80));
	}
	if(comment_length != 0)
	{
		//auto comment = dynamic_pointer_cast<Comment>(apple_single->GetComment());
		// TODO
	}
}

offset_t MacBinary::WriteFile(const std::shared_ptr<Linker::Writer>& wr) const
{
	WriteHeader(wr);
	wr->Seek(::AlignTo(0x80 + secondary_header_size, 0x80));
	/* secondary header */
	if(auto entry = apple_single->FindEntry(AppleSingleDouble::ID_DataFork))
	{
		entry->WriteFile(wr);
		wr->AlignTo(0x80);
	}
	if(auto entry = apple_single->FindEntry(AppleSingleDouble::ID_ResourceFork))
	{
		entry->WriteFile(wr);
		wr->AlignTo(0x80);
	}
	if(version >= MACBIN1_GETINFO)
	{
		if(auto entry = apple_single->FindEntry(AppleSingleDouble::ID_Comment))
		{
			entry->WriteFile(wr);
			wr->AlignTo(0x80);
		}
	}

	return offset_t(-1);
}

void MacBinary::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_macroman);

	dump.SetTitle("MacBinary format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 8);
	file_region.Display(dump, Dumper::Header);

	Dumper::Region header_region("Header", file_offset, 0x80, 8);
	std::string real_name = "";
	if(auto entry = apple_single->FindEntry(AppleSingleDouble::ID_RealName))
	{
		real_name = std::dynamic_pointer_cast<const RealName>(entry)->name;
	}
	header_region.AddField("Real name", Dumper::StringDisplay::Make("'"), real_name);
	auto finder_info = std::dynamic_pointer_cast<FinderInfo>(apple_single->GetFinderInfo());
	if(finder_info)
	{
		header_region.AddField("OS Type", Dumper::StringDisplay::Make(4, "'"), std::string(finder_info->Type));
		header_region.AddField("Creator", Dumper::StringDisplay::Make(4, "'"), std::string(finder_info->Creator));
		header_region.AddField("Flags", Dumper::HexDisplay::Make(8), offset_t(finder_info->Flags)); // TODO: should be a bit field
		header_region.AddField("Location.x", Dumper::DecDisplay::Make(), offset_t(finder_info->Location.x));
		header_region.AddField("Location.x", Dumper::DecDisplay::Make(), offset_t(finder_info->Location.y));
	}
	header_region.AddField("Attributes", Dumper::HexDisplay::Make(4), offset_t(attributes)); // TODO: should be a bit field
	header_region.AddField("Creation", Dumper::TimestampDisplay<Macintosh_clock>::Make(), creation);
	header_region.AddField("Modification", Dumper::TimestampDisplay<Macintosh_clock>::Make(), modification);
	// TODO: MacBinary III "mBIN" field present, script of file and extended Finder flags
	static const std::map<offset_t, std::string> version_values =
	{
		{ MacBinary::MACBIN1, "Revision 1 (1985)" },
		{ MacBinary::MACBIN1_GETINFO, "Revision 1 (1985) with Get Info extension" },
		{ MacBinary::MACBIN2, "MacBinary II, Revision 2 (1987)" },
		{ MacBinary::MACBIN3, "MacBinary III, Revision 3 (1996)" },
	};
	header_region.AddField("Version (value)", Dumper::DecDisplay::Make(), offset_t(version < MACBIN2 ? 0 : version));
	header_region.AddField("Version (name)", Dumper::ChoiceDisplay::Make(version_values), offset_t(version < MACBIN2 ? 0 : version));
	header_region.AddField("Minimum version (value)", Dumper::DecDisplay::Make(), offset_t(minimum_version));
	header_region.AddField("Minimum version (name)", Dumper::ChoiceDisplay::Make(version_values), offset_t(minimum_version));
	header_region.AddField("CRC", Dumper::HexDisplay::Make(4), offset_t(crc));
	header_region.Display(dump, Dumper::Header);

	if(secondary_header_size != 0)
	{
		Dumper::Region secondary_header_region("Secondary header", file_offset + 0x80, secondary_header_size, 8);
		secondary_header_region.Display(dump, Dumper::Header);
	}

	if(auto entry = apple_single->FindEntry(AppleSingleDouble::ID_DataFork))
	{
		entry->Dump(dump);
	}

	if(auto entry = apple_single->FindEntry(AppleSingleDouble::ID_ResourceFork))
	{
		entry->Dump(dump);
	}
}

// OutputDriver

bool OutputDriver::AddSupplementaryOutputFormat(std::string subformat)
{
	if(subformat == "rsrc")
	{
		if(SupportedSupplementaryFormat(PRODUCE_RESOURCE_FORK))
		{
			Linker::Debug << "Debug: Requested to generate resource fork under .rsrc" << std::endl;
			produce = produce_format_t(produce | PRODUCE_RESOURCE_FORK);
			return true;
		}
	}

	if(subformat == "finf")
	{
		if(SupportedSupplementaryFormat(PRODUCE_FINDER_INFO))
		{
			Linker::Debug << "Debug: Requested to generate Finder Info file under .finf" << std::endl;
			produce = produce_format_t(produce | PRODUCE_FINDER_INFO);
			return true;
		}
	}

	if(subformat == "double" || subformat == "appledouble")
	{
		if(SupportedSupplementaryFormat(PRODUCE_APPLE_DOUBLE))
		{
			Linker::Debug << "Debug: Requested to generate AppleDouble" << std::endl;
			produce = produce_format_t(produce | PRODUCE_APPLE_DOUBLE);
			return true;
		}
	}

	if(subformat == "mbin" || subformat == "macbin" || subformat == "macbinary")
	{
		if(SupportedSupplementaryFormat(PRODUCE_MAC_BINARY))
		{
			Linker::Debug << "Debug: Requested to generate MacBinary" << std::endl;
			produce = produce_format_t(produce | PRODUCE_MAC_BINARY);
			return true;
		}
	}

	if(subformat == "naps")
	{
		if(SupportedSupplementaryFormat(PRODUCE_NAPS_SUFFIX))
		{
			Linker::Debug << "Debug: Requested to add NuLib2 attribute preservation string suffix" << std::endl;
			produce = produce_format_t(produce | PRODUCE_NAPS_SUFFIX);
			return true;
		}
	}

	return false;
}

void OutputDriver::OnContainerCreated() { }

void OutputDriver::OnCalculateValues()
{
	// TODO: error
}

void OutputDriver::OnReadFile(const std::shared_ptr<Linker::Reader>& rd)
{
	// TODO: error
}

offset_t OutputDriver::OnWriteFile(const std::shared_ptr<Linker::Writer>& wr) const
{
	// TODO: error
	return offset_t(-1);
}

void OutputDriver::OnDump(Dumper::Dumper& dump) const
{
	// TODO: error
}

void OutputDriver::GenerateFiles(std::string filename, std::shared_ptr<Contents> data_fork, std::shared_ptr<Contents> resource_fork, uint8_t file_type, uint16_t auxiliary_file_type)
{
	container = CONTAINER_NONE; // initial setting

	if((data_fork != nullptr && (target != TARGET_DATA_FORK || produce != 0))
	|| (resource_fork != nullptr && (target != TARGET_RESOURCE_FORK || (produce & ~PRODUCE_RESOURCE_FORK) != 0)))
	{
		// if anything other than a single data fork or single resource fork is required, create an AppleSingleDouble container
		container = CONTAINER_APPLE_SINGLE;
		apple_single = std::make_shared<Apple::AppleSingleDouble>(target == TARGET_APPLE_SINGLE ? Apple::AppleSingleDouble::SINGLE : Apple::AppleSingleDouble::DOUBLE,
			apple_single_double_version, home_file_system);
		if(data_fork != nullptr)
		{
			apple_single->AppendEntry(std::make_shared<Apple::AppleSingleDouble::GenericEntry>(Apple::AppleSingleDouble::ID_DataFork, data_fork));
		}
		if(resource_fork != nullptr)
		{
			apple_single->AppendEntry(std::make_shared<Apple::AppleSingleDouble::GenericEntry>(Apple::AppleSingleDouble::ID_ResourceFork, resource_fork));
		}
		OnContainerCreated();
	}

	if(target == TARGET_MAC_BINARY || (produce & PRODUCE_MAC_BINARY) != 0)
	{
		// the presence of a MacBinary container implies presence of an AppleSingleDouble container
		container = CONTAINER_MAC_BINARY;
		mac_binary = std::make_shared<MacBinary>(apple_single, macbinary_version, macbinary_minimum_version);
		mac_binary->generated_file_name = filename;
	}

	if(target == TARGET_APPLE_SINGLE)
	{
		apple_single->GetDataFork();
	}

	std::string naps_suffix = "";
	if((produce & PRODUCE_NAPS_SUFFIX) != 0)
	{
		// if suffix generation was explicitly specified, the user requested that it be attached to the filename
		// for CiderPress
		std::ostringstream oss;
		oss << "#" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << int(file_type) << std::setw(4) << int(auxiliary_file_type);
		naps_suffix = oss.str();
	}

	switch(container)
	{
	case CONTAINER_NONE:
		OnCalculateValues();
		break;
	case CONTAINER_APPLE_SINGLE:
		apple_single->CalculateValues();
		break;
	case CONTAINER_MAC_BINARY:
		mac_binary->CalculateValues();
		break;
	}

	std::ofstream out;
	switch(target)
	{
	case TARGET_NONE:
		break;
	case TARGET_DATA_FORK:
		out.open(filename + naps_suffix, std::ios_base::out | std::ios_base::binary);
		if(data_fork != nullptr)
		{
			auto wr = std::make_shared<Linker::StreamWriter>(::BigEndian, out);
			data_fork->WriteFile(wr);
		}
		out.close();
		break;
	case TARGET_RESOURCE_FORK:
		out.open(filename + naps_suffix, std::ios_base::out | std::ios_base::binary);
		if(resource_fork != nullptr)
		{
			auto wr = std::make_shared<Linker::StreamWriter>(::BigEndian, out);
			resource_fork->WriteFile(wr);
		}
		out.close();
		break;
	case TARGET_APPLE_SINGLE:
	case TARGET_APPLE_DOUBLE:
		out.open(filename + naps_suffix, std::ios_base::out | std::ios_base::binary);
		{
			auto wr = std::make_shared<Linker::StreamWriter>(::BigEndian, out);
			apple_single->WriteFile(wr);
		}
		out.close();
		break;
	case TARGET_MAC_BINARY:
		out.open(filename + naps_suffix, std::ios_base::out | std::ios_base::binary);
		{
			auto wr = std::make_shared<Linker::StreamWriter>(::BigEndian, out);
			mac_binary->WriteFile(wr);
		}
		out.close();
		break;
	}

	if((produce & PRODUCE_RESOURCE_FORK))
	{
		Linker::Debug << "Debug: Generating resource fork under .rsrc" << std::endl;
		std::error_code err;
		std::filesystem::path path = std::filesystem::path(filename);
		path = path.parent_path() / ".rsrc" / path.filename();
		path += naps_suffix;
		if(!std::filesystem::create_directory(path.parent_path(), err) && err != std::errc(0))
		{
			Linker::Error << "Error: Unable to create folder " << path.parent_path() << ", no resource fork file will be generated" << std::endl;
		}
		else
		{
			out.open(path.string(), std::ios_base::out | std::ios_base::binary);
			if(resource_fork != nullptr)
			{
				auto wr = std::make_shared<Linker::StreamWriter>(::BigEndian, out);
				resource_fork->WriteFile(wr);
			}
			out.close();
		}
	}

	if((produce & PRODUCE_FINDER_INFO))
	{
		Linker::Debug << "Debug: Generating Finder Info file under .finf" << std::endl;
		std::error_code err;
		std::filesystem::path path = std::filesystem::path(filename);
		path = path.parent_path() / ".finf" / path.filename();
		path += naps_suffix;
		if(!std::filesystem::create_directory(path.parent_path(), err) && err != std::errc(0))
		{
			Linker::Error << "Error: Unable to create folder " << path.parent_path() << ", no Finder Info file will be generated" << std::endl;
		}
		else
		{
			out.open(path.string(), std::ios_base::out | std::ios_base::binary);
			if(auto entry = apple_single->FindEntry(AppleSingleDouble::ID_FinderInfo))
			{
				auto wr = std::make_shared<Linker::StreamWriter>(::BigEndian, out);
				entry->WriteFile(wr);
			}
			out.close();
		}
	}

	if((produce & PRODUCE_APPLE_DOUBLE))
	{
		Linker::Debug << "Debug: Generating AppleDouble" << std::endl;
		std::ofstream out;
		out.open(apple_single->GetUNIXDoubleFilename(filename) + naps_suffix, std::ios_base::out | std::ios_base::binary);
		auto wr = std::make_shared<Linker::StreamWriter>(::BigEndian, out);
		if(target != TARGET_APPLE_SINGLE)
		{
			apple_single->WriteFile(wr);
		}
		else
		{
			Apple::AppleSingleDouble apple_double(*apple_single, Apple::AppleSingleDouble::DOUBLE);
			apple_double.WriteFile(wr);
		}
		out.close();
	}

	if((produce & PRODUCE_MAC_BINARY))
	{
		Linker::Debug << "Debug: Generating MacBinary" << std::endl;
		std::ofstream out;
		out.open(
			(target == TARGET_NONE ? filename : filename + ".mbin") + naps_suffix,
			std::ios_base::out | std::ios_base::binary);
		auto wr = std::make_shared<Linker::StreamWriter>(::BigEndian, out);
		mac_binary->WriteFile(wr);
		out.close();
	}
}

void OutputDriver::ReadFile(const std::shared_ptr<Linker::Reader>& rd)
{
	apple_single = nullptr;
	mac_binary = nullptr;

	switch(target)
	{
	case TARGET_DATA_FORK:
	case TARGET_RESOURCE_FORK:
		container = CONTAINER_NONE;
		OnReadFile(rd);
		break;
	case TARGET_APPLE_SINGLE:
	case TARGET_APPLE_DOUBLE:
		{
			container = CONTAINER_APPLE_SINGLE;
			apple_single = std::make_shared<Apple::AppleSingleDouble>();
			apple_single->ReadFile(rd);
		}
		break;
	case TARGET_MAC_BINARY:
		{
			container = CONTAINER_MAC_BINARY;
			mac_binary = std::make_shared<MacBinary>();
			mac_binary->ReadFile(rd);
		}
		break;
	case TARGET_NONE:
		if((produce & PRODUCE_APPLE_DOUBLE))
		{
			container = CONTAINER_APPLE_SINGLE;
			apple_single = std::make_shared<Apple::AppleSingleDouble>();
			apple_single->ReadFile(rd);
		}
		else if((produce & PRODUCE_MAC_BINARY))
		{
			container = CONTAINER_MAC_BINARY;
			mac_binary = std::make_shared<MacBinary>();
			mac_binary->ReadFile(rd);
		}
		else
		{
			container = CONTAINER_NONE;
			OnReadFile(rd);
		}
		break;
	}
}

offset_t OutputDriver::WriteFile(const std::shared_ptr<Linker::Writer>& wr) const
{
	switch(container)
	{
	case CONTAINER_NONE:
		return OnWriteFile(wr);
	case CONTAINER_APPLE_SINGLE:
		return apple_single->WriteFile(wr);
	case CONTAINER_MAC_BINARY:
		return mac_binary->WriteFile(wr);
	default:
		Linker::FatalError("Internal error: file not loaded");
	}
}

void OutputDriver::Dump(Dumper::Dumper& dump) const
{
	switch(container)
	{
	case CONTAINER_NONE:
		OnDump(dump);
		break;
	case CONTAINER_APPLE_SINGLE:
		apple_single->Dump(dump);
		break;
	case CONTAINER_MAC_BINARY:
		mac_binary->Dump(dump);
		break;
	default:
		Linker::FatalError("Internal error: file not loaded");
	}
}

