
#include <algorithm>
#include "omf.h"

/* TODO: unimplemented */

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

uint32_t OMF86Format::ExternalName::ReadLength(OMF86Format * omf, Linker::Reader& rd)
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

uint32_t OMF86Format::ExternalName::GetLengthSize(OMF86Format * omf, uint32_t length)
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

void OMF86Format::ExternalName::WriteLength(OMF86Format * omf, ChecksumWriter& wr, uint32_t length)
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
		extname.value.far.number = ReadLength(omf, rd);
		extname.value.far.element_size = ReadLength(omf, rd);
		break;
	case NearCommon:
		extname.common_type = NearCommon;
		extname.value.near.length = ReadLength(omf, rd);
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
		bytes += 1 + GetLengthSize(omf, value.far.number) + GetLengthSize(omf, value.far.element_size);
		break;
	case NearCommon:
		bytes += 1 + GetLengthSize(omf, value.near.length);
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
		WriteLength(omf, wr, value.far.number);
		WriteLength(omf, wr, value.far.element_size);
		break;
	case NearCommon:
		wr.WriteWord(1, common_type);
		WriteLength(omf, wr, value.near.length);
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

uint16_t OMF86Format::BaseSpecification::GetSize(OMF86Format * omf) const
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

OMF86Format::SymbolDefinition OMF86Format::SymbolDefinition::ReadSymbol(OMF86Format * omf, Linker::Reader& rd, bool local, bool is32bit)
{
	SymbolDefinition name_definition;
	name_definition.name = ReadString(rd);
	name_definition.offset = rd.ReadUnsigned(is32bit ? 4 : 2);
	name_definition.type.index = ReadIndex(rd);
	name_definition.local = local;
	return name_definition;
}

uint16_t OMF86Format::SymbolDefinition::GetSymbolSize(OMF86Format * omf, bool is32bit) const
{
	return name.size() + IndexSize(type.index) + (is32bit ? 5 : 3);
}

void OMF86Format::SymbolDefinition::WriteSymbol(OMF86Format * omf, ChecksumWriter& wr, bool is32bit) const
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

OMF86Format::LineNumber OMF86Format::LineNumber::ReadLineNumber(OMF86Format * omf, Linker::Reader& rd, bool is32bit)
{
	LineNumber line_number;
	line_number.number = rd.ReadUnsigned(2);
	line_number.offset = rd.ReadUnsigned(is32bit ? 4 : 2);
	return line_number;
}

void OMF86Format::LineNumber::WriteLineNumber(OMF86Format * omf, ChecksumWriter& wr, bool is32bit) const
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

uint16_t OMF86Format::Reference::GetSize(OMF86Format * omf, bool is32bit) const
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

//// OMF86Format::Record

bool OMF86Format::Record::Is32Bit(OMF86Format * omf) const
{
	return (record_type & 1) != 0;
}

size_t OMF86Format::Record::GetOffsetSize(OMF86Format * omf) const
{
	return Is32Bit(omf) ? 4 : 2;
}

void OMF86Format::Record::CalculateValues(OMF86Format * omf, Module * mod)
{
}

void OMF86Format::Record::ResolveReferences(OMF86Format * omf, Module * mod)
{
}

std::shared_ptr<OMFFormat::Record<OMF86Format, OMF86Format::Module>> OMF86Format::Record::ReadRecord(OMF86Format * omf, Linker::Reader& rd)
{
	Module * mod = omf->modules.size() > 0 ? &omf->modules.back() : nullptr;
	offset_t record_offset = rd.Tell();
	uint8_t record_type = rd.ReadUnsigned(1);
	uint16_t record_length = rd.ReadUnsigned(2);
	std::shared_ptr<OMFFormat::Record<OMF86Format, Module>> record;
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
		return CommentRecord::ReadCommentRecord(omf, mod, rd, record_length);
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
		record = std::make_shared<IntelLibraryHeaderRecord>(record_type_t(record_type));
		break;
	case LIBNAM:
		record = std::make_shared<IntelLibraryModuleNamesRecord>(record_type_t(record_type));
		break;
	case LIBLOC:
		record = std::make_shared<IntelLibraryModuleLocationsRecord>(record_type_t(record_type));
		break;
	case LIBDIC:
		record = std::make_shared<IntelLibraryDictionaryRecord>(record_type_t(record_type));
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
		record = std::make_shared<LibraryHeaderRecord>(record_type_t(record_type));
		record->record_offset = record_offset;
		record->record_length = record_length;
		record->ReadRecordContents(omf, mod, rd);
		return record;
	case LibraryEnd:
		record = std::make_shared<LibraryEndRecord>(record_type_t(record_type));
		record->record_offset = record_offset;
		record->record_length = record_length;
		record->ReadRecordContents(omf, mod, rd);
		return record;
	default:
		record = std::make_shared<UnknownRecord>(record_type_t(record_type));
	}
	record->record_offset = record_offset;
	record->record_length = record_length;
	record->ReadRecordContents(omf, mod, rd);
	rd.ReadUnsigned(1); // checksum
	omf->records.push_back(record);
	return record;
}

//// OMF86Format::EmptyRecord

void OMF86Format::EmptyRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
}

uint16_t OMF86Format::EmptyRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	return 1;
}

void OMF86Format::EmptyRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
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
	offset_t record_end = rd.Tell() + record_length -  1;
	first_lname = omf->modules.back().lnames.size();
	lname_count = 0;
	while(rd.Tell() < record_end)
	{
		std::string name = OMF86Format::ReadString(rd, record_end - 1 - rd.Tell());
		omf->modules.back().lnames.push_back(name);
		names.push_back(name);
		lname_count++;
	}
	module = &omf->modules.back();
}

uint16_t OMF86Format::ListOfNamesRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	uint16_t bytes = 1;
	for(uint16_t lname_index = first_lname; lname_index < first_lname + lname_count; lname_index++)
	{
		bytes += module->lnames[lname_index].size() + 1;
	}
	return bytes;
}

void OMF86Format::ListOfNamesRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	for(uint16_t lname_index = first_lname; lname_index < first_lname + lname_count; lname_index++)
	{
		OMF86Format::WriteString(wr, module->lnames[lname_index]);
	}
}

void OMF86Format::ListOfNamesRecord::CalculateValues(OMF86Format * omf, Module * mod)
{
	// TODO
}

void OMF86Format::ListOfNamesRecord::ResolveReferences(OMF86Format * omf, Module * mod)
{
	// TODO
}

//// OMF86Format::SegmentDefinitionRecord

void OMF86Format::SegmentDefinitionRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	offset_t record_end = rd.Tell() + record_length -  1;
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

	if(rd.Tell() < record_end && omf->omf_version == OMF_VERSION_PHARLAP)
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

OMF86Format::GroupDefinitionRecord::Component OMF86Format::GroupDefinitionRecord::Component::ReadComponent(OMF86Format * omf, Module * mod, Linker::Reader& rd)
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

uint16_t OMF86Format::GroupDefinitionRecord::Component::GetComponentSize(OMF86Format * omf, Module * mod) const
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

void OMF86Format::GroupDefinitionRecord::Component::WriteComponent(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
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
	offset_t record_end = rd.Tell() + record_length -  1;
	name.index = ReadIndex(rd);
	while(rd.Tell() < record_end)
	{
		components.push_back(Component::ReadComponent(omf, mod, rd));
	}
	omf->modules.back().grpdefs.push_back(shared_from_this());
}

uint16_t OMF86Format::GroupDefinitionRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	uint16_t bytes = 1;
	bytes += IndexSize(name.index);
	for(auto& component : components)
	{
		bytes += component.GetComponentSize(omf, mod);
	}
	return bytes;
}

void OMF86Format::GroupDefinitionRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	WriteIndex(wr, name.index);
	for(auto& component : components)
	{
		component.WriteComponent(omf, mod, wr);
	}
}

void OMF86Format::GroupDefinitionRecord::CalculateValues(OMF86Format * omf, Module * mod)
{
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

OMF86Format::TypeDefinitionRecord::LeafDescriptor OMF86Format::TypeDefinitionRecord::LeafDescriptor::ReadLeaf(OMF86Format * omf, Module * mod, Linker::Reader& rd)
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

uint16_t OMF86Format::TypeDefinitionRecord::LeafDescriptor::GetLeafSize(OMF86Format * omf, Module * mod) const
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

void OMF86Format::TypeDefinitionRecord::LeafDescriptor::WriteLeaf(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
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
	offset_t record_end = rd.Tell() + record_length -  1;
	name = ReadString(rd);
	while(rd.Tell() < record_end)
	{
		uint8_t nice_bits = rd.ReadUnsigned(1);
		for(int leaf_number = 0; leaf_number < 8 && rd.Tell() + 1 < record_end; leaf_number++)
		{
			leafs.push_back(LeafDescriptor::ReadLeaf(omf, mod, rd));
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
		bytes += leafs[leaf_number].GetLeafSize(omf, mod);
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
			leafs[leaf_group_number + leaf_number].WriteLeaf(omf, mod, wr);
		}
	}
}

void OMF86Format::TypeDefinitionRecord::CalculateValues(OMF86Format * omf, Module * mod)
{
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
	offset_t record_end = rd.Tell() + record_length -  1;
	bool local = record_type == LPUBDEF16 || record_type == LPUBDEF32;
	base.Read(omf, rd);
	while(rd.Tell() < record_end)
	{
		symbols.push_back(SymbolDefinition::ReadSymbol(omf, rd, local, Is32Bit(omf)));
	}
}

uint16_t OMF86Format::SymbolsDefinitionRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	bool is32bit = Is32Bit(omf);
	uint16_t bytes = 1 + base.GetSize(omf);
	for(auto& symbol : symbols)
	{
		bytes += symbol.GetSymbolSize(omf, is32bit);
	}
	return bytes;
}

void OMF86Format::SymbolsDefinitionRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	bool is32bit = Is32Bit(omf);
	base.Write(omf, wr);
	for(auto& symbol : symbols)
	{
		symbol.WriteSymbol(omf, wr, is32bit);
	}
}

void OMF86Format::SymbolsDefinitionRecord::CalculateValues(OMF86Format * omf, Module * mod)
{
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
	offset_t record_end = rd.Tell() + record_length -  1;
	bool local = record_type != EXTDEF && record_type != COMDEF && record_type != CEXTDEF;
	bool common = record_type == COMDEF || record_type == LCOMDEF;
	first_extdef.index = omf->modules.back().extdefs.size();
	extdef_count = 0;
	while(rd.Tell() < record_end)
	{
		if(common)
			omf->modules.back().extdefs.push_back(ExternalName::ReadCommonName(omf, rd, local));
		else
			omf->modules.back().extdefs.push_back(ExternalName::ReadExternalName(omf, rd, local));
		extdef_count++;
	}
	module = &omf->modules.back();
}

uint16_t OMF86Format::ExternalNamesDefinitionRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	uint16_t bytes = 1;
	for(uint16_t extdef_index = first_extdef.index; extdef_index < first_extdef.index + extdef_count; extdef_index++)
	{
		bytes += module->extdefs[extdef_index].GetExternalNameSize(omf);
	}
	return bytes;
}

void OMF86Format::ExternalNamesDefinitionRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	for(uint16_t extdef_index = first_extdef.index; extdef_index < first_extdef.index + extdef_count; extdef_index++)
	{
		module->extdefs[extdef_index].WriteExternalName(omf, wr);
	}
}

void OMF86Format::ExternalNamesDefinitionRecord::CalculateValues(OMF86Format * omf, Module * mod)
{
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
	offset_t record_end = rd.Tell() + record_length -  1;
	base.Read(omf, rd);
	while(rd.Tell() < record_end)
	{
		lines.push_back(LineNumber::ReadLineNumber(omf, rd, Is32Bit(omf)));
	}
}

uint16_t OMF86Format::LineNumbersRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	uint16_t bytes = 1 + base.GetSize(omf);
	bytes += lines.size() * (2 + GetOffsetSize(omf));
	return bytes;
}

void OMF86Format::LineNumbersRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	bool is32bit = Is32Bit(omf);
	base.Write(omf, wr);
	for(auto& line : lines)
	{
		line.WriteLineNumber(omf, wr, is32bit);
	}
}

void OMF86Format::LineNumbersRecord::CalculateValues(OMF86Format * omf, Module * mod)
{
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
	return 7 + base.GetSize(omf) + name.size() + ((procedure & 0x80) ? 2 : 0) + (name != "" ? IndexSize(type.index) : 0);
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
	type.CalculateValues(omf, mod);
}

void OMF86Format::BlockDefinitionRecord::ResolveReferences(OMF86Format * omf, Module * mod)
{
	type.ResolveReferences(omf, mod);
}

//// OMF86Format::DebugSymbolsRecord

void OMF86Format::DebugSymbolsRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	offset_t record_end = rd.Tell() + record_length -  1;
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

	while(rd.Tell() < record_end)
	{
		names.push_back(SymbolDefinition::ReadSymbol(omf, rd, true, false));
	}
}

uint16_t OMF86Format::DebugSymbolsRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	uint16_t bytes = 2;
	if(auto spec = std::get_if<BaseSpecification>(&base))
	{
		bytes += spec->GetSize(omf);
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
		bytes += name.GetSymbolSize(omf, false);
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
		name.WriteSymbol(omf, wr, false);
	}
}

void OMF86Format::DebugSymbolsRecord::CalculateValues(OMF86Format * omf, Module * mod)
{
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
	offset_t record_end = rd.Tell() + record_length -  1;
	base.Read(omf, rd);
	offset = rd.ReadUnsigned(2);
	if(record_type == RIDATA)
		data = DataBlock::ReadIteratedDataBlock(omf, rd, false);
	else
		data = DataBlock::ReadEnumeratedDataBlock(omf, rd, record_end - rd.Tell());
}

uint16_t OMF86Format::RelocatableDataRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	uint16_t bytes = 3 + base.GetSize(omf);
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
	base.CalculateValues(omf, mod);
}

void OMF86Format::RelocatableDataRecord::ResolveReferences(OMF86Format * omf, Module * mod)
{
	base.ResolveReferences(omf, mod);
}

//// OMF86Format::PhysicalDataRecord

void OMF86Format::PhysicalDataRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	offset_t record_end = rd.Tell() + record_length -  1;
	address = uint32_t(rd.ReadUnsigned(2)) << 4;
	address += rd.ReadUnsigned(1);
	if(record_type == PIDATA)
		data = DataBlock::ReadIteratedDataBlock(omf, rd, false);
	else
		data = DataBlock::ReadEnumeratedDataBlock(omf, rd, record_end - rd.Tell());
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
	offset_t record_end = rd.Tell() + record_length -  1;
	segment.index = ReadIndex(rd);
	offset = rd.ReadUnsigned(GetOffsetSize(omf));
	if(record_type == LIDATA16)
		data = DataBlock::ReadIteratedDataBlock(omf, rd, false);
	else if(record_type == LIDATA32)
		data = DataBlock::ReadIteratedDataBlock(omf, rd, true);
	else
		data = DataBlock::ReadEnumeratedDataBlock(omf, rd, record_end - rd.Tell());
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

uint16_t OMF86Format::FixupRecord::Thread::GetSize(OMF86Format * omf, Module * mod) const
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

uint16_t OMF86Format::FixupRecord::Fixup::GetSize(OMF86Format * omf, Module * mod, bool is32bit) const
{
	return 2 + ref.GetSize(omf, is32bit);
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
	offset_t record_end = rd.Tell() + record_length -  1;
	while(rd.Tell() < record_end)
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
			bytes += thread->GetSize(omf, mod);
		}
		else if(auto * fixup = std::get_if<Fixup>(&data))
		{
			bytes += fixup->GetSize(omf, mod, Is32Bit(omf));
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

OMF86Format::RegisterInitializationRecord::Register OMF86Format::RegisterInitializationRecord::Register::ReadRegister(OMF86Format * omf, Module * mod, Linker::Reader& rd)
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

uint16_t OMF86Format::RegisterInitializationRecord::Register::GetRegisterSize(OMF86Format * omf, Module * mod) const
{
	if(auto * ref = std::get_if<Reference>(&value))
	{
		return 1 + ref->GetSize(omf, false);
	}
	else if(auto * init = std::get_if<InitialValue>(&value))
	{
		uint16_t bytes = 1 + init->base.GetSize(omf);
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

void OMF86Format::RegisterInitializationRecord::Register::WriteRegister(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
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
	offset_t record_end = rd.Tell() + record_length -  1;
	while(rd.Tell() < record_end)
	{
		registers.push_back(Register::ReadRegister(omf, mod, rd));
	}
}

uint16_t OMF86Format::RegisterInitializationRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	uint16_t bytes = 1;
	for(auto& reg : registers)
	{
		bytes += reg.GetRegisterSize(omf, mod);
	}
	return bytes;
}

void OMF86Format::RegisterInitializationRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	for(auto& reg : registers)
	{
		reg.WriteRegister(omf, mod, wr);
	}
}

void OMF86Format::RegisterInitializationRecord::CalculateValues(OMF86Format * omf, Module * mod)
{
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
	if((main_module & 0x40))
	{
		if((main_module & 0x01))
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
			bytes += ref->GetSize(omf, Is32Bit(omf));
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

//// OMF86Format::IntelLibraryHeaderRecord

void OMF86Format::IntelLibraryHeaderRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	module_count = rd.ReadUnsigned(2);
	block_number = rd.ReadUnsigned(2);
	byte_number = rd.ReadUnsigned(2);
}

uint16_t OMF86Format::IntelLibraryHeaderRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	return 7;
}

void OMF86Format::IntelLibraryHeaderRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	wr.WriteWord(2, module_count);
	wr.WriteWord(2, block_number);
	wr.WriteWord(2, byte_number);
}

//// OMF86Format::IntelLibraryModuleNamesRecord

void OMF86Format::IntelLibraryModuleNamesRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	offset_t record_end = rd.Tell() + record_length -  1;
	while(rd.Tell() < record_end)
	{
		names.push_back(ReadString(rd));
	}
}

uint16_t OMF86Format::IntelLibraryModuleNamesRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	uint16_t bytes = 1;
	for(auto& name : names)
	{
		bytes += name.size() + 1;
	}
	return bytes;
}

void OMF86Format::IntelLibraryModuleNamesRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	for(auto& name : names)
	{
		WriteString(wr, name);
	}
}

//// OMF86Format::IntelLibraryModuleLocationsRecord::Location

OMF86Format::IntelLibraryModuleLocationsRecord::Location OMF86Format::IntelLibraryModuleLocationsRecord::Location::ReadLocation(Linker::Reader& rd)
{
	Location location;
	location.block_number = rd.ReadUnsigned(2);
	location.byte_number = rd.ReadUnsigned(2);
	return location;
}

void OMF86Format::IntelLibraryModuleLocationsRecord::Location::WriteLocation(ChecksumWriter& wr) const
{
	wr.WriteWord(2, block_number);
	wr.WriteWord(2, byte_number);
}

//// OMF86Format::IntelLibraryModuleLocationsRecord

void OMF86Format::IntelLibraryModuleLocationsRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	offset_t record_end = rd.Tell() + record_length -  1;
	while(rd.Tell() < record_end)
	{
		locations.push_back(Location::ReadLocation(rd));
	}
}

uint16_t OMF86Format::IntelLibraryModuleLocationsRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	return 1 + 4 * locations.size();
}

void OMF86Format::IntelLibraryModuleLocationsRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	for(auto& location : locations)
	{
		location.WriteLocation(wr);
	}
}

//// OMF86Format::IntelLibraryDictionaryRecord

void OMF86Format::IntelLibraryDictionaryRecord::Group::ReadGroup(Linker::Reader& rd)
{
	while(true)
	{
		std::string name = ReadString(rd);
		if(name == "")
			break;
		names.push_back(name);
	}
}

uint16_t OMF86Format::IntelLibraryDictionaryRecord::Group::GetGroupSize() const
{
	uint16_t bytes = 1;
	for(auto& name : names)
	{
		bytes += 1 + name.size();
	}
	return bytes;
}

void OMF86Format::IntelLibraryDictionaryRecord::Group::WriteGroup(ChecksumWriter& wr) const
{
	for(auto& name : names)
	{
		WriteString(wr, name);
	}
	WriteString(wr, "");
}

void OMF86Format::IntelLibraryDictionaryRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	offset_t record_end = rd.Tell() + record_length -  1;
	while(rd.Tell() < record_end)
	{
		groups.push_back(Group());
		groups.back().ReadGroup(rd);
	}
}

uint16_t OMF86Format::IntelLibraryDictionaryRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	uint16_t bytes = 1;
	for(auto& group : groups)
	{
		bytes += group.GetGroupSize();
	}
	return bytes;
}

void OMF86Format::IntelLibraryDictionaryRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	for(auto& group : groups)
	{
		group.WriteGroup(wr);
	}
}

//// OMF86Format::BackpatchRecord

void OMF86Format::BackpatchRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	offset_t record_end = rd.Tell() + record_length -  1;
	segment = SegmentIndex(ReadIndex(rd));
	type = rd.ReadUnsigned(1);
	while(rd.Tell() < record_end)
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
	segment.CalculateValues(omf, mod);
}

void OMF86Format::BackpatchRecord::ResolveReferences(OMF86Format * omf, Module * mod)
{
	segment.ResolveReferences(omf, mod);
}

//// OMF86Format::NamedBackpatchRecord

void OMF86Format::NamedBackpatchRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	offset_t record_end = rd.Tell() + record_length -  1;
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
	while(rd.Tell() < record_end)
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
	offset_t record_end = rd.Tell() + record_length -  1;

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
		data = DataBlock::ReadEnumeratedDataBlock(omf, rd, record_end - rd.Tell());
}

uint16_t OMF86Format::InitializedCommunalDataRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	size_t bytes = 4 + GetOffsetSize(omf) + IndexSize(type.index) + base.GetSize(omf);

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
	offset_t record_end = rd.Tell() + record_length -  1;
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
	while(rd.Tell() < record_end)
	{
		line_numbers.push_back(LineNumber::ReadLineNumber(omf, rd, Is32Bit(omf)));
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
		line_number.WriteLineNumber(omf, wr, Is32Bit(omf));
	}
}

void OMF86Format::SymbolLineNumbersRecord::CalculateValues(OMF86Format * omf, Module * mod)
{
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

OMF86Format::AliasDefinitionRecord::AliasDefinition OMF86Format::AliasDefinitionRecord::AliasDefinition::ReadAliasDefinition(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	AliasDefinition alias;
	alias.alias_name = ReadString(rd);
	alias.substitute_name = ReadString(rd);
	return alias;
}

uint16_t OMF86Format::AliasDefinitionRecord::AliasDefinition::GetAliasDefinitionSize(OMF86Format * omf, Module * mod) const
{
	return 2 + alias_name.size() + substitute_name.size();
}

void OMF86Format::AliasDefinitionRecord::AliasDefinition::WriteAliasDefinition(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	WriteString(wr, alias_name);
	WriteString(wr, substitute_name);
}

//// OMF86Format::AliasDefinitionRecord

void OMF86Format::AliasDefinitionRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	offset_t record_end = rd.Tell() + record_length -  1;
	while(rd.Tell() < record_end)
	{
	alias_definitions.push_back(AliasDefinition::ReadAliasDefinition(omf, mod, rd));
	}
}

uint16_t OMF86Format::AliasDefinitionRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	uint16_t bytes = 1;
	for(auto& alias_definition : alias_definitions)
	{
		bytes += alias_definition.GetAliasDefinitionSize(omf, mod);
	}
	return bytes;
}

void OMF86Format::AliasDefinitionRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	for(auto& alias_definition : alias_definitions)
	{
		alias_definition.WriteAliasDefinition(omf, mod, wr);
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
	extension = rd.ReadData(record_length - 3);
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
	record->ReadComment(omf, mod, rd, record_length - 2);
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
	offset_t record_end = rd.Tell() + record_length -  1;
	while(rd.Tell() < record_end)
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
}

void OMF86Format::NoSegmentPaddingRecord::ResolveReferences(OMF86Format * omf, Module * mod)
{
}

//// OMF86Format::ExternalAssociationRecord::ExternalAssociation

OMF86Format::ExternalAssociationRecord::ExternalAssociation OMF86Format::ExternalAssociationRecord::ExternalAssociation::ReadAssociation(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	ExternalAssociation association;
	association.definition = ExternalIndex(ReadIndex(rd));
	association.default_resolution = ExternalIndex(ReadIndex(rd));
	return association;
}

uint16_t OMF86Format::ExternalAssociationRecord::ExternalAssociation::GetAssociationSize(OMF86Format * omf, Module * mod) const
{
	return IndexSize(definition.index) + IndexSize(default_resolution.index);
}

void OMF86Format::ExternalAssociationRecord::ExternalAssociation::WriteAssociation(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
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
	offset_t record_end = rd.Tell() + record_length -  1;
	while(rd.Tell() < record_end)
	{
		associations.push_back(ExternalAssociation::ReadAssociation(omf, mod, rd));
	}
}

uint16_t OMF86Format::ExternalAssociationRecord::GetCommentSize(OMF86Format * omf, Module * mod) const
{
	uint16_t bytes = 0;
	for(auto& association : associations)
	{
		bytes += association.GetAssociationSize(omf, mod);
	}
	return bytes;
}

void OMF86Format::ExternalAssociationRecord::WriteComment(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	for(auto& association : associations)
	{
		association.WriteAssociation(omf, mod, wr);
	}
}

void OMF86Format::ExternalAssociationRecord::CalculateValues(OMF86Format * omf, Module * mod)
{
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

//// OMF86Format::LibraryHeaderRecord

void OMF86Format::LibraryHeaderRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	omf->page_size = record_length + 3;
	dictionary_offset = rd.ReadUnsigned(4);
	dictionary_size = rd.ReadUnsigned(2);
	uint8_t flags = rd.ReadUnsigned(1);
	case_sensitive = (flags & 0x01) != 0;

	rd.Skip(omf->page_size - 10);
}

uint16_t OMF86Format::LibraryHeaderRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	return omf->page_size - 3;
}

void OMF86Format::LibraryHeaderRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	assert(false);
}

void OMF86Format::LibraryHeaderRecord::WriteRecord(OMF86Format * omf, Module * mod, Linker::Writer& wr) const
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

//// OMF86Format::LibraryEndRecord

void OMF86Format::LibraryEndRecord::ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd)
{
	rd.Skip(omf->page_size - 3);
}

uint16_t OMF86Format::LibraryEndRecord::GetRecordSize(OMF86Format * omf, Module * mod) const
{
	return omf->page_size - 3;
}

void OMF86Format::LibraryEndRecord::WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const
{
	assert(false);
}

void OMF86Format::LibraryEndRecord::WriteRecord(OMF86Format * omf, Module * mod, Linker::Writer& wr) const
{
	uint16_t page_size = dynamic_cast<OMF86Format *>(omf)->page_size;
	wr.WriteWord(1, record_type);
	wr.WriteWord(2, page_size - 3);
	wr.Skip(page_size - 3);
}

//// OMF86Format

OMF86Format::index_t OMF86Format::ReadIndex(Linker::Reader& rd)
{
	index_t index = rd.ReadUnsigned(1);
	if((index & 0x80))
	{
		index = ((index & 0x7F) << 8) | rd.ReadUnsigned(1);
	}
	return index;
}

void OMF86Format::WriteIndex(ChecksumWriter& wr, index_t index)
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

size_t OMF86Format::IndexSize(index_t index)
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

void OMF86Format::ReadFile(Linker::Reader& rd)
{
	/* TODO */
}

offset_t OMF86Format::WriteFile(Linker::Writer& wr) const
{
	/* TODO */

	return offset_t(-1);
}

void OMF86Format::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("Intel OMF format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 8);
	file_region.Display(dump);

	// TODO
}

void OMF86Format::GenerateModule(Linker::Module& module) const
{
	/* TODO */
}

