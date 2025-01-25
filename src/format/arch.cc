
#include "arch.h"

using namespace Archive;

ArchiveFormat::FileReader::~FileReader()
{
}

void ArchiveFormat::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = ::EndianType(0); // should not matter
	file_offset = rd.Tell(); // !<arch>\n
	if(file_size == offset_t(-1))
	{
		rd.SeekEnd();
		file_size = rd.Tell() - file_offset;
	}
	rd.Seek(file_offset + 8);
	while(rd.Tell() < file_offset + file_size)
	{
		if((rd.Tell() & 1) != 0)
			rd.Skip(1);
		File entry;
		entry.name = rd.ReadData(16);
		size_t last_space = entry.name.find_last_not_of(' ');
		if(last_space == std::string::npos)
			last_space = 0;
		else
			last_space ++;
		entry.name = entry.name.substr(0, last_space);
		Linker::Debug << "Debug: archive entry name: " << entry.name << std::endl;
		if(entry.name == "//")
		{
			// System V extended file names
			rd.Skip(32);
			// TODO
		}
		else
		{
			entry.modification = std::stoll(rd.ReadData(12), nullptr, 10);
			entry.owner_id = std::stoll(rd.ReadData(6), nullptr, 10);
			entry.group_id = std::stoll(rd.ReadData(6), nullptr, 10);
			entry.mode = std::stoll(rd.ReadData(8), nullptr, 8);
		}
		entry.size = std::stoll(rd.ReadData(10), nullptr, 10);
		Linker::Debug << "Debug: archive entry size: " << entry.size << std::endl;
		rd.Skip(2); // 0x60 0x0A
		if(file_reader == nullptr)
		{
			entry.contents = Linker::Buffer::ReadFromFile(rd, entry.size);
		}
		else
		{
			entry.contents = file_reader->ReadFile(rd, entry.size);
		}
		files.push_back(entry);
	}
}

void ArchiveFormat::WriteFile(Linker::Writer& wr)
{
	wr.endiantype = ::EndianType(0); // should not matter
	wr.Seek(file_offset);
	wr.WriteData("!<arch>\n");
	for(auto& entry : files)
	{
		std::string filename = entry.name;
		filename.resize(16, ' ');
		wr.WriteData(filename);
		// TODO
	}
	// TODO
}

void ArchiveFormat::Dump(Dumper::Dumper& dump)
{
	// TODO
}

void ArchiveFormat::ProduceModule(Linker::Module& module, Linker::Reader& rd)
{
	// TODO
}

void ArchiveFormat::CalculateValues()
{
	// TODO
}

