
#include "peexe.h"

/* TODO: unimplemented */

using namespace Microsoft;

bool PEFormat::PEOptionalHeader::Is64Bit() const
{
	return magic == EXE64;
}

uint32_t PEFormat::PEOptionalHeader::GetSize() const
{
	return (Is64Bit() ? 112 : 96) + 8 * data_directories.size();
}

void PEFormat::PEOptionalHeader::ReadFile(Linker::Reader& rd)
{
	magic = rd.ReadUnsigned(2);
	version_stamp = rd.ReadUnsigned(2);
	code_size = rd.ReadUnsigned(4);
	data_size = rd.ReadUnsigned(4);
	bss_size = rd.ReadUnsigned(4);
	entry_address = rd.ReadUnsigned(4);
	code_address = rd.ReadUnsigned(4);
	if(!Is64Bit())
	{
		data_address = rd.ReadUnsigned(4);
	}
	image_base = rd.ReadUnsigned(Is64Bit() ? 8 : 4);
	section_align = rd.ReadUnsigned(4);
	file_align = rd.ReadUnsigned(4);
	os_version.major = rd.ReadUnsigned(2);
	os_version.minor = rd.ReadUnsigned(2);
	image_version.major = rd.ReadUnsigned(2);
	image_version.minor = rd.ReadUnsigned(2);
	subsystem_version.major = rd.ReadUnsigned(2);
	subsystem_version.minor = rd.ReadUnsigned(2);
	win32_version = rd.ReadUnsigned(4);
	total_image_size = rd.ReadUnsigned(4);
	total_headers_size = rd.ReadUnsigned(4);
	subsystem = SubsystemType(rd.ReadUnsigned(2));
	flags = rd.ReadUnsigned(2);
	reserved_stack_size = rd.ReadUnsigned(Is64Bit() ? 8 : 4);
	committed_stack_size = rd.ReadUnsigned(Is64Bit() ? 8 : 4);
	reserved_heap_size = rd.ReadUnsigned(Is64Bit() ? 8 : 4);
	committed_heap_size = rd.ReadUnsigned(Is64Bit() ? 8 : 4);
	loader_flags = rd.ReadUnsigned(4);
	uint32_t directory_count = rd.ReadUnsigned(4);
	for(uint32_t i = 0; i < directory_count; i++)
	{
		DataDirectory dirent;
		dirent.address = rd.ReadUnsigned(4);
		dirent.size = rd.ReadUnsigned(4);
		data_directories.emplace_back(dirent);
	}
}

void PEFormat::PEOptionalHeader::WriteFile(Linker::Writer& wr) const
{
	wr.WriteWord(2, magic);
	wr.WriteWord(2, version_stamp);
	wr.WriteWord(2, code_size);
	wr.WriteWord(2, data_size);
	wr.WriteWord(2, bss_size);
	wr.WriteWord(2, entry_address);
	wr.WriteWord(2, code_address);
	if(!Is64Bit())
	{
		wr.WriteWord(2, data_address);
	}
	wr.WriteWord(Is64Bit() ? 8 : 4, image_base);
	wr.WriteWord(2, section_align);
	wr.WriteWord(2, file_align);
	wr.WriteWord(2, os_version.major);
	wr.WriteWord(2, os_version.minor);
	wr.WriteWord(2, image_version.major);
	wr.WriteWord(2, image_version.minor);
	wr.WriteWord(2, subsystem_version.major);
	wr.WriteWord(2, subsystem_version.minor);
	wr.WriteWord(2, win32_version);
	wr.WriteWord(2, total_image_size);
	wr.WriteWord(2, total_headers_size);
	wr.WriteWord(2, subsystem);
	wr.WriteWord(2, flags);
	wr.WriteWord(Is64Bit() ? 8 : 4, reserved_stack_size);
	wr.WriteWord(Is64Bit() ? 8 : 4, committed_stack_size);
	wr.WriteWord(Is64Bit() ? 8 : 4, reserved_heap_size);
	wr.WriteWord(Is64Bit() ? 8 : 4, committed_heap_size);
	wr.WriteWord(2, loader_flags);
	wr.WriteWord(2, data_directories.size());
	for(auto& dirent : data_directories)
	{
		wr.WriteWord(2, dirent.address);
		wr.WriteWord(2, dirent.size);
	}
}

offset_t PEFormat::PEOptionalHeader::CalculateValues(COFFFormat& coff)
{
	// TODO
	return offset_t(-1);
}

void PEFormat::PEOptionalHeader::DumpFields(const COFFFormat& coff, Dumper::Dumper& dump, Dumper::Region& header_region) const
{
	// TODO
}

void PEFormat::ReadFile(Linker::Reader& rd)
{
	file_offset = rd.Tell();
	rd.ReadData(4, pe_signature);
	ReadCOFFHeader(rd);
	ReadOptionalHeader(rd);
	ReadRestOfFile(rd);
}

void PEFormat::ReadOptionalHeader(Linker::Reader& rd)
{
	optional_header = std::make_unique<PEOptionalHeader>();
	optional_header->ReadFile(rd);
}

offset_t PEFormat::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::LittleEndian;
	stub.WriteStubImage(wr);
	wr.Seek(file_offset);
	wr.WriteData(4, pe_signature);
	WriteFileContents(wr);
	return offset_t(-1);
}

void PEFormat::Dump(Dumper::Dumper& dump) const
{
	// TODO: Windows encoding
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("PE format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 8);
	file_region.Display(dump);

	// TODO
}

std::string PEFormat::GetDefaultExtension(Linker::Module& module, std::string filename) const
{
	return filename + ".exe";
}

