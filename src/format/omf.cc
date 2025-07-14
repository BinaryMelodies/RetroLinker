
#include <algorithm>
#include "omf.h"

/* TODO: incomplete */

using namespace OMF;

//// OMFFormat

std::string OMFFormat::ReadString(Linker::Reader& rd, size_t max_bytes)
{
	uint8_t length = rd.ReadUnsigned(1);
	return rd.ReadData(std::min(size_t(length), max_bytes));
}

void OMFFormat::WriteString(ChecksumWriter& wr, std::string text)
{
	wr.WriteWord(1, text.size());
	wr.WriteData(text);
}

OMFFormat::index_t OMFFormat::ReadIndex(Linker::Reader& rd)
{
	index_t index = rd.ReadUnsigned(1);
	if((index & 0x80))
	{
		index = ((index & 0x7F) << 8) | rd.ReadUnsigned(1);
	}
	return index;
}

void OMFFormat::WriteIndex(ChecksumWriter& wr, index_t index)
{
	if(index < 0x80)
	{
		wr.WriteWord(1, index);
	}
	else
	{
		wr.WriteWord(1, (index >> 8) | 0x80);
		wr.WriteWord(1, index);
	}
}

size_t OMFFormat::IndexSize(index_t index)
{
	if(index < 0x80)
	{
		return 1;
	}
	else
	{
		return 2;
	}
}

std::shared_ptr<OMFFormat> OMFFormat::ReadOMFFile(Linker::Reader& rd)
{
	rd.endiantype = LittleEndian;

	/* Attempts to read the first record. */

	uint8_t record_type = rd.ReadUnsigned(1);
	uint16_t record_length;
	uint8_t name_length;
	uint8_t version;

	switch(record_type)
	{
	case OMF80Format::LibraryHeader:
		/* Libraries start with a Library Header Record, optionally
		 * followed by a Module Header Record. We seek to the next
		 * record and use that to determine the file type. */
		record_length = rd.ReadUnsigned(2);
		rd.Skip(record_length);
		record_type = rd.ReadUnsigned(1);
		if(record_type != OMF80Format::ModuleHeader)
		{
			// unknown, give up and try to read it as OMF80
			rd.Seek(0);
			return OMF80Format::ReadOMFFile(rd);
		}
		// continue to next field

	case OMF80Format::ModuleHeader:
		/* The OMF80, OMF51 and OMF96 module header formats are very
		 * similar, the value of the byte after the module name string
		 * can be used to distinguish between them. */
		rd.ReadUnsigned(2);
		name_length = rd.ReadUnsigned(1);
		rd.Skip(name_length);
		version = rd.ReadUnsigned(1);
		switch(version & 0xF0)
		{
		default:
			rd.Seek(0);
			return OMF80Format::ReadOMFFile(rd);
		case 0xE0:
			rd.Seek(0);
			return OMF96Format::ReadOMFFile(rd);
		case 0xF0:
			rd.Seek(0);
			return OMF51Format::ReadOMFFile(rd);
		}

	case OMF86Format::RHEADR:
	case OMF86Format::THEADR:
	case OMF86Format::LHEADR:
	case OMF86Format::LIBHED:
	case OMF86Format::LibraryHeader:
		rd.Seek(0);
		return OMF86Format::ReadOMFFile(rd);

	default:
		// make a wild guess, based on the type of the first record
		rd.Seek(0);
		if(record_type < 0x6E)
		{
			return OMF80Format::ReadOMFFile(rd);
		}
		else
		{
			return OMF86Format::ReadOMFFile(rd);
		}
	}
}

//// OMFFormat::ContentRecord

template <typename RecordTypeByte, typename FormatType, typename ModuleType>
	void OMFFormat::ContentRecord<RecordTypeByte, FormatType, ModuleType>::ReadRecordContents(FormatType * omf, ModuleType * mod, Linker::Reader& rd)
{
	segment_id = rd.ReadUnsigned(1);
	offset = rd.ReadUnsigned(2);
	data.resize(Record<RecordTypeByte, FormatType, ModuleType>::record_length - 4);
	rd.ReadData(data);
}

template <typename RecordTypeByte, typename FormatType, typename ModuleType>
	uint16_t OMFFormat::ContentRecord<RecordTypeByte, FormatType, ModuleType>::GetRecordSize(FormatType * omf, ModuleType * mod) const
{
	return 4 + data.size();
}

template <typename RecordTypeByte, typename FormatType, typename ModuleType>
	void OMFFormat::ContentRecord<RecordTypeByte, FormatType, ModuleType>::WriteRecordContents(FormatType * omf, ModuleType * mod, ChecksumWriter& wr) const
{
	wr.WriteWord(1, segment_id);
	wr.WriteWord(2, offset);
	wr.WriteData(data);
}

//// OMFFormat::LineNumber

template <typename RecordTypeByte, typename FormatType, typename ModuleType>
	OMFFormat::LineNumbersRecord<RecordTypeByte, FormatType, ModuleType>::LineNumber OMFFormat::LineNumbersRecord<RecordTypeByte, FormatType, ModuleType>::LineNumber::Read(FormatType * omf, Linker::Reader& rd)
{
	LineNumber line_number;
	line_number.line_number = rd.ReadUnsigned(2);
	line_number.offset = rd.ReadUnsigned(2);
	return line_number;
}

template <typename RecordTypeByte, typename FormatType, typename ModuleType>
	void OMFFormat::LineNumbersRecord<RecordTypeByte, FormatType, ModuleType>::LineNumber::Write(FormatType * omf, ChecksumWriter& wr) const
{
	wr.WriteWord(2, line_number);
	wr.WriteWord(2, offset);
}

//// OMFFormat::LineNumbersRecord

template <typename RecordTypeByte, typename FormatType, typename ModuleType>
	void OMFFormat::LineNumbersRecord<RecordTypeByte, FormatType, ModuleType>::ReadRecordContents(FormatType * omf, ModuleType * mod, Linker::Reader& rd)
{
	segment_id = rd.ReadUnsigned(1);
	while(rd.Tell() < LineNumbersRecord<RecordTypeByte, FormatType, ModuleType>::RecordEnd())
	{
		line_numbers.push_back(LineNumber::Read(omf, rd));
	}
}

template <typename RecordTypeByte, typename FormatType, typename ModuleType>
	uint16_t OMFFormat::LineNumbersRecord<RecordTypeByte, FormatType, ModuleType>::GetRecordSize(FormatType * omf, ModuleType * mod) const
{
	return 1 + 4 * line_numbers.size();
}

template <typename RecordTypeByte, typename FormatType, typename ModuleType>
	void OMFFormat::LineNumbersRecord<RecordTypeByte, FormatType, ModuleType>::WriteRecordContents(FormatType * omf, ModuleType * mod, ChecksumWriter& wr) const
{
	wr.WriteWord(1, segment_id);
	for(auto& line_number : line_numbers)
	{
		line_number.Write(omf, wr);
	}
}

//// OMFFormat::LibraryHeaderRecord

template <typename RecordTypeByte, typename FormatType, typename ModuleType>
	void OMFFormat::LibraryHeaderRecord<RecordTypeByte, FormatType, ModuleType>::ReadRecordContents(FormatType * omf, ModuleType * mod, Linker::Reader& rd)
{
	module_count = rd.ReadUnsigned(2);
	block_number = rd.ReadUnsigned(2);
	byte_number = rd.ReadUnsigned(2);
}

template <typename RecordTypeByte, typename FormatType, typename ModuleType>
	uint16_t OMFFormat::LibraryHeaderRecord<RecordTypeByte, FormatType, ModuleType>::GetRecordSize(FormatType * omf, ModuleType * mod) const
{
	return 7;
}

template <typename RecordTypeByte, typename FormatType, typename ModuleType>
	void OMFFormat::LibraryHeaderRecord<RecordTypeByte, FormatType, ModuleType>::WriteRecordContents(FormatType * omf, ModuleType * mod, ChecksumWriter& wr) const
{
	wr.WriteWord(2, module_count);
	wr.WriteWord(2, block_number);
	wr.WriteWord(2, byte_number);
}

//// OMFFormat::LibraryModuleNamesRecord

template <typename RecordTypeByte, typename FormatType, typename ModuleType>
	void OMFFormat::LibraryModuleNamesRecord<RecordTypeByte, FormatType, ModuleType>::ReadRecordContents(FormatType * omf, ModuleType * mod, Linker::Reader& rd)
{
	while(rd.Tell() < LibraryModuleNamesRecord<RecordTypeByte, FormatType, ModuleType>::RecordEnd())
	{
		names.push_back(ReadString(rd));
	}
}

template <typename RecordTypeByte, typename FormatType, typename ModuleType>
	uint16_t OMFFormat::LibraryModuleNamesRecord<RecordTypeByte, FormatType, ModuleType>::GetRecordSize(FormatType * omf, ModuleType * mod) const
{
	uint16_t bytes = 1;
	for(auto& name : names)
	{
		bytes += name.size() + 1;
	}
	return bytes;
}

template <typename RecordTypeByte, typename FormatType, typename ModuleType>
	void OMFFormat::LibraryModuleNamesRecord<RecordTypeByte, FormatType, ModuleType>::WriteRecordContents(FormatType * omf, ModuleType * mod, ChecksumWriter& wr) const
{
	for(auto& name : names)
	{
		WriteString(wr, name);
	}
}

//// OMFFormat::LibraryModuleLocationsRecord::Location

template <typename RecordTypeByte, typename FormatType, typename ModuleType>
	OMFFormat::LibraryModuleLocationsRecord<RecordTypeByte, FormatType, ModuleType>::Location OMFFormat::LibraryModuleLocationsRecord<RecordTypeByte, FormatType, ModuleType>::Location::Read(Linker::Reader& rd)
{
	Location location;
	location.block_number = rd.ReadUnsigned(2);
	location.byte_number = rd.ReadUnsigned(2);
	return location;
}

template <typename RecordTypeByte, typename FormatType, typename ModuleType>
	void OMFFormat::LibraryModuleLocationsRecord<RecordTypeByte, FormatType, ModuleType>::Location::Write(ChecksumWriter& wr) const
{
	wr.WriteWord(2, block_number);
	wr.WriteWord(2, byte_number);
}

//// OMFFormat::LibraryModuleLocationsRecord

template <typename RecordTypeByte, typename FormatType, typename ModuleType>
	void OMFFormat::LibraryModuleLocationsRecord<RecordTypeByte, FormatType, ModuleType>::ReadRecordContents(FormatType * omf, ModuleType * mod, Linker::Reader& rd)
{
	while(rd.Tell() < LibraryModuleLocationsRecord<RecordTypeByte, FormatType, ModuleType>::RecordEnd())
	{
		locations.push_back(Location::Read(rd));
	}
}

template <typename RecordTypeByte, typename FormatType, typename ModuleType>
	uint16_t OMFFormat::LibraryModuleLocationsRecord<RecordTypeByte, FormatType, ModuleType>::GetRecordSize(FormatType * omf, ModuleType * mod) const
{
	return 1 + 4 * locations.size();
}

template <typename RecordTypeByte, typename FormatType, typename ModuleType>
	void OMFFormat::LibraryModuleLocationsRecord<RecordTypeByte, FormatType, ModuleType>::WriteRecordContents(FormatType * omf, ModuleType * mod, ChecksumWriter& wr) const
{
	for(auto& location : locations)
	{
		location.Write(wr);
	}
}

//// OMFFormat::LibraryDictionaryRecord

template <typename RecordTypeByte, typename FormatType, typename ModuleType>
	void OMFFormat::LibraryDictionaryRecord<RecordTypeByte, FormatType, ModuleType>::Group::Read(Linker::Reader& rd)
{
	while(true)
	{
		std::string name = ReadString(rd);
		if(name == "")
			break;
		names.push_back(name);
	}
}

template <typename RecordTypeByte, typename FormatType, typename ModuleType>
	uint16_t OMFFormat::LibraryDictionaryRecord<RecordTypeByte, FormatType, ModuleType>::Group::Size() const
{
	uint16_t bytes = 1;
	for(auto& name : names)
	{
		bytes += 1 + name.size();
	}
	return bytes;
}

template <typename RecordTypeByte, typename FormatType, typename ModuleType>
	void OMFFormat::LibraryDictionaryRecord<RecordTypeByte, FormatType, ModuleType>::Group::Write(ChecksumWriter& wr) const
{
	for(auto& name : names)
	{
		WriteString(wr, name);
	}
	WriteString(wr, "");
}

template <typename RecordTypeByte, typename FormatType, typename ModuleType>
	void OMFFormat::LibraryDictionaryRecord<RecordTypeByte, FormatType, ModuleType>::ReadRecordContents(FormatType * omf, ModuleType * mod, Linker::Reader& rd)
{
	while(rd.Tell() < LibraryDictionaryRecord<RecordTypeByte, FormatType, ModuleType>::RecordEnd())
	{
		groups.push_back(Group());
		groups.back().Read(rd);
	}
}

template <typename RecordTypeByte, typename FormatType, typename ModuleType>
	uint16_t OMFFormat::LibraryDictionaryRecord<RecordTypeByte, FormatType, ModuleType>::GetRecordSize(FormatType * omf, ModuleType * mod) const
{
	uint16_t bytes = 1;
	for(auto& group : groups)
	{
		bytes += group.Size();
	}
	return bytes;
}

template <typename RecordTypeByte, typename FormatType, typename ModuleType>
	void OMFFormat::LibraryDictionaryRecord<RecordTypeByte, FormatType, ModuleType>::WriteRecordContents(FormatType * omf, ModuleType * mod, ChecksumWriter& wr) const
{
	for(auto& group : groups)
	{
		group.Write(wr);
	}
}

//// OMF86Format::NameIndex

void OMF86Format::NameIndex::CalculateValues(OMF86Format * omf, Module * mod)
{
	auto it = std::find(mod->lnames.begin(), mod->lnames.end(), text);
	if(it != mod->lnames.end())
	{
		index = it - mod->lnames.begin() + 1;
	}
	else
	{
		mod->lnames.push_back(text);
		index = mod->lnames.size();
	}
}

void OMF86Format::NameIndex::ResolveReferences(OMF86Format * omf, Module * mod)
{
	text = mod->lnames[index - 1];
}

//// OMF86Format::SegmentIndex

void OMF86Format::SegmentIndex::CalculateValues(OMF86Format * omf, Module * mod)
{
	// TODO
}

void OMF86Format::SegmentIndex::ResolveReferences(OMF86Format * omf, Module * mod)
{
	record = std::dynamic_pointer_cast<SegmentDefinitionRecord>(omf->records[mod->first_record + index - 1]);
}

//// OMF86Format::GroupIndex

void OMF86Format::GroupIndex::CalculateValues(OMF86Format * omf, Module * mod)
{
	// TODO
}

void OMF86Format::GroupIndex::ResolveReferences(OMF86Format * omf, Module * mod)
{
	record = std::dynamic_pointer_cast<GroupDefinitionRecord>(omf->records[mod->first_record + index - 1]);
}

//// OMF86Format::TypeIndex

void OMF86Format::TypeIndex::CalculateValues(OMF86Format * omf, Module * mod)
{
	// TODO
}

void OMF86Format::TypeIndex::ResolveReferences(OMF86Format * omf, Module * mod)
{
	record = std::dynamic_pointer_cast<TypeDefinitionRecord>(omf->records[mod->first_record + index - 1]);
}

//// OMF86Format::BlockIndex

void OMF86Format::BlockIndex::CalculateValues(OMF86Format * omf, Module * mod)
{
	// TODO
}

void OMF86Format::BlockIndex::ResolveReferences(OMF86Format * omf, Module * mod)
{
	record = std::dynamic_pointer_cast<BlockDefinitionRecord>(omf->records[mod->first_record + index - 1]);
}

//// OMF86Format::OverlayIndex

void OMF86Format::OverlayIndex::CalculateValues(OMF86Format * omf, Module * mod)
{
	// TODO
}

void OMF86Format::OverlayIndex::ResolveReferences(OMF86Format * omf, Module * mod)
{
	record = std::dynamic_pointer_cast<OverlayDefinitionRecord>(omf->records[mod->first_record + index - 1]);
}

//// OMF86Format::ExternalIndex

void OMF86Format::ExternalIndex::CalculateValues(OMF86Format * omf, Module * mod)
{
	name.CalculateValues(omf, mod);
	// TODO
}

void OMF86Format::ExternalIndex::ResolveReferences(OMF86Format * omf, Module * mod)
{
	name = mod->extdefs[index - 1];
	name.ResolveReferences(omf, mod);
}

//// OMF86Format::ExternalName

uint32_t OMF86Format::ExternalName::ReadValue(OMF86Format * omf, Linker::Reader& rd)
{
	uint8_t value = rd.ReadUnsigned(1);
	switch(value)
	{
	case 0x81:
		return rd.ReadUnsigned(2);
	case 0x84:
		return rd.ReadUnsigned(3);
	case 0x88:
		return rd.ReadUnsigned(4);
	default:
		if(value <= 0x80)
			return value;
		// otherwise, error
	}
	return 0;
}

uint32_t OMF86Format::ExternalName::ValueSize(OMF86Format * omf, uint32_t length)
{
	if(length <= 0x80)
		return 1;
	else if(length < 0x10000)
		return 3;
	else if(length < 0x1000000)
		return 4;
	else
		return 5;
}

void OMF86Format::ExternalName::WriteValue(OMF86Format * omf, ChecksumWriter& wr, uint32_t length)
{
	if(length <= 0x80)
	{
		wr.WriteWord(1, length);
	}
	else if(length < 0x10000)
	{
		wr.WriteWord(1, 0x81);
		wr.WriteWord(2, length);
	}
	else if(length < 0x1000000)
	{
		wr.WriteWord(1, 0x84);
		wr.WriteWord(3, length);
	}
	else
	{
		wr.WriteWord(1, 0x88);
		wr.WriteWord(4, length);
	}
}

OMF86Format::ExternalName OMF86Format::ExternalName::ReadExternalName(OMF86Format * omf, Linker::Reader& rd, bool local)
{
	ExternalName extname;
	extname.local = local;
	extname.name_is_index = false;
	extname.name.text = ReadString(rd);
	extname.type.index = ReadIndex(rd);
	return extname;
}

OMF86Format::ExternalName OMF86Format::ExternalName::ReadCommonName(OMF86Format * omf, Linker::Reader& rd, bool local)
{
	ExternalName extname;
	extname.local = local;
	extname.name_is_index = false;
	extname.name.text = ReadString(rd);
	extname.type.index = ReadIndex(rd);
	uint8_t data_type = rd.ReadUnsigned(1);

	switch(data_type)
	{
	case FarCommon:
		extname.common_type = FarCommon;
		extname.value.far.number = ReadValue(omf, rd);
		extname.value.far.element_size = ReadValue(omf, rd);
		break;
	case NearCommon:
		extname.common_type = NearCommon;
		extname.value.near.length = ReadValue(omf, rd);
		break;
	default:
		if(1 <= data_type && data_type <= 0x5F)
		{
			extname.common_type = SegmentIndexCommon;
			extname.value.segment_index = data_type;
		}
		break;
		// TODO: otherwise, error
	}

	return extname;
}

OMF86Format::ExternalName OMF86Format::ExternalName::ReadComdatExternalName(OMF86Format * omf, Linker::Reader& rd)
{
	ExternalName extname;
	extname.local = false;
	extname.name_is_index = true;
	extname.name = NameIndex(ReadIndex(rd));
	extname.type.index = ReadIndex(rd);
	return extname;
}

uint16_t OMF86Format::ExternalName::GetExternalNameSize(OMF86Format * omf) const
{
	uint16_t bytes = 1 + IndexSize(type.index);
	if(name_is_index)
	{
		bytes += IndexSize(name.index);
	}
	else
	{
		bytes += 1 + name.text.size();
	}
	switch(common_type)
	{
	case External:
		break;
	case SegmentIndexCommon:
		bytes += 1;
		break;
	case FarCommon:
		bytes += 1 + ValueSize(omf, value.far.number) + ValueSize(omf, value.far.element_size);
		break;
	case NearCommon:
		bytes += 1 + ValueSize(omf, value.near.length);
		break;
	}
	return bytes;
}

void OMF86Format::ExternalName::WriteExternalName(OMF86Format * omf, ChecksumWriter& wr) const
{
	if(name_is_index)
	{
		WriteIndex(wr, name.index);
	}
	else
	{
		WriteString(wr, name.text);
	}
	WriteIndex(wr, type.index);
	switch(common_type)
	{
	case External:
		break;
	case SegmentIndexCommon:
		wr.WriteWord(1, value.segment_index);
		break;
	case FarCommon:
		wr.WriteWord(1, common_type);
		WriteValue(omf, wr, value.far.number);
		WriteValue(omf, wr, value.far.element_size);
		break;
	case NearCommon:
		wr.WriteWord(1, common_type);
		WriteValue(omf, wr, value.near.length);
		break;
	}
}

void OMF86Format::ExternalName::CalculateValues(OMF86Format * omf, Module * mod)
{
	if(name_is_index)
	{
		name.CalculateValues(omf, mod);
	}
}

void OMF86Format::ExternalName::ResolveReferences(OMF86Format * omf, Module * mod)
{
	if(name_is_index)
	{
		name.ResolveReferences(omf, mod);
	}
}

//// OMF86Format::BaseSpecification

void OMF86Format::BaseSpecification::Read(OMF86Format * omf, Linker::Reader& rd)
{
	index_t group_index = ReadIndex(rd);
	index_t segment_index = ReadIndex(rd);
	if(group_index == 0 && segment_index == 0)
	{
		location = uint16_t(rd.ReadUnsigned(2));
	}
	else
	{
		location = Location { group_index, segment_index };
	}
}

uint16_t OMF86Format::BaseSpecification::Size(OMF86Format * omf) const
{
	if(std::get_if<FrameNumber>(&location))
	{
		return 4;
	}
	else if(auto _location = std::get_if<Location>(&location))
	{
		return IndexSize(_location->group.index) + IndexSize(_location->segment.index);
	}
	else
	{
		assert(false);
	}
}

void OMF86Format::BaseSpecification::Write(OMF86Format * omf, ChecksumWriter& wr) const
{
	if(auto frame_number = std::get_if<FrameNumber>(&location))
	{
		WriteIndex(wr, 0);
		WriteIndex(wr, 0);
		wr.WriteWord(2, *frame_number);
	}
	else if(auto _location = std::get_if<Location>(&location))
	{
		WriteIndex(wr, _location->group.index);
		WriteIndex(wr, _location->segment.index);
	}
	else
	{
		assert(false);
	}
}

void OMF86Format::BaseSpecification::CalculateValues(OMF86Format * omf, Module * mod)
{
	if(auto _location = std::get_if<Location>(&location))
	{
		_location->group.CalculateValues(omf, mod);
		_location->segment.CalculateValues(omf, mod);
	}
}

void OMF86Format::BaseSpecification::ResolveReferences(OMF86Format * omf, Module * mod)
{
	if(auto _location = std::get_if<Location>(&location))
	{
		_location->group.ResolveReferences(omf, mod);
		_location->segment.ResolveReferences(omf, mod);
	}
}

//// OMF86Format::SymbolDefinition

OMF86Format::SymbolDefinition OMF86Format::SymbolDefinition::Read(OMF86Format * omf, Linker::Reader& rd, bool local, bool is32bit)
{
	SymbolDefinition name_definition;
	name_definition.name = ReadString(rd);
	name_definition.offset = rd.ReadUnsigned(is32bit ? 4 : 2);
	name_definition.type.index = ReadIndex(rd);
	name_definition.local = local;
	return name_definition;
}

uint16_t OMF86Format::SymbolDefinition::Size(OMF86Format * omf, bool is32bit) const
{
	return name.size() + IndexSize(type.index) + (is32bit ? 5 : 3);
}

void OMF86Format::SymbolDefinition::Write(OMF86Format * omf, ChecksumWriter& wr, bool is32bit) const
{
	WriteString(wr, name);
	wr.WriteWord(is32bit ? 4 : 2, offset);
	WriteIndex(wr, type.index);
}

void OMF86Format::SymbolDefinition::CalculateValues(OMF86Format * omf, Module * mod)
{
	type.CalculateValues(omf, mod);
}

void OMF86Format::SymbolDefinition::ResolveReferences(OMF86Format * omf, Module * mod)
{
	type.ResolveReferences(omf, mod);
}

//// OMF86Format::LineNumber

OMF86Format::LineNumber OMF86Format::LineNumber::Read(OMF86Format * omf, Linker::Reader& rd, bool is32bit)
{
	LineNumber line_number;
	line_number.number = rd.ReadUnsigned(2);
	line_number.offset = rd.ReadUnsigned(is32bit ? 4 : 2);
	return line_number;
}

void OMF86Format::LineNumber::Write(OMF86Format * omf, ChecksumWriter& wr, bool is32bit) const
{
	wr.WriteWord(2, number);
	wr.WriteWord(is32bit ? 4 : 2, offset);
}

//// OMF86Format::DataBlock

std::shared_ptr<OMF86Format::DataBlock> OMF86Format::DataBlock::ReadEnumeratedDataBlock(OMF86Format * omf, Linker::Reader& rd, uint16_t data_length)
{
	std::shared_ptr<DataBlock> block = std::make_shared<DataBlock>();
	block->repeat_count = 1;
	block->content = Data();
	auto& data = std::get<std::vector<uint8_t>>(block->content);
	data.resize(data_length);
	rd.ReadData(data);
	return block;
}

std::shared_ptr<OMF86Format::DataBlock> OMF86Format::DataBlock::ReadIteratedDataBlock(OMF86Format * omf, Linker::Reader& rd, bool is32bit)
{
	std::shared_ptr<DataBlock> block = std::make_shared<DataBlock>();
	block->repeat_count = rd.ReadUnsigned(is32bit ? 4 : 2); // TODO: Phar Lap always stores 2 bytes
	uint16_t block_count = rd.ReadUnsigned(2);
	if(block_count == 0)
	{
		uint8_t byte_count = rd.ReadUnsigned(1);
		block->content = Data();
		auto& data = std::get<Data>(block->content);
		data.resize(byte_count);
		rd.ReadData(data);
	}
	else
	{
		block->content = Blocks();
		auto& blocks = std::get<Blocks>(block->content);
		for(uint16_t block_number = 0; block_number < block_count; block_number++)
		{
			blocks.push_back(ReadIteratedDataBlock(omf, rd, is32bit));
		}
	}
	return block;
}

uint16_t OMF86Format::DataBlock::GetEnumeratedDataBlockSize(OMF86Format * omf) const
{
	return std::get<Data>(content).size();
}

uint16_t OMF86Format::DataBlock::GetIteratedDataBlockSize(OMF86Format * omf, bool is32bit) const
{
	uint16_t bytes = 2 + (is32bit ? 4 : 2); // TODO: Phar Lap always stores 2 bytes
	if(auto data = std::get_if<Data>(&content))
	{
		bytes += data->size();
	}
	else if(auto blocks = std::get_if<Blocks>(&content))
	{
		for(auto block : *blocks)
		{
			bytes += block->GetIteratedDataBlockSize(omf, is32bit);
		}
	}
	return bytes;
}

void OMF86Format::DataBlock::WriteEnumeratedDataBlock(OMF86Format * omf, ChecksumWriter& wr) const
{
	wr.WriteData(std::get<Data>(content));
}

void OMF86Format::DataBlock::WriteIteratedDataBlock(OMF86Format * omf, ChecksumWriter& wr, bool is32bit) const
{
	wr.WriteWord(is32bit ? 4 : 2, repeat_count); // TODO: Phar Lap always stores 2 bytes
	if(auto data = std::get_if<Data>(&content))
	{
		wr.WriteWord(2, 0);
		wr.WriteData(*data);
	}
	else if(auto blocks = std::get_if<Blocks>(&content))
	{
		wr.WriteWord(2, blocks->size());
		for(auto block : *blocks)
		{
			block->WriteIteratedDataBlock(omf, wr, is32bit);
		}
	}
}

//// OMF86Format::Reference

void OMF86Format::Reference::Read(OMF86Format * omf, Linker::Reader& rd, size_t displacement_size)
{
	uint8_t fixdata = rd.ReadUnsigned(1);

	if((fixdata & 0x80) != 0)
	{
		frame = ThreadNumber((fixdata >> 4) & 3);
	}
	else
	{
		switch((fixdata >> 4) & 7)
		{
		case MethodSegment:
			frame = SegmentIndex(ReadIndex(rd));
			break;
		case MethodGroup:
			frame = GroupIndex(ReadIndex(rd));
			break;
		case MethodExternal:
			frame = ExternalIndex(ReadIndex(rd));
			break;
		case MethodFrame:
			frame = FrameNumber(ReadIndex(rd));
			break;
		case MethodSource:
			frame = UsesSource{};
			break;
		case MethodTarget:
			frame = UsesTarget{};
			break;
		case MethodAbsolute:
			frame = UsesAbsolute{};
			break;
		}
	}

	if((fixdata & 0x08) != 0)
	{
		target = ThreadNumber(fixdata & 3);
	}
	else
	{
		switch(fixdata & 3)
		{
		case MethodSegment:
			target = SegmentIndex(ReadIndex(rd));
			break;
		case MethodGroup:
			target = GroupIndex(ReadIndex(rd));
			break;
		case MethodExternal:
			target = ExternalIndex(ReadIndex(rd));
			break;
		case MethodFrame:
			target = FrameNumber(ReadIndex(rd));
			break;
		}
	}

	if((fixdata & 0x04) == 0)
	{
		displacement = rd.ReadUnsigned(displacement_size);
	}
}

uint16_t OMF86Format::Reference::Size(OMF86Format * omf, bool is32bit) const
{
	uint16_t bytes = 1;

	if(auto segment = std::get_if<SegmentIndex>(&frame))
	{
		bytes += IndexSize(segment->index);
	}
	else if(auto group = std::get_if<GroupIndex>(&frame))
	{
		bytes += IndexSize(group->index);
	}
	else if(auto external = std::get_if<ExternalIndex>(&frame))
	{
		bytes += IndexSize(external->index);
	}
	else if(auto frame_number = std::get_if<FrameNumber>(&frame))
	{
		bytes += IndexSize(*frame_number);
	}

	if(auto segment = std::get_if<SegmentIndex>(&target))
	{
		bytes += IndexSize(segment->index);
	}
	else if(auto group = std::get_if<GroupIndex>(&target))
	{
		bytes += IndexSize(group->index);
	}
	else if(auto external = std::get_if<ExternalIndex>(&target))
	{
		bytes += IndexSize(external->index);
	}
	else if(auto frame_number = std::get_if<FrameNumber>(&target))
	{
		bytes += IndexSize(*frame_number);
	}

	if(displacement != 0)
	{
		switch(omf->omf_version)
		{
		case OMF_VERSION_INTEL_40:
			bytes += displacement > 0xFFFF ? 3 : 2;
			break;
		default:
			bytes += is32bit ? 4 : 2;
			break;
		}
	}

	return bytes;
}

void OMF86Format::Reference::Write(OMF86Format * omf, ChecksumWriter& wr, bool is32bit) const
{
	uint8_t fixdata = 0;

	if(auto thread_number = std::get_if<ThreadNumber>(&frame))
	{
		fixdata |= 0x80 | (*thread_number << 4);
	}
	else if(std::get_if<SegmentIndex>(&frame))
	{
		fixdata |= MethodSegment << 4;
	}
	else if(std::get_if<GroupIndex>(&frame))
	{
		fixdata |= MethodGroup << 4;
	}
	else if(std::get_if<ExternalIndex>(&frame))
	{
		fixdata |= MethodExternal << 4;
	}
	else if(std::get_if<FrameNumber>(&frame))
	{
		fixdata |= MethodFrame << 4;
	}
	else if(std::get_if<UsesSource>(&frame))
	{
		fixdata |= MethodSource << 4;
	}
	else if(std::get_if<UsesTarget>(&frame))
	{
		fixdata |= MethodTarget << 4;
	}
	else if(std::get_if<UsesAbsolute>(&frame))
	{
		fixdata |= MethodAbsolute << 4;
	}
	else
	{
		assert(false);
	}

	if(auto thread_number = std::get_if<ThreadNumber>(&target))
	{
		fixdata |= 0x80 | *thread_number;
	}
	else if(std::get_if<SegmentIndex>(&target))
	{
		fixdata |= MethodSegment;
	}
	else if(std::get_if<GroupIndex>(&target))
	{
		fixdata |= MethodGroup;
	}
	else if(std::get_if<ExternalIndex>(&target))
	{
		fixdata |= MethodExternal;
	}
	else if(std::get_if<FrameNumber>(&target))
	{
		fixdata |= MethodFrame;
	}
	else
	{
		assert(false);
	}

	if(displacement == 0)
	{
		fixdata |= 0x04;
	}

	wr.WriteWord(1, fixdata);

	if(auto segment = std::get_if<SegmentIndex>(&frame))
	{
		WriteIndex(wr, segment->index);
	}
	else if(auto group = std::get_if<GroupIndex>(&frame))
	{
		WriteIndex(wr, group->index);
	}
	else if(auto external = std::get_if<ExternalIndex>(&frame))
	{
		WriteIndex(wr, external->index);
	}
	else if(auto frame_number = std::get_if<FrameNumber>(&frame))
	{
		WriteIndex(wr, *frame_number);
	}

	if(auto segment = std::get_if<SegmentIndex>(&target))
	{
		WriteIndex(wr, segment->index);
	}
	else if(auto group = std::get_if<GroupIndex>(&target))
	{
		WriteIndex(wr, group->index);
	}
	else if(auto external = std::get_if<ExternalIndex>(&target))
	{
		WriteIndex(wr, external->index);
	}
	else if(auto frame_number = std::get_if<FrameNumber>(&target))
	{
		WriteIndex(wr, *frame_number);
	}

	if(displacement != 0)
	{
		switch(omf->omf_version)
		{
		case OMF_VERSION_INTEL_40:
			wr.WriteWord(displacement > 0xFFFF ? 3 : 2, displacement);
			break;
		default:
			wr.WriteWord(is32bit ? 4 : 2, displacement);
			break;
		}
	}
}

void OMF86Format::Reference::CalculateValues(OMF86Format * omf, Module * mod)
{
	if(auto segment = std::get_if<SegmentIndex>(&frame))
	{
		segment->CalculateValues(omf, mod);
	}
	else if(auto group = std::get_if<GroupIndex>(&frame))
	{
		group->CalculateValues(omf, mod);
	}
	else if(auto external = std::get_if<ExternalIndex>(&frame))
	{
		external->CalculateValues(omf, mod);
	}

	if(auto segment = std::get_if<SegmentIndex>(&target))
	{
		segment->CalculateValues(omf, mod);
	}
	else if(auto group = std::get_if<GroupIndex>(&target))
	{
		group->CalculateValues(omf, mod);
	}
	else if(auto external = std::get_if<ExternalIndex>(&target))
	{
		external->CalculateValues(omf, mod);
	}
}

void OMF86Format::Reference::ResolveReferences(OMF86Format * omf, Module * mod)
{
	if(auto segment = std::get_if<SegmentIndex>(&frame))
	{
		segment->ResolveReferences(omf, mod);
	}
	else if(auto group = std::get_if<GroupIndex>(&frame))
	{
		group->ResolveReferences(omf, mod);
	}
	else if(auto external = std::get_if<ExternalIndex>(&frame))
	{
		external->ResolveReferences(omf, mod);
	}

	if(auto segment = std::get_if<SegmentIndex>(&target))
	{
		segment->ResolveReferences(omf, mod);
	}
	else if(auto group = std::get_if<GroupIndex>(&target))
	{
		group->ResolveReferences(omf, mod);
	}
	else if(auto external = std::get_if<ExternalIndex>(&target))
	{
		external->ResolveReferences(omf, mod);
	}
}

//// OMF86Format::ModuleHeaderRecord

void OMF86Format::ModuleHeaderRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	name = rd.ReadData(record_length - 1);

	omf->modules.push_back(Module());
	omf->modules.back().first_record = omf->records.size();
}

uint16_t OMF86Format::ModuleHeaderRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	return name.size() + 1;
}

void OMF86Format::ModuleHeaderRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	wr.WriteData(name);
}

//// OMF86Format::RModuleHeaderRecord

void OMF86Format::RModuleHeaderRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	name = rd.ReadData(record_length - 27);
	module_type = module_type_t(rd.ReadUnsigned(1) & 3);
	segment_record_count = rd.ReadUnsigned(2);
	group_record_count = rd.ReadUnsigned(2);
	overlay_record_count = rd.ReadUnsigned(2);
	overlay_record_offset = rd.ReadUnsigned(4);
	static_size = rd.ReadUnsigned(4);
	maximum_static_size = rd.ReadUnsigned(4);
	dynamic_storage = rd.ReadUnsigned(4);
	maximum_dynamic_storage = rd.ReadUnsigned(4);

	omf->modules.push_back(Module());
	omf->modules.back().first_record = omf->records.size();
}

uint16_t OMF86Format::RModuleHeaderRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	return name.size() + 28;
}

void OMF86Format::RModuleHeaderRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	wr.WriteData(name);
	wr.WriteWord(1, module_type);
	wr.WriteWord(2, segment_record_count);
	wr.WriteWord(2, group_record_count);
	wr.WriteWord(2, overlay_record_count);
	wr.WriteWord(4, overlay_record_offset);
	wr.WriteWord(4, static_size);
	wr.WriteWord(4, maximum_static_size);
	wr.WriteWord(4, dynamic_storage);
	wr.WriteWord(4, maximum_dynamic_storage);
}

//// OMF86Format::ListOfNamesRecord

void OMF86Format::ListOfNamesRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	first_lname = omf->modules.back().lnames.size();
	lname_count = 0;
	while(rd.Tell() < RecordEnd())
	{
		std::string name = OMF86Format::ReadString(rd);
		omf->modules.back().lnames.push_back(name);
		names.push_back(name);
		lname_count++;
	}
}

uint16_t OMF86Format::ListOfNamesRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	uint16_t bytes = 1;
	for(uint16_t lname_index = first_lname; lname_index < first_lname + lname_count; lname_index++)
	{
		bytes += mod->lnames[lname_index].size() + 1;
	}
	return bytes;
}

void OMF86Format::ListOfNamesRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	for(uint16_t lname_index = first_lname; lname_index < first_lname + lname_count; lname_index++)
	{
		OMF86Format::WriteString(wr, mod->lnames[lname_index]);
	}
}

void OMF86Format::ListOfNamesRecord::CalculateValues(OMF86Format * omf, Module * mod)
{
	Record::CalculateValues(omf, mod);
	// TODO
}

void OMF86Format::ListOfNamesRecord::ResolveReferences(OMF86Format * omf, Module * mod)
{
	// TODO
}

//// OMF86Format::SegmentDefinitionRecord

void OMF86Format::SegmentDefinitionRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	uint8_t attributes = rd.ReadUnsigned(1);

	alignment = alignment_t(attributes >> 5);

	switch(int(alignment))
	{
	case AlignPage:
		switch(omf->omf_version)
		{
		case OMF_VERSION_INTEL_40:
		default:
			alignment = Align256;
			break;
		case OMF_VERSION_IBM:
			alignment = Align4096;
			break;
		}
		break;
	case alignment_t(5):
		switch(omf->omf_version)
		{
		case OMF_VERSION_INTEL_40:
			alignment = AlignUnnamed;
			break;
		default:
			alignment = Align32;
			break;
		}
		break;
	default:
		break;
	}

	combination = combination_t((attributes >> 2) & 7);

	switch(int(combination))
	{
	case combination_t(4):
		switch(omf->omf_version)
		{
		case OMF_VERSION_INTEL_40:
			break;
		case OMF_VERSION_TIS_11:
		default:
			combination = Combination_Public4;
			break;
		}
		break;
	case combination_t(5):
		switch(omf->omf_version)
		{
		case OMF_VERSION_INTEL_40:
			combination = Combination_Join_High;
			break;
		case OMF_VERSION_TIS_11:
		default:
			combination = Combination_Stack;
			break;
		}
		break;
	case combination_t(6):
		switch(omf->omf_version)
		{
		case OMF_VERSION_INTEL_40:
			combination = Combination_Merge_Low;
			break;
		case OMF_VERSION_TIS_11:
		default:
			combination = Combination_Common;
			break;
		}
		break;
	case combination_t(7):
		switch(omf->omf_version)
		{
		case OMF_VERSION_INTEL_40:
			break;
		case OMF_VERSION_TIS_11:
		default:
			combination = Combination_Public7;
			break;
		}
		break;
	default:
		break;
	}

	if((attributes & 0x02))
	{
		segment_length = Is32Bit(omf) ? 0x100000000 : 0x10000;
	}
	else
	{
		segment_length = 0;
	}

	if(omf->omf_version == OMF_VERSION_TIS_11)
	{
		use32 = attributes & 0x01;
	}
	else
	{
		page_resident = attributes & 0x01;
	}

	switch(alignment)
	{
	case AlignAbsolute:
	case AlignUnnamed:
		{
			uint16_t frame_number = rd.ReadUnsigned(2);
			uint8_t offset = rd.ReadUnsigned(1);
			location = Absolute((uint32_t(frame_number) << 4) | (offset & 0xF));
		}
		break;
	case AlignLTL16:
		{
			LoadTimeLocatable ltl;
			uint8_t ltl_data = rd.ReadUnsigned(1);
			ltl.group_member = (ltl_data & 0x80) != 0;
			ltl.maximum_segment_length = rd.ReadUnsigned(2) + (ltl_data & 0x01 ? 0x10000 : 0);
			ltl.group_offset = rd.ReadUnsigned(2);
			location = ltl;
		}
		break;
	default:
		location = Relocatable();
		break;
	}

	segment_length += rd.ReadUnsigned(GetOffsetSize(omf));

	if(alignment != AlignUnnamed)
	{
		segment_name.index = ReadIndex(rd);
		class_name.index = ReadIndex(rd);
		overlay_name.index = ReadIndex(rd);
	}

	if(rd.Tell() < RecordEnd() && omf->omf_version == OMF_VERSION_PHARLAP)
	{
		uint8_t bits = rd.ReadUnsigned(1);
		access = access_t(bits & 0x03);
		use32 = (bits & 0x04) != 0;
	}

	omf->modules.back().segdefs.push_back(shared_from_this());
}

uint16_t OMF86Format::SegmentDefinitionRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	uint16_t bytes = 1;
	switch(alignment)
	{
	case AlignAbsolute:
	case AlignUnnamed:
		bytes += 3;
		break;
	case AlignLTL16:
		bytes += 5;
		break;
	default:
		break;
	}
	bytes += segment_length;
	if(alignment != AlignUnnamed)
	{
		bytes += IndexSize(segment_name.index);
		bytes += IndexSize(class_name.index);
		bytes += IndexSize(overlay_name.index);
	}
	if(omf->omf_version == OMF_VERSION_PHARLAP && access)
	{
		bytes += 1;
	}
	return bytes;
}

void OMF86Format::SegmentDefinitionRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	uint8_t attributes = (alignment << 5) | ((combination & 7) << 2);
	if(Is32Bit(omf) ? segment_length > 0xFFFFFFFF : segment_length > 0xFFFF)
	{
		attributes |= 0x02;
	}

	if(omf->omf_version == OMF_VERSION_TIS_11)
	{
		if(use32)
		{
			attributes |= 0x01;
		}
	}
	else
	{
		if(page_resident)
		{
			attributes |= 0x01;
		}
	}

	wr.WriteWord(1, attributes);

	switch(alignment)
	{
	case AlignAbsolute:
	case AlignUnnamed:
		wr.WriteWord(2, std::get<Absolute>(location) >> 4);
		wr.WriteWord(1, std::get<Absolute>(location) & 0x0F);
		break;
	case AlignLTL16:
		{
			LoadTimeLocatable ltl = std::get<LoadTimeLocatable>(location);
			uint8_t ltl_data = (ltl.group_member ? 0x80 : 0) | (ltl.maximum_segment_length > 0xFFFF ? 0x01 : 0);
			wr.WriteWord(1, ltl_data);
			wr.WriteWord(2, ltl.maximum_segment_length);
			wr.WriteWord(2, ltl.group_offset);
		}
		break;
	default:
		break;
	}

	wr.WriteWord(GetOffsetSize(omf), segment_length);

	if(alignment != AlignUnnamed)
	{
		WriteIndex(wr, segment_name.index);
		WriteIndex(wr, class_name.index);
		WriteIndex(wr, overlay_name.index);
	}

	if(omf->omf_version == OMF_VERSION_PHARLAP && access)
	{
		wr.WriteWord(1, access.value() + (use32 ? 0x04 : 0));
	}
}

void OMF86Format::SegmentDefinitionRecord::CalculateValues(OMF86Format * omf, Module * mod)
{
	Record::CalculateValues(omf, mod);
	if(alignment != AlignUnnamed)
	{
		segment_name.CalculateValues(omf, mod);
		class_name.CalculateValues(omf, mod);
		overlay_name.CalculateValues(omf, mod);
	}
}

void OMF86Format::SegmentDefinitionRecord::ResolveReferences(OMF86Format * omf, Module * mod)
{
	if(alignment != AlignUnnamed)
	{
		segment_name.ResolveReferences(omf, mod);
		class_name.ResolveReferences(omf, mod);
		overlay_name.ResolveReferences(omf, mod);
	}
}

//// OMF86Format::GroupDefinitionRecord::Component

OMF86Format::GroupDefinitionRecord::Component OMF86Format::GroupDefinitionRecord::Component::Read(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	Component component;
	uint8_t component_type = rd.ReadUnsigned(1);
	switch(component_type)
	{
	case Absolute_type:
		{
			Absolute absolute = uint32_t(rd.ReadUnsigned(2)) << 4;
			absolute += rd.ReadUnsigned(1);
			component.component = absolute;
		}
		break;
	case LoadTimeLocatable_type:
		{
			uint8_t data = rd.ReadUnsigned(1);
			LoadTimeLocatable lengths;
			lengths.maximum_group_length = rd.ReadUnsigned(2) + (data & 0x01 ? 0x10000 : 0);
			lengths.group_length = rd.ReadUnsigned(2) + (data & 0x02 ? 0x10000 : 0);
			component.component = lengths;
		}
		break;
	case SegmentClassOverlayNames_type:
		{
			SegmentClassOverlayNames names;
			names.segment_name.index = ReadIndex(rd);
			names.class_name.index = ReadIndex(rd);
			names.overlay_name.index = ReadIndex(rd);
			component.component = names;
		}
		break;
	case ExternalIndex_type:
		component.component = ExternalIndex(ReadIndex(rd));
		break;
	case SegmentIndex_type:
		component.component = SegmentIndex(ReadIndex(rd));
		break;
	}
	return component;
}

uint16_t OMF86Format::GroupDefinitionRecord::Component::Size(OMF86Format * omf, Module * mod) const
{
	if(std::get_if<Absolute>(&component))
	{
		return 5;
	}
	else if(std::get_if<LoadTimeLocatable>(&component))
	{
		return 7;
	}
	else if(auto names = std::get_if<SegmentClassOverlayNames>(&component))
	{
		return 2 + IndexSize(names->segment_name.index) + IndexSize(names->class_name.index) + IndexSize(names->overlay_name.index);
	}
	else if(auto external = std::get_if<ExternalIndex>(&component))
	{
		return 2 + IndexSize(external->index);
	}
	else if(auto segment = std::get_if<SegmentIndex>(&component))
	{
		return 2 + IndexSize(segment->index);
	}
	else
	{
		assert(false);
	}
}

void OMF86Format::GroupDefinitionRecord::Component::Write(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	if(auto absolute = std::get_if<Absolute>(&component))
	{
		wr.WriteWord(1, Absolute_type);
		wr.WriteWord(2, *absolute >> 4);
		wr.WriteWord(1, *absolute & 0xF);
	}
	else if(auto lengths = std::get_if<LoadTimeLocatable>(&component))
	{
		wr.WriteWord(1, LoadTimeLocatable_type);
		uint8_t data = 0;
		if(lengths->maximum_group_length > 0xFFFF)
			data |= 0x01;
		if(lengths->group_length > 0xFFFF)
			data |= 0x02;
		wr.WriteWord(1, data);
		wr.WriteWord(2, lengths->maximum_group_length);
		wr.WriteWord(2, lengths->group_length);
	}
	else if(auto names = std::get_if<SegmentClassOverlayNames>(&component))
	{
		wr.WriteWord(1, SegmentClassOverlayNames_type);
		WriteIndex(wr, names->segment_name.index);
		WriteIndex(wr, names->class_name.index);
		WriteIndex(wr, names->overlay_name.index);
	}
	else if(auto external = std::get_if<ExternalIndex>(&component))
	{
		wr.WriteWord(1, ExternalIndex_type);
		WriteIndex(wr, external->index);
	}
	else if(auto segment = std::get_if<SegmentIndex>(&component))
	{
		wr.WriteWord(1, SegmentIndex_type);
		WriteIndex(wr, segment->index);
	}
	else
	{
		assert(false);
	}
}

void OMF86Format::GroupDefinitionRecord::Component::CalculateValues(OMF86Format * omf, Module * mod)
{
	if(auto names = std::get_if<SegmentClassOverlayNames>(&component))
	{
		names->segment_name.CalculateValues(omf, mod);
		names->class_name.CalculateValues(omf, mod);
		names->overlay_name.CalculateValues(omf, mod);
	}
	else if(auto external = std::get_if<ExternalIndex>(&component))
	{
		external->CalculateValues(omf, mod);
	}
	else if(auto segment = std::get_if<SegmentIndex>(&component))
	{
		segment->CalculateValues(omf, mod);
	}
}

void OMF86Format::GroupDefinitionRecord::Component::ResolveReferences(OMF86Format * omf, Module * mod)
{
	if(auto names = std::get_if<SegmentClassOverlayNames>(&component))
	{
		names->segment_name.ResolveReferences(omf, mod);
		names->class_name.ResolveReferences(omf, mod);
		names->overlay_name.ResolveReferences(omf, mod);
	}
	else if(auto external = std::get_if<ExternalIndex>(&component))
	{
		external->ResolveReferences(omf, mod);
	}
	else if(auto segment = std::get_if<SegmentIndex>(&component))
	{
		segment->ResolveReferences(omf, mod);
	}
}

// OMF86Format::GroupDefinitionRecord

void OMF86Format::GroupDefinitionRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	name.index = ReadIndex(rd);
	while(rd.Tell() < RecordEnd())
	{
		components.push_back(Component::Read(omf, mod, rd));
	}
	omf->modules.back().grpdefs.push_back(shared_from_this());
}

uint16_t OMF86Format::GroupDefinitionRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	uint16_t bytes = 1;
	bytes += IndexSize(name.index);
	for(auto& component : components)
	{
		bytes += component.Size(omf, mod);
	}
	return bytes;
}

void OMF86Format::GroupDefinitionRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	WriteIndex(wr, name.index);
	for(auto& component : components)
	{
		component.Write(omf, mod, wr);
	}
}

void OMF86Format::GroupDefinitionRecord::CalculateValues(OMF86Format * omf, Module * mod)
{
	Record::CalculateValues(omf, mod);
	for(auto& component : components)
	{
		component.CalculateValues(omf, mod);
	}
}

void OMF86Format::GroupDefinitionRecord::ResolveReferences(OMF86Format * omf, Module * mod)
{
	for(auto& component : components)
	{
		component.ResolveReferences(omf, mod);
	}
}

//// OMF86Format::TypeDefinitionRecord::LeafDescriptor

OMF86Format::TypeDefinitionRecord::LeafDescriptor OMF86Format::TypeDefinitionRecord::LeafDescriptor::Read(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	LeafDescriptor leaf;
	uint8_t type = rd.ReadUnsigned(1);
	switch(type)
	{
	case NullLeaf:
		leaf.leaf = Null{};
		break;
	case NumericLeaf16:
		leaf.leaf = uint32_t(rd.ReadUnsigned(2));
		break;
	case StringLeaf:
		leaf.leaf = ReadString(rd);
		break;
	case IndexLeaf:
		leaf.leaf = TypeIndex(ReadIndex(rd));
		break;
	case NumericLeaf24:
		leaf.leaf = uint32_t(rd.ReadUnsigned(3));
		break;
	case RepeatLeaf:
		leaf.leaf = Repeat{};
		break;
	case SignedNumericLeaf8:
		leaf.leaf = int32_t(rd.ReadSigned(1));
		break;
	case SignedNumericLeaf16:
		leaf.leaf = int32_t(rd.ReadSigned(2));
		break;
	case SignedNumericLeaf32:
		leaf.leaf = int32_t(rd.ReadSigned(4));
		break;
	default:
		if(type < 0x80)
		{
			leaf.leaf = uint32_t(type);
		}
		// TODO: otherwise, error
	}
	return leaf;
}

uint16_t OMF86Format::TypeDefinitionRecord::LeafDescriptor::Size(OMF86Format * omf, Module * mod) const
{
	if(std::get_if<Null>(&leaf))
	{
		return 1;
	}
	else if(auto number = std::get_if<uint32_t>(&leaf))
	{
		if(*number < 0x80)
		{
			return 1;
		}
		else if(*number <= 0xFFFF)
		{
			return 3;
		}
		else if(*number <= 0xFFFFFF)
		{
			return 4;
		}
		else // simplifying storing values
		{
			return 5;
		}
	}
	else if(auto string = std::get_if<std::string>(&leaf))
	{
		return 2 + string->size();
	}
	else if(auto index = std::get_if<TypeIndex>(&leaf))
	{
		return 1 + IndexSize(index->index);
	}
	else if(std::get_if<Repeat>(&leaf))
	{
		return 1;
	}
	else if(auto signed_number = std::get_if<int32_t>(&leaf))
	{
		if(-0x80 <= *signed_number && *signed_number < 0x80)
		{
			return 2;
		}
		else if(-0x8000 <= *signed_number && *signed_number < 0x8000)
		{
			return 3;
		}
		else
		{
			return 5;
		}
	}
	else
	{
		assert(false);
	}
}

void OMF86Format::TypeDefinitionRecord::LeafDescriptor::Write(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	if(std::get_if<Null>(&leaf))
	{
		wr.WriteWord(1, NullLeaf);
	}
	else if(auto number = std::get_if<uint32_t>(&leaf))
	{
		if(*number < 0x80)
		{
			wr.WriteWord(1, *number);
		}
		else if(*number <= 0xFFFF)
		{
			wr.WriteWord(1, NumericLeaf16);
			wr.WriteWord(2, *number);
		}
		else if(*number <= 0xFFFFFF)
		{
			wr.WriteWord(1, NumericLeaf24);
			wr.WriteWord(3, *number);
		}
		else // simplifying storing values
		{
			wr.WriteWord(1, SignedNumericLeaf32);
			wr.WriteWord(4, *number);
		}
	}
	else if(auto string = std::get_if<std::string>(&leaf))
	{
		wr.WriteWord(1, StringLeaf);
		WriteString(wr, *string);
	}
	else if(auto index = std::get_if<TypeIndex>(&leaf))
	{
		wr.WriteWord(1, IndexLeaf);
		WriteIndex(wr, index->index);
	}
	else if(std::get_if<Repeat>(&leaf))
	{
		wr.WriteWord(1, RepeatLeaf);
	}
	else if(auto signed_number = std::get_if<int32_t>(&leaf))
	{
		if(-0x80 <= *signed_number && *signed_number < 0x80)
		{
			wr.WriteWord(1, SignedNumericLeaf8);
			wr.WriteWord(1, *signed_number);
		}
		else if(-0x8000 <= *signed_number && *signed_number < 0x8000)
		{
			wr.WriteWord(1, SignedNumericLeaf16);
			wr.WriteWord(2, *signed_number);
		}
		else
		{
			wr.WriteWord(1, SignedNumericLeaf32);
			wr.WriteWord(4, *signed_number);
		}
	}
	else
	{
		assert(false);
	}
}

void OMF86Format::TypeDefinitionRecord::LeafDescriptor::CalculateValues(OMF86Format * omf, Module * mod)
{
	if(auto index = std::get_if<TypeIndex>(&leaf))
	{
		index->CalculateValues(omf, mod);
	}
}

void OMF86Format::TypeDefinitionRecord::LeafDescriptor::ResolveReferences(OMF86Format * omf, Module * mod)
{
	if(auto index = std::get_if<TypeIndex>(&leaf))
	{
		index->ResolveReferences(omf, mod);
	}
}

//// OMF86Format::TypeDefinitionRecord

void OMF86Format::TypeDefinitionRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	name = ReadString(rd);
	while(rd.Tell() < RecordEnd())
	{
		uint8_t nice_bits = rd.ReadUnsigned(1);
		for(int leaf_number = 0; leaf_number < 8 && rd.Tell() + 1 < RecordEnd(); leaf_number++)
		{
			leafs.push_back(LeafDescriptor::Read(omf, mod, rd));
			leafs.back().nice = (nice_bits >> leaf_number) & 1;
		}
	}
	omf->modules.back().typdefs.push_back(shared_from_this());
}

uint16_t OMF86Format::TypeDefinitionRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	uint16_t bytes = 2 + name.size();
	for(size_t leaf_number = 0; leaf_number < leafs.size(); leaf_number++)
	{
		if(leaf_number % 8 == 0)
			bytes += 1;
		bytes += leafs[leaf_number].Size(omf, mod);
	}
	return bytes;
}

void OMF86Format::TypeDefinitionRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	WriteString(wr, name);
	for(size_t leaf_group_number = 0; leaf_group_number < leafs.size(); leaf_group_number += 8)
	{
		uint8_t nice_bits = 0;
		for(size_t leaf_number = 0; leaf_number < 8 && leaf_group_number + leaf_number < leafs.size(); leaf_number++)
		{
			if(leafs[leaf_group_number + leaf_number].nice)
				nice_bits |= 1 << leaf_number;
		}
		wr.WriteWord(1, nice_bits);
		for(size_t leaf_number = 0; leaf_number < 8 && leaf_group_number + leaf_number < leafs.size(); leaf_number++)
		{
			leafs[leaf_group_number + leaf_number].Write(omf, mod, wr);
		}
	}
}

void OMF86Format::TypeDefinitionRecord::CalculateValues(OMF86Format * omf, Module * mod)
{
	Record::CalculateValues(omf, mod);
	for(auto& leaf : leafs)
	{
		leaf.CalculateValues(omf, mod);
	}
}

void OMF86Format::TypeDefinitionRecord::ResolveReferences(OMF86Format * omf, Module * mod)
{
	for(auto& leaf : leafs)
	{
		leaf.ResolveReferences(omf, mod);
	}
}

//// OMF86Format::SymbolsDefinitionRecord

void OMF86Format::SymbolsDefinitionRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	bool local = record_type == LPUBDEF16 || record_type == LPUBDEF32;
	base.Read(omf, rd);
	while(rd.Tell() < RecordEnd())
	{
		symbols.push_back(SymbolDefinition::Read(omf, rd, local, Is32Bit(omf)));
	}
}

uint16_t OMF86Format::SymbolsDefinitionRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	bool is32bit = Is32Bit(omf);
	uint16_t bytes = 1 + base.Size(omf);
	for(auto& symbol : symbols)
	{
		bytes += symbol.Size(omf, is32bit);
	}
	return bytes;
}

void OMF86Format::SymbolsDefinitionRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	bool is32bit = Is32Bit(omf);
	base.Write(omf, wr);
	for(auto& symbol : symbols)
	{
		symbol.Write(omf, wr, is32bit);
	}
}

void OMF86Format::SymbolsDefinitionRecord::CalculateValues(OMF86Format * omf, Module * mod)
{
	Record::CalculateValues(omf, mod);
	for(auto& symbol : symbols)
	{
		symbol.CalculateValues(omf, mod);
	}
}

void OMF86Format::SymbolsDefinitionRecord::ResolveReferences(OMF86Format * omf, Module * mod)
{
	for(auto& symbol : symbols)
	{
		symbol.ResolveReferences(omf, mod);
	}
}

//// OMF86Format::ExternalNamesDefinitionRecord

void OMF86Format::ExternalNamesDefinitionRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	bool local = record_type != EXTDEF && record_type != COMDEF && record_type != CEXTDEF;
	bool common = record_type == COMDEF || record_type == LCOMDEF;
	first_extdef.index = omf->modules.back().extdefs.size();
	extdef_count = 0;
	while(rd.Tell() < RecordEnd())
	{
		if(common)
			omf->modules.back().extdefs.push_back(ExternalName::ReadCommonName(omf, rd, local));
		else
			omf->modules.back().extdefs.push_back(ExternalName::ReadExternalName(omf, rd, local));
		extdef_count++;
	}
}

uint16_t OMF86Format::ExternalNamesDefinitionRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	uint16_t bytes = 1;
	for(uint16_t extdef_index = first_extdef.index; extdef_index < first_extdef.index + extdef_count; extdef_index++)
	{
		bytes += mod->extdefs[extdef_index].GetExternalNameSize(omf);
	}
	return bytes;
}

void OMF86Format::ExternalNamesDefinitionRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	for(uint16_t extdef_index = first_extdef.index; extdef_index < first_extdef.index + extdef_count; extdef_index++)
	{
		mod->extdefs[extdef_index].WriteExternalName(omf, wr);
	}
}

void OMF86Format::ExternalNamesDefinitionRecord::CalculateValues(OMF86Format * omf, Module * mod)
{
	Record::CalculateValues(omf, mod);
	for(uint16_t extdef_index = first_extdef.index; extdef_index < first_extdef.index + extdef_count; extdef_index++)
	{
		mod->extdefs[extdef_index].CalculateValues(omf, mod);
	}
	// TODO
}

void OMF86Format::ExternalNamesDefinitionRecord::ResolveReferences(OMF86Format * omf, Module * mod)
{
	for(uint16_t extdef_index = first_extdef.index; extdef_index < first_extdef.index + extdef_count; extdef_index++)
	{
		mod->extdefs[extdef_index].ResolveReferences(omf, mod);
	}
	// TODO
}

//// OMF86Format::LineNumbersRecord

void OMF86Format::LineNumbersRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	base.Read(omf, rd);
	while(rd.Tell() < RecordEnd())
	{
		lines.push_back(LineNumber::Read(omf, rd, Is32Bit(omf)));
	}
}

uint16_t OMF86Format::LineNumbersRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	uint16_t bytes = 1 + base.Size(omf);
	bytes += lines.size() * (2 + GetOffsetSize(omf));
	return bytes;
}

void OMF86Format::LineNumbersRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	bool is32bit = Is32Bit(omf);
	base.Write(omf, wr);
	for(auto& line : lines)
	{
		line.Write(omf, wr, is32bit);
	}
}

void OMF86Format::LineNumbersRecord::CalculateValues(OMF86Format * omf, Module * mod)
{
	Record::CalculateValues(omf, mod);
	base.CalculateValues(omf, mod);
}

void OMF86Format::LineNumbersRecord::ResolveReferences(OMF86Format * omf, Module * mod)
{
	base.ResolveReferences(omf, mod);
}

//// OMF86Format::BlockDefinitionRecord

void OMF86Format::BlockDefinitionRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	base.Read(omf, rd);
	name = ReadString(rd);
	offset = rd.ReadUnsigned(2);
	length = rd.ReadUnsigned(2);
	procedure = block_type_t(rd.ReadUnsigned(1));
	if((procedure & 0x80))
	{
		return_address_offset = rd.ReadUnsigned(2);
	}
	if(name != "")
	{
		type.index = ReadIndex(rd);
	}
	omf->modules.back().blkdefs.push_back(shared_from_this());
}

uint16_t OMF86Format::BlockDefinitionRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	return 7 + base.Size(omf) + name.size() + ((procedure & 0x80) ? 2 : 0) + (name != "" ? IndexSize(type.index) : 0);
}

void OMF86Format::BlockDefinitionRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	base.Write(omf, wr);
	WriteString(wr, name);
	wr.WriteWord(2, offset);
	wr.WriteWord(2, length);
	wr.WriteWord(1, procedure);
	if((procedure & 0x80))
	{
		wr.WriteWord(2, return_address_offset);
	}
	if(name != "")
	{
		WriteIndex(wr, type.index);
	}
}

void OMF86Format::BlockDefinitionRecord::CalculateValues(OMF86Format * omf, Module * mod)
{
	Record::CalculateValues(omf, mod);
	type.CalculateValues(omf, mod);
}

void OMF86Format::BlockDefinitionRecord::ResolveReferences(OMF86Format * omf, Module * mod)
{
	type.ResolveReferences(omf, mod);
}

//// OMF86Format::DebugSymbolsRecord

void OMF86Format::DebugSymbolsRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	uint8_t frame_info = rd.ReadUnsigned(1);
	frame_type = frame_type_t(frame_info & 0xC0);
	uint8_t method_type = frame_info & 0x07;
	switch(method_type)
	{
	case SymbolBaseMethod:
		{
			BaseSpecification spec;
			spec.Read(omf, rd);
			base = spec;
		}
		break;
	case ExternalMethod:
		base = ExternalIndex(ReadIndex(rd));
		break;
	case BlockMethod:
		base = BlockIndex(ReadIndex(rd));
		break;
	default:
		// error
		break;
	}

	while(rd.Tell() < RecordEnd())
	{
		names.push_back(SymbolDefinition::Read(omf, rd, true, false));
	}
}

uint16_t OMF86Format::DebugSymbolsRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	uint16_t bytes = 2;
	if(auto spec = std::get_if<BaseSpecification>(&base))
	{
		bytes += spec->Size(omf);
	}
	else if(auto external = std::get_if<ExternalIndex>(&base))
	{
		bytes += IndexSize(external->index);
	}
	else if(auto block = std::get_if<ExternalIndex>(&base))
	{
		bytes += IndexSize(block->index);
	}
	else
	{
		assert(false);
	}

	for(auto& name : names)
	{
		bytes += name.Size(omf, false);
	}

	return bytes;
}

void OMF86Format::DebugSymbolsRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	if(auto spec = std::get_if<BaseSpecification>(&base))
	{
		wr.WriteWord(1, frame_type | SymbolBaseMethod);
		spec->Write(omf, wr);
	}
	else if(auto external = std::get_if<ExternalIndex>(&base))
	{
		wr.WriteWord(1, frame_type | ExternalMethod);
		WriteIndex(wr, external->index);
	}
	else if(auto block = std::get_if<ExternalIndex>(&base))
	{
		wr.WriteWord(1, frame_type | BlockMethod);
		WriteIndex(wr, block->index);
	}
	else
	{
		assert(false);
	}

	for(auto& name : names)
	{
		name.Write(omf, wr, false);
	}
}

void OMF86Format::DebugSymbolsRecord::CalculateValues(OMF86Format * omf, Module * mod)
{
	Record::CalculateValues(omf, mod);
	if(auto spec = std::get_if<BaseSpecification>(&base))
	{
		spec->CalculateValues(omf, mod);
	}
	else if(auto external = std::get_if<ExternalIndex>(&base))
	{
		external->CalculateValues(omf, mod);
	}
	else if(auto block = std::get_if<ExternalIndex>(&base))
	{
		block->CalculateValues(omf, mod);
	}
}

void OMF86Format::DebugSymbolsRecord::ResolveReferences(OMF86Format * omf, Module * mod)
{
	if(auto spec = std::get_if<BaseSpecification>(&base))
	{
		spec->ResolveReferences(omf, mod);
	}
	else if(auto external = std::get_if<ExternalIndex>(&base))
	{
		external->ResolveReferences(omf, mod);
	}
	else if(auto block = std::get_if<ExternalIndex>(&base))
	{
		block->ResolveReferences(omf, mod);
	}
}

//// OMF86Format::RelocatableDataRecord

void OMF86Format::RelocatableDataRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	base.Read(omf, rd);
	offset = rd.ReadUnsigned(2);
	if(record_type == RIDATA)
		data = DataBlock::ReadIteratedDataBlock(omf, rd, false);
	else
		data = DataBlock::ReadEnumeratedDataBlock(omf, rd, RecordEnd() - rd.Tell());
}

uint16_t OMF86Format::RelocatableDataRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	uint16_t bytes = 3 + base.Size(omf);
	if(record_type == RIDATA)
		bytes += data->GetIteratedDataBlockSize(omf, false);
	else
		bytes += data->GetEnumeratedDataBlockSize(omf);
	return bytes;
}

void OMF86Format::RelocatableDataRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	base.Write(omf, wr);
	wr.WriteWord(2, offset);
	if(record_type == RIDATA)
		data->WriteIteratedDataBlock(omf, wr, false);
	else
		data->WriteEnumeratedDataBlock(omf, wr);
}

void OMF86Format::RelocatableDataRecord::CalculateValues(OMF86Format * omf, Module * mod)
{
	Record::CalculateValues(omf, mod);
	base.CalculateValues(omf, mod);
}

void OMF86Format::RelocatableDataRecord::ResolveReferences(OMF86Format * omf, Module * mod)
{
	base.ResolveReferences(omf, mod);
}

//// OMF86Format::PhysicalDataRecord

void OMF86Format::PhysicalDataRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	address = uint32_t(rd.ReadUnsigned(2)) << 4;
	address += rd.ReadUnsigned(1);
	if(record_type == PIDATA)
		data = DataBlock::ReadIteratedDataBlock(omf, rd, false);
	else
		data = DataBlock::ReadEnumeratedDataBlock(omf, rd, RecordEnd() - rd.Tell());
}

uint16_t OMF86Format::PhysicalDataRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	uint16_t bytes = 4;
	if(record_type == RIDATA)
		bytes += data->GetIteratedDataBlockSize(omf, false);
	else
		bytes += data->GetEnumeratedDataBlockSize(omf);
	return bytes;
}

void OMF86Format::PhysicalDataRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	wr.WriteWord(2, address >> 4);
	wr.WriteWord(1, address & 0xF);
	if(record_type == RIDATA)
		data->WriteIteratedDataBlock(omf, wr, false);
	else
		data->WriteEnumeratedDataBlock(omf, wr);
}

//// OMF86Format::LogicalDataRecord

void OMF86Format::LogicalDataRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	segment.index = ReadIndex(rd);
	offset = rd.ReadUnsigned(GetOffsetSize(omf));
	if(record_type == LIDATA16)
		data = DataBlock::ReadIteratedDataBlock(omf, rd, false);
	else if(record_type == LIDATA32)
		data = DataBlock::ReadIteratedDataBlock(omf, rd, true);
	else
		data = DataBlock::ReadEnumeratedDataBlock(omf, rd, RecordEnd() - rd.Tell());
}

uint16_t OMF86Format::LogicalDataRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	uint16_t bytes = 1 + IndexSize(segment.index) + GetOffsetSize(omf);
	if(record_type == LIDATA16)
		bytes += data->GetIteratedDataBlockSize(omf, false);
	else if(record_type == LIDATA32)
		bytes += data->GetIteratedDataBlockSize(omf, true);
	else
		bytes += data->GetEnumeratedDataBlockSize(omf);
	return bytes;
}

void OMF86Format::LogicalDataRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	WriteIndex(wr, segment.index);
	wr.WriteWord(GetOffsetSize(omf), offset);
	if(record_type == LIDATA16)
		data->WriteIteratedDataBlock(omf, wr, false);
	else if(record_type == LIDATA32)
		data->WriteIteratedDataBlock(omf, wr, true);
	else
		data->WriteEnumeratedDataBlock(omf, wr);
}

void OMF86Format::LogicalDataRecord::CalculateValues(OMF86Format * omf, Module * mod)
{
	Record::CalculateValues(omf, mod);
	segment.CalculateValues(omf, mod);
}

void OMF86Format::LogicalDataRecord::ResolveReferences(OMF86Format * omf, Module * mod)
{
	segment.ResolveReferences(omf, mod);
}

//// OMF86Format::FixupRecord::Thread

OMF86Format::FixupRecord::Thread OMF86Format::FixupRecord::Thread::Read(OMF86Format * omf, Module * mod, Linker::Reader& rd, uint8_t leading_data_byte)
{
	Thread thread;
	thread.frame = (leading_data_byte & 0x40) != 0;
	thread.thread_number = leading_data_byte & 3;
	switch((leading_data_byte >> 2) & (thread.frame ? 7 : 3))
	{
	case MethodSegment:
		thread.reference = SegmentIndex(ReadIndex(rd));
		break;
	case MethodGroup:
		thread.reference = GroupIndex(ReadIndex(rd));
		break;
	case MethodExternal:
		thread.reference = ExternalIndex(ReadIndex(rd));
		break;
	case MethodFrame:
		thread.reference = FrameNumber(ReadIndex(rd));
		break;
	case MethodSource:
		thread.reference = UsesSource{};
		break;
	case MethodTarget:
		thread.reference = UsesTarget{};
		break;
	case MethodAbsolute:
		thread.reference = UsesAbsolute{};
		break;
	}
	return thread;
}

uint16_t OMF86Format::FixupRecord::Thread::Size(OMF86Format * omf, Module * mod) const
{
	size_t bytes = 1;

	if(auto segmentp = std::get_if<SegmentIndex>(&reference))
	{
		bytes += IndexSize(segmentp->index);
	}
	else if(auto groupp = std::get_if<GroupIndex>(&reference))
	{
		bytes += IndexSize(groupp->index);
	}
	else if(auto externalp = std::get_if<ExternalIndex>(&reference))
	{
		bytes += IndexSize(externalp->index);
	}
	else if(auto frame_numberp = std::get_if<FrameNumber>(&reference))
	{
		bytes += IndexSize(*frame_numberp);
	}

	return bytes;
}

void OMF86Format::FixupRecord::Thread::Write(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	uint8_t data_byte = thread_number;
	if(frame)
		data_byte |= 0x40;
	if(auto segmentp = std::get_if<SegmentIndex>(&reference))
	{
		wr.WriteWord(1, data_byte | (MethodSegment << 2));
		WriteIndex(wr, segmentp->index);
	}
	else if(auto groupp = std::get_if<GroupIndex>(&reference))
	{
		wr.WriteWord(1, data_byte | (MethodGroup << 2));
		WriteIndex(wr, groupp->index);
	}
	else if(auto externalp = std::get_if<ExternalIndex>(&reference))
	{
		wr.WriteWord(1, data_byte | (MethodExternal << 2));
		WriteIndex(wr, externalp->index);
	}
	else if(auto frame_numberp = std::get_if<FrameNumber>(&reference))
	{
		wr.WriteWord(1, data_byte | (MethodFrame << 2));
		WriteIndex(wr, *frame_numberp);
	}
	else if(std::get_if<UsesSource>(&reference))
	{
		wr.WriteWord(1, data_byte | (MethodSource << 2));
	}
	else if(std::get_if<UsesTarget>(&reference))
	{
		wr.WriteWord(1, data_byte | (MethodTarget << 2));
	}
	else if(std::get_if<UsesAbsolute>(&reference))
	{
		wr.WriteWord(1, data_byte | (MethodAbsolute << 2));
	}
	else
	{
		assert(false);
	}
}

void OMF86Format::FixupRecord::Thread::CalculateValues(OMF86Format * omf, Module * mod)
{
	if(auto segmentp = std::get_if<SegmentIndex>(&reference))
	{
		segmentp->CalculateValues(omf, mod);
	}
	else if(auto groupp = std::get_if<GroupIndex>(&reference))
	{
		groupp->CalculateValues(omf, mod);
	}
	else if(auto externalp = std::get_if<ExternalIndex>(&reference))
	{
		externalp->CalculateValues(omf, mod);
	}
}

void OMF86Format::FixupRecord::Thread::ResolveReferences(OMF86Format * omf, Module * mod)
{
	if(auto segmentp = std::get_if<SegmentIndex>(&reference))
	{
		segmentp->ResolveReferences(omf, mod);
	}
	else if(auto groupp = std::get_if<GroupIndex>(&reference))
	{
		groupp->ResolveReferences(omf, mod);
	}
	else if(auto externalp = std::get_if<ExternalIndex>(&reference))
	{
		externalp->ResolveReferences(omf, mod);
	}
}

//// OMF86Format::FixupRecord::Fixup

OMF86Format::FixupRecord::Fixup OMF86Format::FixupRecord::Fixup::Read(OMF86Format * omf, Module * mod, Linker::Reader& rd, uint8_t leading_data_byte, bool is32bit)
{
	Fixup fixup;
	fixup.segment_relative = (leading_data_byte & 0x40) != 0;
	switch(omf->omf_version)
	{
	case OMF_VERSION_INTEL_40:
		fixup.type = relocation_type_t((leading_data_byte >> 2) & 0x7);
		break;
	case OMF_VERSION_TIS_11:
	default:
		fixup.type = relocation_type_t((leading_data_byte >> 2) & 0xF);
		break;
	}

	if(fixup.type == relocation_type_t(5))
	{
		switch(omf->omf_version)
		{
		case OMF_VERSION_PHARLAP:
			fixup.type = RelocationOffset32_PharLap;
			break;
		case OMF_VERSION_TIS_11:
		default:
			fixup.type = RelocationOffset16_LoaderResolved;
			break;
		}
	}

	fixup.offset = ((leading_data_byte & 3) << 8) | rd.ReadUnsigned(1);

	fixup.ref.Read(omf, rd,
		omf->omf_version == OMF_VERSION_INTEL_40
			? ((leading_data_byte & 0x20) != 0 ? 3 : 2)
			: (is32bit ? 4 : 2));

	return fixup;
}

uint16_t OMF86Format::FixupRecord::Fixup::Size(OMF86Format * omf, Module * mod, bool is32bit) const
{
	return 2 + ref.Size(omf, is32bit);
}

void OMF86Format::FixupRecord::Fixup::Write(OMF86Format * omf, Module * mod, ChecksumWriter& wr, bool is32bit) const
{
	uint8_t data_byte = 0x80 | ((type & 0xF) << 2) | (offset >> 8);

	if(segment_relative)
	{
		data_byte |= 0x40;
	}

	if(omf->omf_version == OMF_VERSION_INTEL_40 && ref.displacement > 0xFFFF)
	{
		data_byte |= 0x20;
	}

	wr.WriteWord(1, data_byte);
	wr.WriteWord(1, offset);

	ref.Write(omf, wr, is32bit);
}

void OMF86Format::FixupRecord::Fixup::CalculateValues(OMF86Format * omf, Module * mod)
{
	ref.CalculateValues(omf, mod);
}

void OMF86Format::FixupRecord::Fixup::ResolveReferences(OMF86Format * omf, Module * mod)
{
	ref.ResolveReferences(omf, mod);
}

//// OMF86Format::FixupRecord

void OMF86Format::FixupRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	while(rd.Tell() < RecordEnd())
	{
		uint8_t leading_data_byte = rd.ReadUnsigned(1);
		if((leading_data_byte & 0x80))
		{
			fixup_data.push_back(Fixup::Read(omf, mod, rd, leading_data_byte, Is32Bit(omf)));
		}
		else
		{
			fixup_data.push_back(Thread::Read(omf, mod, rd, leading_data_byte));
		}
	}
}

uint16_t OMF86Format::FixupRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	uint16_t bytes = 1;
	for(auto& data : fixup_data)
	{
		if(auto * thread = std::get_if<Thread>(&data))
		{
			bytes += thread->Size(omf, mod);
		}
		else if(auto * fixup = std::get_if<Fixup>(&data))
		{
			bytes += fixup->Size(omf, mod, Is32Bit(omf));
		}
		else
		{
			assert(false);
		}
	}
	return bytes;
}

void OMF86Format::FixupRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	for(auto& data : fixup_data)
	{
		if(auto * thread = std::get_if<Thread>(&data))
		{
			thread->Write(omf, mod, wr);
		}
		else if(auto * fixup = std::get_if<Fixup>(&data))
		{
			fixup->Write(omf, mod, wr, Is32Bit(omf));
		}
		else
		{
			assert(false);
		}
	}
}

void OMF86Format::FixupRecord::CalculateValues(OMF86Format * omf, Module * mod)
{
	Record::CalculateValues(omf, mod);
	for(auto& data : fixup_data)
	{
		if(auto * thread = std::get_if<Thread>(&data))
		{
			thread->CalculateValues(omf, mod);
		}
		else if(auto * fixup = std::get_if<Fixup>(&data))
		{
			fixup->CalculateValues(omf, mod);
		}
	}
}

void OMF86Format::FixupRecord::ResolveReferences(OMF86Format * omf, Module * mod)
{
	for(auto& data : fixup_data)
	{
		if(auto * thread = std::get_if<Thread>(&data))
		{
			thread->ResolveReferences(omf, mod);
		}
		else if(auto * fixup = std::get_if<Fixup>(&data))
		{
			fixup->ResolveReferences(omf, mod);
		}
	}
}

//// OMF86Format::OverlayDefinitionRecord

void OMF86Format::OverlayDefinitionRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	name = ReadString(rd);
	location = rd.ReadUnsigned(4);
	first_data_record_index = size_t(-1);
	for(size_t index = 0; index < omf->records.size(); index++)
	{
		if(omf->records[index]->record_offset == location)
		{
			first_data_record_index = index;
			break;
		}
	}
	uint8_t attributes = rd.ReadUnsigned(1);
	if((attributes & 0x02))
	{
		shared_overlay = OverlayIndex(ReadIndex(rd));
	}
	if((attributes & 0x01))
	{
		adjacent_overlay = OverlayIndex(ReadIndex(rd));
	}
	omf->modules.back().ovldefs.push_back(shared_from_this());
}

uint16_t OMF86Format::OverlayDefinitionRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	return 7 + name.size() + (shared_overlay ? IndexSize(shared_overlay.value().index) : 0) + (adjacent_overlay ? IndexSize(adjacent_overlay.value().index) : 0);
}

void OMF86Format::OverlayDefinitionRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	WriteString(wr, name);
	wr.WriteWord(4, location);
	uint8_t attributes = 0;
	if(shared_overlay)
	{
		attributes |= 0x02;
	}
	if(adjacent_overlay)
	{
		attributes |= 0x01;
	}
	wr.WriteWord(1, attributes);
	if(shared_overlay)
	{
		WriteIndex(wr, shared_overlay.value().index);
	}
	if(adjacent_overlay)
	{
		WriteIndex(wr, adjacent_overlay.value().index);
	}
}

void OMF86Format::OverlayDefinitionRecord::CalculateValues(OMF86Format * omf, Module * mod)
{
	Record::CalculateValues(omf, mod);
	if(shared_overlay)
	{
		shared_overlay.value().CalculateValues(omf, mod);
	}
	if(adjacent_overlay)
	{
		adjacent_overlay.value().CalculateValues(omf, mod);
	}
}

void OMF86Format::OverlayDefinitionRecord::ResolveReferences(OMF86Format * omf, Module * mod)
{
	if(shared_overlay)
	{
		shared_overlay.value().ResolveReferences(omf, mod);
	}
	if(adjacent_overlay)
	{
		adjacent_overlay.value().ResolveReferences(omf, mod);
	}
}

//// OMF86Format::EndRecord

void OMF86Format::EndRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	block_type = block_type_t(rd.ReadUnsigned(1) & 3);
}

uint16_t OMF86Format::EndRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	return 2;
}

void OMF86Format::EndRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	wr.WriteWord(1, block_type);
}

//// OMF86Format::RegisterInitializationRecord::Register

OMF86Format::RegisterInitializationRecord::Register OMF86Format::RegisterInitializationRecord::Register::Read(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	Register reg;
	uint8_t regtype = rd.ReadUnsigned(1);
	reg.reg_id = register_t(regtype >> 6);
	if((regtype & 1))
	{
		Reference ref;
		ref.Read(omf, rd, 2);
		reg.value = ref;
	}
	else
	{
		InitialValue init;
		init.base.Read(omf, rd);
		switch(reg.reg_id)
		{
		case CS_IP:
		case SS_SP:
			init.offset = rd.ReadUnsigned(2);
			break;
		case DS:
		case ES:
			break;
		}
		reg.value = init;
	}
	return reg;
}

uint16_t OMF86Format::RegisterInitializationRecord::Register::Size(OMF86Format * omf, Module * mod) const
{
	if(auto * ref = std::get_if<Reference>(&value))
	{
		return 1 + ref->Size(omf, false);
	}
	else if(auto * init = std::get_if<InitialValue>(&value))
	{
		uint16_t bytes = 1 + init->base.Size(omf);
		switch(reg_id)
		{
		case CS_IP:
		case SS_SP:
			bytes += 2;
			break;
		case DS:
		case ES:
			break;
		}
		return bytes;
	}
	else
	{
		assert(false);
	}
}

void OMF86Format::RegisterInitializationRecord::Register::Write(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	uint8_t regtype = reg_id << 6;
	if(auto * ref = std::get_if<Reference>(&value))
	{
		regtype |= 1;
		wr.WriteWord(1, regtype);
		ref->Write(omf, wr, false);
	}
	else if(auto * init = std::get_if<InitialValue>(&value))
	{
		wr.WriteWord(1, regtype);
		init->base.Write(omf, wr);
		switch(reg_id)
		{
		case CS_IP:
		case SS_SP:
			wr.WriteWord(2, init->offset);
			break;
		case DS:
		case ES:
			break;
		}
	}
	else
	{
		assert(false);
	}
}

void OMF86Format::RegisterInitializationRecord::Register::CalculateValues(OMF86Format * omf, Module * mod)
{
	if(auto * ref = std::get_if<Reference>(&value))
	{
		ref->CalculateValues(omf, mod);
	}
	else if(auto * init = std::get_if<InitialValue>(&value))
	{
		init->base.CalculateValues(omf, mod);
	}
}

void OMF86Format::RegisterInitializationRecord::Register::ResolveReferences(OMF86Format * omf, Module * mod)
{
	if(auto * ref = std::get_if<Reference>(&value))
	{
		ref->ResolveReferences(omf, mod);
	}
	else if(auto * init = std::get_if<InitialValue>(&value))
	{
		init->base.ResolveReferences(omf, mod);
	}
}

//// OMF86Format::RegisterInitializationRecord

void OMF86Format::RegisterInitializationRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	while(rd.Tell() < RecordEnd())
	{
		registers.push_back(Register::Read(omf, mod, rd));
	}
}

uint16_t OMF86Format::RegisterInitializationRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	uint16_t bytes = 1;
	for(auto& reg : registers)
	{
		bytes += reg.Size(omf, mod);
	}
	return bytes;
}

void OMF86Format::RegisterInitializationRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	for(auto& reg : registers)
	{
		reg.Write(omf, mod, wr);
	}
}

void OMF86Format::RegisterInitializationRecord::CalculateValues(OMF86Format * omf, Module * mod)
{
	Record::CalculateValues(omf, mod);
	for(auto& reg : registers)
	{
		reg.CalculateValues(omf, mod);
	}
}

void OMF86Format::RegisterInitializationRecord::ResolveReferences(OMF86Format * omf, Module * mod)
{
	for(auto& reg : registers)
	{
		reg.ResolveReferences(omf, mod);
	}
}

//// OMF86Format::ModuleEndRecord

void OMF86Format::ModuleEndRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	uint8_t module_type = rd.ReadUnsigned(1);
	main_module = (module_type & 0x80) != 0;
	if((module_type & 0x40))
	{
		if((module_type & 0x01))
		{
			Reference ref;
			ref.Read(omf, rd, GetOffsetSize(omf));
			start_address = ref;
		}
		else
		{
			uint16_t frame = rd.ReadUnsigned(2);
			uint16_t offset = rd.ReadUnsigned(2);
			start_address = std::make_tuple(frame, offset);
		}
	}

	omf->modules.back().record_count = omf->records.size() - omf->modules.back().first_record + 1; // including the current one
}

uint16_t OMF86Format::ModuleEndRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	uint16_t bytes = 1;
	if(start_address)
	{
		if(auto * ref = std::get_if<Reference>(&start_address.value()))
		{
			bytes += ref->Size(omf, Is32Bit(omf));
		}
		else if(std::get_if<std::tuple<uint16_t, uint16_t>>(&start_address.value()))
		{
			bytes += 4;
		}
		else
		{
			assert(false);
		}
	}
	return bytes;
}

void OMF86Format::ModuleEndRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	uint8_t module_type = 0;
	if(main_module)
	{
		module_type |= 0x80;
	}
	if(start_address)
	{
		module_type |= 0x40;
		if(auto * ref = std::get_if<Reference>(&start_address.value()))
		{
			module_type |= 1;
			wr.WriteWord(1, module_type);
			ref->Write(omf, wr, Is32Bit(omf));
		}
		else if(auto * spec = std::get_if<std::tuple<uint16_t, uint16_t>>(&start_address.value()))
		{
			wr.WriteWord(1, module_type);
			wr.WriteWord(2, std::get<0>(*spec));
			wr.WriteWord(2, std::get<1>(*spec));
		}
		else
		{
			assert(false);
		}
	}
	else
	{
		wr.WriteWord(1, module_type);
	}
}

//// OMF86Format::BackpatchRecord

void OMF86Format::BackpatchRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	segment = SegmentIndex(ReadIndex(rd));
	type = rd.ReadUnsigned(1);
	while(rd.Tell() < RecordEnd())
	{
		uint32_t offset = rd.ReadUnsigned(GetOffsetSize(omf));
		uint32_t value = rd.ReadUnsigned(GetOffsetSize(omf));
		offset_value_pairs.push_back(OffsetValuePair { offset, value });
	}
}

uint16_t OMF86Format::BackpatchRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	return 2 + IndexSize(segment.index) + 2 * GetOffsetSize(omf) * offset_value_pairs.size();
}

void OMF86Format::BackpatchRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	WriteIndex(wr, segment.index);
	wr.WriteWord(1, type);
	for(auto offset_value_pair : offset_value_pairs)
	{
		wr.WriteWord(GetOffsetSize(omf), offset_value_pair.offset);
		wr.WriteWord(GetOffsetSize(omf), offset_value_pair.value);
	}
}

void OMF86Format::BackpatchRecord::CalculateValues(OMF86Format * omf, Module * mod)
{
	Record::CalculateValues(omf, mod);
	segment.CalculateValues(omf, mod);
}

void OMF86Format::BackpatchRecord::ResolveReferences(OMF86Format * omf, Module * mod)
{
	segment.ResolveReferences(omf, mod);
}

//// OMF86Format::NamedBackpatchRecord

void OMF86Format::NamedBackpatchRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	type = rd.ReadUnsigned(1);
	switch(omf->omf_version)
	{
	case OMF_VERSION_MICROSOFT:
		name = NameIndex(ReadIndex(rd));
		break;
	case OMF_VERSION_IBM:
		name.text = ReadString(rd);
		break;
	default:
		// unable to parse
		assert(false); // TODO
	}

	while(rd.Tell() < RecordEnd())
	{
		uint32_t offset = rd.ReadUnsigned(GetOffsetSize(omf));
		uint32_t value = rd.ReadUnsigned(GetOffsetSize(omf));
		offset_value_pairs.push_back(OffsetValuePair { offset, value });
	}
}

uint16_t OMF86Format::NamedBackpatchRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	uint16_t bytes = 2 + 2 * GetOffsetSize(omf) * offset_value_pairs.size();

	switch(omf->omf_version)
	{
	case OMF_VERSION_MICROSOFT:
		bytes += IndexSize(name.index);
		break;
	case OMF_VERSION_IBM:
		bytes += 1 + name.text.size();
		break;
	default:
		// unable to generate
		bytes += 1;
	}

	return bytes;
}

void OMF86Format::NamedBackpatchRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	wr.WriteWord(1, type);

	switch(omf->omf_version)
	{
	case OMF_VERSION_MICROSOFT:
		WriteIndex(wr, name.index);
		break;
	case OMF_VERSION_IBM:
		WriteString(wr, name.text);
		break;
	default:
		// unable to generate
		wr.WriteWord(1, 0);
	}

	for(auto offset_value_pair : offset_value_pairs)
	{
		wr.WriteWord(GetOffsetSize(omf), offset_value_pair.offset);
		wr.WriteWord(GetOffsetSize(omf), offset_value_pair.value);
	}
}

void OMF86Format::NamedBackpatchRecord::CalculateValues(OMF86Format * omf, Module * mod)
{
	Record::CalculateValues(omf, mod);
	if(omf->omf_version == OMF_VERSION_MICROSOFT)
	{
		name.CalculateValues(omf, mod);
	}
}

void OMF86Format::NamedBackpatchRecord::ResolveReferences(OMF86Format * omf, Module * mod)
{
	if(omf->omf_version == OMF_VERSION_MICROSOFT)
	{
		name.ResolveReferences(omf, mod);
	}
}

//// OMF86Format::InitializedCommunalDataRecord

void OMF86Format::InitializedCommunalDataRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	uint8_t flags = rd.ReadUnsigned(1);
	continued = (flags & 0x01) != 0;
	iterated = (flags & 0x02) != 0;
	local = (flags & 0x04) != 0;
	code_segment = (flags & 0x08) != 0;

	uint8_t attributes = rd.ReadUnsigned(1);
	selection_criterion = selection_criterion_t(attributes & SelectionCriterionMask);
	allocation_type = allocation_type_t(attributes & AllocationTypeMask);

	alignment = SegmentDefinitionRecord::alignment_t(rd.ReadUnsigned(1));

	offset = rd.ReadUnsigned(GetOffsetSize(omf));

	type = TypeIndex(ReadIndex(rd));

	base.Read(omf, rd);

	switch(omf->omf_version)
	{
	case OMF_VERSION_MICROSOFT:
		name = NameIndex(ReadIndex(rd));
		break;
	case OMF_VERSION_IBM:
		name.text = ReadString(rd);
		break;
	default:
		// unable to parse
		assert(false); // TODO
	}

	if(iterated)
		data = DataBlock::ReadIteratedDataBlock(omf, rd, Is32Bit(omf));
	else
		data = DataBlock::ReadEnumeratedDataBlock(omf, rd, RecordEnd() - rd.Tell());
}

uint16_t OMF86Format::InitializedCommunalDataRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	size_t bytes = 4 + GetOffsetSize(omf) + IndexSize(type.index) + base.Size(omf);

	switch(omf->omf_version)
	{
	case OMF_VERSION_MICROSOFT:
		bytes += IndexSize(name.index);
		break;
	case OMF_VERSION_IBM:
		bytes += 1 + name.text.size();
		break;
	default:
		// unable to generate
		bytes += 1;
	}

	if(iterated)
		bytes += data->GetIteratedDataBlockSize(omf, Is32Bit(omf));
	else
		bytes += data->GetEnumeratedDataBlockSize(omf);

	return bytes;
}

void OMF86Format::InitializedCommunalDataRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	uint8_t flags = 0;
	if(continued)
		flags |= 0x01;
	if(iterated)
		flags |= 0x02;
	if(local)
		flags |= 0x04;
	if(code_segment)
		flags |= 0x08;
	wr.WriteWord(1, flags);

	wr.WriteWord(1, uint8_t(selection_criterion) | uint8_t(allocation_type));
	wr.WriteWord(1, alignment);
	wr.WriteWord(GetOffsetSize(omf), offset);
	WriteIndex(wr, type.index);
	base.Write(omf, wr);

	switch(omf->omf_version)
	{
	case OMF_VERSION_MICROSOFT:
		WriteIndex(wr, name.index);
		break;
	case OMF_VERSION_IBM:
		WriteString(wr, name.text);
		break;
	default:
		// unable to generate
		wr.WriteWord(1, 0);
	}

	if(iterated)
		data->WriteIteratedDataBlock(omf, wr, Is32Bit(omf));
	else
		data->WriteEnumeratedDataBlock(omf, wr);
}

void OMF86Format::InitializedCommunalDataRecord::CalculateValues(OMF86Format * omf, Module * mod)
{
	Record::CalculateValues(omf, mod);
	type.CalculateValues(omf, mod);
	base.CalculateValues(omf, mod);
	if(omf->omf_version == OMF_VERSION_MICROSOFT)
	{
		name.CalculateValues(omf, mod);
	}
}

void OMF86Format::InitializedCommunalDataRecord::ResolveReferences(OMF86Format * omf, Module * mod)
{
	type.ResolveReferences(omf, mod);
	base.ResolveReferences(omf, mod);
	if(omf->omf_version == OMF_VERSION_MICROSOFT)
	{
		name.ResolveReferences(omf, mod);
	}
}

//// OMF86Format::SymbolLineNumbersRecord

void OMF86Format::SymbolLineNumbersRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	uint8_t flags = rd.ReadUnsigned(1);
	continued = (flags & 0x01) != 0;
	switch(omf->omf_version)
	{
	case OMF_VERSION_MICROSOFT:
		name = NameIndex(ReadIndex(rd));
		break;
	case OMF_VERSION_IBM:
		name.text = ReadString(rd);
		break;
	default:
		// unable to parse
		assert(false); // TODO
	}

	while(rd.Tell() < RecordEnd())
	{
		line_numbers.push_back(LineNumber::Read(omf, rd, Is32Bit(omf)));
	}
}

uint16_t OMF86Format::SymbolLineNumbersRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	uint16_t bytes = 2;
	switch(omf->omf_version)
	{
	case OMF_VERSION_MICROSOFT:
		bytes += IndexSize(name.index);
		break;
	case OMF_VERSION_IBM:
		bytes += 1 + name.text.size();
		break;
	default:
		// unable to generate
		bytes += 1;
	}
	bytes += line_numbers.size() * (Is32Bit(omf) ? 6 : 4);
	return bytes;
}

void OMF86Format::SymbolLineNumbersRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	wr.WriteWord(1, continued ? 0x01 : 0x00);
	switch(omf->omf_version)
	{
	case OMF_VERSION_MICROSOFT:
		WriteIndex(wr, name.index);
		break;
	case OMF_VERSION_IBM:
		WriteString(wr, name.text);
		break;
	default:
		// unable to generate
		wr.WriteWord(1, 0);
	}
	for(auto& line_number : line_numbers)
	{
		line_number.Write(omf, wr, Is32Bit(omf));
	}
}

void OMF86Format::SymbolLineNumbersRecord::CalculateValues(OMF86Format * omf, Module * mod)
{
	Record::CalculateValues(omf, mod);
	if(omf->omf_version == OMF_VERSION_MICROSOFT)
	{
		name.CalculateValues(omf, mod);
	}
}

void OMF86Format::SymbolLineNumbersRecord::ResolveReferences(OMF86Format * omf, Module * mod)
{
	if(omf->omf_version == OMF_VERSION_MICROSOFT)
	{
		name.ResolveReferences(omf, mod);
	}
}

//// OMF86Format::AliasDefinitionRecord::AliasDefinition

OMF86Format::AliasDefinitionRecord::AliasDefinition OMF86Format::AliasDefinitionRecord::AliasDefinition::Read(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	AliasDefinition alias;
	alias.alias_name = ReadString(rd);
	alias.substitute_name = ReadString(rd);
	return alias;
}

uint16_t OMF86Format::AliasDefinitionRecord::AliasDefinition::Size(OMF86Format * omf, Module * mod) const
{
	return 2 + alias_name.size() + substitute_name.size();
}

void OMF86Format::AliasDefinitionRecord::AliasDefinition::Write(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	WriteString(wr, alias_name);
	WriteString(wr, substitute_name);
}

//// OMF86Format::AliasDefinitionRecord

void OMF86Format::AliasDefinitionRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	while(rd.Tell() < RecordEnd())
	{
		alias_definitions.push_back(AliasDefinition::Read(omf, mod, rd));
	}
}

uint16_t OMF86Format::AliasDefinitionRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	uint16_t bytes = 1;
	for(auto& alias_definition : alias_definitions)
	{
		bytes += alias_definition.Size(omf, mod);
	}
	return bytes;
}

void OMF86Format::AliasDefinitionRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	for(auto& alias_definition : alias_definitions)
	{
		alias_definition.Write(omf, mod, wr);
	}
}

//// OMF86Format::OMFVersionNumberRecord

void OMF86Format::OMFVersionNumberRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	version = ReadString(rd);
}

uint16_t OMF86Format::OMFVersionNumberRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	return 2 + version.size();
}

void OMF86Format::OMFVersionNumberRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	WriteString(wr, version);
}

//// OMF86Format::VendorExtensionRecord

void OMF86Format::VendorExtensionRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	vendor_number = rd.ReadUnsigned(2);
	extension.resize(record_length - 3);
	rd.ReadData(extension);
}

uint16_t OMF86Format::VendorExtensionRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	return 3 + extension.size();
}

void OMF86Format::VendorExtensionRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	wr.WriteWord(2, vendor_number);
	wr.WriteData(extension);
}

//// OMF86Format::CommentRecord

std::shared_ptr<OMF86Format::CommentRecord> OMF86Format::CommentRecord::ReadCommentRecord(OMF86Format * omf, Module * mod, Linker::Reader& rd, uint16_t record_length)
{
	uint8_t comment_type = rd.ReadUnsigned(1);
	uint8_t comment_class = rd.ReadUnsigned(1);

	std::shared_ptr<OMF86Format::CommentRecord> record;

	switch(comment_class)
	{
	case Translator:
	case IntelCopyright:
	case LibrarySpecifier:
	case MSDOSVersion:
		record = std::make_shared<GenericCommentRecord>(comment_class_t(comment_class));
		break;
	case MemoryModel:
		// TODO: parse further?
		record = std::make_shared<GenericCommentRecord>(comment_class_t(comment_class));
		break;
	case DOSSEG:
		record = std::make_shared<EmptyCommentRecord>(comment_class_t(comment_class));
		break;
	case DefaultLibrarySearchName:
		record = std::make_shared<GenericCommentRecord>(comment_class_t(comment_class));
		break;
	case OMFExtension:
		{
			uint8_t subtype = rd.ReadUnsigned(1);
			switch(subtype)
			{
			case OMFExtensionRecord::IMPDEF:
				record = std::make_shared<ImportDefinitionRecord>();
				break;
			case OMFExtensionRecord::EXPDEF:
				record = std::make_shared<ExportDefinitionRecord>();
				break;
			case OMFExtensionRecord::INCDEF:
				record = std::make_shared<IncrementalCompilationRecord>();
				break;
			case OMFExtensionRecord::ProtectedModeLibrary:
				record = std::make_shared<OMFExtensionRecord::EmptyOMFExtensionRecord>(OMFExtensionRecord::extension_type_t(subtype));
				break;
			case OMFExtensionRecord::LNKDIR:
				record = std::make_shared<LinkerDirectivesRecord>();
				break;
			case OMFExtensionRecord::BigEndian:
				record = std::make_shared<OMFExtensionRecord::EmptyOMFExtensionRecord>(OMFExtensionRecord::extension_type_t(subtype));
				break;
			case OMFExtensionRecord::PRECOMP:
				record = std::make_shared<OMFExtensionRecord::EmptyOMFExtensionRecord>(OMFExtensionRecord::extension_type_t(subtype));
				break;
			default:
				record = std::make_shared<OMFExtensionRecord::GenericOMFExtensionRecord>(OMFExtensionRecord::extension_type_t(subtype));
				break;
			}
		}
		break;
	case NewOMFExtension:
		// TODO: parse further?
		record = std::make_shared<GenericCommentRecord>(comment_class_t(comment_class));
		break;
	case LinkPassSeparator:
		record = std::make_shared<EmptyCommentRecord>(comment_class_t(comment_class));
		break;
	case LIBMOD:
		record = std::make_shared<TextCommentRecord>(comment_class_t(comment_class));
		break;
	case EXESTR:
		record = std::make_shared<TextCommentRecord>(comment_class_t(comment_class));
		break;
	case INCERR:
		record = std::make_shared<EmptyCommentRecord>(comment_class_t(comment_class));
		break;
	case NOPAD:
		record = std::make_shared<NoSegmentPaddingRecord>();
		break;
	case WKEXT:
	case LZEXT:
		record = std::make_shared<ExternalAssociationRecord>(comment_class_t(comment_class));
		break;
	case Comment:
	case Compiler:
	case Date:
	case TimeStamp:
	case User:
	case DependencyFile:
	case CommandLine:
		record = std::make_shared<GenericCommentRecord>(comment_class_t(comment_class));
		break;
	}

	record->no_purge = (comment_type & 0x80) != 0;
	record->no_list = (comment_type & 0x40) != 0;
	record->ReadComment(omf, mod, rd, record_length - 3);
	rd.ReadUnsigned(1); // checksum
	return record;
}

void OMF86Format::CommentRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	assert(false);
}

uint16_t OMF86Format::CommentRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	return 3 + GetCommentSize(omf, mod);
}

void OMF86Format::CommentRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	uint8_t comment_type = 0;
	if(no_purge)
		comment_type |= 0x80;
	if(no_list)
		comment_type |= 0x40;
	wr.WriteWord(1, comment_type);
	wr.WriteWord(1, comment_class);
	WriteComment(omf, mod, wr);
}

//// OMF86Format::CommentRecord::GenericCommentRecord

void OMF86Format::CommentRecord::GenericCommentRecord::ReadComment(OMF86Format * omf, Module * mod, Linker::Reader& rd, uint16_t comment_length)
{
	data.resize(comment_length);
	rd.ReadData(data);
}

uint16_t OMF86Format::CommentRecord::GenericCommentRecord::GetCommentSize(OMF86Format * omf, Module * mod) const
{
	return data.size();
}

void OMF86Format::CommentRecord::GenericCommentRecord::WriteComment(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	wr.WriteData(data);
}

//// OMF86Format::CommentRecord::EmptyCommentRecord

void OMF86Format::CommentRecord::EmptyCommentRecord::ReadComment(OMF86Format * omf, Module * mod, Linker::Reader& rd, uint16_t comment_length)
{
}

uint16_t OMF86Format::CommentRecord::EmptyCommentRecord::GetCommentSize(OMF86Format * omf, Module * mod) const
{
	return 0;
}

void OMF86Format::CommentRecord::EmptyCommentRecord::WriteComment(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
}

//// OMF86Format::CommentRecord::TextCommentRecord

void OMF86Format::CommentRecord::TextCommentRecord::ReadComment(OMF86Format * omf, Module * mod, Linker::Reader& rd, uint16_t comment_length)
{
	name = ReadString(rd);
}

uint16_t OMF86Format::CommentRecord::TextCommentRecord::GetCommentSize(OMF86Format * omf, Module * mod) const
{
	return 1 + name.size();
}

void OMF86Format::CommentRecord::TextCommentRecord::WriteComment(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	WriteString(wr, name);
}

//// OMF86Format::NoSegmentPaddingRecord

void OMF86Format::NoSegmentPaddingRecord::ReadComment(OMF86Format * omf, Module * mod, Linker::Reader& rd, uint16_t comment_length)
{
	while(rd.Tell() < RecordEnd())
	{
		segments.push_back(SegmentIndex(ReadIndex(rd)));
	}
}

uint16_t OMF86Format::NoSegmentPaddingRecord::GetCommentSize(OMF86Format * omf, Module * mod) const
{
	uint16_t bytes = 0;
	for(auto& segment : segments)
	{
		bytes += IndexSize(segment.index);
	}
	return bytes;
}

void OMF86Format::NoSegmentPaddingRecord::WriteComment(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	for(auto& segment : segments)
	{
		WriteIndex(wr, segment.index);
	}
}


void OMF86Format::NoSegmentPaddingRecord::CalculateValues(OMF86Format * omf, Module * mod)
{
	Record::CalculateValues(omf, mod);
}

void OMF86Format::NoSegmentPaddingRecord::ResolveReferences(OMF86Format * omf, Module * mod)
{
}

//// OMF86Format::ExternalAssociationRecord::ExternalAssociation

OMF86Format::ExternalAssociationRecord::ExternalAssociation OMF86Format::ExternalAssociationRecord::ExternalAssociation::Read(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	ExternalAssociation association;
	association.definition = ExternalIndex(ReadIndex(rd));
	association.default_resolution = ExternalIndex(ReadIndex(rd));
	return association;
}

uint16_t OMF86Format::ExternalAssociationRecord::ExternalAssociation::Size(OMF86Format * omf, Module * mod) const
{
	return IndexSize(definition.index) + IndexSize(default_resolution.index);
}

void OMF86Format::ExternalAssociationRecord::ExternalAssociation::Write(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	WriteIndex(wr, definition.index);
	WriteIndex(wr, default_resolution.index);
}

void OMF86Format::ExternalAssociationRecord::ExternalAssociation::CalculateValues(OMF86Format * omf, Module * mod)
{
	definition.CalculateValues(omf, mod);
	default_resolution.CalculateValues(omf, mod);
}

void OMF86Format::ExternalAssociationRecord::ExternalAssociation::ResolveReferences(OMF86Format * omf, Module * mod)
{
	definition.ResolveReferences(omf, mod);
	default_resolution.ResolveReferences(omf, mod);
}

//// OMF86Format::ExternalAssociationRecord

void OMF86Format::ExternalAssociationRecord::ReadComment(OMF86Format * omf, Module * mod, Linker::Reader& rd, uint16_t comment_length)
{
	while(rd.Tell() < RecordEnd())
	{
		associations.push_back(ExternalAssociation::Read(omf, mod, rd));
	}
}

uint16_t OMF86Format::ExternalAssociationRecord::GetCommentSize(OMF86Format * omf, Module * mod) const
{
	uint16_t bytes = 0;
	for(auto& association : associations)
	{
		bytes += association.Size(omf, mod);
	}
	return bytes;
}

void OMF86Format::ExternalAssociationRecord::WriteComment(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	for(auto& association : associations)
	{
		association.Write(omf, mod, wr);
	}
}

void OMF86Format::ExternalAssociationRecord::CalculateValues(OMF86Format * omf, Module * mod)
{
	Record::CalculateValues(omf, mod);
	for(auto& association : associations)
	{
		association.CalculateValues(omf, mod);
	}
}

void OMF86Format::ExternalAssociationRecord::ResolveReferences(OMF86Format * omf, Module * mod)
{
	for(auto& association : associations)
	{
		association.ResolveReferences(omf, mod);
	}
}

//// OMF86Format::OMFExtensionRecord::GenericOMFExtensionRecord

void OMF86Format::OMFExtensionRecord::GenericOMFExtensionRecord::ReadComment(OMF86Format * omf, Module * mod, Linker::Reader& rd, uint16_t comment_length)
{
	data.resize(comment_length - 1);
	rd.ReadData(data);
}

uint16_t OMF86Format::OMFExtensionRecord::GenericOMFExtensionRecord::GetCommentSize(OMF86Format * omf, Module * mod) const
{
	return data.size() + 1;
}

void OMF86Format::OMFExtensionRecord::GenericOMFExtensionRecord::WriteComment(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	wr.WriteWord(1, subtype);
	wr.WriteData(data);
}

//// OMF86Format::OMFExtensionRecord::EmptyOMFExtensionRecord

void OMF86Format::OMFExtensionRecord::EmptyOMFExtensionRecord::ReadComment(OMF86Format * omf, Module * mod, Linker::Reader& rd, uint16_t comment_length)
{
}

uint16_t OMF86Format::OMFExtensionRecord::EmptyOMFExtensionRecord::GetCommentSize(OMF86Format * omf, Module * mod) const
{
	return 1;
}

void OMF86Format::OMFExtensionRecord::EmptyOMFExtensionRecord::WriteComment(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	wr.WriteWord(1, subtype);
}

//// OMF86Format::ImportDefinitionRecord

void OMF86Format::ImportDefinitionRecord::ReadComment(OMF86Format * omf, Module * mod, Linker::Reader& rd, uint16_t comment_length)
{
	uint8_t ordinal_flag = rd.ReadUnsigned(1);
	internal_name = ReadString(rd);
	module_name = ReadString(rd);
	if(ordinal_flag)
	{
		entry_ident = uint16_t(rd.ReadUnsigned(2));
	}
	else
	{
		entry_ident = ReadString(rd);
	}
}

uint16_t OMF86Format::ImportDefinitionRecord::GetCommentSize(OMF86Format * omf, Module * mod) const
{
	uint16_t bytes = 4 + internal_name.size() + module_name.size();

	if(std::get_if<uint16_t>(&entry_ident))
	{
		bytes += 2;
	}
	else if(auto * imported_name = std::get_if<std::string>(&entry_ident))
	{
		bytes += 1 + imported_name->size();
	}
	else
	{
		assert(false);
	}

	return bytes;
}

void OMF86Format::ImportDefinitionRecord::WriteComment(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	wr.WriteWord(1, subtype);

	if(std::get_if<uint16_t>(&entry_ident))
	{
		wr.WriteWord(1, 1);
	}
	else
	{
		wr.WriteWord(1, 0);
	}

	WriteString(wr, internal_name);
	WriteString(wr, module_name);

	if(auto * ordinal = std::get_if<uint16_t>(&entry_ident))
	{
		wr.WriteWord(2, *ordinal);
	}
	else if(auto * imported_name = std::get_if<std::string>(&entry_ident))
	{
		WriteString(wr, *imported_name);
	}
	else
	{
		assert(false);
	}
}

//// OMF86Format::ExportDefinitionRecord

void OMF86Format::ExportDefinitionRecord::ReadComment(OMF86Format * omf, Module * mod, Linker::Reader& rd, uint16_t comment_length)
{
	uint8_t exported_flag = rd.ReadUnsigned(1);
	resident_name = (exported_flag & 0x40) != 0;
	no_data = (exported_flag & 0x20) != 0;
	parameter_count = exported_flag & 0x1F;
	exported_name = ReadString(rd);
	internal_name = ReadString(rd);
	if((exported_flag & 0x80))
	{
		ordinal = rd.ReadUnsigned(2);
	}
}

uint16_t OMF86Format::ExportDefinitionRecord::GetCommentSize(OMF86Format * omf, Module * mod) const
{
	uint16_t bytes = 4 + exported_name.size() + internal_name.size();

	if(ordinal)
	{
		bytes += 2;
	}

	return bytes;
}

void OMF86Format::ExportDefinitionRecord::WriteComment(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	wr.WriteWord(1, subtype);

	uint8_t exported_flag = parameter_count;

	if(ordinal)
	{
		exported_flag |= 0x80;
	}

	if(resident_name)
	{
		exported_flag |= 0x40;
	}

	if(no_data)
	{
		exported_flag |= 0x20;
	}

	WriteString(wr, exported_name);
	WriteString(wr, internal_name);

	if(ordinal)
	{
		wr.WriteWord(2, ordinal.value());
	}
}

//// OMF86Format::IncrementalCompilationRecord

void OMF86Format::IncrementalCompilationRecord::ReadComment(OMF86Format * omf, Module * mod, Linker::Reader& rd, uint16_t comment_length)
{
	extdef_delta = rd.ReadUnsigned(2);
	linnum_delta = rd.ReadUnsigned(2);
	padding_byte_count = comment_length - 5;
}

uint16_t OMF86Format::IncrementalCompilationRecord::GetCommentSize(OMF86Format * omf, Module * mod) const
{
	return padding_byte_count + 5;
}

void OMF86Format::IncrementalCompilationRecord::WriteComment(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	wr.WriteWord(1, subtype);

	wr.WriteWord(2, extdef_delta);
	wr.WriteWord(2, linnum_delta);

	wr.Skip(padding_byte_count);
}

//// OMF86Format::LinkerDirectivesRecord

void OMF86Format::LinkerDirectivesRecord::ReadComment(OMF86Format * omf, Module * mod, Linker::Reader& rd, uint16_t comment_length)
{
	uint8_t flags = rd.ReadUnsigned(1);
	new_executable = (flags & FlagNewExecutable) != 0;
	omit_codeview_publics = (flags & FlagOmitCodeViewPublics) != 0;
	run_mpc = (flags & FlagRunMPC) != 0;

	pseudocode_version = rd.ReadUnsigned(1);
	codeview_version = rd.ReadUnsigned(1);
}

uint16_t OMF86Format::LinkerDirectivesRecord::GetCommentSize(OMF86Format * omf, Module * mod) const
{
	return 4;
}

void OMF86Format::LinkerDirectivesRecord::WriteComment(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	wr.WriteWord(1, subtype);

	wr.WriteWord(1, (new_executable ? FlagNewExecutable : 0) | (omit_codeview_publics ? FlagOmitCodeViewPublics : 0) | (run_mpc ? FlagRunMPC : 0));
	wr.WriteWord(1, pseudocode_version);
	wr.WriteWord(1, codeview_version);
}

//// OMF86Format::TISLibraryHeaderRecord

void OMF86Format::TISLibraryHeaderRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	omf->page_size = record_length + 3;
	dictionary_offset = rd.ReadUnsigned(4);
	dictionary_size = rd.ReadUnsigned(2);
	uint8_t flags = rd.ReadUnsigned(1);
	case_sensitive = (flags & 0x01) != 0;

	rd.Skip(omf->page_size - 10);
}

uint16_t OMF86Format::TISLibraryHeaderRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	return omf->page_size - 3;
}

void OMF86Format::TISLibraryHeaderRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	assert(false);
}

void OMF86Format::TISLibraryHeaderRecord::WriteRecord(OMF86Format * omf, Module * mod, Linker::Writer& wr) const
{
	uint16_t page_size = dynamic_cast<OMF86Format *>(omf)->page_size;
	wr.WriteWord(1, record_type);
	wr.WriteWord(2, page_size - 3);
	wr.WriteWord(4, dictionary_offset);
	wr.WriteWord(2, dictionary_size);
	uint8_t flags = 0;
	if(case_sensitive)
		flags |= 0x01;
	wr.WriteWord(1, flags);
	wr.Skip(page_size - 10);
}

//// OMF86Format::TISLibraryEndRecord

void OMF86Format::TISLibraryEndRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	rd.Skip(omf->page_size - 3);
}

uint16_t OMF86Format::TISLibraryEndRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	return omf->page_size - 3;
}

void OMF86Format::TISLibraryEndRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	assert(false);
}

void OMF86Format::TISLibraryEndRecord::WriteRecord(OMF86Format * omf, Module * mod, Linker::Writer& wr) const
{
	uint16_t page_size = dynamic_cast<OMF86Format *>(omf)->page_size;
	wr.WriteWord(1, record_type);
	wr.WriteWord(2, page_size - 3);
	wr.Skip(page_size - 3);
}

//// OMF86Format

std::shared_ptr<OMF86Format::Record> OMF86Format::ReadRecord(Linker::Reader& rd)
{
	Module * mod = modules.size() > 0 ? &modules.back() : nullptr;
	offset_t record_offset = rd.Tell();
	uint8_t record_type = rd.ReadUnsigned(1);
	uint16_t record_length = rd.ReadUnsigned(2);
	std::shared_ptr<Record> record;
Linker::Debug << "Debug: record 0x" << std::hex << int(record_type) << " at offset 0x" << std::hex << record_offset << std::endl;
	switch(record_type)
	{
	case RHEADR:
		record = std::make_shared<RModuleHeaderRecord>(record_type_t(record_type));
		break;
	case REGINT:
		record = std::make_shared<RegisterInitializationRecord>(record_type_t(record_type));
		break;
	case REDATA:
	case RIDATA:
		record = std::make_shared<RelocatableDataRecord>(record_type_t(record_type));
		break;
	case OVLDEF:
		record = std::make_shared<OverlayDefinitionRecord>(record_type_t(record_type));
		break;
	case ENDREC:
		record = std::make_shared<EndRecord>(record_type_t(record_type));
		break;
	case BLKDEF:
		record = std::make_shared<BlockDefinitionRecord>(record_type_t(record_type));
		break;
	case BLKEND:
		record = std::make_shared<EmptyRecord>(record_type_t(record_type));
		break;
	case DEBSYM:
		record = std::make_shared<DebugSymbolsRecord>(record_type_t(record_type));
		break;
	case THEADR:
	case LHEADR:
		record = std::make_shared<ModuleHeaderRecord>(record_type_t(record_type));
		break;
	case PEDATA:
	case PIDATA:
		record = std::make_shared<PhysicalDataRecord>(record_type_t(record_type));
		break;
	case COMENT:
		return CommentRecord::ReadCommentRecord(this, mod, rd, record_length);
	case MODEND16:
	case MODEND32:
		record = std::make_shared<ModuleEndRecord>(record_type_t(record_type));
		break;
	case EXTDEF:
		record = std::make_shared<ExternalNamesDefinitionRecord>(record_type_t(record_type));
		break;
	case TYPDEF:
		record = std::make_shared<TypeDefinitionRecord>(record_type_t(record_type));
		break;
	case PUBDEF16:
	case PUBDEF32:
	case LOCSYM:
		record = std::make_shared<SymbolsDefinitionRecord>(record_type_t(record_type));
		break;
	case LINNUM:
		record = std::make_shared<LineNumbersRecord>(record_type_t(record_type));
		break;
	case LNAMES:
		record = std::make_shared<ListOfNamesRecord>(record_type_t(record_type));
		break;
	case SEGDEF:
		record = std::make_shared<SegmentDefinitionRecord>(record_type_t(record_type));
		break;
	case GRPDEF:
		record = std::make_shared<GroupDefinitionRecord>(record_type_t(record_type));
		break;
	case FIXUPP16:
	case FIXUPP32:
		record = std::make_shared<FixupRecord>(record_type_t(record_type));
		break;
	case LEDATA16:
	case LEDATA32:
	case LIDATA16:
	case LIDATA32:
		record = std::make_shared<LogicalDataRecord>(record_type_t(record_type));
		break;
	case LIBHED:
		record = std::make_shared<LibraryHeaderRecord>(record_type_t(record_type));
		break;
	case LIBNAM:
		record = std::make_shared<LibraryModuleNamesRecord>(record_type_t(record_type));
		break;
	case LIBLOC:
		record = std::make_shared<LibraryModuleLocationsRecord>(record_type_t(record_type));
		break;
	case LIBDIC:
		record = std::make_shared<LibraryDictionaryRecord>(record_type_t(record_type));
		break;
	case COMDEF:
		record = std::make_shared<ExternalNamesDefinitionRecord>(record_type_t(record_type));
		break;
	case BAKPAT16:
	case BAKPAT32:
		record = std::make_shared<BackpatchRecord>(record_type_t(record_type));
		break;
	case LEXTDEF16:
	case LEXTDEF32:
		record = std::make_shared<ExternalNamesDefinitionRecord>(record_type_t(record_type));
		break;
	case LPUBDEF16:
	case LPUBDEF32:
		record = std::make_shared<SymbolsDefinitionRecord>(record_type_t(record_type));
		break;
	case LCOMDEF:
	case CEXTDEF:
		record = std::make_shared<ExternalNamesDefinitionRecord>(record_type_t(record_type));
		break;
	case COMDAT16:
	case COMDAT32:
		record = std::make_shared<InitializedCommunalDataRecord>(record_type_t(record_type));
		break;
	case LINSYM16:
	case LINSYM32:
		record = std::make_shared<SymbolLineNumbersRecord>(record_type_t(record_type));
		break;
	case ALIAS:
		record = std::make_shared<AliasDefinitionRecord>(record_type_t(record_type));
		break;
	case NBKPAT16:
	case NBKPAT32:
		record = std::make_shared<NamedBackpatchRecord>(record_type_t(record_type));
		break;
	case LLNAMES:
		record = std::make_shared<ListOfNamesRecord>(record_type_t(record_type));
		break;
	case VERNUM:
		record = std::make_shared<OMFVersionNumberRecord>(record_type_t(record_type));
		break;
	case VENDEXT:
		record = std::make_shared<VendorExtensionRecord>(record_type_t(record_type));
		break;
	case LibraryHeader:
		record = std::make_shared<TISLibraryHeaderRecord>(record_type_t(record_type));
		record->record_offset = record_offset;
		record->record_length = record_length;
		record->ReadRecordContents(this, mod, rd);
		return record;
	case LibraryEnd:
		record = std::make_shared<TISLibraryEndRecord>(record_type_t(record_type));
		record->record_offset = record_offset;
		record->record_length = record_length;
		record->ReadRecordContents(this, mod, rd);
		return record;
	default:
		record = std::make_shared<UnknownRecord>(record_type_t(record_type));
	}
	record->record_offset = record_offset;
	record->record_length = record_length;
	record->ReadRecordContents(this, mod, rd);
	rd.ReadUnsigned(1); // checksum
	records.push_back(record);
	return record;
}

std::shared_ptr<OMF86Format> OMF86Format::ReadOMFFile(Linker::Reader& rd)
{
	std::shared_ptr<OMF86Format> omf = std::make_shared<OMF86Format>();
	omf->ReadFile(rd);
	return omf;
}

void OMF86Format::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = LittleEndian;
	rd.SeekEnd();
	offset_t end = rd.Tell();
	rd.Seek(0);

	while(rd.Tell() < end)
	{
		ReadRecord(rd);
	}

	file_size = rd.Tell();
}

offset_t OMF86Format::WriteFile(Linker::Writer& wr) const
{
	/* TODO */

	return offset_t(-1);
}

void OMF86Format::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("Intel OMF-86 format");
	Dumper::Region file_region("File", file_offset, file_size, 8);
	file_region.Display(dump);

	size_t record_index = 0;
	ssize_t module_index = -1;
	for(auto record : records)
	{
		if(size_t(module_index + 1) < modules.size() && modules[module_index + 1].first_record == record_index)
		{
			module_index ++;

			Dumper::Region module_region("Module", record->record_offset, records[record_index + modules[module_index].record_count - 1]->RecordEnd() + 1 - record->record_offset, 8);
			module_region.InsertField(0, "Index", Dumper::DecDisplay::Make(), offset_t(module_index + 1));
			module_region.Display(dump);
		}

		record->Dump(dump, this, module_index >= 0 ? &modules[module_index] : nullptr, record_index);
		record_index++;
	}
}

void OMF86Format::GenerateModule(Linker::Module& module) const
{
	/* TODO */
}

//// OMF80Format::ExternalNameIndex

void OMF80Format::ExternalNameIndex::CalculateValues(OMF80Format * omf, Module * mod)
{
	// TODO
}

void OMF80Format::ExternalNameIndex::ResolveReferences(OMF80Format * omf, Module * mod)
{
	external_name = mod->external_names[index];
}

//// OMF80Format::ModuleHeaderRecord::SegmentDefinition

OMF80Format::ModuleHeaderRecord::SegmentDefinition OMF80Format::ModuleHeaderRecord::SegmentDefinition::Read(OMF80Format * omf, Linker::Reader& rd)
{
	OMF80Format::ModuleHeaderRecord::SegmentDefinition segment_definition;
	segment_definition.segment_id = rd.ReadUnsigned(1);
	segment_definition.length = rd.ReadUnsigned(2);
	segment_definition.alignment = alignment_t(rd.ReadUnsigned(1));
	return segment_definition;
}

void OMF80Format::ModuleHeaderRecord::SegmentDefinition::Write(OMF80Format * omf, ChecksumWriter& wr) const
{
	wr.WriteWord(1, segment_id);
	wr.WriteWord(2, length);
	wr.WriteWord(1, alignment);
}

//// OMF80Format::ModuleHeaderRecord

void OMF80Format::ModuleHeaderRecord::ReadRecordContents(OMF80Format * omf, Module * mod, Linker::Reader& rd)
{
	offset_t record_end = record_offset + record_length + 2;
	name = ReadString(rd);
	rd.Skip(2);
	while(rd.Tell() < record_end)
	{
		SegmentDefinition segment_definition = SegmentDefinition::Read(omf, rd);
		segment_definitions.push_back(segment_definition);
	}
	omf->modules.push_back(Module());
}

uint16_t OMF80Format::ModuleHeaderRecord::GetRecordSize(OMF80Format * omf, Module * mod) const
{
	return 3 + name.size() + 4 * segment_definitions.size();
}

void OMF80Format::ModuleHeaderRecord::WriteRecordContents(OMF80Format * omf, Module * mod, ChecksumWriter& wr) const
{
	WriteString(wr, name);
	wr.WriteWord(2, 0); // reserved
	for(auto& segment_definition : segment_definitions)
	{
		segment_definition.Write(omf, wr);
	}
}

void OMF80Format::ModuleHeaderRecord::CalculateValues(OMF80Format * omf, Module * mod)
{
	segment_definitions.clear();
	for(auto& segment_entry : omf->modules.back().segment_definitions)
	{
		segment_definitions.push_back(segment_entry.second);
	}
}

void OMF80Format::ModuleHeaderRecord::ResolveReferences(OMF80Format * omf, Module * mod)
{
	omf->modules.back().segment_definitions.clear();
	for(auto& segment_definition : segment_definitions)
	{
		omf->modules.back().segment_definitions[segment_definition.segment_id] = OMF80Format::SegmentDefinition(segment_definition); // TODO: check it is not duplicated
	}
}

//// OMF80Format::ModuleEndRecord

void OMF80Format::ModuleEndRecord::ReadRecordContents(OMF80Format * omf, Module * mod, Linker::Reader& rd)
{
	main = rd.ReadUnsigned(1) != 0;
	start_segment_id = rd.ReadUnsigned(1);
	start_offset = rd.ReadUnsigned(2);
	if(record_length > 5)
	{
		info.resize(record_length - 5);
		rd.ReadData(info);
	}
	else
	{
		info.clear();
	}
}

uint16_t OMF80Format::ModuleEndRecord::GetRecordSize(OMF80Format * omf, Module * mod) const
{
	return 5 + info.size();
}

void OMF80Format::ModuleEndRecord::WriteRecordContents(OMF80Format * omf, Module * mod, ChecksumWriter& wr) const
{
	wr.WriteWord(1, main ? 1 : 0);
	wr.WriteWord(1, start_segment_id);
	wr.WriteWord(2, start_offset);
	wr.WriteData(info);
}

void OMF80Format::ModuleEndRecord::CalculateValues(OMF80Format * omf, Module * mod)
{
}

void OMF80Format::ModuleEndRecord::ResolveReferences(OMF80Format * omf, Module * mod)
{
}

//// OMF80Format::NamedCommonDefinitionsRecord::NamedCommonDefinition

OMF80Format::NamedCommonDefinitionsRecord::NamedCommonDefinition OMF80Format::NamedCommonDefinitionsRecord::NamedCommonDefinition::ReadNamedCommonDefinition(OMF80Format * omf, Linker::Reader& rd)
{
	OMF80Format::NamedCommonDefinitionsRecord::NamedCommonDefinition named_common_definition;
	named_common_definition.segment_id = rd.ReadUnsigned(1);
	named_common_definition.common_name = ReadString(rd);
	return named_common_definition;
}

uint16_t OMF80Format::NamedCommonDefinitionsRecord::NamedCommonDefinition::GetNamedCommonDefinitionSize(OMF80Format * omf) const
{
	return 2 + common_name.size();
}

void OMF80Format::NamedCommonDefinitionsRecord::NamedCommonDefinition::WriteNamedCommonDefinition(OMF80Format * omf, ChecksumWriter& wr) const
{
	wr.WriteWord(1, segment_id);
	WriteString(wr, common_name);
}

//// OMF80Format::NamedCommonDefinitionsRecord

void OMF80Format::NamedCommonDefinitionsRecord::ReadRecordContents(OMF80Format * omf, Module * mod, Linker::Reader& rd)
{
	offset_t record_end = record_offset + record_length + 2;
	while(rd.Tell() < record_end)
	{
		NamedCommonDefinition named_common_definition = NamedCommonDefinition::ReadNamedCommonDefinition(omf, rd);
		named_common_definitions.push_back(named_common_definition);
	}
}

uint16_t OMF80Format::NamedCommonDefinitionsRecord::GetRecordSize(OMF80Format * omf, Module * mod) const
{
	uint16_t bytes = 1;
	for(auto& named_common_definition : named_common_definitions)
	{
		bytes += 1 + named_common_definition.GetNamedCommonDefinitionSize(omf);
	}
	return bytes;
}

void OMF80Format::NamedCommonDefinitionsRecord::WriteRecordContents(OMF80Format * omf, Module * mod, ChecksumWriter& wr) const
{
	for(auto& named_common_definition : named_common_definitions)
	{
		named_common_definition.WriteNamedCommonDefinition(omf, wr);
	}
}

void OMF80Format::NamedCommonDefinitionsRecord::CalculateValues(OMF80Format * omf, Module * mod)
{
	named_common_definitions.clear();
	for(auto& segment_entry : omf->modules.back().segment_definitions)
	{
		named_common_definitions.push_back(NamedCommonDefinition { segment_entry.second.segment_id, segment_entry.second.common_name });
	}
}

void OMF80Format::NamedCommonDefinitionsRecord::ResolveReferences(OMF80Format * omf, Module * mod)
{
	omf->modules.back().segment_definitions.clear();
	for(auto& named_common_definition : named_common_definitions)
	{
		omf->modules.back().segment_definitions[named_common_definition.segment_id].common_name = named_common_definition.common_name; // TODO: check it is present and not duplicated
	}
}

//// OMF80Format::ExternalDefinitionsRecord

void OMF80Format::ExternalDefinitionsRecord::ReadRecordContents(OMF80Format * omf, Module * mod, Linker::Reader& rd)
{
	first_external_name = mod->external_names.size();
	while(rd.Tell() < record_offset + record_length)
	{
		mod->external_names.push_back(ReadString(rd));
		rd.Skip(1);
	}
	external_name_count = external_names.size();
}

uint16_t OMF80Format::ExternalDefinitionsRecord::GetRecordSize(OMF80Format * omf, Module * mod) const
{
	uint16_t bytes = 1;
	for(uint16_t external_name_index = first_external_name; external_name_index < first_external_name + external_name_count; external_name_index++)
	{
		bytes += 2 + mod->external_names[external_name_index].size();
	}
	return bytes;
}

void OMF80Format::ExternalDefinitionsRecord::WriteRecordContents(OMF80Format * omf, Module * mod, ChecksumWriter& wr) const
{
	for(uint16_t external_name_index = first_external_name; external_name_index < first_external_name + external_name_count; external_name_index++)
	{
		WriteString(wr, mod->external_names[external_name_index]);
		wr.Skip(1);
	}
}

void OMF80Format::ExternalDefinitionsRecord::CalculateValues(OMF80Format * omf, Module * mod)
{
	assert(mod == this->mod);
	first_external_name = mod->external_names.size();
	for(auto& external_name : external_names)
	{
		mod->external_names.push_back(external_name);
	}
	external_name_count = external_names.size();
}

void OMF80Format::ExternalDefinitionsRecord::ResolveReferences(OMF80Format * omf, Module * mod)
{
	assert(mod == this->mod);
}

//// OMF80Format::SymbolDefinitionsRecord::SymbolDefinition

OMF80Format::SymbolDefinitionsRecord::SymbolDefinition OMF80Format::SymbolDefinitionsRecord::SymbolDefinition::Read(OMF80Format * omf, Module * mod, Linker::Reader& rd)
{
	SymbolDefinition public_definition;
	public_definition.offset = rd.ReadUnsigned(2);
	public_definition.name = ReadString(rd);
	rd.Skip(1);
	return public_definition;
}

uint16_t OMF80Format::SymbolDefinitionsRecord::SymbolDefinition::Size(OMF80Format * omf, Module * mod) const
{
	return 4 + name.size();
}

void OMF80Format::SymbolDefinitionsRecord::SymbolDefinition::Write(OMF80Format * omf, Module * mod, ChecksumWriter& wr) const
{
	wr.WriteWord(2, offset);
	WriteString(wr, name);
	wr.Skip(1);
}

//// OMF80Format::SymbolDefinitionsRecord

void OMF80Format::SymbolDefinitionsRecord::ReadRecordContents(OMF80Format * omf, Module * mod, Linker::Reader& rd)
{
	segment_id = rd.ReadUnsigned(1);
	while(rd.Tell() < record_offset + record_length)
	{
		public_definitions.push_back(SymbolDefinition::Read(omf, mod, rd));
	}
}

uint16_t OMF80Format::SymbolDefinitionsRecord::GetRecordSize(OMF80Format * omf, Module * mod) const
{
	uint16_t bytes = 2;
	for(auto& public_definition : public_definitions)
	{
		bytes += public_definition.Size(omf, mod);
	}
	return bytes;
}

void OMF80Format::SymbolDefinitionsRecord::WriteRecordContents(OMF80Format * omf, Module * mod, ChecksumWriter& wr) const
{
	wr.WriteWord(1, segment_id);
	for(auto& public_definition : public_definitions)
	{
		public_definition.Write(omf, mod, wr);
	}
}

void OMF80Format::SymbolDefinitionsRecord::CalculateValues(OMF80Format * omf, Module * mod)
{
}

void OMF80Format::SymbolDefinitionsRecord::ResolveReferences(OMF80Format * omf, Module * mod)
{
}

//// OMF80Format::RelocationsRecord

void OMF80Format::RelocationsRecord::ReadRecordContents(OMF80Format * omf, Module * mod, Linker::Reader& rd)
{
	relocation_type = relocation_type_t(rd.ReadUnsigned(1));
	while(rd.Tell() < record_offset + record_length)
	{
		offsets.push_back(rd.ReadUnsigned(2));
	}
}

uint16_t OMF80Format::RelocationsRecord::GetRecordSize(OMF80Format * omf, Module * mod) const
{
	return 2 + 2 * offsets.size();
}

void OMF80Format::RelocationsRecord::WriteRecordContents(OMF80Format * omf, Module * mod, ChecksumWriter& wr) const
{
	wr.WriteWord(1, relocation_type);
	for(auto offset : offsets)
	{
		wr.WriteWord(2, offset);
	}
}

void OMF80Format::RelocationsRecord::CalculateValues(OMF80Format * omf, Module * mod)
{
}

void OMF80Format::RelocationsRecord::ResolveReferences(OMF80Format * omf, Module * mod)
{
}

//// OMF80Format::InterSegmentReferencesRecord

void OMF80Format::InterSegmentReferencesRecord::ReadRecordContents(OMF80Format * omf, Module * mod, Linker::Reader& rd)
{
	segment_id = rd.ReadUnsigned(1);
	RelocationsRecord::ReadRecordContents(omf, mod, rd);
}

uint16_t OMF80Format::InterSegmentReferencesRecord::GetRecordSize(OMF80Format * omf, Module * mod) const
{
	return 1 + RelocationsRecord::GetRecordSize(omf, mod);
}

void OMF80Format::InterSegmentReferencesRecord::WriteRecordContents(OMF80Format * omf, Module * mod, ChecksumWriter& wr) const
{
	wr.WriteWord(1, segment_id);
	RelocationsRecord::WriteRecordContents(omf, mod, wr);
}

void OMF80Format::InterSegmentReferencesRecord::CalculateValues(OMF80Format * omf, Module * mod)
{
}

void OMF80Format::InterSegmentReferencesRecord::ResolveReferences(OMF80Format * omf, Module * mod)
{
}

//// OMF80Format::ExternalReferencesRecord

void OMF80Format::ExternalReferencesRecord::ReadRecordContents(OMF80Format * omf, Module * mod, Linker::Reader& rd)
{
	relocation_type = relocation_type_t(rd.ReadUnsigned(1));
	while(rd.Tell() < record_offset + record_length)
	{
		ExternalReference external_reference;
		external_reference.name_index.index = rd.ReadUnsigned(2);
		external_reference.offset = rd.ReadUnsigned(2);
		external_references.push_back(external_reference);
	}
}

uint16_t OMF80Format::ExternalReferencesRecord::GetRecordSize(OMF80Format * omf, Module * mod) const
{
	return 2 + 4 * external_references.size();
}

void OMF80Format::ExternalReferencesRecord::WriteRecordContents(OMF80Format * omf, Module * mod, ChecksumWriter& wr) const
{
	wr.WriteWord(1, relocation_type);
	for(auto& external_reference : external_references)
	{
		wr.WriteWord(2, external_reference.name_index.index);
		wr.WriteWord(2, external_reference.offset);
	}
}

void OMF80Format::ExternalReferencesRecord::CalculateValues(OMF80Format * omf, Module * mod)
{
	// TODO
}

void OMF80Format::ExternalReferencesRecord::ResolveReferences(OMF80Format * omf, Module * mod)
{
	for(auto& external_reference : external_references)
	{
		external_reference.name_index.ResolveReferences(omf, mod);
	}
}

//// OMF80Format::ModuleAncestorRecord

void OMF80Format::ModuleAncestorRecord::ReadRecordContents(OMF80Format * omf, Module * mod, Linker::Reader& rd)
{
	name = rd.ReadData(record_length - 1);
}

uint16_t OMF80Format::ModuleAncestorRecord::GetRecordSize(OMF80Format * omf, Module * mod) const
{
	return name.size() + 1;
}

void OMF80Format::ModuleAncestorRecord::WriteRecordContents(OMF80Format * omf, Module * mod, ChecksumWriter& wr) const
{
	wr.WriteData(name);
}

void OMF80Format::ModuleAncestorRecord::CalculateValues(OMF80Format * omf, Module * mod)
{
}

void OMF80Format::ModuleAncestorRecord::ResolveReferences(OMF80Format * omf, Module * mod)
{
}

//// OMF80Format

std::shared_ptr<OMF80Format::Record> OMF80Format::ReadRecord(Linker::Reader& rd)
{
	Module * mod = modules.size() > 0 ? &modules.back() : nullptr;
	offset_t record_offset = rd.Tell();
	uint8_t record_type = rd.ReadUnsigned(1);
	uint16_t record_length = rd.ReadUnsigned(2);
	std::shared_ptr<Record> record;
	switch(record_type)
	{
	case ModuleHeader:
		record = std::make_shared<ModuleHeaderRecord>(record_type_t(record_type));
		break;
	case ModuleEnd:
		record = std::make_shared<ModuleEndRecord>(record_type_t(record_type));
		break;
	case Content:
		record = std::make_shared<ContentRecord>(record_type_t(record_type));
		break;
	case LineNumbers:
		record = std::make_shared<LineNumbersRecord>(record_type_t(record_type));
		break;
	case EndOfFile:
		record = std::make_shared<EmptyRecord>(record_type_t(record_type));
		break;
	case ModuleAncestor:
		record = std::make_shared<ModuleAncestorRecord>(record_type_t(record_type));
		break;
	case LocalSymbols:
	case PublicDefinitions:
		record = std::make_shared<SymbolDefinitionsRecord>(record_type_t(record_type));
		break;
	case ExternalDefinitions:
		record = std::make_shared<ExternalDefinitionsRecord>(record_type_t(record_type));
		break;
	case ExternalReferences:
		record = std::make_shared<ExternalReferencesRecord>(record_type_t(record_type));
		break;
	case Relocations:
		record = std::make_shared<RelocationsRecord>(record_type_t(record_type));
		break;
	case InterSegmentReferences:
		record = std::make_shared<InterSegmentReferencesRecord>(record_type_t(record_type));
		break;
	case LibraryModuleLocations:
		record = std::make_shared<LibraryModuleLocationsRecord>(record_type_t(record_type));
		break;
	case LibraryModuleNames:
		record = std::make_shared<LibraryModuleNamesRecord>(record_type_t(record_type));
		break;
	case LibraryDictionary:
		record = std::make_shared<LibraryDictionaryRecord>(record_type_t(record_type));
		break;
	case LibraryHeader:
		record = std::make_shared<LibraryHeaderRecord>(record_type_t(record_type));
		break;
	case NamedCommonDefinitions:
		record = std::make_shared<NamedCommonDefinitionsRecord>(record_type_t(record_type));
		break;
	default:
		record = std::make_shared<UnknownRecord>(record_type_t(record_type));
	}
	record->record_offset = record_offset;
	record->record_length = record_length;
	record->ReadRecordContents(this, mod, rd);
	rd.ReadUnsigned(1); // checksum
	records.push_back(record);
	return record;
}

std::shared_ptr<OMF80Format> OMF80Format::ReadOMFFile(Linker::Reader& rd)
{
	std::shared_ptr<OMF80Format> omf = std::make_shared<OMF80Format>();
	omf->ReadFile(rd);
	return omf;
}

void OMF80Format::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = LittleEndian;
	rd.SeekEnd();
	offset_t end = rd.Tell();
	rd.Seek(0);

	while(rd.Tell() < end)
	{
		ReadRecord(rd);
	}

	file_size = rd.Tell();
}

offset_t OMF80Format::WriteFile(Linker::Writer& wr) const
{
	/* TODO */

	return offset_t(-1);
}

void OMF80Format::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("Intel OMF-80 format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 8);
	file_region.Display(dump);

	// TODO
}

void OMF80Format::GenerateModule(Linker::Module& module) const
{
	/* TODO */
}

//// OMF51Format::SegmentInfo

void OMF51Format::SegmentInfo::ReadSegmentInfo(OMF51Format * omf, Module * mod, uint8_t segment_info)
{
	overlayable = (segment_info & FlagOverlayable) != 0;
	register_bank = (segment_info & MaskRegisterBank) >> ShiftRegisterBank;
	segment_type = segment_type_t(segment_info & MaskSegmentType);
}

uint8_t OMF51Format::SegmentInfo::WriteSegmentInfo(OMF51Format * omf, Module * mod) const
{
	uint8_t segment_info = (register_bank << ShiftRegisterBank) | segment_type;
	if(overlayable)
		segment_info |= FlagOverlayable;
	return segment_info;
}

//// OMF51Format::SegmentDefinition

OMF51Format::SegmentDefinition OMF51Format::SegmentDefinition::Read(OMF51Format * omf, Module * mod, Linker::Reader& rd)
{
	SegmentDefinition segment_definition;

	segment_definition.segment_id = rd.ReadUnsigned(1);

	uint8_t segment_info = rd.ReadUnsigned(1);
	segment_definition.info.ReadSegmentInfo(omf, mod, segment_info);
	segment_definition.alignment = alignment_t(rd.ReadUnsigned(1));
	rd.Skip(1);

	segment_definition.base = rd.ReadUnsigned(2);

	if((segment_info & SegmentInfo::FlagSegmentEmpty) != 0)
	{
		segment_definition.size = 0;
	}
	else
	{
		segment_definition.size = rd.ReadUnsigned(2);
		if(segment_definition.size == 0)
			segment_definition.size = 0x10000;
	}

	segment_definition.name = ReadString(rd);

	return segment_definition;
}

uint16_t OMF51Format::SegmentDefinition::Size(OMF51Format * omf, Module * mod) const
{
	return 9 + name.size();
}

void OMF51Format::SegmentDefinition::Write(OMF51Format * omf, Module * mod, ChecksumWriter& wr) const
{
	wr.WriteWord(1, segment_id);

	uint8_t segment_info = info.WriteSegmentInfo(omf, mod);
	if(size == 0)
		segment_info |= SegmentInfo::FlagSegmentEmpty;
	wr.WriteWord(1, segment_info);

	wr.WriteWord(1, 0); // reserved
	wr.WriteWord(2, base);
	wr.WriteWord(2, size);
	WriteString(wr, name);
}

//// OMF51Format::SymbolInfo

void OMF51Format::SymbolInfo::Read(OMF51Format * omf, Module * mod, uint8_t symbol_info)
{
	indirectly_callable = (symbol_info & FlagIndirectlyCallable) != 0;
	variable = (symbol_info & FlagVariable) != 0;
	if((symbol_info & FlagRegisterBank) != 0)
		register_bank = (symbol_info & MaskRegisterBank) >> ShiftRegisterBank;
	usage = segment_type_t(symbol_info & MaskSegmentType);
}

uint8_t OMF51Format::SymbolInfo::Write(OMF51Format * omf, Module * mod) const
{
	uint8_t symbol_info = usage;
	if(indirectly_callable)
		symbol_info |= FlagIndirectlyCallable;
	if(variable)
		symbol_info |= FlagVariable;
	if(register_bank)
		symbol_info |= FlagRegisterBank | (register_bank.value() << ShiftRegisterBank);
	return symbol_info;
}

//// OMF51Format::SymbolDefinition

OMF51Format::SymbolDefinition OMF51Format::SymbolDefinition::Read(OMF51Format * omf, Module * mod, Linker::Reader& rd)
{
	SymbolDefinition symbol_definition;
	symbol_definition.segment_id = rd.ReadUnsigned(1);
	symbol_definition.info.Read(omf, mod, rd.ReadUnsigned(1));
	rd.Skip(1);
	symbol_definition.name = ReadString(rd);
	return symbol_definition;
}

uint16_t OMF51Format::SymbolDefinition::Size(OMF51Format * omf, Module * mod) const
{
	return 6 + name.size();
}

void OMF51Format::SymbolDefinition::Write(OMF51Format * omf, Module * mod, ChecksumWriter& wr) const
{
	wr.WriteWord(1, segment_id);
	wr.WriteWord(1, info.Write(omf, mod));
	wr.WriteWord(2, offset);
	wr.WriteWord(1, 0); // reserved
	WriteString(wr, name);
}

//// OMF51Format::ExternalDefinition

OMF51Format::ExternalDefinition OMF51Format::ExternalDefinition::Read(OMF51Format * omf, Module * mod, Linker::Reader& rd)
{
	ExternalDefinition external_definition;
	external_definition.block_id = rd.ReadUnsigned(1);
	external_definition.external_id = rd.ReadUnsigned(1);
	external_definition.info.Read(omf, mod, rd.ReadUnsigned(1));
	rd.Skip(1);
	external_definition.name = ReadString(rd);
	return external_definition;
}

uint16_t OMF51Format::ExternalDefinition::Size(OMF51Format * omf, Module * mod) const
{
	return 5 + name.size();
}

void OMF51Format::ExternalDefinition::Write(OMF51Format * omf, Module * mod, ChecksumWriter& wr) const
{
	wr.WriteWord(1, block_id);
	wr.WriteWord(1, external_id);
	wr.WriteWord(1, info.Write(omf, mod));
	wr.WriteWord(1, 0); // reserved
	WriteString(wr, name);
}

//// OMF51Format::ModuleHeaderRecord

void OMF51Format::ModuleHeaderRecord::ReadRecordContents(OMF51Format * omf, Module * mod, Linker::Reader& rd)
{
	name = ReadString(rd);
	translator_id = translator_id_t(rd.ReadUnsigned(1));
	rd.Skip(1);
	omf->modules.push_back(Module());
}

uint16_t OMF51Format::ModuleHeaderRecord::GetRecordSize(OMF51Format * omf, Module * mod) const
{
	return 4 + name.size();
}

void OMF51Format::ModuleHeaderRecord::WriteRecordContents(OMF51Format * omf, Module * mod, ChecksumWriter& wr) const
{
	WriteString(wr, name);
	wr.WriteWord(1, translator_id);
	wr.WriteWord(1, 0); // reserved
}

//// OMF51Format::ModuleEndRecord

void OMF51Format::ModuleEndRecord::ReadRecordContents(OMF51Format * omf, Module * mod, Linker::Reader& rd)
{
	name = ReadString(rd);
	rd.Skip(2);
	uint8_t register_mask = rd.ReadUnsigned(1);
	for(int bank = 0; bank < 4; bank++)
		banks[bank] = ((register_mask >> bank) & 1) != 0;
	rd.Skip(1);
}

uint16_t OMF51Format::ModuleEndRecord::GetRecordSize(OMF51Format * omf, Module * mod) const
{
	return 6 + name.size();
}

void OMF51Format::ModuleEndRecord::WriteRecordContents(OMF51Format * omf, Module * mod, ChecksumWriter& wr) const
{
	WriteString(wr, name);
	wr.WriteWord(2, 0); // reserved
	uint8_t register_mask = 0;
	for(int bank = 0; bank < 4; bank++)
		if(banks[bank])
			register_mask |= 1 << bank;
	wr.WriteWord(1, register_mask);
	wr.WriteWord(1, 0); // reserved
}

//// OMF51Format::SegmentDefinitionsRecord

void OMF51Format::SegmentDefinitionsRecord::ReadRecordContents(OMF51Format * omf, Module * mod, Linker::Reader& rd)
{
	while(rd.Tell() < RecordEnd())
	{
		segment_definitions.push_back(SegmentDefinition::Read(omf, mod, rd));
		auto& segment_definition = segment_definitions.back();
		mod->segment_definitions[segment_definition.segment_id] = segment_definition; // TODO: check it is not duplicated
	}
}

uint16_t OMF51Format::SegmentDefinitionsRecord::GetRecordSize(OMF51Format * omf, Module * mod) const
{
	uint16_t bytes = 1;
	for(auto& segment_definition : segment_definitions)
	{
		bytes += segment_definition.Size(omf, mod);
	}
	return bytes;
}

void OMF51Format::SegmentDefinitionsRecord::WriteRecordContents(OMF51Format * omf, Module * mod, ChecksumWriter& wr) const
{
	for(auto& segment_definition : segment_definitions)
	{
		segment_definition.Write(omf, mod, wr);
	}
}

void OMF51Format::SegmentDefinitionsRecord::CalculateValues(OMF51Format * omf, Module * mod)
{
	// TODO
}

void OMF51Format::SegmentDefinitionsRecord::ResolveReferences(OMF51Format * omf, Module * mod)
{
	// TODO
}

//// OMF51Format::PublicSymbolsRecord

void OMF51Format::PublicSymbolsRecord::ReadRecordContents(OMF51Format * omf, Module * mod, Linker::Reader& rd)
{
	while(rd.Tell() < RecordEnd())
	{
		symbol_definitions.push_back(SymbolDefinition::Read(omf, mod, rd));
		// TODO: record segment definitions in module
	}
}

uint16_t OMF51Format::PublicSymbolsRecord::GetRecordSize(OMF51Format * omf, Module * mod) const
{
	uint16_t bytes = 1;
	for(auto& symbol_definition : symbol_definitions)
	{
		bytes += symbol_definition.Size(omf, mod);
	}
	return bytes;
}

void OMF51Format::PublicSymbolsRecord::WriteRecordContents(OMF51Format * omf, Module * mod, ChecksumWriter& wr) const
{
	for(auto& symbol_definition : symbol_definitions)
	{
		symbol_definition.Write(omf, mod, wr);
	}
}

void OMF51Format::PublicSymbolsRecord::CalculateValues(OMF51Format * omf, Module * mod)
{
	// TODO
}

void OMF51Format::PublicSymbolsRecord::ResolveReferences(OMF51Format * omf, Module * mod)
{
	// TODO
}

//// OMF51Format::ExternalDefinitionsRecord

void OMF51Format::ExternalDefinitionsRecord::ReadRecordContents(OMF51Format * omf, Module * mod, Linker::Reader& rd)
{
	while(rd.Tell() < RecordEnd())
	{
		external_definitions.push_back(ExternalDefinition::Read(omf, mod, rd));
		auto& external_definition = external_definitions.back();
		mod->external_definitions[external_definition.external_id] = external_definition; // TODO: check it is not duplicated
	}
}

uint16_t OMF51Format::ExternalDefinitionsRecord::GetRecordSize(OMF51Format * omf, Module * mod) const
{
	uint16_t bytes = 1;
	for(auto& external_definition : external_definitions)
	{
		bytes += external_definition.Size(omf, mod);
	}
	return bytes;
}

void OMF51Format::ExternalDefinitionsRecord::WriteRecordContents(OMF51Format * omf, Module * mod, ChecksumWriter& wr) const
{
	for(auto& external_definition : external_definitions)
	{
		external_definition.Write(omf, mod, wr);
	}
}

void OMF51Format::ExternalDefinitionsRecord::CalculateValues(OMF51Format * omf, Module * mod)
{
	// TODO
}

void OMF51Format::ExternalDefinitionsRecord::ResolveReferences(OMF51Format * omf, Module * mod)
{
	// TODO
}

//// OMF51Format::ScopeDefinitionRecord

void OMF51Format::ScopeDefinitionRecord::ReadRecordContents(OMF51Format * omf, Module * mod, Linker::Reader& rd)
{
	block_type = block_type_t(rd.ReadUnsigned(1));
	name = ReadString(rd);
}

uint16_t OMF51Format::ScopeDefinitionRecord::GetRecordSize(OMF51Format * omf, Module * mod) const
{
	return 3 + name.size();
}

void OMF51Format::ScopeDefinitionRecord::WriteRecordContents(OMF51Format * omf, Module * mod, ChecksumWriter& wr) const
{
	wr.WriteWord(1, block_type);
	WriteString(wr, name);
}

void OMF51Format::ScopeDefinitionRecord::CalculateValues(OMF51Format * omf, Module * mod)
{
	// TODO
}

void OMF51Format::ScopeDefinitionRecord::ResolveReferences(OMF51Format * omf, Module * mod)
{
	// TODO
}

//// OMF51Format::DebugItemsRecord::Symbol

OMF51Format::DebugItemsRecord::Symbol OMF51Format::DebugItemsRecord::Symbol::Read(OMF51Format * omf, Module * mod, Linker::Reader& rd)
{
	Symbol symbol;
	symbol.segment_id = rd.ReadUnsigned(1);
	symbol.info.Read(omf, mod, rd.ReadUnsigned(1));
	symbol.offset = rd.ReadUnsigned(2);
	rd.Skip(1);
	symbol.name = ReadString(rd);
	return symbol;
}

uint16_t OMF51Format::DebugItemsRecord::Symbol::Size(OMF51Format * omf, Module * mod) const
{
	return 6 + name.size();
}

void OMF51Format::DebugItemsRecord::Symbol::Write(OMF51Format * omf, Module * mod, ChecksumWriter& wr) const
{
	wr.WriteWord(1, segment_id);
	wr.WriteWord(1, info.Write(omf, mod));
	wr.WriteWord(2, offset);
	wr.WriteWord(1, 0); // reserved
	WriteString(wr, name);
}

//// OMF51Format::DebugItemsRecord::SegmentSymbol

OMF51Format::DebugItemsRecord::SegmentSymbol OMF51Format::DebugItemsRecord::SegmentSymbol::Read(OMF51Format * omf, Module * mod, Linker::Reader& rd)
{
	SegmentSymbol symbol;
	symbol.segment_id = rd.ReadUnsigned(1);
	symbol.info.ReadSegmentInfo(omf, mod, rd.ReadUnsigned(1));
	symbol.offset = rd.ReadUnsigned(2);
	rd.Skip(1);
	symbol.name = ReadString(rd);
	return symbol;
}

uint16_t OMF51Format::DebugItemsRecord::SegmentSymbol::Size(OMF51Format * omf, Module * mod) const
{
	return 6 + name.size();
}

void OMF51Format::DebugItemsRecord::SegmentSymbol::Write(OMF51Format * omf, Module * mod, ChecksumWriter& wr) const
{
	wr.WriteWord(1, segment_id);
	wr.WriteWord(1, info.WriteSegmentInfo(omf, mod));
	wr.WriteWord(2, offset);
	wr.WriteWord(1, 0); // reserved
	WriteString(wr, name);
}

//// OMF51Format::DebugItemsRecord::LineNumber

OMF51Format::DebugItemsRecord::LineNumber OMF51Format::DebugItemsRecord::LineNumber::Read(OMF51Format * omf, Module * mod, Linker::Reader& rd)
{
	LineNumber line_number;
	line_number.segment_id = rd.ReadUnsigned(1);
	line_number.offset = rd.ReadUnsigned(2);
	line_number.line_number = rd.ReadUnsigned(2);
	return line_number;
}

void OMF51Format::DebugItemsRecord::LineNumber::Write(OMF51Format * omf, Module * mod, ChecksumWriter& wr) const
{
}

//// OMF51Format::DebugItemsRecord

void OMF51Format::DebugItemsRecord::ReadRecordContents(OMF51Format * omf, Module * mod, Linker::Reader& rd)
{
	switch(rd.ReadUnsigned(1))
	{
	case Type_LocalSymbols:
		{
			contents = LocalSymbols();
			LocalSymbols& local_symbols = std::get<LocalSymbols>(contents);
			while(rd.Tell() < RecordEnd())
			{
				local_symbols.symbols.push_back(Symbol::Read(omf, mod, rd));
			}
		}
		break;
	case Type_PublicSymbols:
		{
			contents = PublicSymbols();
			PublicSymbols& public_symbols = std::get<PublicSymbols>(contents);
			while(rd.Tell() < RecordEnd())
			{
				public_symbols.symbols.push_back(Symbol::Read(omf, mod, rd));
			}
		}
		break;
	case Type_SegmentSymbols:
		{
			contents = SegmentSymbols();
			SegmentSymbols& segment_symbols = std::get<SegmentSymbols>(contents);
			while(rd.Tell() < RecordEnd())
			{
				segment_symbols.symbols.push_back(SegmentSymbol::Read(omf, mod, rd));
			}
		}
		break;
	case Type_LineNumbers:
		{
			contents = LineNumbers();
			LineNumbers& line_numbers = std::get<LineNumbers>(contents);
			while(rd.Tell() < RecordEnd())
			{
				line_numbers.symbols.push_back(LineNumber::Read(omf, mod, rd));
			}
		}
		break;
	default:
		// TODO: cannot parse record
		break;
	}
}

uint16_t OMF51Format::DebugItemsRecord::GetRecordSize(OMF51Format * omf, Module * mod) const
{
	uint16_t bytes = 2;
	if(auto * local_symbols = std::get_if<LocalSymbols>(&contents))
	{
		for(auto& symbol : local_symbols->symbols)
		{
			bytes += symbol.Size(omf, mod);
		}
	}
	else if(auto * public_symbols = std::get_if<Type_PublicSymbols>(&contents))
	{
		for(auto& symbol : public_symbols->symbols)
		{
			bytes += symbol.Size(omf, mod);
		}
	}
	else if(auto * segment_symbols = std::get_if<SegmentSymbols>(&contents))
	{
		for(auto& symbol : segment_symbols->symbols)
		{
			bytes += symbol.Size(omf, mod);
		}
	}
	else if(auto * line_numbers = std::get_if<LineNumbers>(&contents))
	{
		bytes += 5 * line_numbers->symbols.size();
	}
	else
	{
		assert(false);
	}
	return bytes;
}

void OMF51Format::DebugItemsRecord::WriteRecordContents(OMF51Format * omf, Module * mod, ChecksumWriter& wr) const
{
	if(auto * local_symbols = std::get_if<LocalSymbols>(&contents))
	{
		wr.WriteWord(1, Type_LocalSymbols);
		for(auto& symbol : local_symbols->symbols)
		{
			symbol.Write(omf, mod, wr);
		}
	}
	else if(auto * public_symbols = std::get_if<Type_PublicSymbols>(&contents))
	{
		wr.WriteWord(1, Type_PublicSymbols);
		for(auto& symbol : public_symbols->symbols)
		{
			symbol.Write(omf, mod, wr);
		}
	}
	else if(auto * segment_symbols = std::get_if<SegmentSymbols>(&contents))
	{
		wr.WriteWord(1, Type_SegmentSymbols);
		for(auto& symbol : segment_symbols->symbols)
		{
			symbol.Write(omf, mod, wr);
		}
	}
	else if(auto * line_numbers = std::get_if<LineNumbers>(&contents))
	{
		wr.WriteWord(1, Type_LineNumbers);
		for(auto& line_number : line_numbers->symbols)
		{
			line_number.Write(omf, mod, wr);
		}
	}
	else
	{
		assert(false);
	}
}

void OMF51Format::DebugItemsRecord::CalculateValues(OMF51Format * omf, Module * mod)
{
	// TODO
}

void OMF51Format::DebugItemsRecord::ResolveReferences(OMF51Format * omf, Module * mod)
{
	// TODO
}

////

OMF51Format::FixupRecord::Fixup OMF51Format::FixupRecord::Fixup::Read(OMF51Format * omf, Module * mod, Linker::Reader& rd)
{
	Fixup fixup;
	fixup.location = rd.ReadUnsigned(2);
	fixup.relocation = relocation_type_t(rd.ReadUnsigned(1));
	fixup.reference = reference_type_t(rd.ReadUnsigned(1));
	fixup.id = rd.ReadUnsigned(1);
	fixup.offset = rd.ReadUnsigned(2);
	return fixup;
}

void OMF51Format::FixupRecord::Fixup::Write(OMF51Format * omf, Module * mod, ChecksumWriter& wr) const
{
	wr.WriteWord(2, location);
	wr.WriteWord(1, relocation);
	wr.WriteWord(1, reference);
	wr.WriteWord(1, id);
	wr.WriteWord(2, offset);
}

//// OMF51Format::FixupRecord

void OMF51Format::FixupRecord::ReadRecordContents(OMF51Format * omf, Module * mod, Linker::Reader& rd)
{
	while(rd.Tell() < RecordEnd())
	{
		fixups.push_back(Fixup::Read(omf, mod, rd));
	}
}

uint16_t OMF51Format::FixupRecord::GetRecordSize(OMF51Format * omf, Module * mod) const
{
	return 1 + 7 * fixups.size();
}

void OMF51Format::FixupRecord::WriteRecordContents(OMF51Format * omf, Module * mod, ChecksumWriter& wr) const
{
	for(auto& fixup : fixups)
	{
		fixup.Write(omf, mod, wr);
	}
}

void OMF51Format::FixupRecord::CalculateValues(OMF51Format * omf, Module * mod)
{
	// TODO
}

void OMF51Format::FixupRecord::ResolveReferences(OMF51Format * omf, Module * mod)
{
	// TODO
}

//// OMF51Format

std::shared_ptr<OMF51Format::Record> OMF51Format::ReadRecord(Linker::Reader& rd)
{
	Module * mod = modules.size() > 0 ? &modules.back() : nullptr;
	offset_t record_offset = rd.Tell();
	uint8_t record_type = rd.ReadUnsigned(1);
	uint16_t record_length = rd.ReadUnsigned(2);
	std::shared_ptr<Record> record;
	switch(record_type)
	{
	case ModuleHeader:
		record = std::make_shared<ModuleHeaderRecord>(record_type_t(record_type));
		break;
	case ModuleEnd:
		record = std::make_shared<ModuleEndRecord>(record_type_t(record_type));
		break;
	case Content:
		record = std::make_shared<ContentRecord>(record_type_t(record_type));
		break;
	case Fixups:
		record = std::make_shared<FixupRecord>(record_type_t(record_type));
		break;
	case SegmentDefinitions:
		record = std::make_shared<SegmentDefinitionsRecord>(record_type_t(record_type));
		break;
	case ScopeDefinition:
		record = std::make_shared<ScopeDefinitionRecord>(record_type_t(record_type));
		break;
	case DebugItems:
		record = std::make_shared<DebugItemsRecord>(record_type_t(record_type));
		break;
	case PublicDefinitions:
		record = std::make_shared<PublicSymbolsRecord>(record_type_t(record_type));
		break;
	case ExternalDefinitions:
		record = std::make_shared<ExternalDefinitionsRecord>(record_type_t(record_type));
		break;
	case LibraryModuleLocations:
		record = std::make_shared<LibraryModuleLocationsRecord>(record_type_t(record_type));
		break;
	case LibraryModuleNames:
		record = std::make_shared<LibraryModuleNamesRecord>(record_type_t(record_type));
		break;
	case LibraryDictionary:
		record = std::make_shared<LibraryDictionaryRecord>(record_type_t(record_type));
		break;
	case LibraryHeader:
		record = std::make_shared<LibraryHeaderRecord>(record_type_t(record_type));
		break;
	default:
		record = std::make_shared<UnknownRecord>(record_type_t(record_type));
	}
	record->record_offset = record_offset;
	record->record_length = record_length;
	record->ReadRecordContents(this, mod, rd);
	rd.ReadUnsigned(1); // checksum
	records.push_back(record);
	return record;
}

std::shared_ptr<OMF51Format> OMF51Format::ReadOMFFile(Linker::Reader& rd)
{
	std::shared_ptr<OMF51Format> omf = std::make_shared<OMF51Format>();
	omf->ReadFile(rd);
	return omf;
}

void OMF51Format::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = LittleEndian;
	rd.SeekEnd();
	offset_t end = rd.Tell();
	rd.Seek(0);

	while(rd.Tell() < end)
	{
		ReadRecord(rd);
	}

	file_size = rd.Tell();
}

offset_t OMF51Format::WriteFile(Linker::Writer& wr) const
{
	/* TODO */

	return offset_t(-1);
}

void OMF51Format::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("Intel OMF-51 format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 8);
	file_region.Display(dump);

	// TODO
}

void OMF51Format::GenerateModule(Linker::Module& module) const
{
	/* TODO */
}

//// OMF96Format::SegmentDefinition

OMF96Format::SegmentDefinition OMF96Format::SegmentDefinition::Read(OMF96Format * omf, Module * mod, Linker::Reader& rd)
{
	SegmentDefinition segment_definition;
	segment_definition.segment_id = rd.ReadUnsigned(1);
	if(segment_definition.IsRelocatable())
	{
		segment_definition.MakeRelocatable(alignment_t(rd.ReadUnsigned(1)));
	}
	else
	{
		segment_definition.MakeAbsolute(rd.ReadUnsigned(2));
	}
	segment_definition.size = rd.ReadUnsigned(2);
	return segment_definition;
}

uint16_t OMF96Format::SegmentDefinition::Size(OMF96Format * omf, Module * mod) const
{
	return IsRelocatable() ? 4 : 5;
}

void OMF96Format::SegmentDefinition::Write(OMF96Format * omf, Module * mod, ChecksumWriter& wr) const
{
	wr.WriteWord(1, segment_id);
	if(IsRelocatable())
	{
		wr.WriteWord(1, GetAlignment());
	}
	else
	{
		wr.WriteWord(2, GetBaseAddress());
	}
	wr.WriteWord(2, size);
}

//// OMF96Format::ExternalDefinition

OMF96Format::ExternalDefinition OMF96Format::ExternalDefinition::Read(OMF96Format * omf, Module * mod, Linker::Reader& rd)
{
	ExternalDefinition external_definition;
	external_definition.name = ReadString(rd);
	external_definition.type.index = ReadIndex(rd);
	return external_definition;
}

uint16_t OMF96Format::ExternalDefinition::Size(OMF96Format * omf, Module * mod) const
{
	return 1 + name.size() + IndexSize(type.index);
}

void OMF96Format::ExternalDefinition::Write(OMF96Format * omf, Module * mod, ChecksumWriter& wr) const
{
	WriteString(wr, name);
	WriteIndex(wr, type.index);
}

void OMF96Format::ExternalDefinition::CalculateValues(OMF96Format * omf, Module * mod)
{
	// TODO
}

void OMF96Format::ExternalDefinition::ResolveReferences(OMF96Format * omf, Module * mod)
{
	// TODO
}

//// OMF96Format::SymbolDefinition

OMF96Format::SymbolDefinition OMF96Format::SymbolDefinition::Read(OMF96Format * omf, Module * mod, Linker::Reader& rd)
{
	SymbolDefinition symbol_definition;
	symbol_definition.offset = rd.ReadUnsigned(2);
	symbol_definition.name = ReadString(rd);
	symbol_definition.type.index = ReadIndex(rd);
	return symbol_definition;
}

uint16_t OMF96Format::SymbolDefinition::Size(OMF96Format * omf, Module * mod) const
{
	return 3 + name.size() + IndexSize(type.index);
}

void OMF96Format::SymbolDefinition::Write(OMF96Format * omf, Module * mod, ChecksumWriter& wr) const
{
	wr.WriteWord(2, offset);
	WriteString(wr, name);
	WriteIndex(wr, type.index);
}

void OMF96Format::SymbolDefinition::CalculateValues(OMF96Format * omf, Module * mod)
{
	// TODO
}

void OMF96Format::SymbolDefinition::ResolveReferences(OMF96Format * omf, Module * mod)
{
	// TODO
}

//// OMF96Format::ModuleHeaderRecord

void OMF96Format::ModuleHeaderRecord::ReadRecordContents(OMF96Format * omf, Module * mod, Linker::Reader& rd)
{
	name = ReadString(rd);
	translator_id = rd.ReadUnsigned(1);
	date_time = ReadString(rd);
}

uint16_t OMF96Format::ModuleHeaderRecord::GetRecordSize(OMF96Format * omf, Module * mod) const
{
	return 4 + name.size() + date_time.size();
}

void OMF96Format::ModuleHeaderRecord::WriteRecordContents(OMF96Format * omf, Module * mod, ChecksumWriter& wr) const
{
	WriteString(wr, name);
	wr.WriteWord(1, translator_id);
	WriteString(wr, date_time);
}

//// OMF96Format::ModuleEndRecord

void OMF96Format::ModuleEndRecord::ReadRecordContents(OMF96Format * omf, Module * mod, Linker::Reader& rd)
{
	main = rd.ReadUnsigned(1) == MainModule;
	valid = rd.ReadUnsigned(1) == ValidModule;
}

uint16_t OMF96Format::ModuleEndRecord::GetRecordSize(OMF96Format * omf, Module * mod) const
{
	return 3;
}

void OMF96Format::ModuleEndRecord::WriteRecordContents(OMF96Format * omf, Module * mod, ChecksumWriter& wr) const
{
	wr.WriteWord(1, main ? MainModule : OtherModule);
	wr.WriteWord(1, valid ? ValidModule : ErroneousModule);
}

//// OMF96Format::SegmentDefinitionsRecord

void OMF96Format::SegmentDefinitionsRecord::ReadRecordContents(OMF96Format * omf, Module * mod, Linker::Reader& rd)
{
	while(rd.Tell() < RecordEnd())
	{
		segment_definitions.push_back(SegmentDefinition::Read(omf, mod, rd));
		auto& segment_definition = segment_definitions.back();
		mod->segment_definitions[segment_definition.segment_id] = segment_definition; // TODO: check it is not duplicated
	}
}

uint16_t OMF96Format::SegmentDefinitionsRecord::GetRecordSize(OMF96Format * omf, Module * mod) const
{
	uint16_t bytes = 1;
	for(auto& segment_definition : segment_definitions)
	{
		bytes += segment_definition.Size(omf, mod);
	}
	return bytes;
}

void OMF96Format::SegmentDefinitionsRecord::WriteRecordContents(OMF96Format * omf, Module * mod, ChecksumWriter& wr) const
{
	for(auto& segment_definition : segment_definitions)
	{
		segment_definition.Write(omf, mod, wr);
	}
}

void OMF96Format::SegmentDefinitionsRecord::CalculateValues(OMF96Format * omf, Module * mod)
{
	// TODO
}

void OMF96Format::SegmentDefinitionsRecord::ResolveReferences(OMF96Format * omf, Module * mod)
{
	// TODO
}

//// OMF96Format::TypeDefinitionRecord::LeafDescriptor

OMF96Format::TypeDefinitionRecord::LeafDescriptor OMF96Format::TypeDefinitionRecord::LeafDescriptor::Read(OMF96Format * omf, Module * mod, Linker::Reader& rd)
{
	LeafDescriptor leaf_descriptor;
	uint8_t leaf_type = rd.ReadUnsigned(1);
	leaf_descriptor.nice = leaf_type & 0x80;
	switch(leaf_type & 0x7F)
	{
	case NullLeaf:
		leaf_descriptor.leaf = Null();
		break;
	case SignedNumericLeaf16:
		leaf_descriptor.leaf = int32_t(rd.ReadSigned(2));
		break;
	case SignedNumericLeaf32:
		leaf_descriptor.leaf = int32_t(rd.ReadSigned(4));
		break;
	case StringLeaf:
		leaf_descriptor.leaf = ReadString(rd);
		break;
	case IndexLeaf:
		leaf_descriptor.leaf = TypeIndex(ReadIndex(rd));
		break;
	case RepeatLeaf:
		leaf_descriptor.leaf = Repeat();
		break;
	case EndOfBranchLeaf:
		leaf_descriptor.leaf = EndOfBranch();
		break;
	default:
		if((leaf_type & 0x7F) < 0x64)
		{
			leaf_descriptor.leaf = int32_t(leaf_type & 0x7F);
		}
		// TODO: otherwise, error
		break;
	}
	return leaf_descriptor;
}

uint16_t OMF96Format::TypeDefinitionRecord::LeafDescriptor::Size(OMF96Format * omf, Module * mod) const
{
	if(std::get_if<Null>(&leaf))
	{
		return 1;
	}
	else if(auto numericp = std::get_if<int32_t>(&leaf))
	{
		if(0 <= *numericp && *numericp < 0x64)
		{
			return 1;
		}
		else if(-0x8000 <= *numericp && *numericp < 0x8000)
		{
			return 3;
		}
		else
		{
			return 5;
		}
	}
	else if(auto stringp = std::get_if<std::string>(&leaf))
	{
		return 2 + stringp->size();
	}
	else if(auto indexp = std::get_if<TypeIndex>(&leaf))
	{
		return 1 + IndexSize(indexp->index);
	}
	else if(std::get_if<Repeat>(&leaf))
	{
		return 2;
	}
	else if(std::get_if<EndOfBranch>(&leaf))
	{
		return 2;
	}
	else
	{
		assert(false);
	}
}

void OMF96Format::TypeDefinitionRecord::LeafDescriptor::Write(OMF96Format * omf, Module * mod, ChecksumWriter& wr) const
{
	uint8_t leaf_type = nice ? 0x80 : 0;
	if(std::get_if<Null>(&leaf))
	{
		wr.WriteWord(1, leaf_type | NullLeaf);
	}
	else if(auto numericp = std::get_if<int32_t>(&leaf))
	{
		if(0 <= *numericp && *numericp < 0x64)
		{
			wr.WriteWord(1, leaf_type | *numericp);
		}
		else if(-0x8000 <= *numericp && *numericp < 0x8000)
		{
			wr.WriteWord(1, leaf_type | SignedNumericLeaf16);
			wr.WriteWord(2, *numericp);
		}
		else
		{
			wr.WriteWord(1, leaf_type | SignedNumericLeaf32);
			wr.WriteWord(4, *numericp);
		}
	}
	else if(auto stringp = std::get_if<std::string>(&leaf))
	{
		wr.WriteWord(1, leaf_type | StringLeaf);
		WriteString(wr, *stringp);
	}
	else if(auto indexp = std::get_if<TypeIndex>(&leaf))
	{
		wr.WriteWord(1, leaf_type | IndexLeaf);
		WriteIndex(wr, indexp->index);
	}
	else if(std::get_if<Repeat>(&leaf))
	{
		wr.WriteWord(1, leaf_type | RepeatLeaf);
	}
	else if(std::get_if<EndOfBranch>(&leaf))
	{
		wr.WriteWord(1, leaf_type | EndOfBranchLeaf);
	}
	else
	{
		assert(false);
	}
}

void OMF96Format::TypeDefinitionRecord::LeafDescriptor::CalculateValues(OMF96Format * omf, Module * mod)
{
	// TODO
}

void OMF96Format::TypeDefinitionRecord::LeafDescriptor::ResolveReferences(OMF96Format * omf, Module * mod)
{
	// TODO
}

//// OMF96Format::TypeDefinitionRecord

void OMF96Format::TypeDefinitionRecord::ReadRecordContents(OMF96Format * omf, Module * mod, Linker::Reader& rd)
{
	while(rd.Tell() < RecordEnd())
	{
		leafs.push_back(LeafDescriptor::Read(omf, mod, rd));
	}
	mod->type_definitions.push_back(shared_from_this());
}

uint16_t OMF96Format::TypeDefinitionRecord::GetRecordSize(OMF96Format * omf, Module * mod) const
{
	uint16_t bytes = 1;
	for(auto& leaf : leafs)
	{
		bytes += leaf.Size(omf, mod);
	}
	return bytes;
}

void OMF96Format::TypeDefinitionRecord::WriteRecordContents(OMF96Format * omf, Module * mod, ChecksumWriter& wr) const
{
	for(auto& leaf : leafs)
	{
		leaf.Write(omf, mod, wr);
	}
}

void OMF96Format::TypeDefinitionRecord::CalculateValues(OMF96Format * omf, Module * mod)
{
	// TODO
}

void OMF96Format::TypeDefinitionRecord::ResolveReferences(OMF96Format * omf, Module * mod)
{
	// TODO
}

//// OMF96Format::SymbolDefinitionsRecord

void OMF96Format::SymbolDefinitionsRecord::ReadRecordContents(OMF96Format * omf, Module * mod, Linker::Reader& rd)
{
	segment_id = rd.ReadUnsigned(1);
	while(rd.Tell() < RecordEnd())
	{
		symbol_definitions.push_back(SymbolDefinition::Read(omf, mod, rd));
	}
}

uint16_t OMF96Format::SymbolDefinitionsRecord::GetRecordSize(OMF96Format * omf, Module * mod) const
{
	uint16_t bytes = 2;
	for(auto& symbol_definition : symbol_definitions)
	{
		bytes += symbol_definition.Size(omf, mod);
	}
	return bytes;
}

void OMF96Format::SymbolDefinitionsRecord::WriteRecordContents(OMF96Format * omf, Module * mod, ChecksumWriter& wr) const
{
	for(auto& symbol_definition : symbol_definitions)
	{
		symbol_definition.Write(omf, mod, wr);
	}
}

void OMF96Format::SymbolDefinitionsRecord::CalculateValues(OMF96Format * omf, Module * mod)
{
	// TODO
}

void OMF96Format::SymbolDefinitionsRecord::ResolveReferences(OMF96Format * omf, Module * mod)
{
	// TODO
}

//// OMF96Format::ExternalDefinitionsRecord

void OMF96Format::ExternalDefinitionsRecord::ReadRecordContents(OMF96Format * omf, Module * mod, Linker::Reader& rd)
{
	segment_id = rd.ReadUnsigned(1);
	while(rd.Tell() < RecordEnd())
	{
		external_definitions.push_back(ExternalDefinition::Read(omf, mod, rd));
	}
}

uint16_t OMF96Format::ExternalDefinitionsRecord::GetRecordSize(OMF96Format * omf, Module * mod) const
{
	uint16_t bytes = 2;
	for(auto& external_definition : external_definitions)
	{
		bytes += external_definition.Size(omf, mod);
	}
	return bytes;
}

void OMF96Format::ExternalDefinitionsRecord::WriteRecordContents(OMF96Format * omf, Module * mod, ChecksumWriter& wr) const
{
	for(auto& external_definition : external_definitions)
	{
		external_definition.Write(omf, mod, wr);
	}
}

void OMF96Format::ExternalDefinitionsRecord::CalculateValues(OMF96Format * omf, Module * mod)
{
	// TODO
}

void OMF96Format::ExternalDefinitionsRecord::ResolveReferences(OMF96Format * omf, Module * mod)
{
	// TODO
}

//// OMF96Format::RelocationRecord::Relocation

OMF96Format::RelocationRecord::Relocation OMF96Format::RelocationRecord::Relocation::Read(OMF96Format * omf, Module * mod, Linker::Reader& rd)
{
	Relocation relocation;
	uint8_t relocation_type = rd.ReadUnsigned(1);
	relocation.offset = rd.ReadUnsigned(2);
	relocation.reference_type = reference_type_t((relocation_type >> 2) & 0xF);
	relocation.alignment = alignment_t(relocation_type & 3);
	if((relocation_type & FlagExternal))
	{
		relocation.reference = ExternalReference(rd.ReadUnsigned(2));
	}
	else
	{
		relocation.reference = LocalReference(rd.ReadUnsigned(1));
	}
	if(!(relocation_type & FlagAddendInCode))
	{
		relocation.addend = rd.ReadUnsigned(2);
	}
	return relocation;
}

uint16_t OMF96Format::RelocationRecord::Relocation::Size(OMF96Format * omf, Module * mod) const
{
	uint8_t bytes = 4;
	if(std::get_if<ExternalReference>(&reference))
	{
		bytes += 2;
	}
	else if(std::get_if<LocalReference>(&reference))
	{
		bytes += 1;
	}
	else
	{
		assert(false);
	}
	if(addend)
	{
		bytes += 2;
	}
	return bytes;
}

void OMF96Format::RelocationRecord::Relocation::Write(OMF96Format * omf, Module * mod, ChecksumWriter& wr) const
{
	uint8_t relocation_type = (reference_type << 2) | alignment;
	if(std::get_if<ExternalReference>(&reference))
	{
		relocation_type |= FlagExternal;
	}
	if(!addend)
	{
		relocation_type |= FlagAddendInCode;
	}
	wr.WriteWord(1, relocation_type);
	wr.WriteWord(2, offset);
	if(auto valuep = std::get_if<ExternalReference>(&reference))
	{
		wr.WriteWord(2, *valuep);
	}
	else if(auto valuep = std::get_if<LocalReference>(&reference))
	{
		wr.WriteWord(1, *valuep);
	}
	else
	{
		assert(false);
	}
	if(addend)
	{
		wr.WriteWord(2, addend.value());
	}
}

//// OMF96Format::RelocationRecord

void OMF96Format::RelocationRecord::ReadRecordContents(OMF96Format * omf, Module * mod, Linker::Reader& rd)
{
	while(rd.Tell() < RecordEnd())
	{
		relocations.push_back(Relocation::Read(omf, mod, rd));
	}
}

uint16_t OMF96Format::RelocationRecord::GetRecordSize(OMF96Format * omf, Module * mod) const
{
	uint16_t bytes = 2;
	for(auto& relocation : relocations)
	{
		bytes += relocation.Size(omf, mod);
	}
	return bytes;
}

void OMF96Format::RelocationRecord::WriteRecordContents(OMF96Format * omf, Module * mod, ChecksumWriter& wr) const
{
	for(auto& relocation : relocations)
	{
		relocation.Write(omf, mod, wr);
	}
}

void OMF96Format::RelocationRecord::CalculateValues(OMF96Format * omf, Module * mod)
{
	// TODO
}

void OMF96Format::RelocationRecord::ResolveReferences(OMF96Format * omf, Module * mod)
{
	// TODO
}

//// OMF96Format::ModuleAncestorRecord

void OMF96Format::ModuleAncestorRecord::ReadRecordContents(OMF96Format * omf, Module * mod, Linker::Reader& rd)
{
	name = ReadString(rd);
	if(rd.Tell() < RecordEnd())
	{
		segment_definition = SegmentDefinition::Read(omf, mod, rd);
	}
}

uint16_t OMF96Format::ModuleAncestorRecord::GetRecordSize(OMF96Format * omf, Module * mod) const
{
	return 2 + name.size() + (segment_definition ? segment_definition.value().Size(omf, mod) : 0);
}

void OMF96Format::ModuleAncestorRecord::WriteRecordContents(OMF96Format * omf, Module * mod, ChecksumWriter& wr) const
{
	WriteString(wr, name);
	if(segment_definition)
	{
		segment_definition.value().Write(omf, mod, wr);
	}
}

void OMF96Format::ModuleAncestorRecord::CalculateValues(OMF96Format * omf, Module * mod)
{
	// TODO
}

void OMF96Format::ModuleAncestorRecord::ResolveReferences(OMF96Format * omf, Module * mod)
{
	// TODO
}

//// OMF96Format::BlockDefinitionRecord

void OMF96Format::BlockDefinitionRecord::ReadRecordContents(OMF96Format * omf, Module * mod, Linker::Reader& rd)
{
	std::string name = ReadString(rd);
	segment_id = rd.ReadUnsigned(1);
	offset = rd.ReadUnsigned(2);
	size = rd.ReadUnsigned(2);
	uint8_t flags = rd.ReadUnsigned(1);
	bool is_proc = flags & FlagProcedure;
	if(!(!is_proc && name == ""))
	{
		name_and_type = BlockNameAndType { .name = name, .type = TypeIndex(ReadIndex(rd)) };
	}
	if(is_proc)
	{
		ProcedureInformation info;
		if((flags & FlagExternal))
		{
			info.frame_pointer = ExternalReference(rd.ReadUnsigned(2));
		}
		else
		{
			info.frame_pointer = LocalReference(rd.ReadUnsigned(1));
		}
		info.return_offset = rd.ReadUnsigned(2);
		info.prologue_size = rd.ReadUnsigned(1);
		procedure_info = info;
	}
}

uint16_t OMF96Format::BlockDefinitionRecord::GetRecordSize(OMF96Format * omf, Module * mod) const
{
	uint16_t bytes = 8;
	if(name_and_type)
	{
		bytes += name_and_type.value().name.size();
	}
	if(name_and_type)
	{
		bytes += IndexSize(name_and_type.value().type.index);
	}
	if(procedure_info)
	{
		if(std::get_if<ExternalReference>(&procedure_info.value().frame_pointer))
		{
			bytes += 2;
		}
		else if(std::get_if<LocalReference>(&procedure_info.value().frame_pointer))
		{
			bytes += 1;
		}
		bytes += 3;
	}
	return bytes;
}

void OMF96Format::BlockDefinitionRecord::WriteRecordContents(OMF96Format * omf, Module * mod, ChecksumWriter& wr) const
{
	if(name_and_type)
	{
		WriteString(wr, name_and_type.value().name);
	}
	else
	{
		WriteString(wr, "");
	}
	wr.WriteWord(1, segment_id);
	wr.WriteWord(2, offset);
	wr.WriteWord(2, size);
	uint8_t flags = 0;
	if(procedure_info)
	{
		flags |= FlagProcedure;
		if(std::get_if<ExternalReference>(&procedure_info.value().frame_pointer))
		{
			flags |= FlagExternal;
		}
	}
	wr.WriteWord(1, flags);
	if(name_and_type)
	{
		WriteIndex(wr, name_and_type.value().type.index);
	}
	if(procedure_info)
	{
		if(auto * reference = std::get_if<ExternalReference>(&procedure_info.value().frame_pointer))
		{
			wr.WriteWord(2, *reference);
		}
		else if(auto * reference = std::get_if<LocalReference>(&procedure_info.value().frame_pointer))
		{
			wr.WriteWord(1, *reference);
		}
		wr.WriteWord(2, procedure_info.value().return_offset);
		wr.WriteWord(1, procedure_info.value().prologue_size);
	}
}

void OMF96Format::BlockDefinitionRecord::CalculateValues(OMF96Format * omf, Module * mod)
{
	// TODO
}

void OMF96Format::BlockDefinitionRecord::ResolveReferences(OMF96Format * omf, Module * mod)
{
	// TODO
}

//// OMF96Format

std::shared_ptr<OMF96Format::Record> OMF96Format::ReadRecord(Linker::Reader& rd)
{
	Module * mod = modules.size() > 0 ? &modules.back() : nullptr;
	offset_t record_offset = rd.Tell();
	uint8_t record_type = rd.ReadUnsigned(1);
	uint16_t record_length = rd.ReadUnsigned(2);
	std::shared_ptr<Record> record;
	switch(record_type)
	{
	case ModuleHeader:
		record = std::make_shared<ModuleHeaderRecord>(record_type_t(record_type));
		break;
	case ModuleEnd:
		record = std::make_shared<ModuleEndRecord>(record_type_t(record_type));
		break;
	case Content:
		record = std::make_shared<ContentRecord>(record_type_t(record_type));
		break;
	case LineNumbers:
		record = std::make_shared<LineNumbersRecord>(record_type_t(record_type));
		break;
	case BlockDefinition:
		record = std::make_shared<BlockDefinitionRecord>(record_type_t(record_type));
		break;
	case BlockEnd:
		record = std::make_shared<EmptyRecord>(record_type_t(record_type));
		break;
	case EndOfFile:
		record = std::make_shared<EmptyRecord>(record_type_t(record_type));
		break;
	case ModuleAncestor:
		record = std::make_shared<ModuleAncestorRecord>(record_type_t(record_type));
		break;
	case LocalSymbols:
		record = std::make_shared<SymbolDefinitionsRecord>(record_type_t(record_type));
		break;
	case TypeDefinition:
		record = std::make_shared<TypeDefinitionRecord>(record_type_t(record_type));
		break;
	case PublicDefinitions:
		record = std::make_shared<SymbolDefinitionsRecord>(record_type_t(record_type));
		break;
	case ExternalDefinitions:
		record = std::make_shared<ExternalDefinitionsRecord>(record_type_t(record_type));
		break;
	case SegmentDefinitions:
		record = std::make_shared<SegmentDefinitionsRecord>(record_type_t(record_type));
		break;
	case Relocations:
		record = std::make_shared<RelocationRecord>(record_type_t(record_type));
		break;
	case LibraryModuleLocations:
		record = std::make_shared<LibraryModuleLocationsRecord>(record_type_t(record_type));
		break;
	case LibraryModuleNames:
		record = std::make_shared<LibraryModuleNamesRecord>(record_type_t(record_type));
		break;
	case LibraryDictionary:
		record = std::make_shared<LibraryDictionaryRecord>(record_type_t(record_type));
		break;
	case LibraryHeader:
		record = std::make_shared<LibraryHeaderRecord>(record_type_t(record_type));
		break;
	default:
		record = std::make_shared<UnknownRecord>(record_type_t(record_type));
	}
	record->record_offset = record_offset;
	record->record_length = record_length;
	record->ReadRecordContents(this, mod, rd);
	rd.ReadUnsigned(1); // checksum
	records.push_back(record);
	return record;
}

std::shared_ptr<OMF96Format> OMF96Format::ReadOMFFile(Linker::Reader& rd)
{
	std::shared_ptr<OMF96Format> omf = std::make_shared<OMF96Format>();
	omf->ReadFile(rd);
	return omf;
}

void OMF96Format::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = LittleEndian;
	rd.SeekEnd();
	offset_t end = rd.Tell();
	rd.Seek(0);

	while(rd.Tell() < end)
	{
		ReadRecord(rd);
	}

	file_size = rd.Tell();
}

offset_t OMF96Format::WriteFile(Linker::Writer& wr) const
{
	/* TODO */

	return offset_t(-1);
}

void OMF96Format::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("Intel OMF-96 format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 8);
	file_region.Display(dump);

	// TODO
}

void OMF96Format::GenerateModule(Linker::Module& module) const
{
	/* TODO */
}

//// OMFFormatContainer

void OMFFormatContainer::ReadFile(Linker::Reader& rd)
{
	contents = OMFFormat::ReadOMFFile(rd);
}

void OMFFormatContainer::GenerateModule(Linker::Module& module) const
{
	if(auto input_format = std::dynamic_pointer_cast<Linker::InputFormat>(contents))
	{
		input_format->GenerateModule(module);
	}
	// TODO: otherwise, issue error
}

offset_t OMFFormatContainer::WriteFile(Linker::Writer& wr) const
{
	return contents->WriteFile(wr);
}

offset_t OMFFormatContainer::ImageSize() const
{
	return contents->ImageSize();
}

void OMFFormatContainer::Dump(Dumper::Dumper& dump) const
{
	contents->Dump(dump);
}

