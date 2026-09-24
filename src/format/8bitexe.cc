
#include <filesystem>
#include <sstream>
#include "8bitexe.h"
#include "../linker/position.h"
#include "../linker/resolution.h"

using namespace Binary;

// AppleFormat

void AppleFormat::ReadFile(Linker::Reader& rd)
{
	// the reader assumes there is a DOS 3.3 header, otherwise a different parser could be used

	Clear();

	rd.endiantype = ::LittleEndian;
	dos33_header = true;
	base_address = rd.ReadUnsigned(2);
	uint16_t size = rd.ReadUnsigned(2);
	image = Linker::Buffer::ReadFromFile(rd, size);
}

offset_t AppleFormat::WriteFile(Linker::Writer& wr) const
{
	// the writer works as a raw binary writer, including the DOS 3.3 header, or adding a data fork

	wr.endiantype = ::LittleEndian;
	if(dos33_header)
	{
		wr.WriteWord(2, base_address);
		wr.WriteWord(2, image->ImageSize());
	}
	image->WriteFile(wr);
	return offset_t(-1);
}

void AppleFormat::Dump(Dumper::Dumper& dump) const
{
	if(!dos33_header)
	{
		GenericBinaryFormat::Dump(dump);
		return;
	}

	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("Apple 8-bit format DOS 3.3 header"); // TODO: only if dos33_header is true
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 4);
	file_region.Display(dump, Dumper::Header);

	// TODO
}

// SOSFormat

offset_t SOSFormat::ImageSize() const
{
	return 14 + image->ImageSize() + (optional_header ? optional_header->ImageSize() : 0);
}

void SOSFormat::ReadFile(Linker::Reader& rd)
{
	Clear();

	rd.endiantype = ::LittleEndian;
	rd.Skip(8); // label
	uint16_t opt_header_length = rd.ReadUnsigned(2);
	if(opt_header_length != 0)
	{
		optional_header = Linker::Buffer::ReadFromFile(rd, opt_header_length);
	}
	else
	{
		optional_header = nullptr;
	}
	base_address = rd.ReadUnsigned(2);
	uint16_t size = rd.ReadUnsigned(2);
	image = Linker::Buffer::ReadFromFile(rd, size);
}

offset_t SOSFormat::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::LittleEndian;
	wr.WriteData(8, "SOS NTRP");
	if(optional_header != nullptr)
	{
		wr.WriteWord(2, optional_header->ImageSize());
		optional_header->WriteFile(wr);
	}
	else
	{
		wr.WriteWord(2, 0);
	}
	wr.WriteWord(2, base_address);
	wr.WriteWord(2, image->ImageSize());
	image->WriteFile(wr);
	return ImageSize();
}

void SOSFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("Apple 8-bit SOS format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 4);
	file_region.Display(dump, Dumper::Header);

	// TODO
}

// AppleDriver

void AppleDriver::SetAppleSingleDoubleVersion(offset_t version)
{
	switch(version)
	{
	case 1:
		apple_single_double_version = 1;
		home_file_system = Apple::AppleSingleDouble::HFS_ProDOS;
		break;
	case 2:
		apple_single_double_version = 2;
		home_file_system = Apple::AppleSingleDouble::HFS_UNDEFINED;
		break;
	}
}

std::shared_ptr<Linker::OptionCollector> AppleDriver::GetOptions()
{
	return std::make_shared<DriverOptionCollector>();
}

void AppleDriver::SetOptions(std::map<std::string, std::string>& options)
{
	DriverOptionCollector collector;
	collector.ConsiderOptions(options);

	offset_t asdver = 0;

	if(std::optional<offset_t> option = collector.asver())
	{
		options.erase(collector.asver.name);
		switch(*option)
		{
		case 1:
		case 2:
			asdver = *option;
			break;
		default:
			Linker::Error << "Error: invalid AppleSingle/AppleDouble version: " << std::dec << *option << std::endl;
		}
	}

	if(auto option = collector.adver())
	{
		options.erase(collector.adver.name);
		switch(*option)
		{
		case 1:
		case 2:
			if(asdver == 0)
			{
				asdver = *option;
			}
			else if(asdver != *option)
			{
				Linker::Error << "Error: `asver' and `adver' have been provided with unequivalent values" << std::endl;
			}
			break;
		default:
			Linker::Error << "Error: invalid AppleSingle/AppleDouble version: " << std::dec << *option << std::endl;
		}
	}

	if(asdver != 0)
	{
		SetAppleSingleDoubleVersion(asdver);
	}

	//this->options = options; // TODO
}

void AppleDriver::ReadFile(Linker::Reader& rd)
{
	// reading an Apple ][ executable cannot be done via its resource fork
	if(target == OutputDriver::TARGET_RESOURCE_FORK)
	{
		Linker::FatalError("Fatal error: Reading the specified format is not supported");
	}
	GSOutputDriver::ReadFile(rd);
}

void AppleDriver::GenerateFile(std::string filename, Linker::Module& module)
{
	uint64_t default_base_address;
	switch(file_type)
	{
	case FILE_TYPE_BIN:
		default_base_address = 0x0803;
		break;
	case FILE_TYPE_SYS:
		default_base_address = 0x2000;
		break;
	case FILE_TYPE_SOS:
		default_base_address = 0x9000; // TODO: not sure
		break;
	default:
		// no current meaning assigned to this case
		default_base_address = 0;
		break;
	}

	std::shared_ptr<AppleFormat> bin;
	std::shared_ptr<SOSFormat> sos;

	if(file_type != FILE_TYPE_SOS)
	{
		data_fork = bin = std::make_shared<AppleFormat>(default_base_address, "", UseDOS33Header());
	}
	else
	{
		data_fork = sos = std::make_shared<SOSFormat>(default_base_address, "");
	}

	data_fork->ProcessModule(module);

	GenerateFiles(filename, data_fork, nullptr, GetFileType(), GetAuxiliaryFileType());
}

void AppleDriver::OnContainerCreated()
{
	apple_single->SetProDOSAccess(0xC3); // read/write/rename/delete
	apple_single->SetProDOSFileType(GetFileType());
	apple_single->SetProDOSAuxiliaryType(GetAuxiliaryFileType());
}

void AppleDriver::OnCalculateValues()
{
	data_fork->CalculateValues();
}

void AppleDriver::OnReadFile(Linker::Reader& rd)
{
	if(target == OutputDriver::TARGET_DATA_FORK)
	{
		// TODO: read with DOS 3.3 header
	}
	else
	{
		Linker::FatalError("Fatal error: Reading the specified format is not supported");
	}
}

offset_t AppleDriver::OnWriteFile(Linker::Writer& wr) const
{
	return data_fork->WriteFile(wr);
}

void AppleDriver::OnDump(Dumper::Dumper& dump) const
{
	data_fork->Dump(dump);
}

uint8_t AppleDriver::GetFileType() const
{
	return file_type;
}

uint16_t AppleDriver::GetAuxiliaryFileType() const
{
	switch(file_type)
	{
	case FILE_TYPE_BIN:
		if(!data_fork)
		{
			// unusual behavior
			return 0;
		}
		return std::dynamic_pointer_cast<AppleFormat>(data_fork)->base_address;
	case FILE_TYPE_SYS:
		return 0x2000;
	case FILE_TYPE_SOS:
		return 0; // TODO
	default:
		// unspecified
		return 0;
	}
}

std::string AppleDriver::GetDefaultExtension(Linker::Module& module, std::string filename) const
{
	if(target == OutputDriver::TARGET_APPLE_SINGLE)
	{
		return filename + ".as"; // for CiderPress
	}
	else
	{
		return filename;
	}
}

void AppleDriver::Dump(Dumper::Dumper& dump) const
{
	// TODO
}

// AtariFormat

offset_t AtariFormat::Segment::GetSize() const
{
	return image->ImageSize();
}

bool AtariFormat::HasEntryPoint() const
{
	for(auto& segment : segments)
	{
		if(segment->address <= ENTRY_ADDRESS && ENTRY_ADDRESS + 1 < segment->address + segment->GetSize())
			return true;
	}
	return false;
}

void AtariFormat::AddEntryPoint(uint16_t entry)
{
	std::unique_ptr<Segment> entry_segment = std::make_unique<Segment>();
	entry_segment->address = ENTRY_ADDRESS;
	std::shared_ptr<Linker::Section> entry_section = std::make_shared<Linker::Section>(".entry");
	entry_section->WriteWord(2, entry, ::LittleEndian);
	entry_segment->image = entry_section;
	segments.push_back(std::move(entry_segment));
}

void AtariFormat::Segment::ReadFile(Linker::Reader& rd)
{
	uint16_t word = rd.ReadUnsigned(2);
	if(word >= SIGNATURE_LOW)
	{
		header_type = segment_type(word);
		header_type_optional = false;
		if(header_type == ATARI_SEGMENT || header_type == SDX_FIXED)
			address = rd.ReadUnsigned(2);
	}
	else
	{
		header_type = ATARI_SEGMENT;
		header_type_optional = true;
		address = word;
	}
	switch(header_type)
	{
	case SDX_SYMREQ:
		block_number = rd.ReadUnsigned(1);
		rd.ReadData(8, symbol_name);
		address = rd.ReadUnsigned(2);
		break;
	case SDX_SYMDEF:
		rd.ReadData(8, symbol_name);
		ReadRelocations(rd);
		break;
	case SDX_FIXUPS:
		block_number = rd.ReadUnsigned(1);
		ReadRelocations(rd);
		break;
	case SDX_RAMALLOC:
	//case SDX_POSIND:
		block_number = rd.ReadUnsigned(1);
		control_byte = control_byte_type(rd.ReadUnsigned(1));
		address = rd.ReadUnsigned(2);
		size = rd.ReadUnsigned(2);
		if((control_byte & CB_RAMALLOC) == 0)
		{
			image = Linker::Buffer::ReadFromFile(rd, size);
		}
		break;
	case SDX_FIXED:
	case ATARI_SEGMENT:
		size = (rd.ReadUnsigned(2) + 1 - address) & 0xFFFF;
		image = Linker::Buffer::ReadFromFile(rd, size);
		break;
	default:
		Linker::FatalError("Fatal error: Invalid segment type");
	}
}

void AtariFormat::Segment::WriteFile(Linker::Writer& wr) const
{
	if(uint32_t(address) + GetSize() > 0x10000)
	{
		Linker::Warning << "Warning: Address overflows" << std::endl;
	}
	switch(header_type)
	{
	case SDX_SYMREQ:
		wr.WriteWord(2, header_type);
		wr.WriteWord(1, block_number);
		wr.WriteData(8, symbol_name);
		wr.WriteWord(2, address);
		break;
	case SDX_SYMDEF:
		wr.WriteWord(2, header_type);
		wr.WriteData(8, symbol_name);
		WriteRelocations(wr);
		break;
	case SDX_FIXUPS:
		wr.WriteWord(2, header_type);
		wr.WriteWord(1, block_number);
		WriteRelocations(wr);
		break;
	case SDX_RAMALLOC:
	//case SDX_POSIND:
		wr.WriteWord(2, header_type);
		wr.WriteWord(1, block_number);
		wr.WriteWord(1, control_byte);
		wr.WriteWord(2, address); // TODO: is this the right field?
		wr.WriteWord(2, size);
		if((control_byte & CB_RAMALLOC) == 0)
		{
			image->WriteFile(wr);
		}
		break;
	case SDX_FIXED:
	case ATARI_SEGMENT:
		if(!header_type_optional || header_type != ATARI_SEGMENT)
		{
			wr.WriteWord(2, header_type);
		}
		wr.WriteWord(2, address);
		wr.WriteWord(2, address + GetSize() - 1);
		image->WriteFile(wr);
		break;
	default:
		Linker::FatalError("Fatal error: Invalid segment type");
	}
}

void AtariFormat::Segment::ReadRelocations(Linker::Reader& rd)
{
	// TODO
}

void AtariFormat::Segment::WriteRelocations(Linker::Writer& wr) const
{
	// TODO
}

void AtariFormat::OnNewSegment(std::shared_ptr<Linker::Segment> segment)
{
	std::unique_ptr<Segment> atari_segment = std::make_unique<Segment>(); /* TODO: header type, for now we set it to the default value 0xFFFF */
	segment->Fill();
	atari_segment->address = segment->base_address;
	atari_segment->image = segment;
	if(segments.size() == 0)
		atari_segment->header_type_optional = false;
	segments.push_back(std::move(atari_segment));
}

void AtariFormat::ProcessModule(Linker::Module& module)
{
	GenericBinaryFormat::ProcessModule(module);
	Linker::Location entry;
	if(module.FindGlobalSymbol(".entry", entry))
	{
		AddEntryPoint(entry.GetPosition().address);
	}
	else if(!HasEntryPoint())
	{
		Linker::Warning << "Warning: no entry point has been provided" << std::endl;
	}

	/* TODO: enable multiple segments */
}

void AtariFormat::ReadFile(Linker::Reader& rd)
{
	offset_t end = rd.GetImageEnd();
	uint16_t signature = rd.ReadUnsigned(2);
	if(signature < Segment::segment_type::SIGNATURE_LOW)
	{
		Linker::FatalError("Fatal error: Expected binary image to start with 0xFFFF or valid SpartaDOS X signature");
	}
	rd.Seek(0);
	while(rd.Tell() < end)
	{
		std::unique_ptr<Segment> segment = std::make_unique<Segment>();
		segment->ReadFile(rd);
		segments.push_back(std::move(segment));
	}
}

offset_t AtariFormat::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::LittleEndian;
	for(auto& segment : segments)
	{
		segment->WriteFile(wr);
	}
	return offset_t(-1);
}

void AtariFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("Atari 8-bit format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 4);
	file_region.Display(dump, Dumper::Header);

	// TODO
}

// CommodoreFormat::BASICLine

void CommodoreFormat::BASICLine::AddToken(Token token)
{
	tokens.push_back(token);
}

void CommodoreFormat::BASICLine::AddString(std::string text)
{
	size_t old_length = tokens.size();
	tokens.insert(tokens.end(), text.begin(), text.end());
	static const std::vector<std::tuple<uint8_t, uint8_t>> basic_token_replacements =
	{
		{ '+', _PLUS },
		{ '-', _MINUS },
		{ '*', _MULTIPLY },
		{ '/', _DIVIDE },
		{ '^', _POWER },
		{ '>', _GREATER },
		{ '=', _EQUAL },
		{ '<', _LESS },
	};
	for(auto replacement : basic_token_replacements)
	{
		std::replace(tokens.begin() + old_length, tokens.end(), std::get<0>(replacement), std::get<1>(replacement));
	}
}

void CommodoreFormat::BASICLine::AddDecimal(int value)
{
	std::ostringstream oss;
	oss << value;
	AddString(oss.str());
}

size_t CommodoreFormat::BASICLine::ImageSize() const
{
	return tokens.size() + 5;
}

void CommodoreFormat::BASICLine::ReadFile(Linker::Reader& rd)
{
	tokens.clear();

	next_address = rd.ReadUnsigned(2);
	if(next_address == 0)
	{
		line_number = 0;
		return;
	}
	line_number = rd.ReadUnsigned(2);
	AddString(rd.ReadASCIIZ());
}

offset_t CommodoreFormat::BASICLine::WriteFile(Linker::Writer& wr) const
{
	wr.WriteWord(2, next_address);
	if(next_address != 0)
	{
		wr.WriteWord(2, line_number);
		wr.WriteData(tokens);
		wr.WriteWord(1, 0);
	}
	return ImageSize();
}

void CommodoreFormat::BASICLine::CalculateValues()
{
	next_address = line_address + ImageSize();
}

void CommodoreFormat::BASICLine::Dump(Dumper::Dumper& dump) const
{
	Dump(dump, {}, Dumper::Image);
}

void CommodoreFormat::BASICLine::Dump(Dumper::Dumper& dump, std::optional<uint16_t> line_index, int display_flags) const
{
	Dumper::Region line_region("Line", line_address, ImageSize(), 4);
	if(line_index)
	{
		line_region.InsertField(0, "Index", Dumper::DecDisplay::Make(), offset_t(*line_index + 1));
	}
	line_region.AddField("Address of next line", Dumper::HexDisplay::Make(4), offset_t(next_address)); // TODO: might be redundant
	line_region.AddField("Line number", Dumper::DecDisplay::Make(), offset_t(line_number));

	static const std::map<uint8_t, std::string> token_texts =
	{
		{ END, "END" },
		{ FOR, "FOR" },
		{ NEXT, "NEXT" },
		{ DATA, "DATA" },
		{ INPUT_HASH, "INPUT#" },
		{ INPUT, "INPUT" },
		{ DIM, "DIM" },
		{ READ, "READ" },
		{ LET, "LET" },
		{ GOTO, "GOTO" },
		{ RUN, "RUN" },
		{ IF, "IF" },
		{ RESTORE, "RESTORE" },
		{ GOSUB, "GOSUB" },
		{ RETURN, "RETURN" },
		{ REM, "REM" },
		{ STOP, "STOP" },
		{ ON, "ON" },
		{ WAIT, "WAIT" },
		{ LOAD, "LOAD" },
		{ SAVE, "SAVE" },
		{ VERIFY, "VERIFY" },
		{ DEF, "DEF" },
		{ POKE, "POKE" },
		{ PRINT_HASH, "PRINT#" },
		{ PRINT, "PRINT" },
		{ CONST, "CONST" },
		{ LIST, "LIST" },
		{ CLR, "CLR" },
		{ CMD, "CMD" },
		{ SYS, "SYS" },
		{ OPEN, "OPEN" },
		{ CLOSE, "CLOSE" },
		{ GET, "GET" },
		{ NEW, "NEW" },
		{ TAB_PAREN, "TAB_PAREN" },
		{ TO, "TO" },
		{ FN, "FN" },
		{ SPC_PAREN, "SPC(" },
		{ THEN, "THEN" },
		{ NOT, "NOT" },
		{ STEP, "STEP" },
		{ _PLUS, "+" },
		{ _MINUS, "-" },
		{ _MULTIPLY, "*" },
		{ _DIVIDE, "/" },
		{ _POWER, "^" },
		{ AND, "AND" },
		{ OR, "OR" },
		{ _GREATER, ">" },
		{ _EQUAL, "=" },
		{ _LESS, "<" },
		{ SGN, "SGN" },
		{ INT, "INT" },
		{ ABS, "ABS" },
		{ USR, "USR" },
		{ FRE, "FRE" },
		{ POS, "POS" },
		{ SQR, "SQR" },
		{ RND, "RND" },
		{ LOG, "LOG" },
		{ EXP, "EXP" },
		{ COS, "COS" },
		{ SIN, "SIN" },
		{ TAN, "TAN" },
		{ ATN, "ATN" },
		{ PEEK, "PEEK" },
		{ LEN, "LEN" },
		{ STR_DOLLAR, "STR$" },
		{ VAL, "VAL" },
		{ ASC, "ASC" },
		{ CHR_DOLLAR, "CHR$" },
		{ LEFT_DOLLAR, "LEFT$" },
		{ RIGHT_DOLLAR, "RIGHT$" },
		{ MID_DOLLAR, "MID$" },
		{ GO, "GO" },
		{ _PI, "π" },
	};

	Dumper::RichText line_text;
	for(auto token : tokens)
	{
		if(' ' <= token && token <= '~')
		{
			line_text += char(token);
		}
		else
		{
			auto token_iter = token_texts.find(token);
			if(token_iter != token_texts.end())
			{
				line_text += Dumper::RichText(token_iter->second, Dumper::RichText::Bold);
			}
			else
			{
				std::ostringstream oss;
				oss << "$" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << int(token);
				line_text += Dumper::RichText(oss.str(), Dumper::RichText::Bold);
			}
		}
	}

	line_region.AddField("Line", Dumper::RichTextDisplay::Make("'"), line_text);
	line_region.Display(dump, display_flags);
}

// CommodoreFormat::BASICFile

void CommodoreFormat::BASICFile::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = ::LittleEndian;
	uint16_t current_address = load_address;
	while(true)
	{
		BASICLine line;
		line.line_address = current_address;
		line.ReadFile(rd);
		if(line.next_address == 0)
		{
			break;
		}
		lines.push_back(line);
		ssize_t difference = ssize_t(line.next_address) - ssize_t(current_address + line.ImageSize());
		current_address = line.next_address;
		rd.Skip(difference);
	}
	end_address = current_address + 2;
}

offset_t CommodoreFormat::BASICFile::ImageSize() const
{
	uint16_t total_size = 0;
	for(auto& line : lines)
	{
		total_size += line.ImageSize();
	}
	return total_size + 2;
}

offset_t CommodoreFormat::BASICFile::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::LittleEndian;
	uint16_t total_size = 0;
	for(auto& line : lines)
	{
		total_size += line.WriteFile(wr);
	}
	wr.WriteWord(2, 0);
	return total_size + 2;
}

void CommodoreFormat::BASICFile::CalculateValues()
{
	uint16_t current_address = load_address;
	if(lines.size() == 0)
	{
		Linker::Warning << "Warning: Empty BASIC file" << std::endl;
	}

	for(auto& line : lines)
	{
		line.line_address = current_address;
		line.CalculateValues();
		current_address = line.next_address;
	}

	end_address = current_address + 2;
}

void CommodoreFormat::BASICFile::Dump(Dumper::Dumper& dump) const
{
	Dump(dump, Dumper::Image);
}

void CommodoreFormat::BASICFile::Dump(Dumper::Dumper& dump, int display_flags) const
{
	Dumper::Block basic_block("BASIC file", 2, std::const_pointer_cast<Linker::Image>(AsImage()), load_address, 4, 4);
	basic_block.Display(dump, display_flags);
	uint16_t line_index = 0;
	for(auto& line : lines)
	{
		line.Dump(dump, line_index, display_flags);
		line_index ++;
	}
}

// CommodoreFormat

void CommodoreFormat::Clear()
{
	loader = nullptr;
}

uint16_t CommodoreFormat::GetLoadAddress() const
{
	if(auto basic_file = std::dynamic_pointer_cast<const BASICFile>(loader))
	{
		return basic_file->load_address;
	}
	else if(auto segment = std::dynamic_pointer_cast<const Linker::Segment>(loader))
	{
		return segment->base_address;
	}
	else
	{
		Linker::Error << "Internal error: invalid Commodore .PRG file loader section" << std::endl;
		return 0;
	}
}

void CommodoreFormat::SetupDefaultLoader()
{
	std::shared_ptr<BASICFile> loader_section = std::make_shared<BASICFile>();
	BASICLine line;
	line.line_number = 10;
	line.AddToken(BASICLine::SYS);
	line.AddDecimal(base_address);
	loader_section->lines.push_back(line);
	if(load_address + line.ImageSize() > base_address)
	{
		Linker::Warning << "Warning: base address too low, adjusting load address to 0x" << std::hex << load_address << std::endl;
		load_address = base_address - line.ImageSize();
	}
	loader_section->load_address = load_address;
	loader_section->CalculateValues();
	loader = loader_section;
}

void CommodoreFormat::ProcessModule(Linker::Module& module)
{
	load_address = C64_BASIC_START; // TODO: make configurable
	GenericBinaryFormat::ProcessModule(module);
	if(loader == nullptr)
	{
		SetupDefaultLoader(); /* TODO: if a separate loader is ready, use that instead */
	}
}

uint16_t CommodoreFormat::GetImagePaddingSize() const
{
	if(base_address >= load_address + loader->ImageSize())
	{
		return base_address - (load_address + loader->ImageSize());
	}
	else
	{
		Linker::Error << "Error: image address begins before BASIC load address" << std::endl;
		return 0;
	}
}

void CommodoreFormat::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = ::LittleEndian;

	std::shared_ptr<BASICFile> loader_section = std::make_shared<BASICFile>();
	loader_section->load_address = load_address = rd.ReadUnsigned(2);
	loader_section->ReadFile(rd);
	loader = loader_section;

	base_address = load_address + loader_section->ImageSize();
	image = Linker::Buffer::ReadFromFile(rd);
}

void CommodoreFormat::CalculateValues()
{
	GenericBinaryFormat::CalculateValues();
	load_address = GetLoadAddress();
}

offset_t CommodoreFormat::ImageSize() const
{
	return 2 + loader->ImageSize() + GetImagePaddingSize() + image->ImageSize();
}

offset_t CommodoreFormat::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::LittleEndian;
	wr.WriteWord(2, load_address);
	loader->WriteFile(wr);
	wr.Skip(GetImagePaddingSize());
	image->WriteFile(wr);
	return ImageSize();
}

void CommodoreFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("Commodore 8-bit format");
	Dumper::Region file_region("File", file_offset, ImageSize(), 4);
	file_region.Display(dump, Dumper::Header);

	bool binary_blob_present = image != nullptr && image->ImageSize() != 0;
	int loader_options = binary_blob_present ? Dumper::Miscellaneous : Dumper::Image;
	if(auto basic_file = std::dynamic_pointer_cast<const BASICFile>(loader))
	{
		basic_file->Dump(dump, loader_options);
	}
	else
	{
		Dumper::Block loader_block("Loader", 2, loader->AsImage(), load_address, 4, 4);
		loader_block.Display(dump, loader_options);
	}

	if(binary_blob_present)
	{
		Dumper::Block image_block("Image", 2 + loader->ImageSize() + GetImagePaddingSize(), image->AsImage(), load_address + GetImagePaddingSize(), 4, 4);
		image_block.Display(dump, Dumper::Image);
	}
}

std::string CommodoreFormat::GetDefaultExtension(Linker::Module& module, std::string filename) const
{
	return filename + ".prg";
}

// CPM3Format

void CPM3Format::rsx_record::OpenAndPrepare()
{
	if(rsx_file_name != "")
	{
		std::ifstream rsx_file;
		rsx_file.open(rsx_file_name, std::ios_base::in | std::ios_base::binary);
		if(rsx_file.is_open())
		{
			Linker::Reader rd(::LittleEndian, &rsx_file);
			module = std::make_shared<PRLFormat>(PRLFormat::APPL_RSX);
			module->ReadFile(rd);
			rsx_file.close();

			uint8_t nonbanked_flag = module->image->AsImage()->GetByte(15);
			switch(nonbanked_flag)
			{
			case 0x00:
				nonbanked_only = false;
				break;
			case 0xFF:
				nonbanked_only = true;
				break;
			default:
				Linker::Warning << "Warning: invalid nonbank flag in RSX file, pretending to be 0" << std::endl;
				break;
			}
		}
		else
		{
			Linker::Error << "Error: unable to open RSX file " << rsx_file_name << ", generating dummy entry" << std::endl;
		}
	}
}

void CPM3Format::Clear()
{
	rsx_table.clear();
}

std::shared_ptr<Linker::OptionCollector> CPM3Format::GetOptions()
{
	return std::make_shared<CPM3OptionCollector>();
}

void CPM3Format::SetOptions(std::map<std::string, std::string>& options)
{
	CPM3OptionCollector collector;
	collector.ConsiderOptions(options);

	if(auto rsx_file_names = collector.rsx_file_names())
	{
		for(auto rsx_file_name : rsx_file_names.value())
		{
			rsx_table.push_back(rsx_record());
			rsx_table.back().rsx_file_name = rsx_file_name;
		}
	}

	for(auto& rsx : rsx_table)
	{
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

		rsx.name.resize(8, ' ');
		std::transform(rsx.name.begin(), rsx.name.end(), rsx.name.begin(), ::toupper);
	}
}

void CPM3Format::ReadFile(Linker::Reader& rd)
{
	Clear();

	rd.endiantype = ::LittleEndian;
	rd.Skip(1);
	uint16_t data_size = rd.ReadUnsigned(2);
	rd.ReadData(10, preinit_code);
	loader_active = rd.ReadUnsigned(1) != 0;
	rd.Skip(1);
	uint8_t rsx_count = rd.ReadUnsigned(1);
	for(int i = 0; i < rsx_count; i++)
	{
		rsx_record rsx;
		rsx.offset = rd.ReadUnsigned(2);
		rsx.length = rd.ReadUnsigned(2);
		rsx.nonbanked_only = rd.ReadUnsigned(1) != 0;
		rd.Skip(1);
		rsx.name = rd.ReadData(8, true);
		rd.Skip(2);
		rsx_table.push_back(rsx);
	}
	rd.Seek(0x100);
	std::shared_ptr<Linker::Buffer> buffer = std::make_shared<Linker::Section>(".code");
	image = buffer;
	buffer->ReadFile(rd, data_size);
	for(auto& rsx : rsx_table)
	{
		rd.Seek(rsx.offset);
		std::shared_ptr<Linker::Buffer> buffer = std::make_shared<Linker::Section>(".code");
		rsx.module->image = buffer;
		buffer->ReadFile(rd, rsx.length);
	}
}

offset_t CPM3Format::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::LittleEndian;
	wr.WriteWord(1, 0xC9);
	wr.WriteWord(2, image->ImageSize());
	wr.WriteData(10, preinit_code);
	wr.WriteWord(1, rsx_table.size() == 0 && loader_active ? 0xFF : 0);
	wr.Skip(1);
	wr.WriteWord(1, rsx_table.size());
	for(auto& rsx : rsx_table)
	{
		wr.WriteWord(2, rsx.offset);
		wr.WriteWord(2, rsx.module->image->ImageSize());
		wr.WriteWord(1, rsx.nonbanked_only ? 0xFF : 0);
		wr.Skip(1);
		wr.WriteData(8, rsx.name);
		wr.Skip(2);
	}
	wr.Seek(0x100);
	image->WriteFile(wr);
	for(auto& rsx : rsx_table)
	{
		wr.Seek(rsx.offset);
		rsx.module->WriteWithoutHeader(wr);
	}
	return offset_t(-1);
}

void CPM3Format::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("CP/M Plus format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 4);
	file_region.Display(dump, Dumper::Header);

	// TODO
}

void CPM3Format::CalculateValues()
{
	uint16_t offset = 0x100 + image->ImageSize();
	for(auto& rsx : rsx_table)
	{
		rsx.offset = offset;
		rsx.OpenAndPrepare();
		offset += rsx.module->image->ImageSize() + ((rsx.module->image->ImageSize() + 7) >> 3);
	}
}

// FLEXFormat

void FLEXFormat::Segment::WriteFile(Linker::Writer& wr) const
{
	for(uint16_t offset = 0; offset < image->ImageSize(); offset += 0xFF)
	{
		/* cut the segment up into 255 byte morcels */
		wr.WriteWord(1, 0x02);
		wr.WriteWord(2, address + offset);
		uint16_t count = std::min(offset_t(0xFF), image->ImageSize() - address - offset);
		wr.WriteWord(1, count);
		image->WriteFile(wr, count, offset);
	}
}

void FLEXFormat::OnNewSegment(std::shared_ptr<Linker::Segment> segment)
{
	std::unique_ptr<Segment> flex_segment = std::make_unique<Segment>();
	segment->Fill();
	flex_segment->address = segment->base_address;
	flex_segment->image = segment;
	segments.push_back(std::move(flex_segment));
}

offset_t FLEXFormat::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::BigEndian;
	for(auto& segment : segments)
	{
		segment->WriteFile(wr);
	}
	return offset_t(-1);
}

void FLEXFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("FLEX format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 4);
	file_region.Display(dump, Dumper::Header);

	// TODO
}

std::string FLEXFormat::GetDefaultExtension(Linker::Module& module, std::string filename) const
{
	return filename + ".cmd";
}

// PRLFormat

std::shared_ptr<Linker::OptionCollector> PRLFormat::GetOptions()
{
	return std::make_shared<PRLOptionCollector>();
}

void PRLFormat::SetOptions(std::map<std::string, std::string>& options)
{
	PRLOptionCollector collector;
	collector.ConsiderOptions(options);

	option_banked_bios = collector.banked();
}

std::unique_ptr<Script::List> PRLFormat::GetScript(Linker::Module& module)
{
	if(option_banked_bios)
	{
		// separates code and data segments, align data on next 0x100 byte page

		static const char * SplitScript = R"(
".code"
{
	at ?base_address?;
	all exec;
	align 0x100; // TODO: this also expands the code segment size
	all not zero align ?section_align?;
	all not ".stack" align ?section_align?;
	all align ?section_align?;
};
)";

		return Script::parse_string(SplitScript);
	}
	else
	{
		return GenericBinaryFormat::GetScript(module);
	}
}

uint16_t PRLFormat::GetDefaultBaseAddress(application_type application)
{
	switch(application)
	{
	case APPL_UNKNOWN:
	case APPL_PRL:
	case APPL_RSX:
	case APPL_RSM:
	case APPL_RSP:
	case APPL_BRS:
	default:
		return 0x0100;
	case APPL_SPR:
		return 0x0000;
	case APPL_OVL:
		// overlays do not have a default base address
		return 0x0000;
	}
}

std::string PRLFormat::GetDefaultApplicationExtension(application_type application)
{
	switch(application)
	{
	case APPL_UNKNOWN:
	case APPL_PRL:
	default:
		return ".prl";
	case APPL_RSX:
		return ".rsx";
	case APPL_RSM:
		return ".rsm";
	case APPL_RSP:
		return ".rsp";
	case APPL_BRS:
		return ".brs";
	case APPL_SPR:
		return ".spr";
	case APPL_OVL:
		return ".ovl";
	}
}

void PRLFormat::OnNewSegment(std::shared_ptr<Linker::Segment> segment)
{
	bool is_first_segment = image == nullptr;
	GenericBinaryFormat::OnNewSegment(segment);
	if(is_first_segment)
	{
		zero_fill = segment->zero_fill;
	}
}

bool PRLFormat::ProcessRelocation(Linker::Module& module, Linker::Relocation& rel, Linker::Resolution resolution)
{
	rel.WriteWord(resolution.value);
	if(resolution.target != nullptr && resolution.reference == nullptr)
	{
		if(rel.size == 2)
		{
			Linker::Debug << "Debug: PRL relocation: " << rel << " at " <<
				rel.source.GetPosition().GetSegmentOffset() + 1
				<< std::endl;
			relocations.insert(rel.source.GetPosition().GetSegmentOffset() + 1);
		}
	}
	return true;
}

void PRLFormat::ProcessModule(Linker::Module& module)
{
	GenericBinaryFormat::ProcessModule(module);

	if(option_banked_bios)
	{
		cslen = 0;
		for(auto& section : std::dynamic_pointer_cast<Linker::Segment>(image)->sections)
		{
			if(!section->IsExecutable())
				break;
			assert(section->GetStartAddress() == cslen); // note: this will fail unless the base_address is 0
			cslen += section->Size();
		}
	}
}

void PRLFormat::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = ::LittleEndian;
	rd.Seek(1);
	uint16_t image_size = rd.ReadUnsigned(2);
	rd.Skip(1);
	zero_fill = rd.ReadUnsigned(2);
	rd.Skip(1);
	load_address = rd.ReadUnsigned(2);
	rd.Skip(1);
	cslen = rd.ReadUnsigned(2);
	rd.Seek(0x0100);
	ReadWithoutHeader(rd, image_size);
}

void PRLFormat::ReadWithoutHeader(Linker::Reader& rd, uint16_t image_size)
{
	offset_t offset = rd.Tell();
	offset_t end = rd.GetImageEnd();

	image = Linker::Buffer::ReadFromFile(rd, image_size);

	relocations.clear();
	Linker::Debug << "Debug: File end: " << end << ", expected end with relocations: " << (offset + image_size + ((image_size + 7) >> 3)) << std::endl;
	if(end >= offset + image_size + ((image_size + 7) >> 3))
	{
		suppress_relocations = false;
		for(uint16_t byte_offset = 0; byte_offset < image_size; byte_offset += 8)
		{
			uint8_t reloc_byte = rd.ReadUnsigned(1);
			for(int byte = 7; byte >= 0; byte --)
			{
				if((reloc_byte & 1) != 0 && byte_offset + byte < image_size)
				{
					relocations.insert(byte_offset + byte);
				}
				reloc_byte >>= 1;
			}
		}
	}
	else
	{
		suppress_relocations = true;
	}
}

offset_t PRLFormat::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::LittleEndian;
	wr.WriteWord(1, 0);
	wr.WriteWord(2, image->ImageSize());
	wr.WriteWord(1, 0);
	wr.WriteWord(2, zero_fill);
	wr.WriteWord(1, 0);
	wr.WriteWord(2, load_address); /* load address, non-zero only for OVL files */
	wr.WriteWord(1, 0);
	wr.WriteWord(2, cslen); /* length of code group, usually left zero, unless .SPR file on a banked system */
	wr.Seek(0x0100);
	WriteWithoutHeader(wr);
	return offset_t(-1);
}

void PRLFormat::WriteWithoutHeader(Linker::Writer& wr) const
{
	wr.endiantype = ::LittleEndian;
	image->WriteFile(wr);

	if(!suppress_relocations) /* suppress relocations only for OVL files */
	{
		Linker::Debug << "Debug: Writing relocations" << std::endl;
		for(uint16_t offset = 0; offset < ::AlignTo(image->ImageSize(), 8); offset += 8)
		{
			uint8_t reloc_byte = 0;
			for(int byte = 0; byte < 8; byte ++)
			{
				if(relocations.find(offset + byte) != relocations.end())
				{
					reloc_byte |= 1 << (7 - byte);
				}
			}
			wr.WriteWord(1, reloc_byte);
		}
	}
}

void PRLFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("PRL format");
	Dumper::Region file_region("File", 0, offset_t(0x0100) + image->ImageSize() + (suppress_relocations ? 0 : (image->ImageSize() + 7) >> 3), 4);
	file_region.AddField("Zero fill", Dumper::HexDisplay::Make(4), offset_t(zero_fill));
	file_region.AddOptionalField("Load address", Dumper::HexDisplay::Make(4), offset_t(load_address));
	file_region.AddOptionalField("Code segment length", Dumper::HexDisplay::Make(4), offset_t(cslen));
	file_region.AddField("Relocations", Dumper::ChoiceDisplay::Make("present", "missing"), offset_t(!suppress_relocations));
	file_region.Display(dump, Dumper::Header);

	// TODO: determine application type from file extension?
	Dumper::Block image_block("Image", 0x0100, image->AsImage(),
		load_address != 0 ? load_address : application == APPL_SPR ? 0x0000 : 0x0100,
		4);
	for(auto relocation : relocations)
	{
		image_block.AddSignal(relocation, 1);
	}
	image_block.Display(dump, Dumper::Image);

	unsigned i = 0;
	for(auto relocation : relocations)
	{
		Dumper::Entry relocation_entry("Relocation", i + 1, 0x100 + image->ImageSize() + (relocation >> 3), 4);
		relocation_entry.AddField("Source", Dumper::HexDisplay::Make(4), offset_t(relocation));
		relocation_entry.AddOptionalField("Addend", Dumper::HexDisplay::Make(2), offset_t(image->AsImage()->ReadUnsigned(1, relocation, ::LittleEndian)));
		relocation_entry.Display(dump, Dumper::Relocation);
		i++;
	}
}

// UZIFormat

/* TODO: base address should be 0x0103 */
void UZIFormat::ProcessModule(Linker::Module& module)
{
	GenericBinaryFormat::ProcessModule(module);
	entry = 0x0103; /* TODO: enable entry point */
}

offset_t UZIFormat::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::LittleEndian;
	wr.WriteWord(1, 0xC3);
	wr.WriteWord(2, entry);
	if(uzi180_header)
	{
		wr.WriteData("UZI");
	}
	image->WriteFile(wr);
	return offset_t(-1);
}

void UZIFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("UZI format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 4);
	file_region.Display(dump, Dumper::Header);

	// TODO
}

std::string UZIFormat::GetDefaultExtension(Linker::Module& module) const
{
	return "a.out";
}

// UZI280Format

void UZI280Format::OnNewSegment(std::shared_ptr<Linker::Segment> segment)
{
	if(segment->name == ".code")
	{
		code = segment;
	}
	else if(segment->name == ".data")
	{
		data = segment;
	}
	else
	{
		Linker::Error << "Error: unknown segment `" << segment->name << "` for format, expected `.code`, `.data`, ignoring" << std::endl;
	}
}

/* TODO: apparently both .code and .data are loaded at 0x0100 */

offset_t UZI280Format::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::LittleEndian;
	wr.WriteWord(2, 0x00FF);
	wr.WriteWord(2, data->ImageSize());
	wr.WriteWord(2, code->ImageSize());
	wr.AlignTo(512);
	data->WriteFile(wr);
	wr.AlignTo(512);
	code->WriteFile(wr);
	wr.AlignTo(512);
	return offset_t(-1);
}

void UZI280Format::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("UZI280 format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 4);
	file_region.Display(dump, Dumper::Header);

	// TODO
}

std::string UZI280Format::GetDefaultExtension(Linker::Module& module) const
{
	return "a.out";
}

