
#include <cstring>
#include "macos.h"
#include "../linker/options.h"
#include "../linker/position.h"
#include "../linker/reader.h"
#include "../linker/resolution.h"
#include "../linker/section.h"

using namespace Apple;

uint32_t Apple::OSTypeToUInt32(const OSType& type)
{
	return
		(uint32_t(uint8_t(type[0])) << 24)
		| (uint32_t(uint8_t(type[1])) << 16)
		| (uint32_t(uint8_t(type[2])) << 8)
		| uint32_t(uint8_t(type[3]));
}

void Apple::UInt32ToOSType(OSType& type, uint32_t value)
{
	type[0] = value >> 24;
	type[1] = value >> 16;
	type[2] = value >> 8;
	type[3] = value;
}

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

bool AppleSingleDouble::FormatSupportsResources() const
{
	return true;
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
	entry_region.Display(dump);
}

void AppleSingleDouble::Entry::ProcessModule(Linker::Module& module)
{
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
	block.Display(dump);
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

void AppleSingleDouble::SetOptions(std::map<std::string, std::string>& options)
{
	/* TODO */

	std::dynamic_pointer_cast<ResourceFork>(GetResourceFork())->SetOptions(options);
}

std::vector<Linker::OptionDescription<void>> AppleSingleDouble::GetMemoryModelNames()
{
	return std::dynamic_pointer_cast<ResourceFork>(GetResourceFork())->GetMemoryModelNames();
}

void AppleSingleDouble::SetModel(std::string model)
{
	std::dynamic_pointer_cast<ResourceFork>(GetResourceFork())->SetModel(model);
}

void AppleSingleDouble::SetLinkScript(std::string script_file, std::map<std::string, std::string>& options)
{
	std::dynamic_pointer_cast<ResourceFork>(GetResourceFork())->SetLinkScript(script_file, options);
}

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
	std::shared_ptr<Entry> entry = GetMacintoshFileInfo();
	if(entry == nullptr)
		return 0;
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

void AppleSingleDouble::ProcessModule(Linker::Module& module)
{
	unsigned entry_bitmap = 0;
	for(auto entry : entries)
	{
		if(entry->id < 32)
			entry_bitmap |= 1 << entry->id;
	}
	if(type == SINGLE && !(entry_bitmap & (1 << ID_DataFork)))
	{
		GetDataFork();
	}
	if(!(entry_bitmap & (1 << ID_ResourceFork)))
	{
		GetResourceFork();
	}
	if(!(entry_bitmap & (1 << ID_FinderInfo)))
	{
		GetFinderInfo();
	}
	for(auto entry : entries)
	{
		entry->ProcessModule(module);
	}
}

void AppleSingleDouble::CalculateValues()
{
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
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("AppleSingle/AppleDouble format");
	Dumper::Region file_region("File", file_offset, ImageSize(), 8);
	file_region.Display(dump);

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

void AppleSingleDouble::GenerateFile(std::string filename, Linker::Module& module)
{
	if(module.cpu != Linker::Module::M68K)
	{
		Linker::Error << "Error: Format only supports Motorola 68000 binaries" << std::endl;
	}

	ProcessModule(module);
	CalculateValues();

	std::ofstream out;
	Linker::Writer wr(::BigEndian);
	switch(type)
	{
	case SINGLE:
		out.open(filename, std::ios_base::out | std::ios_base::binary);
		wr.out = &out;
		WriteFile(wr);
		out.close();
		break;
	case DOUBLE:
		{
			std::ofstream empty;
			empty.open(filename, std::ios_base::out | std::ios_base::binary);
			empty.close();
		}

		// TODO: check host operating system
		out.open(GetUNIXDoubleFilename(filename), std::ios_base::out | std::ios_base::binary);
		wr.out = &out;
		WriteFile(wr);
		out.close();
		break;
	}
}

std::string AppleSingleDouble::GetDefaultExtension(Linker::Module& module) const
{
	return "a.out";
}

// DataFork

offset_t DataFork::ImageSize() const
{
	return image->ImageSize();
}

void DataFork::ReadFile(Linker::Reader& rd)
{
	image = Linker::Buffer::ReadFromFile(rd, image_size);
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
	Dumper::Block block("Data fork", file_offset, image->AsImage(), 0, 8);
	block.Display(dump);
}

void DataFork::CalculateValues()
{
	image_size = image != nullptr ? image->ImageSize() : 0;
}

// ResourceFork

void ResourceFork::SetOptions(std::map<std::string, std::string>& options)
{
	/* TODO */
}

std::vector<Linker::OptionDescription<void>> ResourceFork::MemoryModelNames =
{
	Linker::OptionDescription<void>("default", "Normal model, symbols in zero-filled sectioned must be accessed as A5 relative addresses (\"A5 world\")"),
	Linker::OptionDescription<void>("tiny", "Tiny model, symbols in zero-filled sections are placed in the CODE segment"),
};

std::vector<Linker::OptionDescription<void>> ResourceFork::GetMemoryModelNames()
{
	return MemoryModelNames;
}

void ResourceFork::SetModel(std::string model)
{
	if(model == "" || model == "default")
	{
		Linker::Debug << "Debug: new memory model: default" << std::endl;
		memory_model = MODEL_DEFAULT;
	}
	else if(model == "tiny")
	{
		Linker::Debug << "Debug: new memory model: tiny" << std::endl;
		memory_model = MODEL_TINY;
	}
	else
	{
		Linker::Error << "Error: unsupported memory model" << std::endl;
		memory_model = MODEL_DEFAULT;
	}
}

void ResourceFork::Resource::Dump(Dumper::Dumper& dump) const
{
	Dump(dump, 0);
}

void ResourceFork::Resource::Dump(Dumper::Dumper& dump, offset_t file_offset) const
{
	std::unique_ptr<Dumper::Region> resource_region = CreateRegion("Resource", file_offset, ImageSize(), 8);
	resource_region->AddField("OSType", Dumper::StringDisplay::Make(4, "'"), std::string(type, 4));
	resource_region->AddField("ID", Dumper::HexDisplay::Make(4), offset_t(id));
	if(name)
		resource_region->AddField("Name", Dumper::StringDisplay::Make("\""), *name);
	resource_region->AddField("Attributes", Dumper::HexDisplay::Make(2), offset_t(attributes));
	AddFields(dump, *resource_region, file_offset);
	resource_region->Display(dump);
}

void ResourceFork::Resource::AddFields(Dumper::Dumper& dump, Dumper::Region& region, offset_t file_offset) const
{
}

std::unique_ptr<Dumper::Region> ResourceFork::Resource::CreateRegion(std::string name, offset_t offset, offset_t length, unsigned display_width) const
{
	return std::make_unique<Dumper::Region>(name, offset, length, display_width);
}

void ResourceFork::GenericResource::ProcessModule(Linker::Module& module)
{
}

void ResourceFork::GenericResource::CalculateValues()
{
}

offset_t ResourceFork::GenericResource::ImageSize() const
{
	return image->ImageSize();
}

void ResourceFork::GenericResource::ReadFile(Linker::Reader& rd)
{
	image = Linker::Buffer::ReadFromFile(rd);
}

void ResourceFork::GenericResource::ReadFile(Linker::Reader& rd, offset_t length)
{
	image = Linker::Buffer::ReadFromFile(rd, length);
}

offset_t ResourceFork::GenericResource::WriteFile(Linker::Writer& wr) const
{
	return image->WriteFile(wr);
}

std::unique_ptr<Dumper::Region> ResourceFork::GenericResource::CreateRegion(std::string name, offset_t offset, offset_t length, unsigned display_width) const
{
	return std::make_unique<Dumper::Block>(name, offset, image->AsImage(), 0, display_width);
}

void ResourceFork::JumpTableCodeResource::ProcessModule(Linker::Module& module)
{
}

void ResourceFork::JumpTableCodeResource::CalculateValues()
{
	jump_table_offset = 32;
	if(far_entries.size() == 0)
	{
		above_a5 = 0x30 + 8 * near_entries.size();
	}
	else
	{
		above_a5 = 0x30 + 8 + 8 * (near_entries.size() + far_entries.size());
	}
}

offset_t ResourceFork::JumpTableCodeResource::ImageSize() const
{
	if(far_entries.size() == 0)
	{
		return 16 + 8 * near_entries.size();
	}
	else
	{
		return 24 + 8 * (near_entries.size() + far_entries.size());
	}
}

void ResourceFork::JumpTableCodeResource::ReadFile(Linker::Reader& rd)
{
	/* TODO */
}

void ResourceFork::JumpTableCodeResource::ReadFile(Linker::Reader& rd, offset_t length)
{
	above_a5 = rd.ReadUnsigned(4);
	below_a5 = rd.ReadUnsigned(4);
	uint32_t total_entry_size = rd.ReadUnsigned(4);
	jump_table_offset = rd.ReadUnsigned(4);
	uint32_t i;
	for(i = 0; i < total_entry_size; i += 8)
	{
		Entry entry;
		entry.offset = rd.ReadUnsigned(2);
		uint16_t _move_data_sp = rd.ReadUnsigned(2); // MOVE_DATA_SP
		entry.segment = rd.ReadUnsigned(2);
		uint16_t _loadseg = rd.ReadUnsigned(2); // LOADSEG
		if(entry.offset == 0 && _move_data_sp == 0xFFFF && entry.segment == 0 && _loadseg == 0)
		{
			break;
		}
		near_entries.push_back(entry);
	}

	for(; i < total_entry_size; i += 8)
	{
		Entry entry;
		entry.segment = rd.ReadUnsigned(2);
		rd.Skip(2); // LOADSEG
		entry.offset = rd.ReadUnsigned(4);
		far_entries.push_back(entry);
	}
}

offset_t ResourceFork::JumpTableCodeResource::WriteFile(Linker::Writer& wr) const
{
	wr.WriteWord(4, above_a5);
	wr.WriteWord(4, below_a5);
	if(far_entries.size() == 0)
	{
		wr.WriteWord(4, 8 * near_entries.size());
	}
	else
	{
		wr.WriteWord(4, 8 + 8 * (near_entries.size() + far_entries.size()));
	}
	wr.WriteWord(4, jump_table_offset);
	for(const Entry& entry : near_entries)
	{
		wr.WriteWord(2, entry.offset);
		wr.WriteWord(2, MOVE_DATA_SP);
		wr.WriteWord(2, entry.segment);
		wr.WriteWord(2, LOADSEG);
	}
	if(far_entries.size() != 0)
	{
		wr.WriteData(8, "\0\0\xFF\xFF\0\0\0\0");
		for(const Entry& entry : far_entries)
		{
			wr.WriteWord(2, entry.segment);
			wr.WriteWord(2, LOADSEG);
			wr.WriteWord(4, entry.offset);
		}
	}

	return ImageSize();
}

void ResourceFork::JumpTableCodeResource::AddFields(Dumper::Dumper& dump, Dumper::Region& region, offset_t file_offset) const
{
	region.AddField("Above A5", Dumper::HexDisplay::Make(8), offset_t(above_a5));
	region.AddField("Below A5", Dumper::HexDisplay::Make(8), offset_t(below_a5));
	offset_t jump_table_size;
	if(far_entries.size() == 0)
	{
		jump_table_size = 8 * near_entries.size();
	}
	else
	{
		jump_table_size = 8 + 8 * (near_entries.size() + far_entries.size());
	}
	region.AddField("Jump table size", Dumper::HexDisplay::Make(8), offset_t(jump_table_size));
	region.AddField("Jump table offset", Dumper::HexDisplay::Make(8), offset_t(32));
}

void ResourceFork::JumpTableCodeResource::Dump(Dumper::Dumper& dump, offset_t file_offset) const
{
	Resource::Dump(dump, file_offset);

	unsigned i = 0;
	for(auto& entry : near_entries)
	{
		Dumper::Entry entry_entry("Entry", i + 1, file_offset + 16 + i * 8);
		entry_entry.AddField("Type", Dumper::ChoiceDisplay::Make("near"), offset_t(true));
		entry_entry.AddField("Value", Dumper::SegmentedDisplay::Make(4), offset_t(entry.segment), offset_t(entry.offset));
		entry_entry.Display(dump);
		i++;
	}

	// skip one value for separator
	i++;

	for(auto& entry : far_entries)
	{
		Dumper::Entry entry_entry("Entry", i + 1, file_offset + 16 + i * 8);
		entry_entry.AddField("Type", Dumper::ChoiceDisplay::Make("near"), offset_t(true));
		entry_entry.AddField("Value", Dumper::SegmentedDisplay::Make(8), offset_t(entry.segment), offset_t(entry.offset));
		entry_entry.Display(dump);
		i++;
	}
}

void ResourceFork::CodeResource::ProcessModule(Linker::Module& module)
{
}

void ResourceFork::CodeResource::CalculateValues()
{
	if(Linker::Segment * segment = dynamic_cast<Linker::Segment *>(image.get()))
	{
		zero_fill = segment->zero_fill;
	}

	near_entry_count = near_entries.size();
	far_entry_count = far_entries.size();
	if(!is_far)
	{
		resource_size = 4 + image->ImageSize() + zero_fill;
	}
	else
	{
		a5_relocation_offset = 0x28 + ImageSize() + zero_fill;
		segment_relocation_offset = a5_relocation_offset + MeasureRelocations(a5_relocations);
		resource_size = segment_relocation_offset + MeasureRelocations(segment_relocations);
	}
}

offset_t ResourceFork::CodeResource::ImageSize() const
{
	return resource_size;
}

uint32_t ResourceFork::CodeResource::MeasureRelocations(std::set<uint32_t>& relocations) const
{
	uint32_t count = 2;
	uint32_t last_relocation = 0;
	for(uint32_t relocation : relocations)
	{
		uint32_t offset = relocation - last_relocation;
		if(offset < 0x100)
		{
			count ++;
		}
		else if(offset < 0x10000)
		{
			count += 2;
		}
		else
		{
			count += 5;
		}
		last_relocation = relocation;
	}
	return count;
}

void ResourceFork::CodeResource::ReadRelocations(Linker::Reader& rd, std::set<uint32_t>& relocations) const
{
	/* TODO: test */
	uint32_t last_relocation = 0;
	while(true)
	{
		uint32_t offset = rd.ReadUnsigned(1);
		if(offset == 0)
		{
			if((rd.ReadUnsigned(1) & 0x80) == 0)
			{
				break;
			}
			rd.Skip(-1);
			offset = rd.ReadUnsigned(4) & ~0x80000000;
		}
		else if((offset & 0x80) != 0)
		{
			rd.Skip(-1);
			offset = rd.ReadUnsigned(2) & ~0x8000;
		}
		last_relocation += offset << 1;
		relocations.insert(last_relocation);
	}
}

void ResourceFork::CodeResource::WriteRelocations(Linker::Writer& wr, const std::set<uint32_t>& relocations) const
{
	/* TODO: test */
	uint32_t last_relocation = 0;
	for(uint32_t relocation : relocations)
	{
		uint32_t offset = relocation - last_relocation;
		if(offset < 0x100)
		{
			wr.WriteWord(1, offset >> 1);
		}
		else if(offset < 0x10000)
		{
			wr.WriteWord(2, 0x8000 | (offset >> 1));
		}
		else
		{
			wr.WriteWord(1, 0);
			wr.WriteWord(4, 0x80000000 | (offset >> 1));
		}
		last_relocation = relocation;
	}
	wr.WriteWord(2, 0);
}

void ResourceFork::CodeResource::ReadFile(Linker::Reader& rd)
{
	/* TODO */
}

void ResourceFork::CodeResource::ReadFile(Linker::Reader& rd, offset_t length)
{
	resource_size = length;
	first_near_entry_offset = rd.ReadUnsigned(2);
	near_entry_count = rd.ReadUnsigned(2);
	if(first_near_entry_offset == 0xFFFF && near_entry_count == 0)
	{
		is_far = true;
		first_near_entry_offset = rd.ReadUnsigned(4);
		near_entry_count = rd.ReadUnsigned(4);
		first_far_entry_offset = rd.ReadUnsigned(4);
		far_entry_count = rd.ReadUnsigned(4);
		a5_relocation_offset = rd.ReadUnsigned(4);
		a5_address = rd.ReadUnsigned(4);
		segment_relocation_offset = rd.ReadUnsigned(4);
		base_address = rd.ReadUnsigned(4);
		rd.Skip(4);
		image = Linker::Buffer::ReadFromFile(rd, a5_relocation_offset - 0x28);
		ReadRelocations(rd, a5_relocations);
		ReadRelocations(rd, segment_relocations);
	}
	else
	{
		is_far = false;
		image = Linker::Buffer::ReadFromFile(rd, length - 4);
	}
}

offset_t ResourceFork::CodeResource::WriteFile(Linker::Writer& wr) const
{
	if(!is_far)
	{
		wr.WriteWord(2, first_near_entry_offset);
		wr.WriteWord(2, near_entry_count);
		image->WriteFile(wr);
		wr.Skip(zero_fill);
	}
	else
	{
		wr.WriteData(4, "\xFF\xFF\0\0");
		wr.WriteWord(4, first_near_entry_offset);
		wr.WriteWord(4, near_entry_count);
		wr.WriteWord(4, first_far_entry_offset);
		wr.WriteWord(4, far_entry_count);
		wr.WriteWord(4, a5_relocation_offset);
		wr.WriteWord(4, a5_address);
		wr.WriteWord(4, segment_relocation_offset);
		wr.WriteWord(4, base_address);
		wr.WriteWord(4, 0);
		image->WriteFile(wr);
		WriteRelocations(wr, a5_relocations);
		WriteRelocations(wr, segment_relocations);
	}

	return ImageSize();
}

void ResourceFork::CodeResource::AddFields(Dumper::Dumper& dump, Dumper::Region& region, offset_t file_offset) const
{
	region.AddField("Type", Dumper::ChoiceDisplay::Make("far", "near"), offset_t(is_far));
	region.AddField("Near entry count", Dumper::DecDisplay::Make(), offset_t(near_entry_count));
	region.AddField("First near entry offset", Dumper::HexDisplay::Make(is_far ? 8 : 4), offset_t(first_near_entry_offset));
	if(is_far)
	{
		region.AddField("Far entry count", Dumper::DecDisplay::Make(), offset_t(far_entry_count));
		region.AddField("First far entry offset", Dumper::HexDisplay::Make(8), offset_t(first_far_entry_offset));
		region.AddField("A5 relocation offset", Dumper::HexDisplay::Make(8), offset_t(a5_relocation_offset));
		region.AddField("A5 value", Dumper::HexDisplay::Make(8), offset_t(a5_address));
		region.AddField("Segment relocation offset", Dumper::HexDisplay::Make(8), offset_t(segment_relocation_offset));
		region.AddField("Segment address", Dumper::HexDisplay::Make(8), offset_t(base_address));
	}

}

void ResourceFork::CodeResource::Dump(Dumper::Dumper& dump, offset_t file_offset) const
{
	Resource::Dump(dump, file_offset);
	// TODO: print relocations
}

std::unique_ptr<Dumper::Region> ResourceFork::CodeResource::CreateRegion(std::string name, offset_t offset, offset_t length, unsigned display_width) const
{
	return std::make_unique<Dumper::Block>(name, offset + (is_far ? 0x28 : 4), image->AsImage(), base_address, display_width);
}

void ResourceFork::AddResource(std::shared_ptr<Resource> resource)
{
	uint32_t typeval = OSTypeToUInt32(resource->type);
	resources[typeval][resource->id] = resource;
}

void ResourceFork::OnNewSegment(std::shared_ptr<Linker::Segment> segment)
{
	if(segment->name == ".a5world")
	{
		a5world = segment;
	}
	else if(segment->sections.size() == 0)
	{
		return;
	}
	else if(!(segment->sections.front()->GetFlags() & Linker::Section::Resource))
	{
		std::shared_ptr<CodeResource> codeN = std::make_shared<CodeResource>(codes.size() + 1, jump_table);
		codeN->image = segment;
		codes.push_back(codeN);
		segments[segment] = codeN;
		AddResource(codeN);
	}
	else
	{
		std::shared_ptr<Linker::Section> section = segment->sections.front();
		/* other resources */
		std::string * type = std::get_if<std::string>(&section->resource_type);
		if(type == nullptr || type->size() != 4)
		{
			Linker::Error << "Error: resources are expected to have a 4-character type" << std::endl;
			return;
		}
		uint16_t * id = std::get_if<uint16_t>(&section->resource_id);
		if(id == nullptr)
		{
			Linker::Error << "Error: resources are expected to have a 16-bit ID" << std::endl;
			return;
		}
		Linker::Debug << "Debug: Adding resource type " << *type << ", id " << *id << std::endl;
		std::shared_ptr<GenericResource> rsrc = std::make_shared<GenericResource>(type->c_str(), *id);
//		rsrc->resource = std::make_shared<Linker::Segment>(".rsrc");
//		rsrc->resource->Append(section);
		rsrc->image = segment;
		AddResource(rsrc);
	}
}

std::unique_ptr<Script::List> ResourceFork::GetScript(Linker::Module& module)
{
	/* TODO: make placing .comm/.bss data inside the .a5world optional */

	static const char * SimpleScript = R"(
".a5world"
{
	all ".comm" align 2;
	all zero and not ".a5world" align 2;
#	all ".globals" align 2; # TODO: not currently used
	all ".a5world" align 2;
	align 2;
} at -size of ".a5world";

".code"
{
	at 0;
	all ".code" or ".text" or ".data" or ".rodata"
		align 2;
	align 2;
};

for not resource
{
	at 0;
	all any
		align 2;
	align 2;
};

# TODO: resources
for any
{
	at 0;
	all any;
};
)";

	static const char * TinyScript = R"(
".a5world"
{
	all ".a5world" align 2;
	align 2;
} at -size of ".a5world";

".code"
{
	at 0;
	all ".code" or ".text" or ".data" or ".rodata"
		align 2;
	all not resource align 2;
	all ".comm" align 2;
	all zero align 2;
	align 2;
};

for any
{
	at 0;
	all any;
};
)";

	if(linker_script != "")
	{
		return SegmentManager::GetScript(module);
	}
	else
	{
		switch(memory_model)
		{
		case MODEL_DEFAULT:
		default:
			return Script::parse_string(SimpleScript);
		case MODEL_TINY:
			return Script::parse_string(TinyScript);
		}
	}
}

void ResourceFork::Link(Linker::Module& module)
{
	std::unique_ptr<Script::List> script = GetScript(module);

	ProcessScript(script, module);
}

void ResourceFork::ProcessModule(Linker::Module& module)
{
	jump_table = std::make_shared<JumpTableCodeResource>();
	AddResource(jump_table);

for(auto section : module.Sections())
{
	Linker::Debug << "Debug: " << *section << std::endl;
}
	Link(module);
	jump_table->below_a5 = a5world->zero_fill;
	Linker::Debug << "Debug: Setting the A5 world to " << a5world->zero_fill << std::endl;

	uint32_t entry_offset = 0;
	Linker::Location entry;
	if(module.FindGlobalSymbol(".entry", entry))
	{
		Linker::Position position = entry.GetPosition();
		if(position.segment != codes[0]->image)
		{
			Linker::Error << "Error: entry point must be in `.code' segment, using offset .code:0 instead" << std::endl;
		}
		else
		{
			entry_offset = position.address;
		}
	}
	/* must be the first entry */
	codes[0]->near_entries.insert(entry_offset);

	for(Linker::Relocation& rel : module.relocations)
	{
		Linker::Resolution resolution;
		if(!rel.Resolve(module, resolution))
		{
			Linker::Error << "Error: Unable to resolve relocation: " << rel << std::endl;
		}
		rel.WriteWord(resolution.value);
		if(resolution.target != nullptr)
		{
			/* TODO: how do we deal with relocations? */
			/* idea: jsr method_name(a5) can be replaced by an entry */
		}
	}

	/* must be the first entry */
	jump_table->near_entries.push_back(JumpTableCodeResource::Entry{1, entry_offset});

	for(auto& resource : codes)
	{
		/* since the first entry is already loaded, we have to skip it */
		resource->first_near_entry_offset = resource == codes[0] ? 0 : jump_table->near_entries.size() * 8;
		for(uint16_t entry : resource->near_entries)
		{
			if(resource == codes[0] && entry == entry_offset)
				continue; /* already inserted */
			jump_table->near_entries.push_back(JumpTableCodeResource::Entry{1, entry}); /* TODO: segment number */
		}
	}
	for(auto& resource : codes)
	{
		if(resource->far_entries.size() == 0)
			continue;
		resource->first_far_entry_offset = jump_table->far_entries.size() * 8;
		//jump_table->far_entries.insert(jump_table->far_entries.end(), resource->far_entries.begin(), resource->far_entries.end());
		for(uint32_t entry : resource->far_entries)
		{
			jump_table->far_entries.push_back(JumpTableCodeResource::Entry{1, entry}); /* TODO: segment number */
		}
	}
}

void ResourceFork::CalculateValues()
{
	data_length = 0;
	uint32_t name_list_length = 0;
	uint16_t reference_list_offset = 2 + resources.size() * 8;
	resource_types.clear();
	resource_names.clear();
	for(auto it : resources)
	{
		if(it.second.size() == 0)
			continue;

		ResourceType type;
		UInt32ToOSType(type.type, it.first);
		type.offset = reference_list_offset;
		reference_list_offset += 12 * it.second.size();
		name_list_offset += 12 * it.second.size();
		for(auto it2 : it.second)
		{
			ResourceReference reference;
			reference.id = it2.first;
			std::shared_ptr<Resource> resource = it2.second;
			reference.data = resource;
			reference.data_offset = data_length;
			resource->CalculateValues();
			data_length += 4 + resource->ImageSize();
			if(resource->name)
			{
				resource_names.push_back(*resource->name);
				reference.name_offset = name_list_length;
				name_list_length += 1 + resource->name->size();
			}
			else
			{
				reference.name_offset = 0xFFFF;
			}
			reference.attributes = resource->attributes;
			type.references.push_back(reference);
		}

		resource_types.push_back(type);
	}

	resource_type_list_offset = 28;
	name_list_offset = resource_type_list_offset + reference_list_offset;
	if(data_offset < 16)
		data_offset = 0x0100;
	map_offset = data_offset + data_length;
	map_length = name_list_offset + name_list_length;
}

offset_t ResourceFork::ImageSize() const
{
	return std::max(data_offset + data_length, map_offset + map_length);
}

void ResourceFork::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = ::BigEndian; /* in case we write the resource fork directly, without an AppleSingle/AppleDouble wrapper */
	offset_t read_offset = rd.Tell();
	data_offset = rd.ReadUnsigned(4);
	map_offset = rd.ReadUnsigned(4);
	data_length = rd.ReadUnsigned(4);
	map_length = rd.ReadUnsigned(4);

	rd.Seek(read_offset + map_offset + 22);
	attributes = rd.ReadUnsigned(2);
	resource_type_list_offset = rd.ReadUnsigned(2);
	name_list_offset = rd.ReadUnsigned(2);

	rd.Seek(read_offset + map_offset + resource_type_list_offset);
	offset_t resource_count = offset_t(rd.ReadUnsigned(2)) + 1;

	/* type list */
	for(offset_t i = 0; i < resource_count; i++)
	{
		ResourceType type;
		rd.ReadData(4, type.type);
		type.count = uint32_t(rd.ReadUnsigned(2)) + 1;
		type.offset = rd.ReadUnsigned(2);
		resource_types.push_back(type);
	}

	/* reference list */
	for(auto& type : resource_types)
	{
		rd.Seek(read_offset + map_offset + resource_type_list_offset + type.offset);
		for(offset_t i = 0; i < type.count; i++)
		{
			ResourceReference reference;
			reference.id = rd.ReadUnsigned(2);
			reference.name_offset = rd.ReadUnsigned(2);
			reference.data_offset = rd.ReadUnsigned(4);
			reference.attributes = reference.data_offset >> 24;
			reference.data_offset &= 0x00FFFFFF;
			rd.Skip(4);
			type.references.push_back(reference);
		}
	}

	/* name list */
	rd.Seek(read_offset + map_offset + name_list_offset);
	// first read all the names
	while(rd.Tell() < read_offset + map_offset + map_length)
	{
		uint8_t size = rd.ReadUnsigned(1);
		resource_names.push_back(rd.ReadData(size));
	}

	for(auto& type : resource_types)
	{
		for(auto& reference : type.references)
		{
			// read resource data
			rd.Seek(read_offset + data_offset + reference.data_offset);
			reference.data = ReadResource(rd, type, reference);

			// read resource name
			if(reference.name_offset != 0xFFFF)
			{
				rd.Seek(read_offset + map_offset + name_list_offset + reference.name_offset);
				uint8_t size = rd.ReadUnsigned(1);
				reference.name = rd.ReadData(size);
			}

			// register this resource for convenience
			AddResource(reference.data);
		}
	}
}

offset_t ResourceFork::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::BigEndian; /* in case we write the resource fork directly, without an AppleSingle/AppleDouble wrapper */
	offset_t write_offset = wr.Tell();
	wr.WriteWord(4, data_offset);
	wr.WriteWord(4, map_offset);
	wr.WriteWord(4, data_length);
	wr.WriteWord(4, map_length);
	/* data start */
	for(auto& type : resource_types)
	{
		for(auto& reference : type.references)
		{
			std::shared_ptr<Resource> resource = reference.data;
			wr.Seek(write_offset + data_offset + reference.data_offset);
			wr.WriteWord(4, resource->ImageSize());
			resource->WriteFile(wr);
		}
	}
	/* map start */
	wr.Seek(write_offset + map_offset + 22);
	wr.WriteWord(2, attributes);
	wr.WriteWord(2, resource_type_list_offset);
	wr.WriteWord(2, name_list_offset);
	wr.Seek(write_offset + map_offset + resource_type_list_offset);
	wr.WriteWord(2, resources.size() - 1);
	/* type list */
	for(auto& type : resource_types)
	{
		if(type.references.size() == 0)
			continue;
		wr.WriteData(4, type.type);
		wr.WriteWord(2, type.references.size() - 1);
		wr.WriteWord(2, type.offset);
	}
	/* reference list */
	for(auto& type : resource_types)
	{
		wr.Seek(write_offset + map_offset + resource_type_list_offset + type.offset);
		for(auto& reference : type.references)
		{
			wr.WriteWord(2, reference.id);
			wr.WriteWord(2, reference.name_offset);
			wr.WriteWord(4, (reference.data_offset & 0x00FFFFFF) | (reference.attributes << 24));
			wr.Skip(4);
		}
	}
	/* name list */
	wr.Seek(write_offset + map_offset + name_list_offset);
	for(auto& name : resource_names)
	{
		wr.WriteWord(1, name.size());
		wr.WriteData(name);
	}

	return ImageSize();
}

void ResourceFork::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("Macintosh resource fork format");
	Dumper::Region file_region("File", file_offset, ImageSize(), 8);
	file_region.Display(dump);

	Dumper::Region data_region("Resource data", file_offset + data_offset, data_length, 8);
	data_region.Display(dump);

	Dumper::Region map_region("Resource map", file_offset + map_offset, map_length, 8);
	map_region.AddField("Attributes", Dumper::HexDisplay::Make(4), offset_t(attributes));
	map_region.Display(dump);

	offset_t resource_type_list_size = 2 + resource_types.size() * 8;
	for(auto& type : resource_types)
	{
		resource_type_list_size += type.references.size() * 12;
	}
	Dumper::Region resource_type_list_region("Resource type list", file_offset + map_offset + resource_type_list_offset, resource_type_list_size, 8);
	resource_type_list_region.Display(dump);

	unsigned i = 0;
	for(auto& type : resource_types)
	{
		Dumper::Entry resource_type_entry("Resource type", i + 1, file_offset + map_offset + resource_type_list_offset + i * 8, 8);
		resource_type_entry.AddField("OSType", Dumper::StringDisplay::Make(4, "'"), std::string(type.type, 4));
		resource_type_entry.Display(dump);

		unsigned j = 0;
		for(auto& reference : type.references)
		{
			Dumper::Entry resource_reference_entry("Resource reference", j + 1, file_offset + map_offset + resource_type_list_offset + type.offset + j * 12, 8);
			resource_reference_entry.AddField("OSType", Dumper::StringDisplay::Make(4, "'"), std::string(type.type, 4));
			resource_reference_entry.AddField("ID", Dumper::HexDisplay::Make(4), offset_t(reference.id));
			if(reference.name)
				resource_reference_entry.AddField("Name", Dumper::StringDisplay::Make("\""), *reference.name);
			resource_reference_entry.AddOptionalField("Name offset", Dumper::HexDisplay::Make(4), offset_t(reference.name_offset != 0xFFFF ? file_offset + map_offset + name_list_offset + reference.name_offset : 0));
			resource_reference_entry.AddField("Attributes", Dumper::HexDisplay::Make(2), offset_t(reference.attributes));
			resource_reference_entry.AddField("Data offset", Dumper::HexDisplay::Make(8), offset_t(reference.data_offset));
			resource_reference_entry.AddField("Data length", Dumper::HexDisplay::Make(8), offset_t(reference.data->ImageSize()));
			resource_reference_entry.Display(dump);
			j++;
		}

		i++;
	}

	offset_t resource_name_list_size = 0;
	for(auto& name : resource_names)
	{
		resource_name_list_size += name.size() + 1;
	}
	Dumper::Region resource_name_list_region("Resource name list", file_offset + map_offset + name_list_offset, resource_type_list_size, 8);
	resource_name_list_region.Display(dump);

	offset_t current_offset = file_offset + map_offset + name_list_offset;
	i = 0;
	for(auto& name : resource_names)
	{
		Dumper::Entry name_entry("Name", i + 1, current_offset, 8);
		name_entry.AddField("Value", Dumper::StringDisplay::Make("'"), name);
		name_entry.Display(dump);
		current_offset += name.size() + 1;
		i++;
	}

	for(auto& type : resource_types)
	{
		for(auto& reference : type.references)
		{
			reference.data->Dump(dump, file_offset + data_offset + reference.data_offset);
		}
	}

	// TODO: display all resource data
}

void ResourceFork::GenerateFile(std::string filename, Linker::Module& module)
{
	if(module.cpu != Linker::Module::M68K)
	{
		Linker::Error << "Error: Format only supports Motorola 68000 binaries" << std::endl;
	}

	Linker::OutputFormat::GenerateFile(filename, module);
}

std::string ResourceFork::GetDefaultExtension(Linker::Module& module) const
{
	return "a.out";
}

std::shared_ptr<ResourceFork::Resource> ResourceFork::ReadResource(Linker::Reader& rd, const ResourceType& type, const ResourceReference& reference)
{
	std::shared_ptr<Resource> resource = nullptr;
	uint32_t length = rd.ReadUnsigned(4);
	switch(OSTypeToUInt32(type.type))
	{
	case CodeResource::OSType:
		if(reference.id == 0)
			resource = std::make_shared<JumpTableCodeResource>();
		else
			resource = std::make_shared<CodeResource>(reference.id);
		break;
	default:
		resource = std::make_shared<GenericResource>(type.type, reference.id);
		break;
	}
	resource->name = reference.name;
	resource->ReadFile(rd, length);
	return resource;
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

void FinderInfo::ProcessModule(Linker::Module& module)
{
	memcpy(Type, "APPL", 4);
	memcpy(Creator, "????", 4);
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

void MacBinary::Skip(Linker::Writer& wr, size_t count) const
{
	for(size_t i = 0; i < count; i++)
	{
		CRC_Step(0);
	}
	wr.Skip(count);
}

void MacBinary::WriteData(Linker::Writer& wr, size_t count, const void * data) const
{
	for(size_t i = 0; i < count; i++)
	{
		CRC_Step(static_cast<const char *>(data)[i]);
	}
	wr.WriteData(count, data);
}

void MacBinary::WriteData(Linker::Writer& wr, size_t count, std::string text) const
{
	for(size_t i = 0; i < count; i++)
	{
		CRC_Step(i < text.size() ? text[i] : 0);
	}
	wr.WriteData(count, text);
}

void MacBinary::WriteWord(Linker::Writer& wr, size_t bytes, uint64_t value) const
{
	uint8_t data[bytes];
	::WriteWord(bytes, bytes, data, value, EndianType::BigEndian);
	WriteData(wr, bytes, data);
}

void MacBinary::WriteHeader(Linker::Writer& wr) const
{
	CRC_Initialize();
	WriteWord(wr, 1, 0);
	if(auto entry = FindEntry(ID_RealName))
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
	std::shared_ptr<const FinderInfo> info = nullptr;
	if(auto entry = FindEntry(ID_FinderInfo))
	{
		info = std::dynamic_pointer_cast<const FinderInfo>(entry);
		WriteData(wr, 4, info->Type);
		WriteData(wr, 4, info->Creator);
		WriteWord(wr, 1, info->Flags >> 1);
		WriteWord(wr, 1, 0);
		WriteWord(wr, 2, info->Location.y);
		WriteWord(wr, 2, info->Location.x);
		WriteWord(wr, 2, 0); /* window/folder info */
	}
	else
	{
		WriteData(wr, 16, "");
	}
	WriteWord(wr, 1, attributes);
	WriteWord(wr, 1, 0);
	if(auto entry = FindEntry(ID_DataFork))
	{
		WriteWord(wr, 4, entry->ImageSize());
	}
	else
	{
		WriteWord(wr, 4, 0);
	}
	if(auto entry = FindEntry(ID_ResourceFork))
	{
		WriteWord(wr, 4, entry->ImageSize());
	}
	else
	{
		WriteWord(wr, 4, 0);
	}
	WriteWord(wr, 4, creation);
	WriteWord(wr, 4, modification);
	if(version < MACBIN1_GETINFO)
	{
		return;
	}
	if(auto entry = FindEntry(ID_Comment))
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
	if(info != nullptr)
	{
		WriteWord(wr, 1, info->Flags & 0xFF);
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
	wr.WriteWord(2, crc);
}

void MacBinary::CalculateValues()
{
	attributes = GetMacintoshAttributes();
	creation = GetCreationDate();
	modification = GetModificationDate();
	AppleSingleDouble::CalculateValues();
}

void MacBinary::ReadFile(Linker::Reader& rd)
{
	// TODO
}

offset_t MacBinary::WriteFile(Linker::Writer& wr) const
{
	WriteHeader(wr);
	wr.Seek(::AlignTo(0x80 + secondary_header_size, 0x80));
	/* secondary header */
	if(auto entry = FindEntry(ID_DataFork))
	{
		entry->WriteFile(wr);
		wr.AlignTo(0x80);
	}
	if(auto entry = FindEntry(ID_ResourceFork))
	{
		entry->WriteFile(wr);
		wr.AlignTo(0x80);
	}
	if(version >= MACBIN1_GETINFO)
	{
		if(auto entry = FindEntry(ID_Comment))
		{
			entry->WriteFile(wr);
			wr.AlignTo(0x80);
		}
	}

	return offset_t(-1);
}

void MacBinary::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("MacBinary format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 8);
	file_region.Display(dump);

	// TODO
}

void MacBinary::GenerateFile(std::string filename, Linker::Module& module)
{
	generated_file_name = filename;
	AppleSingleDouble::GenerateFile(filename, module);
}

// MacDriver

bool MacDriver::FormatSupportsResources() const
{
	return true;
}

bool MacDriver::AddSupplementaryOutputFormat(std::string subformat)
{
	if(subformat == "rsrc")
	{
		Linker::Debug << "Debug: Requested to generate resource fork under .rsrc" << std::endl;
		produce = produce_format_t(produce | PRODUCE_RESOURCE_FORK);
	}
	else if(subformat == "finf")
	{
		Linker::Debug << "Debug: Requested to generate Finder Info file under .finf" << std::endl;
		produce = produce_format_t(produce | PRODUCE_FINDER_INFO);
	}
	else if(subformat == "double" || subformat == "appledouble")
	{
		Linker::Debug << "Debug: Requested to generate AppleDouble" << std::endl;
		produce = produce_format_t(produce | PRODUCE_APPLE_DOUBLE);
		/* TODO: versions */
	}
	else if(subformat == "mbin" || subformat == "macbin" || subformat == "macbinary")
	{
		Linker::Debug << "Debug: Requested to generate MacBinary" << std::endl;
		produce = produce_format_t(produce | PRODUCE_MAC_BINARY);
		/* TODO: versions */
	}
	else
	{
		return false;
	}
	return true;
}

std::shared_ptr<const ResourceFork> MacDriver::GetResourceFork() const
{
	if(auto pointer = std::get_if<std::shared_ptr<ResourceFork>>(&container))
	{
		return *pointer;
	}
	else
	{
		return nullptr;
	}
}

std::shared_ptr<ResourceFork> MacDriver::GetResourceFork()
{
	return std::const_pointer_cast<ResourceFork>(const_cast<const MacDriver *>(this)->GetResourceFork());
}

std::shared_ptr<const AppleSingleDouble> MacDriver::GetAppleSingleDouble() const
{
	if(auto pointer = std::get_if<std::shared_ptr<AppleSingleDouble>>(&container))
	{
		if(std::dynamic_pointer_cast<AppleSingleDouble>(*pointer) == nullptr)
			return *pointer;
		else
			return nullptr; // this is actually a MacBinary
	}
	else
	{
		return nullptr;
	}
}

std::shared_ptr<AppleSingleDouble> MacDriver::GetAppleSingleDouble()
{
	return std::const_pointer_cast<AppleSingleDouble>(const_cast<const MacDriver *>(this)->GetAppleSingleDouble());
}

std::shared_ptr<const MacBinary> MacDriver::GetMacBinary() const
{
	if(auto pointer = std::get_if<std::shared_ptr<AppleSingleDouble>>(&container))
	{
		return std::dynamic_pointer_cast<MacBinary>(*pointer);
	}
	else
	{
		return nullptr;
	}
}

std::shared_ptr<MacBinary> MacDriver::GetMacBinary()
{
	return std::const_pointer_cast<MacBinary>(const_cast<const MacDriver *>(this)->GetMacBinary());
}

void MacDriver::SetOptions(std::map<std::string, std::string>& options)
{
	this->options = options;
}

std::vector<Linker::OptionDescription<void>> MacDriver::GetMemoryModelNames()
{
	ResourceFork tmp;
	return tmp.GetMemoryModelNames();
}

void MacDriver::SetModel(std::string model)
{
	this->model = model;
}

void MacDriver::SetLinkScript(std::string script_file, std::map<std::string, std::string>& options)
{
	this->script_file = script_file;
	this->script_options = options;
}

void MacDriver::GenerateFile(std::string filename, Linker::Module& module)
{
	if(module.cpu != Linker::Module::M68K)
	{
		Linker::Error << "Error: Format only supports Motorola 68000 binaries" << std::endl;
	}

	std::shared_ptr<AppleSingleDouble> container;
	this->container = container = std::make_shared<AppleSingleDouble>(target == TARGET_APPLE_SINGLE ? AppleSingleDouble::SINGLE : AppleSingleDouble::DOUBLE,
		apple_single_double_version, home_file_system);

	container->SetOptions(options);
	container->SetModel(model);
	container->SetLinkScript(script_file, script_options);

	container->ProcessModule(module);
	container->CalculateValues();

	std::ofstream out;
	Linker::Writer wr(::BigEndian);
	switch(target)
	{
	case TARGET_NONE:
		break;
	case TARGET_DATA_FORK:
		out.open(filename, std::ios_base::out | std::ios_base::binary);
		if(auto entry = container->FindEntry(AppleSingleDouble::ID_DataFork))
		{
			wr.out = &out;
			entry->WriteFile(wr);
		}
		out.close();
		break;
	case TARGET_RESOURCE_FORK:
		out.open(filename, std::ios_base::out | std::ios_base::binary);
		if(auto entry = container->FindEntry(AppleSingleDouble::ID_ResourceFork))
		{
			wr.out = &out;
			entry->WriteFile(wr);
		}
		out.close();
		break;
	case TARGET_APPLE_SINGLE:
		out.open(filename, std::ios_base::out | std::ios_base::binary);
		wr.out = &out;
		WriteFile(wr);
		out.close();
		break;
	case TARGET_MAC_BINARY:
		// TODO: untested
		out.open(filename, std::ios_base::out | std::ios_base::binary);
		wr.out = &out;
		{
			MacBinary macbinary(*container, macbinary_version, macbinary_minimum_version);
			macbinary.generated_file_name = filename;
			macbinary.WriteFile(wr);
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
		if(!std::filesystem::create_directory(path.parent_path(), err) && err != std::errc(0))
		{
			Linker::Error << "Error: Unable to create folder " << path.parent_path() << ", no resource fork file will be generated" << std::endl;
		}
		else
		{
			out.open(path.string(), std::ios_base::out | std::ios_base::binary);
			if(auto entry = container->FindEntry(AppleSingleDouble::ID_ResourceFork))
			{
				wr.out = &out;
				entry->WriteFile(wr);
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
		if(!std::filesystem::create_directory(path.parent_path(), err) && err != std::errc(0))
		{
			Linker::Error << "Error: Unable to create folder " << path.parent_path() << ", no Finder Info file will be generated" << std::endl;
		}
		else
		{
			out.open(path.string(), std::ios_base::out | std::ios_base::binary);
			if(auto entry = container->FindEntry(AppleSingleDouble::ID_FinderInfo))
			{
				wr.out = &out;
				entry->WriteFile(wr);
			}
			out.close();
		}
	}

	if((produce & PRODUCE_APPLE_DOUBLE))
	{
		Linker::Debug << "Debug: Generating AppleDouble" << std::endl;
		std::ofstream out;
		out.open(container->GetUNIXDoubleFilename(filename), std::ios_base::out | std::ios_base::binary);
		wr.out = &out;
		if(target != TARGET_APPLE_SINGLE)
		{
			container->WriteFile(wr);
		}
		else
		{
			AppleSingleDouble apple_double(*container, AppleSingleDouble::DOUBLE);
			apple_double.WriteFile(wr);
		}
		out.close();
	}

	if((produce & PRODUCE_MAC_BINARY))
	{
		Linker::Debug << "Debug: Generating MacBinary" << std::endl;
		std::ofstream out;
		out.open(
			target == TARGET_NONE ? filename : filename + ".mbin",
			std::ios_base::out | std::ios_base::binary);
		wr.out = &out;
		MacBinary macbinary(*container, macbinary_version, macbinary_minimum_version);
		macbinary.generated_file_name = filename;
		macbinary.WriteFile(wr);
		out.close();
	}
}

void MacDriver::ReadFile(Linker::Reader& rd)
{
	switch(target)
	{
	case TARGET_RESOURCE_FORK:
		{
			std::shared_ptr<ResourceFork> container;
			this->container = container = std::make_shared<ResourceFork>();
			container->ReadFile(rd);
		}
		break;
	case TARGET_APPLE_SINGLE:
		{
			std::shared_ptr<AppleSingleDouble> container;
			this->container = container = std::make_shared<AppleSingleDouble>();
			container->ReadFile(rd);
		}
		break;
	case TARGET_MAC_BINARY:
		{
			std::shared_ptr<MacBinary> container;
			container = std::make_shared<MacBinary>();
			this->container = std::static_pointer_cast<AppleSingleDouble>(container);
			container->ReadFile(rd);
		}
		break;
	default:
		if((produce & PRODUCE_APPLE_DOUBLE))
		{
			std::shared_ptr<AppleSingleDouble> container;
			this->container = container = std::make_shared<AppleSingleDouble>();
			container->ReadFile(rd);
		}
		else if((produce & PRODUCE_RESOURCE_FORK))
		{
			std::shared_ptr<ResourceFork> container;
			this->container = container = std::make_shared<ResourceFork>();
			container->ReadFile(rd);
		}
		else if((produce & PRODUCE_MAC_BINARY))
		{
			std::shared_ptr<MacBinary> container;
			container = std::make_shared<MacBinary>();
			this->container = std::static_pointer_cast<AppleSingleDouble>(container);
			container->ReadFile(rd);
		}
		else
		{
			Linker::FatalError("Fatal error: Reading the specified format is not supported");
		}
		break;
	}
}

offset_t MacDriver::WriteFile(Linker::Writer& wr) const
{
	if(auto resource_fork = GetResourceFork())
	{
		return resource_fork->WriteFile(wr);
	}
	else if(auto apple_single_double = GetAppleSingleDouble())
	{
		return apple_single_double->WriteFile(wr);
	}
	else if(auto mac_binary = GetMacBinary())
	{
		return mac_binary->WriteFile(wr);
	}
	else
	{
		Linker::FatalError("Internal error: file not loaded");
	}
}

void MacDriver::Dump(Dumper::Dumper& dump) const
{
	if(auto resource_fork = GetResourceFork())
	{
		resource_fork->Dump(dump);
	}
	else if(auto apple_single_double = GetAppleSingleDouble())
	{
		apple_single_double->Dump(dump);
	}
	else if(auto mac_binary = GetMacBinary())
	{
		mac_binary->Dump(dump);
	}
	else
	{
		Linker::FatalError("Internal error: file not loaded");
	}
}

