
#include <cstring>
#include "macos.h"
#include "../linker/position.h"
#include "../linker/resolution.h"
#include "../linker/section.h"

using namespace Apple;

/* AppleSingle/AppleDouble */

void AppleSingleDouble::ReadFile(Linker::Reader& rd)
{
	/* TODO */
}

bool AppleSingleDouble::FormatSupportsResources() const
{
	return true;
}

void AppleSingleDouble::Entry::ReadFile(Linker::Reader& rd)
{
	/* TODO */
}

offset_t AppleSingleDouble::Entry::GetLength()
{
	return 0;
}

offset_t AppleSingleDouble::Entry::WriteFile(Linker::Writer& out)
{
	/* TODO */
	return offset_t(-1);
}

void AppleSingleDouble::Entry::Dump(Dumper::Dumper& dump)
{
	// TODO
}

void AppleSingleDouble::Entry::ProcessModule(Linker::Module& module)
{
}

void AppleSingleDouble::Entry::CalculateValues()
{
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

void AppleSingleDouble::SetModel(std::string model)
{
	std::dynamic_pointer_cast<ResourceFork>(GetResourceFork())->SetModel(model);
}

void AppleSingleDouble::SetLinkScript(std::string script_file, std::map<std::string, std::string>& options)
{
	std::dynamic_pointer_cast<ResourceFork>(GetResourceFork())->SetLinkScript(script_file, options);
}

std::shared_ptr<AppleSingleDouble::Entry> AppleSingleDouble::FindEntry(uint32_t id)
{
	for(auto entry : entries)
	{
		if(entry->id == id)
			return entry;
	}
	return nullptr;
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
	std::shared_ptr<Entry> entry = GetMacintoshFileInfo();
	if(entry == nullptr)
		return 0;
	switch(version)
	{
	case 1:
		return std::dynamic_pointer_cast<FileInfo::Macintosh>(entry)->Attributes;
	case 2:
		return std::dynamic_pointer_cast<MacintoshFileInfo>(entry)->Attributes;
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
	for(auto entry : entries)
	{
		entry->CalculateValues();
	}
}

offset_t AppleSingleDouble::WriteFile(Linker::Writer& wr)
{
	wr.endiantype = ::BigEndian;
	switch(type)
	{
	case SINGLE:
		wr.WriteWord(4, 0x00051600);
		break;
	case DOUBLE:
		wr.WriteWord(4, 0x00051607);
		break;
	default:
		Linker::FatalError("Internal error: invalid AppleSingle/AppleDouble type");
	}
	wr.WriteWord(4, version << 16);
	switch(home_file_system)
	{
	default:
		wr.WriteData(16, TXT_undefined);
		break;
	case HFS_Macintosh:
		wr.WriteData(16, TXT_Macintosh);
		break;
	case HFS_ProDOS:
		wr.WriteData(16, TXT_ProDOS);
		break;
	case HFS_MSDOS:
		wr.WriteData(16, TXT_MS_DOS);
		break;
	case HFS_UNIX:
		wr.WriteData(16, TXT_UNIX);
		break;
	case HFS_VAX_VMS:
		wr.WriteData(16, TXT_VAX_VMS);
		break;
	}
	wr.WriteWord(2, entries.size());
	uint32_t entry_offset = 26 + 12 * entries.size();
	for(auto entry : entries)
	{
		wr.WriteWord(4, entry->id);
		wr.WriteWord(4, entry_offset);
		wr.WriteWord(4, entry->GetLength());
		entry_offset += entry->GetLength();
	}
	for(auto entry : entries)
	{
		entry->WriteFile(wr);
	}

	return offset_t(-1);
}

void AppleSingleDouble::Dump(Dumper::Dumper& dump)
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("AppleSingle/AppleDouble format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 8);
	file_region.Display(dump);

	// TODO
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

std::string AppleSingleDouble::GetDefaultExtension(Linker::Module& module)
{
	return "a.out";
}

void ResourceFork::SetOptions(std::map<std::string, std::string>& options)
{
	/* TODO */
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

void ResourceFork::Resource::ReadFile(Linker::Reader& rd)
{
	/* TODO */
}

offset_t ResourceFork::Resource::WriteFile(Linker::Writer& wr)
{
	/* TODO */

	return offset_t(-1);
}

void ResourceFork::Resource::Dump(Dumper::Dumper& dump)
{
	// TODO
}

void ResourceFork::GenericResource::ProcessModule(Linker::Module& module)
{
}

void ResourceFork::GenericResource::CalculateValues()
{
}

offset_t ResourceFork::GenericResource::GetLength()
{
	return resource->data_size;
}

offset_t ResourceFork::GenericResource::WriteFile(Linker::Writer& wr)
{
	resource->WriteFile(wr);

	return offset_t(-1);
}

void ResourceFork::GenericResource::Dump(Dumper::Dumper& dump)
{
	// TODO
}

void ResourceFork::JumpTableCodeResource::ProcessModule(Linker::Module& module)
{
}

void ResourceFork::JumpTableCodeResource::CalculateValues()
{
	if(far_entries.size() == 0)
	{
		above_a5 = 0x30 + 8 * near_entries.size();
	}
	else
	{
		above_a5 = 0x30 + 8 + 8 * (near_entries.size() + far_entries.size());
	}
}

offset_t ResourceFork::JumpTableCodeResource::GetLength()
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

offset_t ResourceFork::JumpTableCodeResource::WriteFile(Linker::Writer& wr)
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
	wr.WriteWord(4, 32);
	for(Entry& entry : near_entries)
	{
		wr.WriteWord(2, entry.offset);
		wr.WriteWord(2, MOVE_DATA_SP);
		wr.WriteWord(2, entry.segment);
		wr.WriteWord(2, LOADSEG);
	}
	if(far_entries.size() != 0)
	{
		wr.WriteData(8, "\0\0\xFF\xFF\0\0\0\0");
		for(Entry& entry : far_entries)
		{
			wr.WriteWord(2, entry.segment);
			wr.WriteWord(2, LOADSEG);
			wr.WriteWord(4, entry.offset);
		}
	}

	return offset_t(-1);
}

void ResourceFork::JumpTableCodeResource::Dump(Dumper::Dumper& dump)
{
	// TODO
}

void ResourceFork::CodeResource::ProcessModule(Linker::Module& module)
{
}

void ResourceFork::CodeResource::CalculateValues()
{
	if(is_far)
	{
		a5_relocation_offset = 0x28 + image->data_size;
		segment_relocation_offset = a5_relocation_offset + MeasureRelocations(a5_relocations);
		resource_size = segment_relocation_offset + MeasureRelocations(segment_relocations);
	}
}

offset_t ResourceFork::CodeResource::GetLength()
{
	if(!is_far)
	{
//		return 4 + image->data_size;
		return 4 + image->TotalSize();
	}
	else
	{
		return resource_size;
	}
}

uint32_t ResourceFork::CodeResource::MeasureRelocations(std::set<uint32_t>& relocations)
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

void ResourceFork::CodeResource::WriteRelocations(Linker::Writer& wr, std::set<uint32_t>& relocations)
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

offset_t ResourceFork::CodeResource::WriteFile(Linker::Writer& wr)
{
	if(!is_far)
	{
		wr.WriteWord(2, first_near_entry_offset);
		wr.WriteWord(2, near_entries.size());
		image->WriteFile(wr);
		wr.Skip(image->zero_fill);
	}
	else
	{
		wr.WriteData(4, "\xFF\xFF\0\0");
		wr.WriteWord(4, first_near_entry_offset);
		wr.WriteWord(4, near_entries.size());
		wr.WriteWord(4, first_far_entry_offset);
		wr.WriteWord(4, far_entries.size());
		wr.WriteWord(4, a5_relocation_offset);
		wr.WriteWord(4, a5_address);
		wr.WriteWord(4, segment_relocation_offset);
		wr.WriteWord(4, base_address);
		wr.WriteWord(4, 0);
		image->WriteFile(wr);
		WriteRelocations(wr, a5_relocations);
		WriteRelocations(wr, segment_relocations);
	}

	return offset_t(-1);
}

void ResourceFork::CodeResource::Dump(Dumper::Dumper& dump)
{
	// TODO
}

void ResourceFork::AddResource(std::shared_ptr<Resource> resource)
{
	uint32_t typeval =
		(uint32_t(uint8_t(resource->type[0])) << 24)
		| (uint32_t(uint8_t(resource->type[1])) << 16)
		| (uint32_t(uint8_t(resource->type[2])) << 8)
		| uint32_t(uint8_t(resource->type[3]));
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
		segments[codeN->image] = codeN;
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
		rsrc->resource = segment;
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
std::cout << SimpleScript << std::endl;
			return Script::parse_string(SimpleScript);
		case MODEL_TINY:
std::cout << TinyScript << std::endl;
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
	for(auto it : resources)
	{
		reference_list_offsets[it.first] = reference_list_offset;
		reference_list_offset += 12 * it.second.size();
		name_list_offset += 12 * it.second.size();
		for(auto it2 : it.second)
		{
			std::shared_ptr<Resource> resource = it2.second;
			resource->data_offset = data_length;
			resource->CalculateValues();
			data_length += 4 + resource->GetLength();
			if(resource->name)
			{
				resource->name_offset = name_list_length;
				name_list_length += 1 + resource->name->size();
			}
			else
			{
				resource->name_offset = 0xFFFF;
			}
		}
	}
	name_list_offset = 28 + reference_list_offset;
	if(data_offset < 16)
		data_offset = 0x0100;
	map_offset = data_offset + data_length;
	map_length = name_list_offset + name_list_length;
}

offset_t ResourceFork::GetLength()
{
	return map_offset + map_length;
}

offset_t ResourceFork::WriteFile(Linker::Writer& wr)
{
	wr.endiantype = ::BigEndian; /* in case we write the resource fork directly, without an AppleSingle/AppleDouble wrapper */
	wr.WriteWord(4, data_offset);
	wr.WriteWord(4, map_offset);
	wr.WriteWord(4, data_length);
	wr.WriteWord(4, map_length);
	wr.Skip(data_offset - 16);
	for(auto it : resources)
	{
		for(auto it2 : it.second)
		{
			std::shared_ptr<Resource> resource = it2.second;
			wr.WriteWord(4, resource->GetLength());
			resource->WriteFile(wr);
		}
	}
	/* map start */
	wr.Skip(22);
	wr.WriteWord(2, attributes);
	wr.WriteWord(2, 28); /* offset of resource type list from map start */
	wr.WriteWord(2, name_list_offset);
	wr.WriteWord(2, resources.size() - 1);
	/* type list */
	for(auto it : resources)
	{
		wr.WriteWord(4, it.first); /* resource type */
		wr.WriteWord(2, it.second.size() - 1);
		wr.WriteWord(2, reference_list_offsets[it.first]);
	}
	/* reference list */
	for(auto it : resources)
	{
		for(auto it2 : it.second)
		{
			std::shared_ptr<Resource> resource = it2.second;
			wr.WriteWord(2, it2.first);
			wr.WriteWord(2, resource->name_offset);
			wr.WriteWord(4, (resource->data_offset & 0x00FFFFFF) | (resource->attributes << 24));
			wr.Skip(4);
		}
	}
	/* name list */
	for(auto it : resources)
	{
		for(auto it2 : it.second)
		{
			std::shared_ptr<Resource> resource = it2.second;
			if(resource->name)
			{
				wr.WriteWord(1, resource->name->size());
				wr.WriteData(resource->name->size(), resource->name->c_str());
			}
		}
	}

	return offset_t(-1);
}

void ResourceFork::Dump(Dumper::Dumper& dump)
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("Macintosh resource fork format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 8);
	file_region.Display(dump);

	// TODO
}

void ResourceFork::GenerateFile(std::string filename, Linker::Module& module)
{
	if(module.cpu != Linker::Module::M68K)
	{
		Linker::Error << "Error: Format only supports Motorola 68000 binaries" << std::endl;
	}

	Linker::OutputFormat::GenerateFile(filename, module);
}

std::string ResourceFork::GetDefaultExtension(Linker::Module& module)
{
	return "a.out";
}

offset_t RealName::GetLength()
{
	return name.size();
}

offset_t RealName::WriteFile(Linker::Writer& wr)
{
	wr.endiantype = ::BigEndian;
	wr.WriteData(name.size(), name.c_str());

	return offset_t(-1);
}

void RealName::Dump(Dumper::Dumper& dump)
{
	// TODO
}

offset_t FileInfo::Macintosh::GetLength()
{
	return 16;
}

offset_t FileInfo::Macintosh::WriteFile(Linker::Writer& wr)
{
	wr.endiantype = ::BigEndian;
	wr.WriteWord(4, CreationDate);
	wr.WriteWord(4, ModificationDate);
	wr.WriteWord(4, LastBackupDate);
	wr.WriteWord(4, Attributes);

	return offset_t(-1);
}

void FileInfo::Macintosh::Dump(Dumper::Dumper& dump)
{
	// TODO
}

offset_t FileInfo::ProDOS::GetLength()
{
	return 16;
}

offset_t FileInfo::ProDOS::WriteFile(Linker::Writer& wr)
{
	wr.endiantype = ::BigEndian;
	wr.WriteWord(4, CreationDate);
	wr.WriteWord(4, ModificationDate);
	wr.WriteWord(2, Access);
	wr.WriteWord(2, FileType);
	wr.WriteWord(4, AUXType);

	return offset_t(-1);
}

void FileInfo::ProDOS::Dump(Dumper::Dumper& dump)
{
	// TODO
}

offset_t FileInfo::MSDOS::GetLength()
{
	return 6;
}

offset_t FileInfo::MSDOS::WriteFile(Linker::Writer& wr)
{
	wr.endiantype = ::BigEndian;
	wr.WriteWord(4, ModificationDate);
	wr.WriteWord(2, Attributes);

	return offset_t(-1);
}

void FileInfo::MSDOS::Dump(Dumper::Dumper& dump)
{
	// TODO
}

offset_t FileInfo::AUX::GetLength()
{
	return 12;
}

offset_t FileInfo::AUX::WriteFile(Linker::Writer& wr)
{
	wr.endiantype = ::BigEndian;
	wr.WriteWord(4, CreationDate);
	wr.WriteWord(4, AccessDate);
	wr.WriteWord(4, ModificationDate);

	return offset_t(-1);
}

void FileInfo::AUX::Dump(Dumper::Dumper& dump)
{
	// TODO
}

offset_t FileDatesInfo::GetLength()
{
	return 16;
}

offset_t FileDatesInfo::WriteFile(Linker::Writer& wr)
{
	wr.endiantype = ::BigEndian;
	wr.WriteWord(4, CreationDate);
	wr.WriteWord(4, ModificationDate);
	wr.WriteWord(4, BackupDate);
	wr.WriteWord(4, AccessDate);

	return offset_t(-1);
}

void FileDatesInfo::Dump(Dumper::Dumper& dump)
{
	// TODO
}

offset_t FinderInfo::GetLength()
{
	return 32;
}

offset_t FinderInfo::WriteFile(Linker::Writer& wr)
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

void FinderInfo::Dump(Dumper::Dumper& dump)
{
	// TODO
}

void FinderInfo::ProcessModule(Linker::Module& module)
{
	memcpy(Type, "APPL", 4);
	memcpy(Creator, "????", 4);
}

offset_t MacintoshFileInfo::GetLength()
{
	return 4;
}

offset_t MacintoshFileInfo::WriteFile(Linker::Writer& wr)
{
	wr.endiantype = ::BigEndian;
	wr.WriteWord(4, Attributes);

	return offset_t(-1);
}

void MacintoshFileInfo::Dump(Dumper::Dumper& dump)
{
	// TODO
}

offset_t ProDOSFileInfo::GetLength()
{
	return 8;
}

offset_t ProDOSFileInfo::WriteFile(Linker::Writer& wr)
{
	wr.endiantype = ::BigEndian;
	wr.WriteWord(2, Access);
	wr.WriteWord(2, FileType);
	wr.WriteWord(4, AUXType);

	return offset_t(-1);
}

void ProDOSFileInfo::Dump(Dumper::Dumper& dump)
{
	// TODO
}

offset_t MSDOSFileInfo::GetLength()
{
	return 2;
}

offset_t MSDOSFileInfo::WriteFile(Linker::Writer& wr)
{
	wr.endiantype = ::BigEndian;
	wr.WriteWord(2, Attributes);

	return offset_t(-1);
}

void MSDOSFileInfo::Dump(Dumper::Dumper& dump)
{
	// TODO
}

uint16_t MacBinary::crc_step[256];

void MacBinary::CRC_Initialize()
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

void MacBinary::CRC_Step(uint8_t byte)
{
	crc = (crc << 8) ^ crc_step[(crc >> 8) ^ byte];
}

void MacBinary::Skip(Linker::Writer& wr, size_t count)
{
	for(size_t i = 0; i < count; i++)
	{
		CRC_Step(0);
	}
	wr.Skip(count);
}

void MacBinary::WriteData(Linker::Writer& wr, size_t count, const void * data)
{
	for(size_t i = 0; i < count; i++)
	{
		CRC_Step(static_cast<const char *>(data)[i]);
	}
	wr.WriteData(count, data);
}

void MacBinary::WriteData(Linker::Writer& wr, size_t count, std::string text)
{
	for(size_t i = 0; i < count; i++)
	{
		CRC_Step(i < text.size() ? text[i] : 0);
	}
	wr.WriteData(count, text);
}

void MacBinary::WriteWord(Linker::Writer& wr, size_t bytes, uint64_t value)
{
	uint8_t data[bytes];
	::WriteWord(bytes, bytes, data, value, EndianType::BigEndian);
	WriteData(wr, bytes, data);
}

void MacBinary::WriteHeader(Linker::Writer& wr)
{
	CRC_Initialize();
	WriteWord(wr, 1, 0);
	if(auto entry = FindEntry(ID_RealName))
	{
		std::string& name = std::dynamic_pointer_cast<RealName>(entry)->name;
		WriteWord(wr, 1, name.size() > 63 ? 63 : name.size());
		WriteData(wr, 63, name);
	}
	else
	{
		WriteWord(wr, 1, generated_file_name.size() > 63 ? 63 : generated_file_name.size());
		WriteData(wr, 63, generated_file_name);
	}
	std::shared_ptr<FinderInfo> info = nullptr;
	if(auto entry = FindEntry(ID_FinderInfo))
	{
		info = std::dynamic_pointer_cast<FinderInfo>(entry);
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
	WriteWord(wr, 1, GetMacintoshAttributes());
	WriteWord(wr, 1, 0);
	if(auto entry = FindEntry(ID_DataFork))
	{
		WriteWord(wr, 4, entry->GetLength());
	}
	else
	{
		WriteWord(wr, 4, 0);
	}
	if(auto entry = FindEntry(ID_ResourceFork))
	{
		WriteWord(wr, 4, entry->GetLength());
	}
	else
	{
		WriteWord(wr, 4, 0);
	}
	WriteWord(wr, 4, GetCreationDate());
	WriteWord(wr, 4, GetModificationDate());
	if(version < MACBIN1_GETINFO)
	{
		return;
	}
	if(auto entry = FindEntry(ID_Comment))
	{
		WriteWord(wr, 2, entry->GetLength());
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

offset_t MacBinary::WriteFile(Linker::Writer& wr)
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

void MacBinary::Dump(Dumper::Dumper& dump)
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

void MacDriver::SetOptions(std::map<std::string, std::string>& options)
{
	this->options = options;
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

	container = std::make_shared<AppleSingleDouble>(target == TARGET_APPLE_SINGLE ? AppleSingleDouble::SINGLE : AppleSingleDouble::DOUBLE,
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
	Linker::FatalError("Fatal error: Reading the Apple output driver is not supported");
}

offset_t MacDriver::WriteFile(Linker::Writer& wr)
{
	Linker::FatalError("Fatal error: Writing the Apple output driver is not supported");
}

