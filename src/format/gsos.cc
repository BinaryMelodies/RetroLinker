
#include "gsos.h"

using namespace Apple;

offset_t OMFFormat::Segment::ReadUnsigned(Linker::Reader& rd)
{
	return rd.ReadUnsigned(number_length);
}

void OMFFormat::Segment::WriteWord(Linker::Writer& wr, offset_t value)
{
	wr.WriteWord(number_length, value);
}

std::string OMFFormat::Segment::ReadLabel(Linker::Reader& rd)
{
	uint8_t length = label_length;
	if(length == 0)
	{
		length = rd.ReadUnsigned(1);
	}
	return rd.ReadData(length);
}

void OMFFormat::Segment::WriteLabel(Linker::Writer& wr, std::string text)
{
	uint8_t length = label_length;
	if(length == 0)
	{
		length = text.size();
		wr.WriteWord(1, length);
	}
	wr.WriteData(length, text);
}

offset_t OMFFormat::Segment::CalculateValues(uint16_t _segment_number, offset_t current_offset)
{
	segment_offset = current_offset;
	if(version == OMF_VERSION_1 && (flags & FLAG_ABSOLUTE_BANK))
	{
		// likely expected behavior
		kind = SEG_ABSBANK;
	}
	else if(version >= OMF_VERSION_2 && kind == SEG_ABSBANK)
	{
		// TODO: unsure, let us switch to data segment
		kind = SEG_DATA;
		// likely expected behavior
		flags |= FLAG_ABSOLUTE_BANK;
	}
	segment_number = _segment_number;
	if(version == OMF_VERSION_2_1 && temp_org != 0)
	{
		segment_name_offset = 0x30;
	}
	else if(segment_name_offset < 0x2C)
	{
		segment_name_offset = 0x2C;
	}
	offset_t minimum_segment_data_offset = segment_name_offset + 10 + (label_length == 0 ? 1 + segment_name.size() : label_length);
	if(segment_data_offset < minimum_segment_data_offset)
	{
		segment_data_offset = minimum_segment_data_offset;
	}
	linker_segment_name.resize(10, ' ');
	if(label_length != 0)
	{
		segment_name.resize(label_length, ' ');
	}
	//total_segment_size = ?; // TODO
	if(version == OMF_VERSION_1)
	{
		total_segment_size = ::AlignTo(total_segment_size, 0x200);
	}
	return segment_offset + total_segment_size;
}

void OMFFormat::Segment::ReadFile(Linker::Reader& rd)
{
	segment_offset = rd.Tell();

	rd.Skip(0x20);
	endiantype = rd.ReadUnsigned(1);
	switch(endiantype)
	{
	case 0:
		rd.endiantype = ::LittleEndian;
		break;
	case 1:
		rd.endiantype = ::BigEndian;
		break;
	default:
		Linker::Error << "Invalid NUMSEX field: " << endiantype << ", expected 0 or 1" << std::endl;
	}

	rd.Seek(segment_offset + 0x0F);
	version = omf_version(rd.ReadUnsigned(1) << 8);
	if(version != OMF_VERSION_1 && version != OMF_VERSION_2)
	{
		Linker::Error << "Invalid VERSION field: " << (version >> 8) << ", expected 1 or 2" << std::endl;
	}

	rd.Seek(segment_offset);
	total_segment_size = rd.ReadUnsigned(4);
	if(version == OMF_VERSION_1)
	{
		total_segment_size <<= 9;
	}
	bss_size = rd.ReadUnsigned(4);
	total_size = rd.ReadUnsigned(4);
	if(version == OMF_VERSION_1)
	{
		uint8_t value = rd.ReadUnsigned(1);
		kind = segment_kind(value & 0x1F);
		flags = (value & 0xE0) << 8;
	}
	else
	{
		rd.Skip(1);
	}
	label_length = rd.ReadUnsigned(1);
	number_length = rd.ReadUnsigned(4);
	if(number_length != 4)
	{
		Linker::Error << "Invalid NUMLEN field " << number_length << ", only 4 is allowed" << std::endl;
	}
	rd.Skip(1); // VERSION
	bank_size = rd.ReadUnsigned(4);
	if(version == OMF_VERSION_1)
	{
		rd.Skip(4);
	}
	else
	{
		uint16_t value = rd.ReadUnsigned(2);
		kind = segment_kind(value & 0x1F);
		flags = value & 0xFFE0;
		rd.Skip(2);
	}

	if(kind != SEG_CODE
	&& kind != SEG_DATA
	&& kind != SEG_JUMPTABLE
	&& kind != SEG_PATHNAME
	&& kind != SEG_LIBDICT
	&& kind != SEG_INIT
	&& (kind != SEG_ABSBANK || version > OMF_VERSION_1)
	&& kind != SEG_DIRPAGE)
	{
		Linker::Error << "Invalid segment kind" << std::endl;
	}

	base_address = rd.ReadUnsigned(4);
	align = rd.ReadUnsigned(4);
	rd.Skip(1); // NUMSEX
	if(version == OMF_VERSION_1)
	{
		language_card_bank = rd.ReadUnsigned(1);
	}
	else
	{
		version = omf_version(version + rd.ReadUnsigned(1));
		if(version != OMF_VERSION_2 && version != OMF_VERSION_2_1)
		{
			Linker::Error << "Invalid REVISION field: " << (version & 0xFF) << ", expected 0 or 1" << std::endl;
		}
	}
	segment_number = rd.ReadUnsigned(2);
	entry = rd.ReadUnsigned(4);
	segment_name_offset = rd.ReadUnsigned(2);
	segment_data_offset = rd.ReadUnsigned(2);
	/* we permit this field to be present in version 2.0 as well */
	if(version >= OMF_VERSION_2 && segment_name_offset >= 0x30)
	{
		temp_org = rd.ReadUnsigned(4);
	}

	rd.Seek(segment_offset + segment_name_offset);
	linker_segment_name = rd.ReadData(10);
	segment_name = ReadLabel(rd);

	rd.Seek(segment_offset + segment_data_offset);
	/* TODO: read contents */
}

void OMFFormat::Segment::WriteFile(Linker::Writer& wr)
{
	wr.Seek(segment_offset);
	switch(endiantype)
	{
	case 0:
		wr.endiantype = ::LittleEndian;
		break;
	case 1:
		wr.endiantype = ::BigEndian;
		break;
	default:
		Linker::Error << "Invalid NUMSEX field: " << endiantype << ", expected 0 or 1" << std::endl;
	}

	wr.WriteWord(4, version == OMF_VERSION_1 ? total_segment_size >> 9 : total_segment_size);
	wr.WriteWord(4, bss_size);
	wr.WriteWord(4, total_size);
	if(version == OMF_VERSION_1)
	{
		wr.WriteWord(1, (kind & 0x1F) | ((flags >> 8) & 0xE0));
	}
	else
	{
		wr.Skip(1);
	}
	wr.WriteWord(1, label_length);
	wr.WriteWord(1, number_length);
	wr.WriteWord(1, version >> 8);
	wr.WriteWord(4, bank_size);
	if(version == OMF_VERSION_1)
	{
		wr.Skip(4);
	}
	else
	{
		wr.WriteWord(2, (kind & 0x1F) | (flags & 0xFFE0));
		wr.Skip(2);
	}

	wr.WriteWord(4, base_address);
	wr.WriteWord(4, align);
	wr.WriteWord(1, endiantype);
	if(version == OMF_VERSION_1)
	{
		wr.WriteWord(1, language_card_bank);
	}
	else
	{
		wr.WriteWord(1, version & 0xFF);
	}
	wr.WriteWord(2, segment_number);
	wr.WriteWord(4, entry);
	wr.WriteWord(2, segment_name_offset);
	wr.WriteWord(2, segment_data_offset);
	/* we permit this field to be present in version 2.0 as well */
	if(version >= OMF_VERSION_2 && segment_name_offset >= 0x30)
	{
		wr.WriteWord(4, temp_org);
	}

	wr.Seek(segment_offset + segment_name_offset);
	wr.WriteData(10, linker_segment_name);
	WriteLabel(wr, segment_name);

	wr.Seek(segment_offset + segment_data_offset);
	// TODO: read contents
}

void OMFFormat::ReadFile(Linker::Reader& rd)
{
	rd.SeekEnd();
	offset_t end = rd.Tell();
	rd.Seek(0);
	while(rd.Tell() < end)
	{
		segments.push_back(new Segment);
		segments.back()->ReadFile(rd);
		rd.Seek(segments.back()->segment_offset + segments.back()->total_segment_size);
	}
}

void OMFFormat::WriteFile(Linker::Writer& wr)
{
	for(Segment * segment : segments)
	{
		segment->WriteFile(wr);
	}
}

OMFFormat::Segment::Record::~Record()
{
}

offset_t OMFFormat::Segment::Record::GetLength(Segment& segment)
{
	return 1;
}

void OMFFormat::Segment::Record::ReadFile(Segment& segment, Linker::Reader& rd)
{
}

void OMFFormat::Segment::Record::WriteFile(Segment& segment, Linker::Writer& wr)
{
	wr.WriteWord(1, type);
}

offset_t OMFFormat::Segment::DataRecord::GetLength(Segment& segment)
{
	if(OPC_CONST_FIRST <= type && type <= OPC_CONST_LAST)
	{
		return 1 + data.size();
	}
	else
	{
		return 5 + data.size();
	}
}

void OMFFormat::Segment::DataRecord::ReadFile(Segment& segment, Linker::Reader& rd)
{
	if(OPC_CONST_FIRST <= type && type <= OPC_CONST_LAST)
	{
		rd.ReadData(data.size(), reinterpret_cast<char *>(data.data()));
	}
	else
	{
		size_t length = rd.ReadUnsigned(4);
		data.resize(length);
		rd.ReadData(length, reinterpret_cast<char *>(data.data()));
	}
}

void OMFFormat::Segment::DataRecord::WriteFile(Segment& segment, Linker::Writer& wr)
{
	Record::WriteFile(segment, wr);
	if(OPC_CONST_FIRST <= type && type <= OPC_CONST_LAST)
	{
		wr.WriteData(data.size(), reinterpret_cast<char *>(data.data()));
	}
	else
	{
		wr.WriteWord(4, data.size());
		wr.WriteData(data.size(), reinterpret_cast<char *>(data.data()));
	}
}

offset_t OMFFormat::Segment::ValueRecord::GetLength(Segment& segment)
{
	return 1 + segment.number_length;
}

void OMFFormat::Segment::ValueRecord::ReadFile(Segment& segment, Linker::Reader& rd)
{
	value = segment.ReadUnsigned(rd);
}

void OMFFormat::Segment::ValueRecord::WriteFile(Segment& segment, Linker::Writer& wr)
{
	Record::WriteFile(segment, wr);
	segment.WriteWord(wr, value);
}

offset_t OMFFormat::Segment::StringRecord::GetLength(Segment& segment)
{
	return 1 + (segment.label_length == 0 ? 1 + value.size() : segment.label_length);
}

void OMFFormat::Segment::StringRecord::ReadFile(Segment& segment, Linker::Reader& rd)
{
	value = segment.ReadLabel(rd);
}

void OMFFormat::Segment::StringRecord::WriteFile(Segment& segment, Linker::Writer& wr)
{
	Record::WriteFile(segment, wr);
	segment.WriteLabel(wr, value);
}

offset_t OMFFormat::Segment::RelocationRecord::GetLength(Segment& segment)
{
	if(type == OPC_RELOC)
	{
		return 3 + 2 * segment.number_length;
	}
	else /*if(type == OPC_C_RELOC)*/
	{
		return 7;
	}
}

void OMFFormat::Segment::RelocationRecord::ReadFile(Segment& segment, Linker::Reader& rd)
{
	size = rd.ReadUnsigned(1);
	shift = rd.ReadUnsigned(1);
	if(type == OPC_RELOC)
	{
		source = segment.ReadUnsigned(rd);
		target = segment.ReadUnsigned(rd);
	}
	else /*if(type == OPC_C_RELOC)*/
	{
		source = rd.ReadUnsigned(2);
		target = rd.ReadUnsigned(2);
	}
}

void OMFFormat::Segment::RelocationRecord::WriteFile(Segment& segment, Linker::Writer& wr)
{
	Record::WriteFile(segment, wr);
	wr.WriteWord(1, size);
	wr.WriteWord(1, shift);
	if(type == OPC_RELOC)
	{
		segment.WriteWord(wr, source);
		segment.WriteWord(wr, target);
	}
	else /*if(type == OPC_C_RELOC)*/
	{
		wr.WriteWord(2, source);
		wr.WriteWord(2, target);
	}
}

offset_t OMFFormat::Segment::IntersegmentRelocationRecord::GetLength(Segment& segment)
{
	if(type == OPC_INTERSEG)
	{
		return 7 + 2 * segment.number_length;
	}
	else /*if(type == OPC_C_INTERSEG)*/
	{
		return 8;
	}
}

void OMFFormat::Segment::IntersegmentRelocationRecord::ReadFile(Segment& segment, Linker::Reader& rd)
{
	size = rd.ReadUnsigned(1);
	shift = rd.ReadUnsigned(1);
	if(type == OPC_INTERSEG)
	{
		source = segment.ReadUnsigned(rd);
		file_number = rd.ReadUnsigned(2);
		segment_number = rd.ReadUnsigned(2);
		target = segment.ReadUnsigned(rd);
	}
	else /*if(type == OPC_C_INTERSEG)*/
	{
		source = rd.ReadUnsigned(2);
		file_number = 0;
		segment_number = rd.ReadUnsigned(1);
		target = rd.ReadUnsigned(2);
	}
}

void OMFFormat::Segment::IntersegmentRelocationRecord::WriteFile(Segment& segment, Linker::Writer& wr)
{
	Record::WriteFile(segment, wr);
	wr.WriteWord(1, size);
	wr.WriteWord(1, shift);
	if(type == OPC_INTERSEG)
	{
		segment.WriteWord(wr, source);
		wr.WriteWord(2, file_number);
		wr.WriteWord(2, segment_number);
		segment.WriteWord(wr, target);
	}
	else /*if(type == OPC_C_INTERSEG)*/
	{
		wr.WriteWord(2, source);
		wr.WriteWord(1, segment_number);
		wr.WriteWord(2, target);
	}
}

offset_t OMFFormat::Segment::LabelRecord::GetLength(Segment& segment)
{
	return 5 + segment.number_length;
}

void OMFFormat::Segment::LabelRecord::ReadFile(Segment& segment, Linker::Reader& rd)
{
	name = segment.ReadLabel(rd);
	line_length = rd.ReadUnsigned(2);
	operation = operation_type(rd.ReadUnsigned(1));
	private_flag = rd.ReadUnsigned(1);
}

void OMFFormat::Segment::LabelRecord::WriteFile(Segment& segment, Linker::Writer& wr)
{
	Record::WriteFile(segment, wr);
	segment.WriteLabel(wr, name);
	wr.WriteWord(2, line_length);
	wr.WriteWord(1, operation);
	wr.WriteWord(1, private_flag);
}

offset_t OMFFormat::Segment::LabelExpressionRecord::GetLength(Segment& segment)
{
	return 5 + segment.number_length + expression->GetLength(segment);
}

void OMFFormat::Segment::LabelExpressionRecord::ReadFile(Segment& segment, Linker::Reader& rd)
{
	name = segment.ReadLabel(rd);
	line_length = rd.ReadUnsigned(2);
	operation = operation_type(rd.ReadUnsigned(1));
	private_flag = rd.ReadUnsigned(1);
	expression = segment.ReadExpression(rd);
}

void OMFFormat::Segment::LabelExpressionRecord::WriteFile(Segment& segment, Linker::Writer& wr)
{
	Record::WriteFile(segment, wr);
	segment.WriteLabel(wr, name);
	wr.WriteWord(2, line_length);
	wr.WriteWord(1, operation);
	wr.WriteWord(1, private_flag);
	expression->WriteFile(segment, wr);
}

offset_t OMFFormat::Segment::RangeRecord::GetLength(Segment& segment)
{
	return 1 + 2 * segment.number_length;
}

void OMFFormat::Segment::RangeRecord::ReadFile(Segment& segment, Linker::Reader& rd)
{
	start = segment.ReadUnsigned(rd);
	end = segment.ReadUnsigned(rd);
}

void OMFFormat::Segment::RangeRecord::WriteFile(Segment& segment, Linker::Writer& wr)
{
	Record::WriteFile(segment, wr);
	segment.WriteWord(wr, start);
	segment.WriteWord(wr, end);
}

offset_t OMFFormat::Segment::ExpressionRecord::GetLength(Segment& segment)
{
	return 2 + expression->GetLength(segment);
}

void OMFFormat::Segment::ExpressionRecord::ReadFile(Segment& segment, Linker::Reader& rd)
{
	size = rd.ReadUnsigned(1);
	expression = segment.ReadExpression(rd);
}

void OMFFormat::Segment::ExpressionRecord::WriteFile(Segment& segment, Linker::Writer& wr)
{
	Record::WriteFile(segment, wr);
	wr.WriteWord(1, size);
	expression->WriteFile(segment, wr);
}

offset_t OMFFormat::Segment::RelativeExpressionRecord::GetLength(Segment& segment)
{
	return 2 + segment.number_length + expression->GetLength(segment);
}

void OMFFormat::Segment::RelativeExpressionRecord::ReadFile(Segment& segment, Linker::Reader& rd)
{
	size = rd.ReadUnsigned(1);
	origin = segment.ReadUnsigned(rd);
	expression = segment.ReadExpression(rd);
}

void OMFFormat::Segment::RelativeExpressionRecord::WriteFile(Segment& segment, Linker::Writer& wr)
{
	Record::WriteFile(segment, wr);
	wr.WriteWord(1, size);
	segment.WriteWord(wr, origin);
	expression->WriteFile(segment, wr);
}

offset_t OMFFormat::Segment::EntryRecord::GetLength(Segment& segment)
{
	return 3 + segment.number_length + (segment.label_length == 0 ? 1 + name.size() : segment.label_length);
}

void OMFFormat::Segment::EntryRecord::ReadFile(Segment& segment, Linker::Reader& rd)
{
	segment_number = rd.ReadUnsigned(2);
	location = segment.ReadUnsigned(rd);
	name = segment.ReadLabel(rd);
}

void OMFFormat::Segment::EntryRecord::WriteFile(Segment& segment, Linker::Writer& wr)
{
	Record::WriteFile(segment, wr);
	wr.WriteWord(2, segment_number);
	segment.WriteWord(wr, location);
	segment.WriteLabel(wr, name);
}

offset_t OMFFormat::Segment::SuperCompactRecord::GetLength(Segment& segment)
{
	offset_t record_size = 1;
	uint16_t current_page = 0;
	for(uint16_t offset : offsets)
	{
		if((offset & 0xFF00) == current_page)
		{
			/* append byte for next patch */
			record_size ++;
		}
		else if((offset & 0xFF00) == current_page + 0x100)
		{
			/* append byte for next page and next patch */
			record_size += 2;
		}
		else if((offset & 0xFF00) < offset_t(current_page) + 0x8000)
		{
			/* append byte for skip patch, next page and next patch */
			record_size += 3;
		}
		else
		{
			/* we need two skip patch bytes */
			record_size += 4;
		}
		current_page = offset & 0xFF00;
	}
	return 6 + record_size;
}

void OMFFormat::Segment::SuperCompactRecord::ReadFile(Segment& segment, Linker::Reader& rd)
{
	offset_t record_size = rd.ReadUnsigned(4);
	offset_t start = rd.Tell();
	super_type = super_record_type(rd.ReadUnsigned(1));
	uint16_t current_page = 0;
	while(rd.Tell() < start + record_size)
	{
		uint8_t count = rd.ReadUnsigned(1);
		if(count <= 0x80)
		{
			for(int i = 0; i < count + 1; i++)
			{
				offsets.push_back(current_page + rd.ReadUnsigned(1));
			}
			current_page += 0x100;
		}
		else
		{
			current_page += (count - 0x80) << 8;
		}
	}
}

void OMFFormat::Segment::SuperCompactRecord::WriteFile(Segment& segment, Linker::Writer& wr)
{
	Record::WriteFile(segment, wr);
	wr.WriteWord(4, GetLength(segment) - 5);
	wr.WriteWord(1, super_type);
	uint16_t current_page = 0;
	std::vector<uint8_t> patches;
	for(uint16_t offset : offsets)
	{
		if((offset & 0xFF00) == current_page)
		{
			/* append byte for next patch */
			patches.push_back(offset & 0xFF);
		}
		else if((offset & 0xFF00) == current_page + 0x100)
		{
			/* append byte for next page and next patch */
			WritePatchList(wr, patches);
			patches.clear();
			patches.push_back(offset & 0xFF);
		}
		else if((offset & 0xFF00) < offset_t(current_page) + 0x8000)
		{
			/* append byte for skip patch, next page and next patch */
			WritePatchList(wr, patches);
			patches.clear();
			wr.WriteWord(1, 0x7F + ((offset - current_page) >> 8));
			patches.push_back(offset & 0xFF);
		}
		else
		{
			/* we need two skip patch bytes */
			WritePatchList(wr, patches);
			patches.clear();
			wr.WriteWord(1, 0xFF);
			wr.WriteWord(1, (offset - current_page) >> 8);
			patches.push_back(offset & 0xFF);
		}
	}
	WritePatchList(wr, patches);
}

void OMFFormat::Segment::SuperCompactRecord::WritePatchList(Linker::Writer& wr, const std::vector<uint8_t>& patches)
{
	wr.WriteWord(1, patches.size() - 1);
	for(uint8_t patch : patches)
	{
		wr.WriteWord(1, patch);
	}
}

OMFFormat::Segment::Expression * OMFFormat::Segment::ReadExpression(Linker::Reader& rd)
{
	// TODO
	return nullptr;
}

OMFFormat::Segment::Record * OMFFormat::Segment::ReadRecord(Linker::Reader& rd)
{
	uint8_t type = rd.ReadUnsigned(1);
	Record * record = nullptr;
	switch(type)
	{
	case Record::OPC_END:
		record = makeEND();
		break;
	case Record::OPC_ALIGN:
		record = makeALIGN();
		break;
	case Record::OPC_ORG:
		record = makeORG();
		break;
	case Record::OPC_RELOC:
		record = makeRELOC();
		break;
	case Record::OPC_INTERSEG:
		record = makeINTERSEG();
		break;
	case Record::OPC_USING:
		record = makeUSING();
		break;
	case Record::OPC_STRONG:
		record = makeSTRONG();
		break;
	case Record::OPC_GLOBAL:
		record = makeGLOBAL();
		break;
	case Record::OPC_GEQU:
		record = makeGEQU();
		break;
	case Record::OPC_MEM:
		record = makeMEM();
		break;
	case Record::OPC_EXPR:
		record = makeEXPR();
		break;
	case Record::OPC_ZEXPR:
		record = makeZEXPR();
		break;
	case Record::OPC_BEXPR:
		record = makeBEXPR();
		break;
	case Record::OPC_RELEXPR:
		record = makeRELEXPR();
		break;
	case Record::OPC_LOCAL:
		record = makeLOCAL();
		break;
	case Record::OPC_EQU:
		record = makeEQU();
		break;
	case Record::OPC_DS:
		record = makeDS();
		break;
	case Record::OPC_LCONST:
		record = makeLCONST();
		break;
	case Record::OPC_LEXPR:
		record = makeLEXPR();
		break;
	case Record::OPC_ENTRY:
		record = makeENTRY();
		break;
	case Record::OPC_C_RELOC:
		record = makecRELOC();
		break;
	case Record::OPC_C_INTERSEG:
		record = makecINTERSEG();
		break;
	case Record::OPC_SUPER: // V2
		if(version >= OMF_VERSION_2)
		{
			record = makeSUPER();
		}
	default:
		if(Record::OPC_CONST_FIRST <= type && type <= Record::OPC_CONST_LAST)
		{
			record = makeCONST(type);
		}
		else
		{
			record = new DataRecord(Record::record_type(type));
		}
	}
	// TODO: nullptr
	record->ReadFile(*this, rd);
	return record;
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeEND()
{
	return new Record(Record::OPC_END);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeCONST(std::vector<uint8_t> data)
{
	size_t length = data.size();
	if(length >= Record::OPC_CONST_LAST)
	{
		length = Record::OPC_CONST_LAST;
	}
	return makeCONST(data, length);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeCONST(std::vector<uint8_t> data, size_t length)
{
	if(length >= Record::OPC_CONST_LAST)
	{
		length = Record::OPC_CONST_LAST;
	}
	if(length > data.size())
	{
		length = data.size();
	}
	return new DataRecord(Record::record_type(Record::OPC_CONST_BASE + length), std::vector<uint8_t>(data.begin(), data.begin() + length));
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeCONST(size_t length)
{
	if(length >= Record::OPC_CONST_LAST)
	{
		length = Record::OPC_CONST_LAST;
	}
	return new DataRecord(Record::record_type(Record::OPC_CONST_BASE - 1 + length), length);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeALIGN(offset_t align)
{
	return new ValueRecord(Record::Record::OPC_ALIGN, align);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeORG(offset_t value)
{
	return new ValueRecord(Record::OPC_ORG, value);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeRELOC(uint8_t size, uint8_t shift, offset_t source, offset_t target)
{
	return new RelocationRecord(Record::OPC_RELOC, size, shift, source, target);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeRELOC()
{
	return new RelocationRecord(Record::OPC_RELOC);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeINTERSEG(uint8_t size, uint8_t shift, offset_t source, uint16_t file_number, uint16_t segment_number, offset_t target)
{
	return new IntersegmentRelocationRecord(Record::OPC_INTERSEG, size, shift, source, file_number, segment_number, target);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeINTERSEG()
{
	return new IntersegmentRelocationRecord(Record::OPC_INTERSEG);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeUSING(std::string name)
{
	return new StringRecord(Record::OPC_USING, name);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeSTRONG(std::string name)
{
	return new StringRecord(Record::OPC_STRONG, name);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeGLOBAL()
{
	return new LabelRecord(Record::OPC_GLOBAL);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeGLOBAL(std::string name, uint16_t line_length, int operation, uint16_t private_flag)
{
	return new LabelRecord(Record::OPC_GLOBAL, name, line_length, operation, private_flag);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeGEQU()
{
	return new LabelExpressionRecord(Record::OPC_GEQU);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeGEQU(std::string name, uint16_t line_length, int operation, uint16_t private_flag, Expression * expression)
{
	return new LabelExpressionRecord(Record::OPC_GEQU, name, line_length, operation, private_flag, expression);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeMEM()
{
	return new RangeRecord(Record::Record::OPC_MEM);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeMEM(offset_t start, offset_t end)
{
	return new RangeRecord(Record::Record::OPC_MEM, start, end);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeEXPR()
{
	return new ExpressionRecord(Record::Record::OPC_EXPR);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeEXPR(uint8_t size, Expression * expression)
{
	return new ExpressionRecord(Record::Record::OPC_EXPR, size, expression);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeZEXPR()
{
	return new ExpressionRecord(Record::Record::OPC_ZEXPR);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeZEXPR(uint8_t size, Expression * expression)
{
	return new ExpressionRecord(Record::Record::OPC_ZEXPR, size, expression);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeBEXPR()
{
	return new ExpressionRecord(Record::Record::OPC_BEXPR);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeBEXPR(uint8_t size, Expression * expression)
{
	return new ExpressionRecord(Record::Record::OPC_BEXPR, size, expression);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeRELEXPR()
{
	return new RelativeExpressionRecord(Record::Record::OPC_RELEXPR);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeRELEXPR(uint8_t size, offset_t origin, Expression * expression)
{
	return new RelativeExpressionRecord(Record::Record::OPC_RELEXPR, size, origin, expression);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeLOCAL()
{
	return new LabelRecord(Record::OPC_LOCAL);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeLOCAL(std::string name, uint16_t line_length, int operation, uint16_t private_flag)
{
	return new LabelRecord(Record::OPC_LOCAL, name, line_length, operation, private_flag);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeEQU()
{
	return new LabelExpressionRecord(Record::OPC_EQU);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeEQU(std::string name, uint16_t line_length, int operation, uint16_t private_flag, Expression * expression)
{
	return new LabelExpressionRecord(Record::OPC_EQU, name, line_length, operation, private_flag, expression);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeDS(offset_t count)
{
	return new ValueRecord(Record::OPC_DS, count);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeLCONST(std::vector<uint8_t> data)
{
	return makeLCONST(data, data.size());
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeLCONST(std::vector<uint8_t> data, size_t length)
{
	if(length > data.size())
	{
		length = data.size();
	}
	return new DataRecord(Record::OPC_LCONST, std::vector<uint8_t>(data.begin(), data.begin() + length));
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeLCONST()
{
	return new DataRecord(Record::OPC_LCONST);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeLEXPR()
{
	return new ExpressionRecord(Record::Record::OPC_LEXPR);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeLEXPR(uint8_t size, Expression * expression)
{
	return new ExpressionRecord(Record::Record::OPC_LEXPR, size, expression);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeENTRY()
{
	return new EntryRecord(Record::Record::OPC_ENTRY);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeENTRY(uint16_t segment_number, offset_t location, std::string name)
{
	return new EntryRecord(Record::Record::OPC_ENTRY, segment_number, location, name);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makecRELOC(uint8_t size, uint8_t shift, uint16_t source, uint16_t target)
{
	return new RelocationRecord(Record::OPC_C_RELOC, size, shift, source, target);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makecRELOC()
{
	return new RelocationRecord(Record::OPC_C_RELOC);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makecINTERSEG(uint8_t size, uint8_t shift, uint16_t source, uint16_t segment_number, uint16_t target)
{
	return new IntersegmentRelocationRecord(Record::OPC_C_INTERSEG, size, shift, source, 0, segment_number, target);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makecINTERSEG()
{
	return new IntersegmentRelocationRecord(Record::OPC_C_INTERSEG);
}

OMFFormat::Segment::Record * OMFFormat::Segment::makeSUPER(SuperCompactRecord::super_record_type super_type)
{
	return new SuperCompactRecord(Record::OPC_SUPER, super_type);
}

