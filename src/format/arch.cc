
#include "arch.h"

using namespace Archive;

ArchiveFormat::FileReader::~FileReader()
{
}

void ArchiveFormat::SetFileReader(std::shared_ptr<FileReader> file_reader)
{
	this->file_reader = file_reader;
}

class FileReaderWrapper : public ArchiveFormat::FileReader
{
public:
	std::shared_ptr<Linker::Image> (* file_reader)(Linker::Reader&, offset_t);

	FileReaderWrapper(std::shared_ptr<Linker::Image> (* file_reader)(Linker::Reader&, offset_t))
		: file_reader(file_reader)
	{
	}

	std::shared_ptr<Linker::Image> ReadFile(Linker::Reader& rd, offset_t size) override
	{
		return file_reader(rd, size);
	}
};

void ArchiveFormat::SetFileReader(std::shared_ptr<Linker::Image> (* file_reader)(Linker::Reader& rd, offset_t size))
{
	this->file_reader = std::make_shared<FileReaderWrapper>(file_reader);
}

class FileReaderWrapper1 : public ArchiveFormat::FileReader
{
public:
	std::shared_ptr<Linker::Image> (* file_reader)(Linker::Reader&);

	FileReaderWrapper1(std::shared_ptr<Linker::Image> (* file_reader)(Linker::Reader&))
		: file_reader(file_reader)
	{
	}

	std::shared_ptr<Linker::Image> ReadFile(Linker::Reader& rd, offset_t size) override
	{
		return file_reader(rd);
	}
};

void ArchiveFormat::SetFileReader(std::shared_ptr<Linker::Image> (* file_reader)(Linker::Reader& rd))
{
	this->file_reader = std::make_shared<FileReaderWrapper1>(file_reader);
}

ArchiveFormat::ArchiveFormat(std::shared_ptr<Linker::Image> (* file_reader)(Linker::Reader& rd, offset_t size))
{
	SetFileReader(file_reader);
}

ArchiveFormat::ArchiveFormat(std::shared_ptr<Linker::Image> (* file_reader)(Linker::Reader& rd))
{
	SetFileReader(file_reader);
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
	offset_t extended_file_name_table = 0;
	while(rd.Tell() < file_offset + file_size)
	{
		File entry;
		entry.name = rd.ReadData(16);
		size_t last_space = entry.name.find_last_not_of(' ');
		if(last_space == std::string::npos)
			last_space = 0;
		else
			last_space ++;
		entry.name = entry.name.substr(0, last_space);
		Linker::Debug << "Debug: archive entry name: " << entry.name << std::endl;
		if(entry.name == "/")
		{
			// System V symbol table
			rd.Skip(32);
			// TODO
		}
		else if(entry.name == "//")
		{
			// System V extended file names
			rd.Skip(32);
			extended_file_name_table = rd.Tell() + 12;
		}
		else
		{
			if(entry.name.size() >= 1 && entry.name[0] == '/')
			{
				try
				{
					entry.extended_name_offset = std::stoll(entry.name.substr(1), nullptr, 10);
					entry.name = "";
				}
				catch(std::invalid_argument&)
				{
				}
			}
			if(entry.name.size() >= 1 && entry.name[entry.name.size() - 1] == '/')
			{
				entry.sysv_filename = true;
				entry.name = entry.name.substr(0, entry.name.size() - 1);
			}
			entry.modification = std::stoll(rd.ReadData(12), nullptr, 10);
			entry.owner_id = std::stoll(rd.ReadData(6), nullptr, 10);
			entry.group_id = std::stoll(rd.ReadData(6), nullptr, 10);
			entry.mode = std::stoll(rd.ReadData(8), nullptr, 8);
		}
		entry.size = std::stoll(rd.ReadData(10), nullptr, 10);
		Linker::Debug << "Debug: archive entry size: " << entry.size << std::endl;
		rd.Skip(2); // 0x60 0x0A
		offset_t entry_start = rd.Tell();
		if(file_reader == nullptr || entry.name == "//")
		{
			entry.contents = Linker::Buffer::ReadFromFile(rd, entry.size);
		}
		else
		{
			entry.contents = file_reader->ReadFile(rd, entry.size);
		}
		files.push_back(entry);
		rd.Seek(entry_start + entry.size);
		if((rd.Tell() & 1) != 0)
			rd.Skip(1);
	}

	if(extended_file_name_table != 0)
	{
		for(auto& entry : files)
		{
			if(entry.name == "" && entry.extended_name_offset != 0)
			{
				rd.Seek(extended_file_name_table + entry.extended_name_offset);
				entry.name = rd.ReadASCII('\n');
				Linker::Debug << "Debug: extended file name `" << entry.name << "' from offset " << entry.extended_name_offset << std::endl;
			}
		}
	}
}

offset_t ArchiveFormat::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::EndianType(0); // should not matter
	wr.Seek(file_offset);
	wr.WriteData("!<arch>\n");
	for(auto& entry : files)
	{
		if((wr.Tell() & 1) != 0)
			wr.Skip(1);
		std::string filename = entry.name;
		filename.resize(16, ' ');
		wr.WriteData(filename);
		// TODO
	}
	// TODO
	return file_size; // TODO
}

offset_t ArchiveFormat::ImageSize() const
{
	return file_size;
}

void ArchiveFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("Archive format");
	Dumper::Region file_region("File", file_offset, file_size != offset_t(-1) ? file_size : 0, 8);
	file_region.Display(dump);

	offset_t current_offset = 8;
	unsigned i = 0;
	for(auto& file : files)
	{
		Dumper::Entry file_entry("Entry", i + 1, current_offset, 8);
		file_entry.AddField("Name", Dumper::StringDisplay::Make(), file.name);
		file_entry.AddField("Offset", Dumper::HexDisplay::Make(8), current_offset + 60);
		file_entry.AddField("Length", Dumper::HexDisplay::Make(8), file.size);
		// TODO: other fields
		file_entry.Display(dump);
		current_offset += 60 + file.size;
		i++;
	}

	for(auto& file : files)
	{
		if(Linker::Format * format = dynamic_cast<Linker::Format *>(file.contents.get()))
		{
			format->Dump(dump);
		}
		else
		{
			// TODO
		}
	}
}

void ArchiveFormat::GenerateModule(Linker::ModuleCollector& linker, std::string file_name, bool is_library) const
{
	//is_library = true;
	for(auto& entry : files)
	{
		if(entry.name == "/" || entry.name == "//")
			continue;

		if(const std::shared_ptr<Linker::InputFormat> input_format = std::dynamic_pointer_cast<Linker::InputFormat>(entry.contents))
		{
			input_format->GenerateModule(linker, file_name + ":" + entry.name, true);
		}
		else
		{
			Linker::Error << "Error: unable to process module " << entry.name << " from archive " << file_name << ", ignoring" << std::endl;
		}
	}
}

void ArchiveFormat::CalculateValues()
{
	// TODO
}

