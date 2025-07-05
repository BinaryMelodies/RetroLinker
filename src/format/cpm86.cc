
#include <algorithm>
#include <filesystem>
#include "cpm86.h"
#include "../linker/buffer.h"
#include "../linker/section.h"
#include "../linker/position.h"
#include "../linker/reader.h"
#include "../linker/resolution.h"

using namespace DigitalResearch;

void CPM86Format::Descriptor::Clear()
{
	image = nullptr;
}

uint16_t CPM86Format::Descriptor::GetSizeParas(const CPM86Format& module) const
{
	if(type == ActualFixups)
	{
		return module.GetRelocationSizeParas();
	}
	uint32_t size = image ? image->ImageSize() : 0;
	uint16_t zero_page_extra = attach_zero_page ? 0x10 : 0;
	return ((size + 0xF) >> 4) + zero_page_extra;
}

void CPM86Format::Descriptor::ReadDescriptor(Linker::Reader& rd)
{
	uint8_t type_byte = rd.ReadUnsigned(1);
	type = group_type(type_byte);
	size_paras = rd.ReadUnsigned(2);
	load_segment = rd.ReadUnsigned(2);
	min_size_paras = rd.ReadUnsigned(2);
	max_size_paras = rd.ReadUnsigned(2);
}

void CPM86Format::Descriptor::Prepare(CPM86Format& module)
{
	if(type == ActualFixups)
	{
		min_size_paras = max_size_paras = size_paras;
	}
}

void CPM86Format::Descriptor::WriteDescriptor(Linker::Writer& wr, const CPM86Format& module) const
{
	wr.WriteWord(1, type & 0xFF);
	wr.WriteWord(2, size_paras);
	wr.WriteWord(2, load_segment);
	wr.WriteWord(2, min_size_paras);
	wr.WriteWord(2, max_size_paras);
}

void CPM86Format::Descriptor::WriteData(Linker::Writer& wr, const CPM86Format& module) const
{
	if(type == Undefined || type == ActualFixups || GetSizeParas(module) == 0)
		return;
	wr.Seek(attach_zero_page ? offset + 0x100 : offset);
	if(image)
	{
		image->WriteFile(wr);
	}
}

std::string CPM86Format::Descriptor::GetDefaultName() const
{
	switch(type & 0xF)
	{
	case 1:
	case 9:
		return ".text";
	case 2:
		return ".data";
	case 3:
		return ".extra";
	case 4:
		return ".stack";
	case 5:
		return ".aux1";
	case 6:
		return ".aux2";
	case 7:
		return ".aux3";
	case 8:
		return ".aux4";
	case 0:
		return ".unknown0";
	case 10:
		return ".unknown10";
	case 11:
		return ".unknown11";
	case 12:
		return ".unknown12";
	case 13:
		return ".unknown13";
	case 14:
		return ".unknown14";
	case 15:
		return ".unknown15"; /* supposed to be escaped */
	default:
		assert(false);
	}
}

void CPM86Format::Descriptor::ReadData(Linker::Reader& rd, const CPM86Format& module)
{
	if(type == Undefined || type == ActualFixups || size_paras == 0)
		return;
	std::shared_ptr<Linker::Buffer> buffer = std::make_shared<Linker::Section>(GetDefaultName());
	image = buffer;
	buffer->ReadFile(rd, uint32_t(size_paras) << 4);
}

bool CPM86Format::relocation_source::operator==(const relocation_source& other) const
{
	return segment == other.segment && offset < other.offset;
}

bool CPM86Format::relocation_source::operator<(const relocation_source& other) const
{
	return segment < other.segment || (segment == other.segment && offset < other.offset);
}

CPM86Format::Relocation::operator bool() const
{
	return source != 0 || target != 0;
}

void CPM86Format::Relocation::Read(Linker::Reader& rd, CPM86Format& module, bool is_library)
{
	uint8_t segments = rd.ReadUnsigned(1);
	if(segments == 0)
		return;
	if(is_library && (segments & 0xF) != 0)
	{
		Linker::Error << "Error: library relocation target must be 0, not " << (segments & 0xF) << std::endl;
	}
	if(!is_library)
		module.CheckValidSegmentGroup(segments & 0xF);
	module.CheckValidSegmentGroup(segments >> 4);
	source = segments >> 4;
	target = segments & 0xF;
	paragraph = rd.ReadUnsigned(2);
	offset = rd.ReadUnsigned(1);
	return;
}

void CPM86Format::Relocation::Write(Linker::Writer& wr) const
{
	wr.WriteWord(1, (source << 4) | target);
	wr.WriteWord(2, paragraph);
	wr.WriteWord(1, offset);
}

CPM86Format::relocation_source CPM86Format::Relocation::GetSource() const
{
	return relocation_source { source, (uint32_t(paragraph) << 4) + offset };
}

void CPM86Format::rsx_record::Clear()
{
	contents = nullptr;
}

void CPM86Format::rsx_record::Read(Linker::Reader& rd)
{
	offset_record = rd.ReadUnsigned(2);
	name = rd.ReadData(8, true);
	rd.Skip(6);
}

void CPM86Format::rsx_record::ReadModule(Linker::Reader& rd)
{
	if(offset_record == RSX_TERMINATE || offset_record == RSX_DYNAMIC)
		return;

	std::shared_ptr<CPM86Format> module = std::make_shared<CPM86Format>();
	module->file_offset = offset_record << 7;
	rd.Seek(module->file_offset);
	module->ReadFile(rd);
	contents = module;
}

void CPM86Format::rsx_record::Write(Linker::Writer& wr) const
{
	wr.WriteWord(2, offset_record);
	wr.WriteData(8, name, '\0');
	wr.Skip(6);
}

void CPM86Format::rsx_record::WriteModule(Linker::Writer& wr) const
{
	if(offset_record == RSX_TERMINATE || offset_record == RSX_DYNAMIC)
		return;
	wr.Seek(offset_record << 7);
	contents->WriteFile(wr);
}

offset_t CPM86Format::rsx_record::GetFullFileSize() const
{
	return contents->ImageSize();
}

void CPM86Format::rsx_record::SetOffset(offset_t new_offset)
{
	offset_record = new_offset >> 7;
	if(const std::shared_ptr<CPM86Format> module = std::dynamic_pointer_cast<CPM86Format>(contents))
	{
		module->file_offset = offset_record << 7;
	}
}

void CPM86Format::rsx_record::Dump(Dumper::Dumper& dump) const
{
	if(const std::shared_ptr<CPM86Format> module = std::dynamic_pointer_cast<CPM86Format>(contents))
	{
		module->Dump(dump);
	}
	else
	{
		Dumper::Block block("Image", uint32_t(offset_record) << 7, contents->AsImage(), contents->ImageSize(), 6);
		block.Display(dump);
	}
}

CPM86Format::library_id CPM86Format::library_id::ParseNameAndVersion(std::string name_and_version)
{
	// FILENAME[.MAJ[.MIN]]
	// FILENAME.MAJ.MIN.FLAGS

	std::string filename;
	uint16_t major_version = 0;
	uint16_t minor_version = 0;

	size_t dot1 = name_and_version.find('.');
	if(dot1 == std::string::npos)
	{
		filename = name_and_version;
	}
	else
	{
		filename = name_and_version.substr(0, dot1);

		dot1++;
		size_t dot2 = name_and_version.find('.', dot1);
		if(dot2 == std::string::npos)
		{
			major_version = stoll(name_and_version.substr(dot1), nullptr, 10);
		}
		else
		{
			major_version = stoll(name_and_version.substr(dot1, dot2 - dot1), nullptr, 10);

			dot2++;
			size_t dot3 = name_and_version.find('.', dot2);
			if(dot3 != std::string::npos)
			{
				minor_version = stoll(name_and_version.substr(dot2, dot3 - dot2), nullptr, 10);
				uint32_t flags = stoll(name_and_version.substr(dot3 + 1), nullptr, 16);
				return library_id(filename, major_version, minor_version, flags);
			}
			else
			{
				minor_version = stoll(name_and_version.substr(dot2), nullptr, 10);
			}
		}
	}

	return library_id(filename, major_version, minor_version);
}

bool CPM86Format::library_id::operator ==(const library_id& other) const
{
	return name == other.name && major_version == other.major_version && minor_version == other.minor_version && flags == other.flags;
}

void CPM86Format::library_id::Write(Linker::Writer& wr) const
{
	wr.WriteData(8, name, '\0');
	wr.WriteWord(2, major_version);
	wr.WriteWord(2, minor_version);
	wr.WriteWord(4, flags);
}

void CPM86Format::library_id::Read(Linker::Reader& rd)
{
	name = rd.ReadData(8, true);
	major_version = rd.ReadUnsigned(2);
	minor_version = rd.ReadUnsigned(2);
	flags = rd.ReadUnsigned(4);
}

void CPM86Format::library::Write(Linker::Writer& wr) const
{
	library_id::Write(wr);
	wr.WriteWord(2, relocation_count);
}

void CPM86Format::library::WriteExtended(Linker::Writer& wr) const
{
	Write(wr);
	wr.WriteWord(2, first_selector);
	wr.WriteWord(2, unknown);
}

void CPM86Format::library::Read(Linker::Reader& rd)
{
	library_id::Read(rd);
	relocation_count = rd.ReadUnsigned(2);
}

void CPM86Format::library::ReadExtended(Linker::Reader& rd)
{
	Read(rd);
	first_selector = rd.ReadUnsigned(2);
	unknown = rd.ReadUnsigned(2);
}

void CPM86Format::LibraryDescriptor::Clear()
{
	Descriptor::Clear();
	libraries.clear();
}

void CPM86Format::LibraryDescriptor::Prepare(CPM86Format& module)
{
	for(auto& lib : libraries)
	{
		lib.relocation_count = lib.relocations.size();
	}
}

CPM86Format::library& CPM86Format::LibraryDescriptor::FetchImportedLibrary(std::string name_dot_version)
{
	return FetchImportedLibrary(library_id::ParseNameAndVersion(name_dot_version));
}

CPM86Format::library& CPM86Format::LibraryDescriptor::FetchImportedLibrary(library_id id)
{
	auto lib = std::find(libraries.begin(), libraries.end(), id);
	if(lib == libraries.end())
	{
		libraries.push_back(library(id));
		return libraries.back();
	}
	else
	{
		return *lib;
	}
}

uint16_t CPM86Format::LibraryDescriptor::GetSizeParas(const CPM86Format& module) const
{
	if(module.IsFastLoadFormat())
		return ::AlignTo(2 + 0x16 * libraries.size(), 0x10) >> 4;
	else
		return ::AlignTo(2 + 0x12 * libraries.size(), 0x10) >> 4;
}

void CPM86Format::LibraryDescriptor::WriteData(Linker::Writer& wr, const CPM86Format& module) const
{
	if(type == Undefined)
		return;
	wr.Seek(offset);
	wr.WriteWord(2, libraries.size());
	for(auto& lib : libraries)
	{
		if(module.IsFastLoadFormat())
			lib.WriteExtended(wr);
		else
			lib.Write(wr);
	}
	wr.AlignTo(0x10);
}

void CPM86Format::LibraryDescriptor::ReadData(Linker::Reader& rd, const CPM86Format& module)
{
	rd.Seek(offset);
	uint16_t count = rd.ReadUnsigned(2);
	int srtl_entry_size = module.IsFastLoadFormat() ? 22 : 18;
	if(2 + count * srtl_entry_size > (size_paras << 16))
	{
		Linker::Error << "Error: Actual STRL group is too short, reading all libraries anyway" << std::endl;
	}
	else if(::AlignTo(2 + count * srtl_entry_size, 16) < (uint32_t(size_paras) << 4))
	{
		Linker::Warning << "Warning: Actual STRL group is too long, ignoring extra parts" << std::endl;
	}
	for(int i = 0; i < count; i++)
	{
		library lib;
		if(module.IsFastLoadFormat())
			lib.ReadExtended(rd);
		else
			lib.Read(rd);
		libraries.push_back(lib);
	}
	rd.Seek(offset + (uint32_t(size_paras) << 4));
}

void CPM86Format::FastLoadDescriptor::ldt_descriptor::Read(Linker::Reader& rd)
{
	limit = rd.ReadUnsigned(2);
	address = rd.ReadUnsigned(3);
	group = rd.ReadUnsigned(1);
	reserved = rd.ReadUnsigned(2);
}

void CPM86Format::FastLoadDescriptor::ldt_descriptor::Write(Linker::Writer& wr) const
{
	wr.WriteWord(2, limit);
	wr.WriteWord(3, address);
	wr.WriteWord(1, group);
	wr.WriteWord(2, reserved);
}

void CPM86Format::FastLoadDescriptor::Clear()
{
	Descriptor::Clear();
	ldt.clear();
}

uint16_t CPM86Format::FastLoadDescriptor::GetSizeParas(const CPM86Format& module) const
{
	return ::AlignTo(8 + 8 * ldt.size(), 0x10) >> 4;
}

void CPM86Format::FastLoadDescriptor::WriteData(Linker::Writer& wr, const CPM86Format& module) const
{
	wr.WriteWord(2, maximum_entries);
	wr.WriteWord(2, first_free_entry);
	wr.WriteWord(2, index_base);
	wr.WriteWord(2, first_used_index);

	for(auto desc : ldt)
		desc.Write(wr);
}

void CPM86Format::FastLoadDescriptor::ReadData(Linker::Reader& rd, const CPM86Format& module)
{
	maximum_entries = rd.ReadUnsigned(2);
	first_free_entry = rd.ReadUnsigned(2);
	index_base = rd.ReadUnsigned(2);
	first_used_index = rd.ReadUnsigned(2);

	for(size_t i = 8; i < uint32_t(size_paras) << 4; i += 8)
	{
		ldt_descriptor desc;
		desc.Read(rd);
		ldt.push_back(desc);
	}
}

void CPM86Format::Clear()
{
	/* format fields */
	for(int i = 0; i < 8; i++)
	{
		descriptors[i].Clear();
	}
	library_descriptor.Clear();
	fastload_descriptor.Clear();
	relocations.clear();
	for(int i = 0; i < 8; i++)
	{
		rsx_table[i].Clear();
	}
	/* writer fields */
	segment_types.clear();
}

uint16_t CPM86Format::GetRelocationSizeParas() const
{
	uint16_t count = relocations.size() + 1;
	for(auto& library : library_descriptor.libraries)
	{
		count += library.relocations.size() + 1;
	}
	return (count * 4 + 0xF) >> 4;
}

size_t CPM86Format::CountValidGroups()
{
	for(int i = 0; i < 8; i++)
	{
		if(descriptors[i].type == Descriptor::Undefined)
			return i;
	}
	return 8;
}

number_t CPM86Format::FindSegmentGroup(unsigned group) const
{
	for(size_t i = 0; i < 8; i++)
	{
		if(descriptors[i].type == Descriptor::Undefined)
			break;
		if((descriptors[i].type & 0xFF) == group)
			return i;
		if(descriptors[i].type == Descriptor::SharedCode && group == 1)
			return i;
	}
	return size_t(-1);
}

void CPM86Format::CheckValidSegmentGroup(unsigned group)
{
	if(FindSegmentGroup(group) == size_t(-1))
		Linker::Error << "Error: invalid group " << group << std::endl;
}

bool CPM86Format::IsFastLoadFormat() const
{
	return fastload_descriptor.type == Descriptor::FastLoad;
}

bool CPM86Format::IsSharedRunTimeLibrary() const
{
	return lib_id.name != "";
}

void CPM86Format::ReadRelocations(Linker::Reader& rd)
{
	rd.Seek(file_offset + relocations_offset);
	while(true)
	{
		Relocation rel;
		rel.Read(rd, *this);
		if(!rel)
			break;
		relocations.push_back(rel);
	}
	rd.Skip(3);
	for(auto& library : library_descriptor.libraries)
	{
		for(int i = 0; i < library.relocation_count; i++)
		{
			Relocation rel;
			rel.Read(rd, *this, true);
			if(!rel)
				break;
			library.relocations.push_back(rel);
		}
		rd.Skip(4);
	}
}

void CPM86Format::WriteRelocations(Linker::Writer& wr) const
{
	wr.Seek(file_offset + relocations_offset);
	for(auto rel : relocations)
	{
		rel.Write(wr);
	}
	/* terminate list */
	wr.WriteWord(4, 0);
	for(auto& library : library_descriptor.libraries)
	{
		for(auto relocation : library.relocations)
		{
			relocation.Write(wr);
		}
		/* terminate list */
		wr.WriteWord(4, 0);
	}
	/* align tail */
	wr.AlignTo(0x80);
}

offset_t CPM86Format::MeasureRelocations()
{
	offset_t size = relocations.size() * 4 + 4;
	for(auto& library : library_descriptor.libraries)
	{
		size += library.relocations.size() * 4 + 4;
	}
	return size;
}

void CPM86Format::ReadFile(Linker::Reader& rd)
{
	Clear();

	rd.endiantype = ::LittleEndian;
	file_offset = rd.Tell();
	for(size_t i = 0; i < 8; i++)
	{
		descriptors[i].ReadDescriptor(rd);
		if(descriptors[i].type == Descriptor::Undefined)
			break;
	}

	rd.Seek(file_offset + 0x48);
	library_descriptor.ReadDescriptor(rd);
	if(library_descriptor.type != Descriptor::Libraries)
		library_descriptor.type = Descriptor::Undefined;

	rd.Seek(file_offset + 0x51);
	fastload_descriptor.ReadDescriptor(rd);
	if(fastload_descriptor.type != Descriptor::FastLoad)
		fastload_descriptor.type = Descriptor::Undefined;

	rd.Seek(file_offset + 0x60);
	lib_id.Read(rd);

	rd.Seek(file_offset + 0x7A);
	cpm_flags = rd.ReadUnsigned(1);
	rsx_table_offset = rd.ReadUnsigned(2) << 7;
	relocations_offset = rd.ReadUnsigned(2) << 7;
	flags = rd.ReadUnsigned(1);

	if(library_descriptor.type != Descriptor::Undefined)
	{
		library_descriptor.offset = rd.Tell();
		library_descriptor.ReadData(rd, *this);
	}

	if(fastload_descriptor.type != Descriptor::Undefined)
	{
		fastload_descriptor.offset = rd.Tell();
		fastload_descriptor.ReadData(rd, *this);
	}

	for(size_t i = 0; i < 8; i++)
	{
		if(descriptors[i].type == Descriptor::Undefined)
			break;
		if(descriptors[i].type == Descriptor::Fixups)
		{
			if((flags & FLAG_FIXUPS) && relocations_offset == rd.Tell())
			{
				descriptors[i].type = Descriptor::ActualFixups;
				rd.Skip(uint32_t(descriptors[i].size_paras) << 4);
				continue;
			}
			else
			{
				descriptors[i].type = Descriptor::ActualAuxiliary4;
			}
		}
		descriptors[i].offset = rd.Tell();
		descriptors[i].ReadData(rd, *this);
	}

	if((flags & FLAG_FIXUPS))
	{
		ReadRelocations(rd);
	}

	if(rsx_table_offset != 0)
	{
		rd.Seek(file_offset + rsx_table_offset);
		for(int i = 0; i < 8; i++)
		{
			rsx_table[i].Read(rd);
			if(rsx_table[i].offset_record == rsx_record::RSX_TERMINATE)
				break; /* should be the case for the last record */
		}
		for(int i = 0; i < 8 && rsx_table[i].offset_record != rsx_record::RSX_TERMINATE; i++)
		{
			rsx_table[i].ReadModule(rd);
		}
	}
}

offset_t CPM86Format::ImageSize() const
{
	return file_size;
}

offset_t CPM86Format::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::LittleEndian;
	wr.Seek(file_offset);
	for(size_t i = 0; i < 8; i++)
	{
		if(descriptors[i].type == Descriptor::Undefined)
			break;
		descriptors[i].WriteDescriptor(wr, *this);
	}
	if(library_descriptor.type != Descriptor::Undefined)
	{
		assert(format == FORMAT_FLEXOS || format == FORMAT_FASTLOAD);
		wr.Seek(file_offset + 0x48);
		library_descriptor.WriteDescriptor(wr, *this);
	}
	if(fastload_descriptor.type != Descriptor::Undefined)
	{
		assert(format == FORMAT_FASTLOAD);
		wr.Seek(file_offset + 0x51);
		fastload_descriptor.WriteDescriptor(wr, *this);
	}
	if(lib_id.name != "")
	{
		assert(format == FORMAT_FLEXOS);
		wr.Seek(file_offset + 0x60);
		lib_id.Write(wr);
	}
	wr.Seek(file_offset + 0x7A);
	wr.WriteWord(1, cpm_flags);
	wr.WriteWord(2, rsx_table_offset >> 7);
	if((flags & FLAG_FIXUPS))
	{
		wr.Seek(file_offset + 0x7D);
		wr.WriteWord(2, relocations_offset >> 7);
	}
	wr.Seek(file_offset + 0x7F);
	wr.WriteWord(1, flags);
	if(library_descriptor.type != Descriptor::Undefined)
	{
		assert(format == FORMAT_FLEXOS || format == FORMAT_FASTLOAD);
		library_descriptor.WriteData(wr, *this);
	}
	if(fastload_descriptor.type != Descriptor::Undefined)
	{
		assert(format == FORMAT_FASTLOAD);
		fastload_descriptor.WriteData(wr, *this);
	}
	for(size_t i = 0; i < 8; i++)
	{
		if(descriptors[i].type == Descriptor::Undefined)
			break;
		descriptors[i].WriteData(wr, *this);
	}
	if((flags & FLAG_FIXUPS))
	{
		WriteRelocations(wr);
	}
	if(rsx_table_offset != 0)
	{
		wr.Seek(file_offset + rsx_table_offset);
		for(int i = 0; i < 8; i++)
		{
			rsx_table[i].Write(wr);
			if(rsx_table[i].offset_record == rsx_record::RSX_TERMINATE)
				break; /* should be the case for the last record */
		}
		for(int i = 0; i < 8; i++)
		{
			rsx_table[i].WriteModule(wr);
		}
	}

	return ImageSize();
}

offset_t CPM86Format::GetFullFileSize() const
{
	if(file_size != offset_t(-1))
	{
		return file_size;
	}

	offset_t image_size = 0x80;
	for(int i = 0; i < 8; i++)
	{
		if(descriptors[i].type == Descriptor::Undefined)
			break;
		image_size += uint32_t(descriptors[i].GetSizeParas(*this)) << 4;
	}
	if(fastload_descriptor.type != Descriptor::Undefined)
	{
		image_size += uint32_t(fastload_descriptor.GetSizeParas(*this)) << 4;
	}
	if(library_descriptor.type != Descriptor::Undefined)
	{
		image_size += uint32_t(library_descriptor.GetSizeParas(*this)) << 4;
	}

	image_size = std::max(image_size, offset_t(relocations_offset + (uint32_t(GetRelocationSizeParas()) << 4)));

	offset_t rsx_end = rsx_table_offset;
	for(int i = 0; i < 8; i++)
	{
		if(rsx_table[i].offset_record == rsx_record::RSX_TERMINATE)
			break;
		else
			rsx_end += 16;
	}

	image_size = std::max(image_size, rsx_end);

	// TODO: do we want to also measure the internal RSX modules?

	return file_size = image_size;
}

void CPM86Format::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_cp437);

	dump.SetTitle("CMD format");

	Dumper::Region file_region("File", file_offset, GetFullFileSize(), 6);
	static const std::map<offset_t, std::string> x87_values =
	{
		{ 1, "8087 required" },
		{ 2, "8087 optional" },
		{ 3, "invalid 8087 requirement" },
	};
	file_region.AddField("Flags",
		Dumper::BitFieldDisplay::Make(2)
			->AddBitField(7, 1, Dumper::ChoiceDisplay::Make("fixups"), true)
			->AddBitField(5, 2, Dumper::ChoiceDisplay::Make(x87_values), true)
			->AddBitField(4, 1, Dumper::ChoiceDisplay::Make("RSX"), true)
			->AddBitField(3, 1, Dumper::ChoiceDisplay::Make("direct video access"), true)
			->AddBitField(2, 1, Dumper::ChoiceDisplay::Make("MP/M style record lock access"), true)
			->AddBitField(1, 1, Dumper::ChoiceDisplay::Make("cannot run in banked memory"), true),
		offset_t(flags));

	file_region.AddOptionalField("CP/M flags",
		Dumper::BitFieldDisplay::Make(2)
			->AddBitField(3, 1, Dumper::ChoiceDisplay::Make("F4"), true)
			->AddBitField(2, 1, Dumper::ChoiceDisplay::Make("F3"), true)
			->AddBitField(1, 1, Dumper::ChoiceDisplay::Make("F2"), true)
			->AddBitField(0, 1, Dumper::ChoiceDisplay::Make("F1"), true),
		offset_t(cpm_flags));

	file_region.AddOptionalField("Library name", Dumper::StringDisplay::Make(8, "\"", "\""), lib_id.name);
	file_region.AddOptionalField("Library version", Dumper::VersionDisplay::Make(), offset_t(lib_id.major_version), offset_t(lib_id.minor_version));
	file_region.AddOptionalField("Library flags", Dumper::HexDisplay::Make(8), offset_t(lib_id.flags));

	file_region.Display(dump);

	static const std::map<offset_t, std::string> group_types =
	{
		{ Descriptor::Code,             "Code group" },
		{ Descriptor::Data,             "Data group" },
		{ Descriptor::Extra,            "Extra group" },
		{ Descriptor::Stack,            "Stack group" },
		{ Descriptor::Auxiliary1,       "Auxiliary 1 group" },
		{ Descriptor::Auxiliary2,       "Auxiliary 2 group" },
		{ Descriptor::Auxiliary3,       "Auxiliary 3 group" },
		{ Descriptor::Auxiliary4,       "Auxiliary 4 or fixup group" },
		{ Descriptor::ActualFixups,     "Fixup group" },
		{ Descriptor::ActualAuxiliary4, "Auxiliary 4 group" },
		{ Descriptor::SharedCode,       "Shared code group" },
	};

	std::shared_ptr<Dumper::Region> fixups = nullptr;
	std::vector<std::shared_ptr<Dumper::Region>> groups;
	for(int i = 0; i < 8; i++)
	{
		if(descriptors[i].type == Descriptor::Undefined)
			break;
		std::shared_ptr<Dumper::Region> group;
		if(descriptors[i].type == Descriptor::ActualFixups)
			group = fixups = Dumper::Region::Make("Group", descriptors[i].offset, uint32_t(descriptors[i].size_paras) << 4, 5);
		else
			group = Dumper::Block::Make("Group", descriptors[i].offset, descriptors[i].image ? descriptors[i].image->AsImage() : nullptr, uint32_t(descriptors[i].load_segment) << 4, 5);
		group->InsertField(0, "Type", Dumper::ChoiceDisplay::Make(group_types), offset_t(descriptors[i].type));
		group->AddField("Minimum", Dumper::HexDisplay::Make(5), offset_t(uint32_t(descriptors[i].min_size_paras) << 4));
		group->AddField("Maximum", Dumper::HexDisplay::Make(5), offset_t(uint32_t(descriptors[i].max_size_paras) << 4));
		group->AddHiddenField("number", Dumper::DecDisplay::Make(), offset_t(i + 1));
		groups.push_back(group);
	}

	offset_t fixups_paras = GetRelocationSizeParas();
	if(fixups == nullptr && fixups_paras != 0)
	{
		fixups = Dumper::Region::Make("Fixups", relocations_offset, uint32_t(fixups_paras) << 4, 5);
		fixups->Display(dump);
	}

	size_t i = 0;
	for(auto rel : relocations)
	{
		Dumper::Entry relocation_entry("Relocation", i + 1, relocations_offset + i * 4, 5);
		relocation_source source = rel.GetSource();
		relocation_entry.AddField("Source", Dumper::SectionedDisplay<offset_t>::Make(Dumper::HexDisplay::Make(5)), offset_t(source.segment), source.offset);
		relocation_entry.AddField("Target", Dumper::DecDisplay::Make(), offset_t(rel.target));
		// TODO: fill addend
		relocation_entry.Display(dump);

		number_t segment = FindSegmentGroup(source.segment);
		if(segment != size_t(-1))
		{
			if(auto group = std::dynamic_pointer_cast<Dumper::Block>(groups[segment]))
			{
				group->AddSignal(source.offset, 2);
			}
		}
		i++;
	}

	size_t next_relocation_offset = relocations_offset + relocations.size() * 4;
	std::map<size_t, library> first_selectors;
	if(library_descriptor.type != Descriptor::Undefined)
	{
		Dumper::Region libraries("SRTL group", library_descriptor.offset, uint32_t(library_descriptor.size_paras) << 4, 5);
		libraries.Display(dump);
		size_t j = 0;
		for(auto& library : library_descriptor.libraries)
		{
			Dumper::Container lib("Library");
			lib.AddField("Name", Dumper::StringDisplay::Make(8, "\"", "\""), library.name);
			lib.AddField("Version", Dumper::VersionDisplay::Make(), offset_t(library.major_version), offset_t(library.minor_version));
			lib.AddField("Flags", Dumper::HexDisplay::Make(8), offset_t(library.flags));
			if(IsFastLoadFormat())
			{
				lib.AddField("First selector", Dumper::DecDisplay::Make(), offset_t(library.first_selector));
				//lib.AddField("Unknown", Dumper::HexDisplay::Make(4), offset_t(library.unknown)); // TODO: only display if not 1
				first_selectors[library.first_selector] = library;
			}

			lib.AddHiddenField("number", Dumper::DecDisplay::Make(), offset_t(j + 1));
			lib.Display(dump);
			j += 1;

			size_t i = 0;
			for(auto rel : library.relocations)
			{
				Dumper::Entry relocation_entry("Relocation", i + 1, next_relocation_offset, 5);
				next_relocation_offset += 4;
				relocation_source source = rel.GetSource();
				relocation_entry.AddField("Source", Dumper::SectionedDisplay<offset_t>::Make(Dumper::HexDisplay::Make(5)), offset_t(source.segment), source.offset);
				relocation_entry.Display(dump);

				number_t segment = FindSegmentGroup(source.segment);
				if(segment != size_t(-1))
				{
					if(auto group = std::dynamic_pointer_cast<Dumper::Block>(groups[segment]))
					{
						group->AddSignal(source.offset, 2);
					}
				}
				i++;
			}
		}
	}

	if(fastload_descriptor.type != Descriptor::Undefined)
	{
		Dumper::Region postlink("Postlink group", fastload_descriptor.offset, uint32_t(fastload_descriptor.size_paras) << 4, 5);

		postlink.AddField("Maximum entries", Dumper::HexDisplay::Make(4), offset_t(fastload_descriptor.maximum_entries));
		postlink.AddField("First free entry", Dumper::HexDisplay::Make(4), offset_t(fastload_descriptor.first_free_entry));
		postlink.AddField("Index base", Dumper::HexDisplay::Make(4), offset_t(fastload_descriptor.index_base));
		postlink.AddField("First used index", Dumper::HexDisplay::Make(4), offset_t(fastload_descriptor.first_used_index));

		postlink.Display(dump);

		size_t i = 0;
		library current_library;
		for(auto desc : fastload_descriptor.ldt)
		{
			auto selector_it = first_selectors.find(i);
			if(selector_it != first_selectors.end())
				current_library = selector_it->second; // following selectors correspond to library

			if(i >= fastload_descriptor.first_free_entry)
				break;
			if(i >= fastload_descriptor.first_used_index)
			{
				if(desc.limit != 0)
					current_library.name = ""; // back to non-library selectors
				Dumper::Entry descriptor_entry("Descriptor", i, fastload_descriptor.offset + 8 + 8 * i, 4);
				descriptor_entry.AddField("Selector", Dumper::HexDisplay::Make(4), offset_t(i * 8 + 7));
				descriptor_entry.AddField("Limit", Dumper::HexDisplay::Make(4), offset_t(desc.limit));
				descriptor_entry.AddField("Address", Dumper::HexDisplay::Make(6), offset_t(desc.address));
				descriptor_entry.AddField("Group", Dumper::HexDisplay::Make(2), offset_t(desc.group));
				if(current_library.name != "")
					descriptor_entry.AddField("Library", Dumper::StringDisplay::Make(8, "\"", "\""), current_library.name);
				descriptor_entry.Display(dump);
			}
			i ++;
		}
	}

	for(auto& group : groups)
	{
		group->Display(dump);
	}

	if(rsx_table_offset != 0)
	{
		size_t rsx_count = 0;
		for(rsx_count = 0; rsx_count < 8; rsx_count++)
		{
			if(rsx_table[rsx_count].offset_record == rsx_record::RSX_TERMINATE)
				break;
		}

		Dumper::Region rsx_table_region("RSX table", rsx_table_offset, (rsx_count + 1) * 16, 6);
		rsx_table_region.Display(dump);

		for(int i = 0; i < 8; i++)
		{
			if(rsx_table[i].offset_record == rsx_record::RSX_TERMINATE)
				break;
			Dumper::Region rsx_entry("RSX", rsx_table[i].offset_record << 7, rsx_table[i].GetFullFileSize(), 6);
			rsx_entry.AddField("Name", Dumper::StringDisplay::Make(8, "\""), rsx_table[i].name);
			rsx_entry.AddHiddenField("number", Dumper::DecDisplay::Make(), offset_t(i + 1));
			rsx_entry.Display(dump);
		}

		for(int i = 0; i < 8; i++)
		{
			if(rsx_table[i].offset_record == rsx_record::RSX_TERMINATE)
				break;
			std::cout << "=== RSX " << (i + 1) << " ===" << std::endl;
			if(rsx_table[i].offset_record == rsx_record::RSX_TERMINATE)
				continue;
			rsx_table[i].Dump(dump);
		}
	}
}

void CPM86Format::CalculateValues()
{
	uint32_t offset = file_offset + 0x80;

	if(library_descriptor.type != Descriptor::Undefined)
	{
		library_descriptor.offset = offset;
		offset += uint32_t(library_descriptor.GetSizeParas(*this)) << 4;
	}

	if(fastload_descriptor.type != Descriptor::Undefined)
	{
		fastload_descriptor.offset = offset;
		offset += uint32_t(fastload_descriptor.GetSizeParas(*this)) << 4;
	}

	uint32_t group8_index = 0;

	for(size_t i = 0; i < 8; i++)
	{
		if(descriptors[i].type == Descriptor::Undefined)
			continue;
		if(descriptors[i].type == Descriptor::ActualFixups)
			group8_index = i + 1;
		descriptors[i].offset = offset;
		offset += uint32_t(descriptors[i].GetSizeParas(*this)) << 4;
	}

	if((flags & FLAG_FIXUPS))
	{
		if(group8_index != 0)
		{
			// we must also update the group descriptor
			relocations_offset = std::max(offset, descriptors[group8_index - 1].offset);
			relocations_offset = ::AlignTo(relocations_offset, 0x80);
			if(relocations_offset > descriptors[group8_index - 1].offset)
			{
				// increase size of previous segment group
				uint32_t increment = relocations_offset - descriptors[group8_index - 1].offset;
				descriptors[group8_index - 2].size_paras += increment >> 4;
				if(descriptors[group8_index - 2].min_size_paras != 0)
					descriptors[group8_index - 2].min_size_paras = std::max(descriptors[group8_index - 2].min_size_paras, descriptors[group8_index - 2].size_paras);
				if(descriptors[group8_index - 2].max_size_paras != 0)
					descriptors[group8_index - 2].max_size_paras = std::max(descriptors[group8_index - 2].max_size_paras, descriptors[group8_index - 2].size_paras);

				// move current group
				descriptors[group8_index - 1].offset = relocations_offset;
			}
		}
		else
		{
			relocations_offset = ::AlignTo(offset, 0x80);
		}
		offset = relocations_offset + MeasureRelocations();
	}

	if(rsx_table[0].offset_record != rsx_record::RSX_TERMINATE)
	{
		offset = ::AlignTo(offset, 0x80);

		for(unsigned i = 0; i < 8; i++)
		{
			Linker::Debug << "Debug: Record #" << i + 1 << " from " << rsx_table[i].rsx_file_name << " (offset: " << rsx_table[i].offset_record << ")" << std::endl;
			if(rsx_table[i].offset_record == rsx_record::RSX_TERMINATE)
				break;
			rsx_table[i].offset_record = offset >> 7;
			Linker::Debug << "Debug: New offset: " << rsx_table[i].offset_record << std::endl;
			offset += rsx_table[i].GetFullFileSize();
			offset = ::AlignTo(offset, 0x80);
		}
		rsx_table_offset = offset;
		offset += 0x80;
	}

	if(file_size == offset_t(-1) || file_size < offset)
	{
		file_size = offset;
	}

	for(size_t i = 0; i < 8; i++)
	{
		if(descriptors[i].type == Descriptor::Undefined)
			break;
		descriptors[i].Prepare(*this);
	}
	if(library_descriptor.type != Descriptor::Undefined)
	{
		library_descriptor.Prepare(*this);
	}
	if(fastload_descriptor.type != Descriptor::Undefined)
	{
		fastload_descriptor.Prepare(*this);
	}
}

/* * * Writer members * * */

bool CPM86Format::FormatSupportsSegmentation() const
{
	return true;
}

bool CPM86Format::FormatIs16bit() const
{
	return true;
}

bool CPM86Format::FormatIsProtectedMode() const
{
	return format == FORMAT_FLEXOS || format == FORMAT_FASTLOAD;
}

bool CPM86Format::FormatSupportsLibraries() const
{
	return format == FORMAT_FLEXOS || format == FORMAT_FASTLOAD;
}

unsigned CPM86Format::FormatAdditionalSectionFlags(std::string section_name) const
{
	unsigned flags;
	if(section_name == ".stack" || section_name.rfind(".stack.", 0) == 0)
	{
		flags = Linker::Section::Stack;
	}
	else
	{
		flags = 0;
	}
	if(section_name == ".opt" || Linker::ends_with(section_name, ".opt"))
	{
		flags |= Linker::Section::Optional;
	}
	return flags;
}

std::vector<std::shared_ptr<Linker::Segment>>& CPM86Format::Segments()
{
	return segment_vector;
}

void CPM86Format::AssignSegmentTypes()
{
	// bitfield for segment types already assigned
	// possible segment types are 1/9 (either 1 for code or 9 for shared code may appear) and 2 to 8
	// using the formula (type - 1) & 7, we will assign bit offsets 0 to 7 to each of these, with 1 and 9 sharing the same bit
	uint8_t groups = 0;

	segment_types.clear();
	for(size_t i = 0; i < Segments().size(); i++)
	{
		auto segment = Segments()[i];
		Descriptor::group_type type = Descriptor::Undefined;
		if(segment->name == ".code")
		{
			if(option_shared_code)
				type = Descriptor::SharedCode;
			else
				type = Descriptor::Code;
		}
		else if(segment->name == ".data")
		{
			type = Descriptor::Data;
		}
		else if(segment->name == ".extra")
		{
			type = Descriptor::Extra;
		}
		else if(segment->name == ".stack")
		{
			type = Descriptor::Stack;
		}
		else if(segment->name == ".aux1")
		{
			type = Descriptor::Auxiliary1;
		}
		else if(segment->name == ".aux2")
		{
			type = Descriptor::Auxiliary2;
		}
		else if(segment->name == ".aux3")
		{
			type = Descriptor::Auxiliary3;
		}
		else if(segment->name == ".aux4")
		{
			type = Descriptor::ActualAuxiliary4;
		}

		// the first .code or .data segment must be included, even if it is empty, the others can be excluded
		if(segment->IsMissing()
		&& !(segment->name == ".code" && (groups & 1) == 0)
		&& !(segment->name == ".data" && (groups & 2) == 0))
		{
			continue;
		}

		if(type != Descriptor::Undefined)
		{
			if((groups & (1 << ((type - 1) & 7))) != 0)
			{
				Linker::FatalError("Duplicate segment type");
			}
			segment_types[segment] = Descriptor::group_type(type);
			groups |= 1 << (type - 1);
		}
	}

	for(size_t i = 0; i < Segments().size(); i++)
	{
		auto segment = Segments()[i];
		if(segment->IsMissing() && i != 0 && i != 1)
			continue;
		if(segment_types.find(segment) != segment_types.end())
			continue;
		int type;
		for(type = 1; type < 9; type++)
		{
			if((groups & (1 << (type - 1))) == 0)
				break;
		}
		if(type >= 9)
		{
			Linker::FatalError("Too many segments");
		}
		segment_types[segment] = Descriptor::group_type(type);
		groups |= 1 << (type - 1);
	}

	for(size_t i = 0; i < Segments().size(); i++)
	{
		Linker::Debug << "Debug: segment " << Segments()[i]->name << " assigned " << segment_types[Segments()[i]] << std::endl;
	}
}

unsigned CPM86Format::GetSegmentNumber(std::shared_ptr<Linker::Segment> segment)
{
	unsigned number = segment_types[segment];
	// in fixups (relocations), shared code and code both have value 1
	return number == Descriptor::SharedCode ? Descriptor::Code : number;
}

std::vector<Linker::OptionDescription<void>> CPM86Format::MemoryModelNames =
{
	Linker::OptionDescription<void>("default", "Depends on format: tiny for 8080 format, small for small format or FlexOS, compact for compact format"),
	Linker::OptionDescription<void>("tiny", "Tiny model, all symbols have the same preferred segment, 8080 format only"),
	Linker::OptionDescription<void>("small", "Small model, symbols in .code have a separate preferred segment, not supported on compact format"),
	Linker::OptionDescription<void>("compact", "Compact model, symbols in .data, .bss and .comm have a common preferred segment, all other sections are treated as separate segments"),
};

std::vector<Linker::OptionDescription<void>> CPM86Format::GetMemoryModelNames()
{
	return MemoryModelNames;
}

void CPM86Format::SetModel(std::string model)
{
	if(model == "")
	{
		memory_model = MODEL_DEFAULT;
	}
	else if(model == "tiny")
	{
		if(format != FORMAT_8080)
		{
			Linker::FatalError("Fatal error: Invalid memory model for format");
		}
		memory_model = MODEL_TINY;
	}
	else if(model == "small")
	{
		if(format == FORMAT_COMPACT)
		{
			Linker::FatalError("Fatal error: Invalid memory model for format");
		}
		memory_model = MODEL_SMALL;
	}
	else if(model == "compact")
	{
		memory_model = MODEL_COMPACT;
	}
	else
	{
		Linker::Error << "Error: unsupported memory model" << std::endl;
		memory_model = MODEL_DEFAULT;
	}
}

std::shared_ptr<Linker::OptionCollector> CPM86Format::GetOptions()
{
	return std::make_shared<CPM86OptionCollector>();
}

void CPM86Format::SetOptions(std::map<std::string, std::string>& options)
{
	CPM86OptionCollector collector;
	collector.ConsiderOptions(options);

	option_no_relocation = collector.noreloc();
	option_shared_code = collector.sharedcode();
	option_generate_fixup_group = collector.fixupgroup();

	if(collector.x87() == "on")
	{
		flags |= FLAG_REQUIRED_8087;
	}
	else if(collector.x87() == "auto")
	{
		flags |= FLAG_OPTIONAL_8087;
	}
	else if(collector.x87() != "off")
	{
		Linker::Error << "Error: Unknown option for 8087: " << collector.x87() << std::endl;
	}

	if(collector.directvideo())
	{
		flags |= FLAG_DIRECT_VIDEO;
	}
	if(collector.mpmlock())
	{
		flags |= FLAG_MPMLOCK;
	}
	if(collector.nobank())
	{
		flags |= FLAG_NO_BANKING;
	}

	if(collector.f1())
	{
		cpm_flags |= CPM_FLAG_F1;
	}
	if(collector.f2())
	{
		cpm_flags |= CPM_FLAG_F2;
	}
	if(collector.f3())
	{
		cpm_flags |= CPM_FLAG_F3;
	}
	if(collector.f4())
	{
		cpm_flags |= CPM_FLAG_F4;
	}

	if(collector.srtl())
	{
		lib_id = library_id::ParseNameAndVersion(collector.srtl().value());
	}

	unsigned rsx_count = 0;
	if(auto rsx_file_names = collector.rsx_file_names())
	{
		for(auto& rsx_file_name : rsx_file_names.value())
		{
			if(rsx_count < 7)
			{
				rsx_table[rsx_count].rsx_file_name = rsx_file_name;
//				Linker::Debug << "Debug: Adding RSX #" << rsx_count + 1 << " as " << rsx_table[rsx_count].rsx_file_name << std::endl;
				rsx_count ++;
			}
		}
	}

	for(unsigned i = 0; i < rsx_count; i++)
	{
		auto& rsx = rsx_table[i];
		rsx.offset_record = 0;
		size_t eq_offset = rsx.rsx_file_name.find('=');
		if(eq_offset != std::string::npos)
		{
			rsx.name = rsx.rsx_file_name.substr(0, eq_offset);
			rsx.rsx_file_name = rsx.rsx_file_name.substr(eq_offset + 1);
		}
		else
		{
			std::filesystem::path rsx_file_path(rsx.rsx_file_name);
			rsx.name = rsx_file_path.stem();
		}

		Linker::Debug << "Debug: Adding RSX #" << i + 1 << " as " << rsx.rsx_file_name << std::endl;

		rsx.name.resize(8, ' ');
		std::transform(rsx.name.begin(), rsx.name.end(), rsx.name.begin(), ::toupper);
	}
	rsx_table[rsx_count].offset_record = rsx_record::RSX_TERMINATE;
}

std::unique_ptr<Script::List> CPM86Format::GetScript(Linker::Module& module)
{
	/* The formats (what Digital Research documents call "model") and actual memory models interact in the following ways:
	- Tiny model
		8080 format
			TinyScript - everything is packed inside one segment
	- Small model
		8080 format
			SmallScript_8080 - everything is packed inside one segment, but data sections have a bias at the boundary between code and data
		Small format
			SmallScript - two segments are prepared, code and data
	- Compact model
		8080 format
			CompactScript_8080 - everything is packed inside one segment, but data sections are grouped
		Small format
			CompactScript_Small - two segments are prepared, but data sections are grouped, with stack at the very end
		Compact format
			CompactScript - up to 7 separate data segments are created
	*/
	static const char * TinyScript = R"(
".code"
{
	at 0x100;
	all not zero;
	align 16;
	all not ".stack";
	align 16;
	all;
	align 16;
};
)";

	static const char * SmallScript_8080 = R"(
".code"
{
	at 0x100;
	all exec;
	align 16;
	base here;
	all not zero align 16; # TODO: why align here?
	align 16;
	all not ".stack" align 16; # TODO: why align here?
	align 16;
	all;
	align 16;
};
)";

	static const char * SmallScript = R"(
".code"
{
	all exec align 16; # TODO: why align here?
	align 16;
};

".data"
{
	at 0x100; # inserts 0x100 bytes
	base here - 0x100;
	all not zero align 16; # TODO: why align here?
	align 16;
	all not ".stack" align 16; # TODO: why align here?
	align 16;
	all;
	align 16;
};
)";

	static const char * SmallScript_FlexOS = R"(
".code"
{
	all exec align 16; # TODO: why align here?
	align 16;
};

".data"
{
	at 0;
	base here;
	all not zero align 16; # TODO: why align here?
	align 16;
	all not ".stack" align 16; # TODO: why align here?
	align 16;
};

".stack"
{
	at 0;
	base here;
	all ".stack.data" align 16;
	all ".stack" or ".stack.bss" align 16;
	align 16;
};
)";

	static const char * CompactScript_8080 = R"(
".code"
{
	at 0x100;
	base here - 0x100;
	all exec align 16; # TODO: why align here?
	align 16; # TODO: why align here?
	all not zero and not ".data" and not ".rodata"
		align 16 base here;
	align 16;
	base here;
	all ".data" or ".rodata" align 16; # TODO: why align here?
	align 16;
	all ".bss" or ".comm" align 16; # TODO: why align here?
	all not ".stack" align 16 base here;
	all base here;
};
)";

	static const char * CompactScript_Small = R"(
".code"
{
	base here - 0x100;
	all exec align 16; # TODO: why align here?
	align 16;
};

".data"
{
	at 0x100;
	all not zero and not ".data" and not ".rodata"
		align 16 base here;
	base 0;
	all ".data" or ".rodata" align 16; # TODO: why align here?
	align 16;
	all ".bss" or ".comm" align 16; # TODO: why align here?
	all not ".stack" align 16 base here;
	all base here;
};
)";

	static const char * CompactScript_FlexOS = R"(
".code"
{
	base here - 0x100;
	all exec align 16; # TODO: why align here?
	align 16;
};

".data"
{
	at 0;
	all not zero and not ".data" and not ".rodata"
		align 16 base here;
	base 0;
	all ".data" or ".rodata" align 16; # TODO: why align here?
	align 16;
	all ".bss" or ".comm" align 16; # TODO: why align here?
	all not ".stack" align 16 base here;
};

".stack"
{
	at 0;
	base here;
	all ".stack.data" align 16;
	all ".stack" or ".stack.bss" align 16;
	align 16;
};
)";

	static const char * CompactScript = R"(
".code"
{
	base here - 0x100;
	all exec align 16; # TODO: why align here?
	align 16;
};

".data"
{
	at 0x100;
	base here - 0x100;
	all ".data" or ".rodata" align 16; # TODO: why align here?
	align 16;
	all ".bss" or ".comm" align 16; # TODO: why align here?
	align 16;
};

".extra"
{
	at 0;
	base here;
	all ".extra" or ".extra.data" align 16;
	all ".extra.bss" align 16;
	align 16;
};

".stack"
{
	at 0;
	base here;
	all ".stack.data" align 16;
	all ".stack" or ".stack.bss" align 16;
	align 16;
};

for suffix ".data" or suffix ".bss" or any max 3
{
	at 0;
	base here;
	all (suffix ".data" or suffix ".bss" or any) and not zero
		align 16;
	all (suffix ".data" or suffix ".bss" or any) and zero
		align 16;
	align 16;
};

for any
{
	# TODO: combine different names (new syntax?)
	at 0;
	base here;
	all not zero
		align 16;
	all zero
		align 16;
	align 16;
};
)";

	if(linker_script != "")
	{
		if(memory_model != MODEL_DEFAULT)
		{
			Linker::Warning << "Warning: linker script provided, overriding memory model" << std::endl;
		}
		return SegmentManager::GetScript(module);
	}
	else
	{
		switch(format)
		{
		case FORMAT_8080:
			switch(memory_model)
			{
			case MODEL_DEFAULT:
			case MODEL_TINY:
				return Script::parse_string(TinyScript);
			case MODEL_SMALL:
				return Script::parse_string(SmallScript_8080);
			case MODEL_COMPACT:
				return Script::parse_string(CompactScript_8080);
			default:
				Linker::FatalError("Internal error: invalid memory model");
			}
			break;
		case FORMAT_UNKNOWN:
			Linker::Error << "Error: unspecified format, using small" << std::endl;
		case FORMAT_SMALL:
			switch(memory_model)
			{
			default:
				Linker::FatalError("Internal error: invalid memory model");
			case MODEL_DEFAULT:
			case MODEL_SMALL:
				return Script::parse_string(SmallScript);
			case MODEL_COMPACT:
				return Script::parse_string(CompactScript_Small);
			}
			break;
		case FORMAT_FLEXOS:
		case FORMAT_FASTLOAD:
			switch(memory_model)
			{
			default:
				Linker::FatalError("Internal error: invalid memory model");
			case MODEL_DEFAULT:
			case MODEL_SMALL:
				return Script::parse_string(SmallScript_FlexOS);
			case MODEL_COMPACT:
				return Script::parse_string(CompactScript_FlexOS);
			}
			break;
		case FORMAT_COMPACT:
			return Script::parse_string(CompactScript);
		default:
			Linker::FatalError("Internal error: invalid memory model");
		}
	}
}

void CPM86Format::Link(Linker::Module& module)
{
	std::unique_ptr<Script::List> script = GetScript(module);
	ProcessScript(script, module);
}

void CPM86Format::BuildLDTImage(Linker::Module& module)
{
	std::array<std::set<uint32_t>, 8> segment_boundaries;

	// divide the groups into segments
	// also collect libraries
	for(Linker::Relocation& rel : module.relocations)
	{
		Linker::Resolution resolution;
		if(rel.kind != Linker::Relocation::SelectorIndex && rel.Resolve(module, resolution))
		{
			if(resolution.reference == nullptr)
			{
				resolution.value = ((resolution.value - rel.addend) >> 4) + rel.addend; // TODO: too complex code
				uint32_t offset = uint32_t(resolution.value) << 4;
				unsigned dst_segment = GetSegmentNumber(resolution.target);
				segment_boundaries[dst_segment - 1].insert(offset);
			}
		}
		else if(Linker::SymbolName * symbol = std::get_if<Linker::SymbolName>(&rel.target.target))
		{
			std::string library;
			if(symbol->GetImportedLibrary(library))
			{
				Linker::Debug << "Debug: reference library " << library << std::endl;
				library_descriptor.FetchImportedLibrary(library);
			}
		}
	}

	// calculate segment limits for each segment in each group
	std::array<std::map<uint32_t, uint32_t>, 8> segment_limits;

	for(size_t i = 0; i < Segments().size(); i++)
	{
		auto segment = Segments()[i];
		if(segment_types.find(segment) == segment_types.end())
			continue;
		unsigned segment_number = GetSegmentNumber(segment);
		uint32_t segment_start = 0;
		for(auto offset : segment_boundaries[segment_number - 1])
		{
			if(offset == 0)
				continue;
			segment_limits[segment_number - 1][segment_start] = std::min(0xFFFFU, offset - segment_start - 1);
			segment_start = offset;
		}
		segment_limits[segment_number - 1][segment_start] =
			segment->ImageSize() == segment_start
				? 0
				: std::min(offset_t(0xFFFF), segment->ImageSize() - segment_start - 1);
	}

	// create the LDT image
	fastload_descriptor.Clear();

	// imitating POSTLINK behavior
	fastload_descriptor.ldt.resize(0xC9);
	fastload_descriptor.index_base = 0x000C;
	fastload_descriptor.first_used_index = 0x000C;
	fastload_descriptor.first_free_entry = fastload_descriptor.first_used_index;

	// allocate internal selectors for each relocation
	std::map<relocation_target, uint16_t> selectors;

	for(Linker::Relocation& rel : module.relocations)
	{
		Linker::Resolution resolution;
		if(rel.kind == Linker::Relocation::SelectorIndex && rel.Resolve(module, resolution) && resolution.reference == nullptr)
		{
			resolution.value = ((resolution.value - rel.addend) >> 4) + rel.addend; // TODO: too complex code
			unsigned dst_segment = GetSegmentNumber(resolution.target);
			relocation_target target { dst_segment, resolution.value };
			uint16_t selector;
			if(selectors.find(target) == selectors.end())
			{
				size_t index = fastload_descriptor.first_free_entry++;
				selector = index * 8 + 7;
				fastload_descriptor.ldt[index].limit = segment_limits[dst_segment - 1][resolution.value << 4];
				fastload_descriptor.ldt[index].address = resolution.value << 4;
				fastload_descriptor.ldt[index].group = dst_segment;
				selectors[target] = selector;
			}
			else
			{
				selector = selectors[target];
			}
			rel.WriteWord(selector);
		}
	}

	// allocate external selectors
	for(auto& library : library_descriptor.libraries)
	{
		for(Linker::Relocation& rel : module.relocations)
		{
			Linker::Resolution resolution;
			if(rel.Resolve(module, resolution))
				continue;

			Linker::SymbolName * symbol = std::get_if<Linker::SymbolName>(&rel.target.target);
			if(!symbol)
				continue;

			std::string library_name;
			if(!symbol->GetImportedLibrary(library_name))
				continue;

			Linker::Debug << "Debug: reference library " << library_name << std::endl;

			if(&library_descriptor.FetchImportedLibrary(library_name) != &library)
				continue;

			Linker::Debug << "Debug: reference library " << library.name << " offset " << std::hex << (uint32_t(symbol->addend + rel.addend) << 4) << " in group " << std::hex << GetSegmentNumber(rel.source.GetPosition().segment) << " offset " << std::hex << rel.source.GetPosition().address << std::endl;

			size_t index = fastload_descriptor.first_free_entry++;
			uint16_t selector = index * 8 + 7;
			if(library.first_selector == 0)
				library.first_selector = index;
			fastload_descriptor.ldt[index].limit = 0;
			fastload_descriptor.ldt[index].address = uint32_t(symbol->addend + rel.addend) << 4;
			fastload_descriptor.ldt[index].group = 1;
			rel.WriteWord(selector);
		}
	}

	// allocate remaining internal selectors
	for(size_t i = 0; i < Segments().size(); i++)
	{
		auto segment = Segments()[i];
		if(segment_types.find(segment) == segment_types.end())
			continue;
		unsigned segment_number = GetSegmentNumber(segment);

		relocation_target target { segment_number, 0 };
		if(selectors.find(target) != selectors.end())
			continue;

		Linker::Debug << "Debug: segment " << segment->name << " has segment number " << segment_number << std::endl;
		size_t index = fastload_descriptor.first_free_entry++;
		fastload_descriptor.ldt[index].limit = std::min(0xFFFFU, segment_limits[segment_number - 1][0]);
		fastload_descriptor.ldt[index].address = 0;
		fastload_descriptor.ldt[index].group = segment_number;
	}

	fastload_descriptor.maximum_entries = fastload_descriptor.first_free_entry + 2;

	fastload_descriptor.type = Descriptor::FastLoad;
	fastload_descriptor.size_paras =
		fastload_descriptor.min_size_paras = fastload_descriptor.GetSizeParas(*this);
	fastload_descriptor.max_size_paras = 0;

	for(auto& library : library_descriptor.libraries)
	{
		library.flags |= 0x00800000; // POSTLINK behavior
	}
}

void CPM86Format::ProcessModule(Linker::Module& module)
{
	Link(module);

	if(application == APPL_RSX)
	{
		flags |= FLAG_RSX;
	}

	AssignSegmentTypes();

	if(format == FORMAT_FASTLOAD)
	{
		BuildLDTImage(module);
	}

	std::map<relocation_source, size_t> relocation_targets;
	for(Linker::Relocation& rel : module.relocations)
	{
		Linker::Resolution resolution;
		if(rel.Resolve(module, resolution))
		{
			if(resolution.reference != nullptr)
			{
				Linker::Error << "Error: Format does not support inter-segment distances: " << rel << ", ignoring" << std::endl;
				continue;
			}

			switch(rel.kind)
			{
			case Linker::Relocation::Direct:
				rel.WriteWord(resolution.value);
				break;
			case Linker::Relocation::SelectorIndex: // FlexOS 286
				resolution.value = ((resolution.value - rel.addend) >> 4) + rel.addend; // TODO: too complex code
			case Linker::Relocation::ParagraphAddress: // CP/M-86
				if(format == FORMAT_FASTLOAD)
					continue; // already relocated
				if(resolution.target != nullptr)
				{
					if(option_no_relocation)
					{
						Linker::Error << "Error: relocations suppressed, generating image anyway" << std::endl;
					}
					else
					{
						Linker::Position source = rel.source.GetPosition();
						unsigned src_segment = GetSegmentNumber(source.segment);
						relocation_targets[relocation_source { src_segment, source.address }] = GetSegmentNumber(resolution.target);
						flags |= FLAG_FIXUPS;
					}
				}
				rel.WriteWord(resolution.value);
				break;
			default:
				Linker::Error << "Error: unsupported reference type, ignoring" << std::endl;
				continue;
			}
		}
		else if(FormatSupportsLibraries())
		{
			if(Linker::SymbolName * symbol = std::get_if<Linker::SymbolName>(&rel.target.target))
			{
				std::string library;
				if(symbol->GetImportedLibrary(library))
				{
					if(format == FORMAT_FASTLOAD)
						continue; // already relocated
					Linker::Position source = rel.source.GetPosition();
					unsigned src_segment = GetSegmentNumber(source.segment);
					library_descriptor.FetchImportedLibrary(library).relocation_targets.insert(relocation_source { src_segment, source.address });
					rel.WriteWord(symbol->addend + rel.addend);
					flags |= FLAG_FIXUPS;
					continue;
				}
			}

			Linker::Error << "Error: Unable to resolve relocation: " << rel << ", ignoring" << std::endl;
			continue;
		}
	}

	for(auto rel : relocation_targets)
	{
		relocations.push_back(Relocation(rel.first, rel.second));
	}

	for(auto& lib : library_descriptor.libraries)
	{
		for(auto rel : lib.relocation_targets)
		{
			lib.relocations.push_back(Relocation(rel));
		}
	}

	Linker::Location stack_top;
	if(module.FindGlobalSymbol(".stack_top", stack_top))
	{
		Linker::Warning << "Warning: setting stack top not supported, ignoring" << std::endl;
	}

	Linker::Location entry;
	if(module.FindGlobalSymbol(".entry", entry))
	{
		Linker::Position position = entry.GetPosition();
		if(position.segment != Segments()[0] || position.address != (format == FORMAT_8080 ? 0x100 : 0))
		{
			Linker::Error << "Error: entry point must be beginning of .code segment, ignoring" << std::endl;
		}
	}

	size_t j = 0;
	for(size_t i = 0; i < Segments().size(); i++)
	{
		if(segment_types.find(Segments()[i]) == segment_types.end())
			continue;
		Descriptor::group_type type = segment_types[Segments()[i]];
		if(IsSharedRunTimeLibrary() && type != Descriptor::Code && type != Descriptor::SharedCode)
			continue; // shared run-time libraries only include the code segment
		descriptors[j].type = type;
		bool is_zero_page = (format != FORMAT_FLEXOS && format != FORMAT_FASTLOAD) && (format == FORMAT_8080 ? i == 0 : i == 1);
		descriptors[j].attach_zero_page = is_zero_page;
		descriptors[j].image = Segments()[i];
		descriptors[j].size_paras = descriptors[j].GetSizeParas(*this);
		descriptors[j].load_segment = 0;
		descriptors[j].min_size_paras = ((Segments()[i]->TotalSize() + 0xF) >> 4) + (is_zero_page ? 0x10 : 0);
		descriptors[j].max_size_paras =
			Segments()[i]->zero_fill == 0 && Segments()[i]->optional_extra == 0
				? 0
				: ((Segments()[i]->TotalSize() + Segments()[i]->optional_extra + 0xF) >> 4) + (is_zero_page ? 0x10 : 0);
		/* TODO: optional extra */
		j++;
	}

	if(option_generate_fixup_group)
	{
		assert(j < 8);
		descriptors[j].type = Descriptor::ActualFixups;
		descriptors[j].attach_zero_page = false;
		descriptors[j].image = nullptr;
		descriptors[j].size_paras = descriptors[j].GetSizeParas(*this);
		descriptors[j].load_segment = 0;
		descriptors[j].min_size_paras = descriptors[j].size_paras;
		descriptors[j].max_size_paras = 0;
		j++;
	}

	for(unsigned i = 0; i < 7; i++)
	{
		Linker::Debug << "Debug: RSX module " << rsx_table[i].rsx_file_name << std::endl;
		if(rsx_table[i].rsx_file_name != "")
		{
			std::ifstream rsx_file;
			rsx_file.open(rsx_table[i].rsx_file_name, std::ios_base::in | std::ios_base::binary);
			if(rsx_file.is_open())
			{
				Linker::Reader rd(::LittleEndian, &rsx_file);
				rsx_table[i].contents = Linker::Buffer::ReadFromFile(rd);
				rsx_file.close();
				Linker::Debug << "Debug: read " << rsx_table[i].GetFullFileSize() << " from " << rsx_table[i].rsx_file_name << std::endl;
			}
			else
			{
				Linker::Error << "Error: unable to open RSX file " << rsx_table[i].rsx_file_name << ", generating dummy entry" << std::endl;
			}
		}
	}

	if(library_descriptor.libraries.size() > 0)
	{
		assert(format == FORMAT_FLEXOS || format == FORMAT_FASTLOAD);
		library_descriptor.type = Descriptor::Libraries;
		library_descriptor.size_paras =
			library_descriptor.min_size_paras =
			library_descriptor.max_size_paras = library_descriptor.GetSizeParas(*this);
	}
}

void CPM86Format::GenerateFile(std::string filename, Linker::Module& module)
{
	if(module.cpu != Linker::Module::I86)
	{
		Linker::FatalError("Fatal error: Format only supports Intel 8086 binaries");
	}

	Linker::OutputFormat::GenerateFile(filename, module);
}

std::string CPM86Format::GetDefaultExtension(Linker::Module& module, std::string filename) const
{
	switch(application)
	{
	case APPL_UNKNOWN:
	default:
		switch(format)
		{
		case FORMAT_8080:
		case FORMAT_SMALL:
		case FORMAT_COMPACT:
			return filename + ".cmd";
		case FORMAT_FLEXOS:
			if(IsSharedRunTimeLibrary())
				return filename + ".srl";
		case FORMAT_FASTLOAD:
			return filename + ".286";
		default:
			Linker::FatalError("Internal error: invalid memory model");
		}
		break;
	case APPL_CMD:
		return filename + ".cmd";
	case APPL_RSX:
		return filename + ".rsx";
	case APPL_RSP:
		return filename + ".rsp";
	case APPL_MPM:
		return filename + ".mpm";
	case APPL_CON:
		return filename + ".con";
	case APPL_OVR:
		return filename + ".ovr";
	case APPL_286:
		return filename + ".286";
	case APPL_SRL:
		return filename + ".srl";
	}
}

