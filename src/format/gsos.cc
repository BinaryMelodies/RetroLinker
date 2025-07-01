
#include <sstream>
#include "gsos.h"
#include "../linker/buffer.h"
#include "../linker/reader.h"

using namespace Apple;

::EndianType OMFFormat::Segment::GetEndianType() const
{
	switch(endiantype)
	{
	case 0:
		return ::LittleEndian;
	case 1:
		return ::BigEndian;
	default:
		return ::UndefinedEndian;
	}
}

offset_t OMFFormat::Segment::ReadUnsigned(Linker::Reader& rd) const
{
	return rd.ReadUnsigned(number_length);
}

void OMFFormat::Segment::WriteWord(Linker::Writer& wr, offset_t value) const
{
	wr.WriteWord(number_length, value);
}

std::string OMFFormat::Segment::ReadLabel(Linker::Reader& rd) const
{
	uint8_t length = label_length;
	if(length == 0)
	{
		length = rd.ReadUnsigned(1);
	}
	return rd.ReadData(length);
}

void OMFFormat::Segment::WriteLabel(Linker::Writer& wr, std::string text) const
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
	rd.endiantype = GetEndianType();
	if(rd.endiantype == ::UndefinedEndian)
	{
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
	number_length = rd.ReadUnsigned(1);
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
	while(rd.Tell() < segment_offset + total_segment_size)
	{
		records.emplace_back(ReadRecord(rd));
		if(records.back()->type == Record::OPC_END)
			break;
	}
}

void OMFFormat::Segment::WriteFile(Linker::Writer& wr) const
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
	for(auto& record : records)
	{
		record->WriteFile(*this, wr);
	}
}

void OMFFormat::Segment::Dump(Dumper::Dumper& dump, const OMFFormat& omf, unsigned segment_index) const
{
	Dumper::Region segment_region("Segment", segment_offset, total_segment_size, 8);
	segment_region.AddField("Segment index", Dumper::DecDisplay::Make(), offset_t(segment_index + 1));
	if(segment_index + 1 != segment_number)
		segment_region.AddField("Segment number", Dumper::DecDisplay::Make(), offset_t(segment_number));
	segment_region.AddField("Segment name", Dumper::StringDisplay::Make("\""), segment_name);
	segment_region.AddField("Object name", Dumper::StringDisplay::Make("\""), linker_segment_name);
	segment_region.AddField("Segment name offset", Dumper::HexDisplay::Make(8), segment_name_offset);
	segment_region.AddField("OMF version", Dumper::VersionDisplay::Make(), offset_t(version >> 8), offset_t(version & 0xFF));
	segment_region.AddField("Memory size", Dumper::HexDisplay::Make(8), offset_t(total_size));
	segment_region.AddField("Zero fill", Dumper::HexDisplay::Make(8), offset_t(bss_size));
	segment_region.AddField("Address", Dumper::HexDisplay::Make(8), offset_t(base_address));
	segment_region.AddOptionalField("Virtual address", Dumper::HexDisplay::Make(8), offset_t(temp_org));
	segment_region.AddField("Align", Dumper::HexDisplay::Make(8), offset_t(align));
	static const std::map<offset_t, std::string> byte_order_descriptions =
	{
		{ 0, "little endian" },
		{ 1, "big endian" },
	};
	segment_region.AddField("Byte order", Dumper::ChoiceDisplay::Make(byte_order_descriptions), offset_t(endiantype));
	segment_region.AddField("Number size", Dumper::DecDisplay::Make(), offset_t(number_length));
	static std::map<offset_t, std::string> kind_descriptions =
	{
		{ 0x00, "code segment" },
		{ 0x01, "data segment" },
		{ 0x02, "jump table segment" },
		{ 0x04, "pathname segment" },
		{ 0x08, "library dictionary segment" },
		{ 0x10, "initialization segment" },
		{ 0x12, "direct page/stack segment" },
	};
	if(version < OMF_VERSION_2)
	{
		kind_descriptions[0x11] = "absolute bank segment";
	}
	else
	{
		kind_descriptions.erase(0x11);
	}
	segment_region.AddField("Kind", Dumper::ChoiceDisplay::Make(kind_descriptions, Dumper::HexDisplay::Make(2)), offset_t(kind));
	if(version < OMF_VERSION_1)
	{
		segment_region.AddField("Flags",
			Dumper::BitFieldDisplay::Make(2)
				->AddBitField(5, 1, Dumper::ChoiceDisplay::Make("position independent"), true)
				->AddBitField(6, 1, Dumper::ChoiceDisplay::Make("private"), true)
				->AddBitField(7, 1, Dumper::ChoiceDisplay::Make("dynamic", "static"), false),
			offset_t(flags >> 8));
	}
	else
	{
		segment_region.AddField("Flags",
			Dumper::BitFieldDisplay::Make(4)
				->AddBitField(8, 1, Dumper::ChoiceDisplay::Make("bank relative"), true)
				->AddBitField(9, 1, Dumper::ChoiceDisplay::Make("skip segment"), true)
				->AddBitField(10, 1, Dumper::ChoiceDisplay::Make("reload segment"), true)
				->AddBitField(11, 1, Dumper::ChoiceDisplay::Make("absolute bank"), true)
				->AddBitField(12, 1, Dumper::ChoiceDisplay::Make("not special memory"), true)
				->AddBitField(13, 1, Dumper::ChoiceDisplay::Make("position independent"), true)
				->AddBitField(14, 1, Dumper::ChoiceDisplay::Make("private"), true)
				->AddBitField(15, 1, Dumper::ChoiceDisplay::Make("dynamic", "static"), false),
			offset_t(flags));
	}
	if(label_length == 0)
		segment_region.AddField("Label size", Dumper::ChoiceDisplay::Make("variable"), offset_t(true));
	else
		segment_region.AddField("Label size", Dumper::DecDisplay::Make(), offset_t(label_length));
	segment_region.AddField("Bank size", Dumper::HexDisplay::Make(8), offset_t(bank_size));
	if(version < OMF_VERSION_2)
	{
		segment_region.AddOptionalField("Language card bank", Dumper::HexDisplay::Make(2), offset_t(language_card_bank));
	}
	segment_region.AddField("Entry", Dumper::HexDisplay::Make(8), offset_t(entry));
	segment_region.Display(dump);

	Dumper::Region records_region("Segment records", segment_offset + segment_data_offset, total_segment_size - segment_data_offset, 8);
	records_region.Display(dump);

	offset_t current_offset = segment_offset + segment_data_offset;
	offset_t current_address = base_address;
	unsigned record_index = 0;
	for(auto& record : records)
	{
		record->Dump(dump, omf, *this, record_index, current_offset, current_address);
		current_offset += record->GetLength(*this);
		current_address += record->GetMemoryLength(*this, current_address);
		record_index++;
	}
}

size_t OMFFormat::Segment::ReadData(size_t bytes, offset_t offset, void * buffer) const
{
	offset_t current_offset = 0;
	size_t count = 0;
	for(auto& record : records)
	{
		offset_t length = record->GetMemoryLength(*this, base_address + current_offset);
		if(offset < current_offset + length)
		{
			size_t actual_bytes = std::min(offset_t(bytes), current_offset + length - offset);
			record->ReadData(actual_bytes, offset, buffer);
			count += actual_bytes;
			bytes -= actual_bytes;
			buffer = reinterpret_cast<char *>(buffer) + actual_bytes;
			if(bytes == 0)
				return count;
		}
		current_offset += length;
		if(offset < length)
			offset = 0;
		else
			offset -= length;
	}
	return count;
}

offset_t OMFFormat::Segment::ReadUnsigned(size_t bytes, offset_t offset) const
{
	uint8_t buffer[bytes];
	offset_t count = ReadData(bytes, offset, buffer);
	return ::ReadUnsigned(bytes, count, buffer, GetEndianType());
}

offset_t OMFFormat::Segment::Expression::GetLength(const Segment& segment) const
{
	offset_t length = 1;
	for(auto& operand : operands)
	{
		length += operand->GetLength(segment);
	}
	return length;
}

void OMFFormat::Segment::Expression::ReadFile(Segment& segment, Linker::Reader& rd)
{
	while(ReadSingleOperation(segment, rd) != End)
	{
	}
}

void OMFFormat::Segment::Expression::WriteFile(const Segment& segment, Linker::Writer& wr) const
{
	for(auto& operand : operands)
	{
		operand->WriteFile(segment, wr);
	}
	if(value)
	{
		if(const offset_t * numberp = std::get_if<offset_t>(&value.value()))
		{
			segment.WriteWord(wr, *numberp);
		}
		else if(const std::string * labelp = std::get_if<std::string>(&value.value()))
		{
			segment.WriteLabel(wr, *labelp);
		}
	}
	if(operation != StackUnderflow)
		wr.WriteWord(1, operation);
}

std::string OMFFormat::Segment::Expression::GetStandardNotation() const
{
	std::ostringstream oss;
	GetStandardNotation(oss, precedence::Or);
	return oss.str();
}

void OMFFormat::Segment::Expression::PopElementsInto(size_t count, std::vector<std::unique_ptr<Expression>>& target)
{
	size_t actual_count = count;
	if(actual_count > operands.size())
	{
		actual_count = operands.size();
	}
	for(size_t i = actual_count; i < count; i++)
	{
		target.emplace_back(std::make_unique<Expression>(StackUnderflow));
	}
	target.insert(target.end(), std::make_move_iterator(operands.end() - count), std::make_move_iterator(operands.end()));
	operands.resize(operands.size() - count);
}

uint8_t OMFFormat::Segment::Expression::ReadSingleOperation(Segment& segment, Linker::Reader& rd)
{
	uint8_t next_operation = rd.ReadUnsigned(1);
	if(next_operation == End)
		return next_operation;

	std::unique_ptr<Expression> expression = std::make_unique<Expression>(next_operation);
	switch(next_operation)
	{
	case End:
		assert(false);

	case Negation:
	case Not:
	case BitNot:
		PopElementsInto(1, expression->operands);
		break;

	case Addition:
	case Subtraction:
	case Multiplication:
	case Division:
	case IntegerRemainder:
	case BitShift:
	case And:
	case Or:
	case EOr:
	case LessOrEqualTo:
	case GreaterOrEqualTo:
	case NotEqual:
	case LessThan:
	case GreaterThan:
	case EqualTo:
	case BitAnd:
	case BitOr:
	case BitEOr:
		PopElementsInto(2, expression->operands);
		break;

	case LocationCounterOperand:
		break;

	case ConstantOperand:
	case RelativeOffsetOperand:
		value = segment.ReadUnsigned(rd);
		break;

	case WeakLabelReferenceOperand:
	case LabelReferenceOperand:
	case LengthOfLabelReferenceOperand:
	case TypeOfLabelReferenceOperand:
	case CountOfLabelReferenceOperand:
		value = segment.ReadLabel(rd);
		break;

	default:
		break;
	}
	operands.emplace_back(std::move(expression));

	return next_operation;
}

void OMFFormat::Segment::Expression::GetStandardNotation(std::ostream& out, precedence_type precedence) const
{
	precedence_type required_precedence = precedence::Or;
	switch(operation)
	{
	default:
	case Negation:
	case Not:
	case BitNot:
		required_precedence = precedence::Negation;
		break;
	case Multiplication:
	case Division:
	case IntegerRemainder:
		required_precedence = precedence::Multiplication;
		break;
	case Addition:
	case Subtraction:
		required_precedence = precedence::Addition;
		break;
	case BitShift:
		required_precedence = precedence::BitShift;
		break;
	case LessOrEqualTo:
	case GreaterOrEqualTo:
	case LessThan:
	case GreaterThan:
		required_precedence = precedence::LessThan;
		break;
	case NotEqual:
	case EqualTo:
		required_precedence = precedence::EqualTo;
		break;
	case BitAnd:
		required_precedence = precedence::BitAnd;
		break;
	case BitEOr:
		required_precedence = precedence::BitEOr;
		break;
	case BitOr:
		required_precedence = precedence::BitOr;
		break;
	case And:
		required_precedence = precedence::And;
		break;
	case EOr:
		required_precedence = precedence::EOr;
		break;
	case Or:
	case End:
		required_precedence = precedence::Or;
		break;
	}
	if(precedence > required_precedence)
	{
		out << "(";
	}
	switch(operation)
	{
	case End:
		operands.back()->GetStandardNotation(out, required_precedence);
		break;
	case Addition:
	case Subtraction:
	case Multiplication:
	case Division:
	case IntegerRemainder:
	case BitShift:
	case And:
	case Or:
	case EOr:
	case LessOrEqualTo:
	case GreaterOrEqualTo:
	case NotEqual:
	case LessThan:
	case GreaterThan:
	case EqualTo:
	case BitAnd:
	case BitOr:
	case BitEOr:
		operands[0]->GetStandardNotation(out, precedence_type(required_precedence + 1));
		switch(operation)
		{
		case Addition:
			out << " + ";
			break;
		case Subtraction:
			out << " - ";
			break;
		case Multiplication:
			out << " * ";
			break;
		case Division:
			out << " / ";
			break;
		case IntegerRemainder:
			out << " % ";
			break;
		case BitShift:
			out << " << ";
			break;
		case And:
			out << " && ";
			break;
		case Or:
			out << " || ";
			break;
		case EOr:
			out << " ^^ ";
			break;
		case LessOrEqualTo:
			out << " <= ";
			break;
		case GreaterOrEqualTo:
			out << " >= ";
			break;
		case NotEqual:
			out << " != ";
			break;
		case LessThan:
			out << " < ";
			break;
		case GreaterThan:
			out << " > ";
			break;
		case EqualTo:
			out << " == ";
			break;
		case BitAnd:
			out << " & ";
			break;
		case BitOr:
			out << " | ";
			break;
		case BitEOr:
			out << " ^ ";
			break;
		default:
			assert(false);
		}
		operands[1]->GetStandardNotation(out, required_precedence);
		break;
	case Negation:
		out << "-";
		operands[0]->GetStandardNotation(out, required_precedence);
		break;
	case Not:
		out << "!";
		operands[0]->GetStandardNotation(out, required_precedence);
		break;
	case BitNot:
		out << "~";
		operands[0]->GetStandardNotation(out, required_precedence);
		break;
	case LocationCounterOperand:
		out << ".";
		break;
	case ConstantOperand:
		out << *std::get_if<offset_t>(&value.value());
		break;
	case WeakLabelReferenceOperand:
		out << "weak " << *std::get_if<std::string>(&value.value());
		break;
	case LabelReferenceOperand:
		out << *std::get_if<std::string>(&value.value());
		break;
	case LengthOfLabelReferenceOperand:
		out << "length(" << *std::get_if<std::string>(&value.value()) << ")";
		break;
	case TypeOfLabelReferenceOperand:
		out << "type(" << *std::get_if<std::string>(&value.value()) << ")";
		break;
	case CountOfLabelReferenceOperand:
		out << "defined(" << *std::get_if<std::string>(&value.value()) << ")";
		break;
	case RelativeOffsetOperand:
		out << "..";
		break;
	case StackUnderflow:
		out << "?";
		break;
	}
	if(precedence > required_precedence)
	{
		out << ")";
	}
}

void OMFFormat::CalculateValues()
{
	offset_t current_offset = 0;
	for(unsigned i = 0; i < segments.size(); i++)
	{
		current_offset = segments[i]->CalculateValues(i, current_offset);
	}
}

void OMFFormat::ReadFile(Linker::Reader& rd)
{
	rd.SeekEnd();
	offset_t end = rd.Tell();
	rd.Seek(0);
	while(rd.Tell() < end)
	{
		segments.push_back(std::make_unique<Segment>());
		segments.back()->ReadFile(rd);
		rd.Seek(segments.back()->segment_offset + segments.back()->total_segment_size);
	}
}

offset_t OMFFormat::WriteFile(Linker::Writer& wr) const
{
	for(auto& segment : segments)
	{
		segment->WriteFile(wr);
	}

	return ImageSize();
}

offset_t OMFFormat::ImageSize() const
{
	if(segments.size() == 0)
		return 0;
	return segments.back()->segment_offset + segments.back()->total_segment_size;
}

void OMFFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("GS/OS OMF format");
	Dumper::Region file_region("File", file_offset, ImageSize(), 8);
	file_region.Display(dump);

	unsigned segment_index = 0;
	for(auto& segment : segments)
	{
		segment->Dump(dump, *this, segment_index);
		segment_index++;
	}
}

offset_t OMFFormat::Segment::Record::GetLength(const Segment& segment) const
{
	return 1;
}

offset_t OMFFormat::Segment::Record::GetMemoryLength(const Segment& segment, offset_t current_address) const
{
	return 0;
}

void OMFFormat::Segment::Record::ReadFile(Segment& segment, Linker::Reader& rd)
{
}

void OMFFormat::Segment::Record::WriteFile(const Segment& segment, Linker::Writer& wr) const
{
	wr.WriteWord(1, type);
}

void OMFFormat::Segment::Record::Dump(Dumper::Dumper& dump, const OMFFormat& omf, const Segment& segment, unsigned index, offset_t file_offset, offset_t address) const
{
	Dumper::Region record_region("Record", file_offset, GetLength(segment), 8);
	record_region.InsertField(0, "Number", Dumper::DecDisplay::Make(), offset_t(index + 1));
	static std::map<offset_t, std::string> opcode_description =
	{
		{ OPC_END, "END" },
		{ OPC_ALIGN, "ALIGN" },
		{ OPC_ORG, "ORG" },
		{ OPC_RELOC, "RELOC" },
		{ OPC_INTERSEG, "INTERSEG" },
		{ OPC_USING, "USING" },
		{ OPC_STRONG, "STRONG" },
		{ OPC_GLOBAL, "GLOBAL" },
		{ OPC_GEQU, "GEQU" },
		{ OPC_MEM, "MEM" },
		{ OPC_EXPR, "EXPR" },
		{ OPC_ZEXPR, "ZEXPR" },
		{ OPC_BEXPR, "BEXPR" },
		{ OPC_RELEXPR, "RELEXPR" },
		{ OPC_LOCAL, "LOCAL" },
		{ OPC_EQU, "EQU" },
		{ OPC_DS, "DS" },
		{ OPC_LCONST, "LCONST" },
		{ OPC_LEXPR, "LEXPR" },
		{ OPC_ENTRY, "ENTRY" },
		{ OPC_C_RELOC, "cRELOC" },
		{ OPC_C_INTERSEG, "cINTERSEG" },
		{ OPC_SUPER, "SUPER" },
	};
	if(opcode_description.find(OPC_CONST_FIRST) == opcode_description.end())
	{
		for(int opcode = OPC_CONST_FIRST; opcode <= OPC_CONST_LAST; opcode++)
		{
			opcode_description[opcode] = "CONST";
		}
	}
	record_region.AddField("Record opcode", Dumper::ChoiceDisplay::Make(opcode_description, Dumper::HexDisplay::Make(2)), offset_t(type));
	AddFields(dump, record_region, omf, segment, index, file_offset, address);
	record_region.Display(dump);
}

void OMFFormat::Segment::Record::AddFields(Dumper::Dumper& dump, Dumper::Region& region, const OMFFormat& omf, const Segment& segment, unsigned index, offset_t file_offset, offset_t address) const
{
}

void OMFFormat::Segment::Record::AddSignals(Dumper::Block& block, offset_t current_segment_offset) const
{
}

void OMFFormat::Segment::Record::ReadData(size_t bytes, offset_t offset, void * buffer) const
{
}

offset_t OMFFormat::Segment::DataRecord::GetLength(const Segment& segment) const
{
	if(OPC_CONST_FIRST <= type && type <= OPC_CONST_LAST)
	{
		return 1 + image->ImageSize();
	}
	else
	{
		return 5 + image->ImageSize();
	}
}

offset_t OMFFormat::Segment::DataRecord::GetMemoryLength(const Segment& segment, offset_t current_address) const
{
	return image->ImageSize();
}

void OMFFormat::Segment::DataRecord::ReadFile(Segment& segment, Linker::Reader& rd)
{
	if(OPC_CONST_FIRST <= type && type <= OPC_CONST_LAST)
	{
		image = Linker::Buffer::ReadFromFile(rd, type - OPC_CONST_BASE);
	}
	else
	{
		size_t length = rd.ReadUnsigned(4);
		image = Linker::Buffer::ReadFromFile(rd, length);
	}
}

void OMFFormat::Segment::DataRecord::WriteFile(const Segment& segment, Linker::Writer& wr) const
{
	Record::WriteFile(segment, wr);
	if(OPC_CONST_FIRST <= type && type <= OPC_CONST_LAST)
	{
		image->WriteFile(wr);
	}
	else
	{
		wr.WriteWord(4, image->ImageSize());
		image->WriteFile(wr);
	}
}

void OMFFormat::Segment::DataRecord::Dump(Dumper::Dumper& dump, const OMFFormat& omf, const Segment& segment, unsigned index, offset_t file_offset, offset_t address) const
{
	OMFFormat::Segment::Record::Dump(dump, omf, segment, index, file_offset, address);
	
	Dumper::Block data_block("Data", type == OPC_LCONST ? file_offset + 5 : file_offset + 1,
		image->AsImage(), address, 8);
	for(auto& record : segment.records)
	{
		record->AddSignals(data_block, address - segment.base_address);
	}
	data_block.Display(dump);
}

void OMFFormat::Segment::DataRecord::ReadData(size_t bytes, offset_t offset, void * buffer) const
{
	// CONST or LCONST
	image->AsImage()->ReadData(bytes, offset, buffer);
}

offset_t OMFFormat::Segment::ValueRecord::GetLength(const Segment& segment) const
{
	return 1 + segment.number_length;
}

offset_t OMFFormat::Segment::ValueRecord::GetMemoryLength(const Segment& segment, offset_t current_address) const
{
	switch(type)
	{
	case OPC_DS:
	case OPC_ORG:
		return value;
	case OPC_ALIGN:
		return ::AlignTo(current_address, value) - current_address;
	default:
		assert(false);
	}
}

void OMFFormat::Segment::ValueRecord::ReadFile(Segment& segment, Linker::Reader& rd)
{
	value = segment.ReadUnsigned(rd);
}

void OMFFormat::Segment::ValueRecord::WriteFile(const Segment& segment, Linker::Writer& wr) const
{
	Record::WriteFile(segment, wr);
	segment.WriteWord(wr, value);
}

void OMFFormat::Segment::ValueRecord::AddFields(Dumper::Dumper& dump, Dumper::Region& region, const OMFFormat& omf, const Segment& segment, unsigned index, offset_t file_offset, offset_t address) const
{
	if(type == OPC_DS)
	{
		region.AddField("Address", Dumper::HexDisplay::Make(8), offset_t(address));
		region.AddField("Length", Dumper::HexDisplay::Make(8), offset_t(value));
	}
	else
	{
		region.AddField("Value", Dumper::HexDisplay::Make(8), offset_t(value));
	}
}

void OMFFormat::Segment::ValueRecord::ReadData(size_t bytes, offset_t offset, void * buffer) const
{
	memset(buffer, 0, bytes);
}

offset_t OMFFormat::Segment::RelocationRecord::GetLength(const Segment& segment) const
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
	shift = rd.ReadSigned(1);
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

void OMFFormat::Segment::RelocationRecord::WriteFile(const Segment& segment, Linker::Writer& wr) const
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

void OMFFormat::Segment::RelocationRecord::AddFields(Dumper::Dumper& dump, Dumper::Region& region, const OMFFormat& omf, const Segment& segment, unsigned index, offset_t file_offset, offset_t address) const
{
	region.AddField("Size", Dumper::DecDisplay::Make(), offset_t(size));
	region.AddOptionalField("Shift (left)", Dumper::DecDisplay::Make(), offset_t(shift));
	region.AddField("Source", Dumper::HexDisplay::Make(8), offset_t(source));
	region.AddField("Target", Dumper::HexDisplay::Make(8), offset_t(target));
}

void OMFFormat::Segment::RelocationRecord::AddSignals(Dumper::Block& block, offset_t current_segment_offset) const
{
	if(current_segment_offset <= source)
	{
		block.AddSignal(source - current_segment_offset, size);
	}
}

offset_t OMFFormat::Segment::IntersegmentRelocationRecord::GetLength(const Segment& segment) const
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
		file_number = 1;
		segment_number = rd.ReadUnsigned(1);
		target = rd.ReadUnsigned(2);
	}
}

void OMFFormat::Segment::IntersegmentRelocationRecord::WriteFile(const Segment& segment, Linker::Writer& wr) const
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

void OMFFormat::Segment::IntersegmentRelocationRecord::AddFields(Dumper::Dumper& dump, Dumper::Region& region, const OMFFormat& omf, const Segment& segment, unsigned index, offset_t file_offset, offset_t address) const
{
	region.AddField("Size", Dumper::DecDisplay::Make(), offset_t(size));
	region.AddOptionalField("Shift (left)", Dumper::DecDisplay::Make(), offset_t(shift));
	region.AddField("Source", Dumper::HexDisplay::Make(8), offset_t(source));
	if(file_number == 1)
		region.AddField("Target", Dumper::SectionedDisplay<offset_t>::Make(Dumper::HexDisplay::Make(8)), offset_t(segment_number), offset_t(target));
	else
		region.AddField("Target", Dumper::SectionedDisplay<offset_t, offset_t>::Make(Dumper::SectionedDisplay<offset_t>::Make(Dumper::HexDisplay::Make(8))), offset_t(file_number), offset_t(segment_number), offset_t(target));
}

offset_t OMFFormat::Segment::StringRecord::GetLength(const Segment& segment) const
{
	return 1 + (segment.label_length == 0 ? 1 + name.size() : segment.label_length);
}

void OMFFormat::Segment::StringRecord::ReadFile(Segment& segment, Linker::Reader& rd)
{
	name = segment.ReadLabel(rd);
}

void OMFFormat::Segment::StringRecord::WriteFile(const Segment& segment, Linker::Writer& wr) const
{
	Record::WriteFile(segment, wr);
	segment.WriteLabel(wr, name);
}

void OMFFormat::Segment::StringRecord::AddFields(Dumper::Dumper& dump, Dumper::Region& region, const OMFFormat& omf, const Segment& segment, unsigned index, offset_t file_offset, offset_t address) const
{
	region.AddField("Name", Dumper::StringDisplay::Make(), name);
}

offset_t OMFFormat::Segment::LabelRecord::GetLength(const Segment& segment) const
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

void OMFFormat::Segment::LabelRecord::WriteFile(const Segment& segment, Linker::Writer& wr) const
{
	Record::WriteFile(segment, wr);
	segment.WriteLabel(wr, name);
	wr.WriteWord(2, line_length);
	wr.WriteWord(1, operation);
	wr.WriteWord(1, private_flag);
}

void OMFFormat::Segment::LabelRecord::AddFields(Dumper::Dumper& dump, Dumper::Region& region, const OMFFormat& omf, const Segment& segment, unsigned index, offset_t file_offset, offset_t address) const
{
	region.AddField("Name", Dumper::StringDisplay::Make(), name);
	if(line_length != 0xFFFF)
		region.AddField("Line length", Dumper::DecDisplay::Make(), offset_t(line_length));
	else
		region.AddField("Line length", Dumper::ChoiceDisplay::Make("longer or equal to 65535"), offset_t(true));

	static const std::map<offset_t, std::string> operation_descriptions =
	{
		{ OP_ADDRESS_DC, "address type DC statement" },
		{ OP_BOOL_DC, "Boolean type DC statement" },
		{ OP_CHAR_DC, "character type DC statement" },
		{ OP_DOUBLE_DC, "double precision type DC statement" },
		{ OP_FLOAT_DC, "floating point type DC statement" },
		{ OP_EQU_GEQU, "EQU/GEQU statement" },
		{ OP_HEX_DC, "hexadecimal type DC statement" },
		{ OP_INT_DC, "integer type DC statement" },
		{ OP_REF_ADDRESS_DC, "reference address type DC statement" },
		{ OP_SOFT_REF_DC, "soft reference type DC statement" },
		{ OP_INSTRUCTION, "instruction" },
		{ OP_ASM_DIRECTIVE, "assembler directive" },
		{ OP_ORG, "ORG statement" },
		{ OP_ALIGN, "ALIGN statement" },
		{ OP_DS, "DS statement" },
		{ OP_ARITHMETIC_SYMBOL, "arithmetic symbol parameter" },
		{ OP_BOOL_SYMBOL, "Boolean symbol parameter" },
		{ OP_CHAR_SYMBOL, "character symbol parameter" },
	};
	region.AddField("Operation type", Dumper::ChoiceDisplay::Make(operation_descriptions), offset_t(operation));

	static const std::map<offset_t, std::string> private_description =
	{
		{ 1, "true" },
	};
	region.AddOptionalField("Private", Dumper::ChoiceDisplay::Make(private_description), offset_t(private_flag));
}

offset_t OMFFormat::Segment::LabelExpressionRecord::GetLength(const Segment& segment) const
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

void OMFFormat::Segment::LabelExpressionRecord::WriteFile(const Segment& segment, Linker::Writer& wr) const
{
	Record::WriteFile(segment, wr);
	segment.WriteLabel(wr, name);
	wr.WriteWord(2, line_length);
	wr.WriteWord(1, operation);
	wr.WriteWord(1, private_flag);
	expression->WriteFile(segment, wr);
}

void OMFFormat::Segment::LabelExpressionRecord::AddFields(Dumper::Dumper& dump, Dumper::Region& region, const OMFFormat& omf, const Segment& segment, unsigned index, offset_t file_offset, offset_t address) const
{
	LabelRecord::AddFields(dump, region, omf, segment, index, file_offset, address);
	region.AddField("Expression", Dumper::StringDisplay::Make(), expression->GetStandardNotation());
}

offset_t OMFFormat::Segment::RangeRecord::GetLength(const Segment& segment) const
{
	return 1 + 2 * segment.number_length;
}

void OMFFormat::Segment::RangeRecord::ReadFile(Segment& segment, Linker::Reader& rd)
{
	start = segment.ReadUnsigned(rd);
	end = segment.ReadUnsigned(rd);
}

void OMFFormat::Segment::RangeRecord::WriteFile(const Segment& segment, Linker::Writer& wr) const
{
	Record::WriteFile(segment, wr);
	segment.WriteWord(wr, start);
	segment.WriteWord(wr, end);
}

void OMFFormat::Segment::RangeRecord::AddFields(Dumper::Dumper& dump, Dumper::Region& region, const OMFFormat& omf, const Segment& segment, unsigned index, offset_t file_offset, offset_t address) const
{
	region.AddField("Start", Dumper::HexDisplay::Make(8), offset_t(start));
	region.AddField("End", Dumper::HexDisplay::Make(8), offset_t(end));
}

offset_t OMFFormat::Segment::ExpressionRecord::GetLength(const Segment& segment) const
{
	return 2 + expression->GetLength(segment);
}

offset_t OMFFormat::Segment::ExpressionRecord::GetMemoryLength(const Segment& segment, offset_t current_address) const
{
	return size;
}

void OMFFormat::Segment::ExpressionRecord::ReadFile(Segment& segment, Linker::Reader& rd)
{
	size = rd.ReadUnsigned(1);
	expression = segment.ReadExpression(rd);
}

void OMFFormat::Segment::ExpressionRecord::WriteFile(const Segment& segment, Linker::Writer& wr) const
{
	Record::WriteFile(segment, wr);
	wr.WriteWord(1, size);
	expression->WriteFile(segment, wr);
}

void OMFFormat::Segment::ExpressionRecord::AddFields(Dumper::Dumper& dump, Dumper::Region& region, const OMFFormat& omf, const Segment& segment, unsigned index, offset_t file_offset, offset_t address) const
{
	region.AddField("Size", Dumper::DecDisplay::Make(), offset_t(size));
	region.AddField("Expression", Dumper::StringDisplay::Make(), expression->GetStandardNotation());
}

void OMFFormat::Segment::ExpressionRecord::ReadData(size_t bytes, offset_t offset, void * buffer) const
{
	memset(buffer, 0, bytes);
}

offset_t OMFFormat::Segment::RelativeExpressionRecord::GetLength(const Segment& segment) const
{
	return 2 + segment.number_length + expression->GetLength(segment);
}

void OMFFormat::Segment::RelativeExpressionRecord::ReadFile(Segment& segment, Linker::Reader& rd)
{
	size = rd.ReadUnsigned(1);
	origin = segment.ReadUnsigned(rd);
	expression = segment.ReadExpression(rd);
}

void OMFFormat::Segment::RelativeExpressionRecord::WriteFile(const Segment& segment, Linker::Writer& wr) const
{
	Record::WriteFile(segment, wr);
	wr.WriteWord(1, size);
	segment.WriteWord(wr, origin);
	expression->WriteFile(segment, wr);
}

void OMFFormat::Segment::RelativeExpressionRecord::AddFields(Dumper::Dumper& dump, Dumper::Region& region, const OMFFormat& omf, const Segment& segment, unsigned index, offset_t file_offset, offset_t address) const
{
	ExpressionRecord::AddFields(dump, region, omf, segment, index, file_offset, address);
	region.AddField("Offset of origin", Dumper::HexDisplay::Make(8), offset_t(origin));
}

offset_t OMFFormat::Segment::EntryRecord::GetLength(const Segment& segment) const
{
	return 3 + segment.number_length + (segment.label_length == 0 ? 1 + name.size() : segment.label_length);
}

void OMFFormat::Segment::EntryRecord::ReadFile(Segment& segment, Linker::Reader& rd)
{
	segment_number = rd.ReadUnsigned(2);
	location = segment.ReadUnsigned(rd);
	name = segment.ReadLabel(rd);
}

void OMFFormat::Segment::EntryRecord::WriteFile(const Segment& segment, Linker::Writer& wr) const
{
	Record::WriteFile(segment, wr);
	wr.WriteWord(2, segment_number);
	segment.WriteWord(wr, location);
	segment.WriteLabel(wr, name);
}

void OMFFormat::Segment::EntryRecord::AddFields(Dumper::Dumper& dump, Dumper::Region& region, const OMFFormat& omf, const Segment& segment, unsigned index, offset_t file_offset, offset_t address) const
{
	region.AddField("Segment name", Dumper::StringDisplay::Make(), name);
	region.AddField("Location", Dumper::SectionedDisplay<offset_t>::Make(Dumper::HexDisplay::Make(8)), offset_t(segment_number), offset_t(location));
}

offset_t OMFFormat::Segment::SuperCompactRecord::GetLength(const Segment& segment) const
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

void OMFFormat::Segment::SuperCompactRecord::WriteFile(const Segment& segment, Linker::Writer& wr) const
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

void OMFFormat::Segment::SuperCompactRecord::WritePatchList(Linker::Writer& wr, const std::vector<uint8_t>& patches) const
{
	wr.WriteWord(1, patches.size() - 1);
	for(uint8_t patch : patches)
	{
		wr.WriteWord(1, patch);
	}
}

void OMFFormat::Segment::SuperCompactRecord::Dump(Dumper::Dumper& dump, const OMFFormat& omf, const Segment& segment, unsigned index, offset_t file_offset, offset_t address) const
{
	Record::Dump(dump, omf, segment, index, file_offset, address);
	IntersegmentRelocationRecord relocation;
	for(uint32_t i = 0; ; i++)
	{
		if(!GetRelocation(relocation, i, segment))
			break;
		Dumper::Entry relocation_entry("Relocation", i + 1); /* TODO: offset */
		relocation_entry.AddField("Size", Dumper::DecDisplay::Make(), offset_t(relocation.size));
		relocation_entry.AddOptionalField("Shift (left)", Dumper::DecDisplay::Make(), offset_t(relocation.shift));
		relocation_entry.AddField("Source", Dumper::HexDisplay::Make(8), offset_t(relocation.source));
		if(super_type == SUPER_RELOC2 || super_type == SUPER_RELOC3)
			relocation_entry.AddField("Target", Dumper::HexDisplay::Make(8), offset_t(relocation.target));
		else if(relocation.file_number == 1)
			relocation_entry.AddField("Target", Dumper::SectionedDisplay<offset_t>::Make(Dumper::HexDisplay::Make(8)), offset_t(relocation.segment_number), offset_t(relocation.target));
		else
			relocation_entry.AddField("Target", Dumper::SectionedDisplay<offset_t, offset_t>::Make(Dumper::SectionedDisplay<offset_t>::Make(Dumper::HexDisplay::Make(8))), offset_t(relocation.file_number), offset_t(relocation.segment_number), offset_t(relocation.target));
		relocation_entry.AddOptionalField("Addend", Dumper::HexDisplay::Make(relocation.size * 2), offset_t(segment.ReadUnsigned(relocation.size, relocation.source)));
		relocation_entry.Display(dump);
	}
}

void OMFFormat::Segment::SuperCompactRecord::AddFields(Dumper::Dumper& dump, Dumper::Region& region, const OMFFormat& omf, const Segment& segment, unsigned index, offset_t file_offset, offset_t address) const
{
	static std::map<offset_t, std::string> type_descriptions =
	{
		{ SUPER_RELOC2, "SUPER RELOC2" },
		{ SUPER_RELOC3, "SUPER RELOC3" },
	};
	if(type_descriptions.find(SUPER_INTERSEG1) == type_descriptions.end())
	{
		for(unsigned i = SUPER_INTERSEG1; i <= SUPER_INTERSEG36; i++)
		{
			std::ostringstream oss;
			oss << "SUPER INTERSEG" << (i - SUPER_INTERSEG1 + 1);
			type_descriptions[i] = oss.str();
		}
	}
	region.AddField("Type", Dumper::ChoiceDisplay::Make(type_descriptions), offset_t(super_type));
}

void OMFFormat::Segment::SuperCompactRecord::AddSignals(Dumper::Block& block, offset_t current_segment_offset) const
{
	for(unsigned index = 0; ; index++)
	{
		IntersegmentRelocationRecord relocation;
		if(!GetRelocation(relocation, index))
			break;
		relocation.AddSignals(block, current_segment_offset);
	}
}

bool OMFFormat::Segment::SuperCompactRecord::GetRelocation(IntersegmentRelocationRecord& relocation, unsigned index) const
{
	if(index >= offsets.size())
		return false;

	if(super_type == SUPER_RELOC2)
	{
		relocation.size = 2;
		relocation.shift = 0;
		relocation.source = offsets[index];
		relocation.target = 0; // needs to be read from data
		relocation.file_number = 1;
		relocation.segment_number = 0;
		return true;
	}
	else if(super_type == SUPER_RELOC3)
	{
		relocation.size = 3;
		relocation.shift = 0;
		relocation.source = offsets[index];
		relocation.target = 0; // needs to be read from data
		relocation.file_number = 1;
		relocation.segment_number = 0;
		return true;
	}
	else if(SUPER_INTERSEG1 <= super_type && super_type < SUPER_INTERSEG13)
	{
		relocation.size = 3;
		relocation.shift = 0;
		relocation.source = offsets[index];
		relocation.target = 0; // needs to be read from data
		relocation.file_number = super_type - SUPER_INTERSEG1 + 1;
		relocation.segment_number = 0; // needs to be read from data
		return true;
	}
	else if(SUPER_INTERSEG13 <= super_type && super_type < SUPER_INTERSEG25)
	{
		relocation.size = 2;
		relocation.shift = 0;
		relocation.source = offsets[index];
		relocation.target = 0; // needs to be read from data
		relocation.file_number = 1;
		relocation.segment_number = super_type - SUPER_INTERSEG13 + 1;
		return true;
	}
	else if(SUPER_INTERSEG25 <= super_type && super_type <= SUPER_INTERSEG36)
	{
		relocation.size = 2;
		relocation.shift = -16;
		relocation.source = offsets[index];
		relocation.target = 0; // needs to be read from data
		relocation.file_number = 1;
		relocation.segment_number = super_type - SUPER_INTERSEG25 + 1;
		return true;
	}
	else
	{
		return false;
	}
}

bool OMFFormat::Segment::SuperCompactRecord::GetRelocation(IntersegmentRelocationRecord& relocation, unsigned index, const Segment& segment) const
{
	if(!GetRelocation(relocation, index))
		return false;

	if(super_type == SUPER_RELOC2)
	{
		relocation.target = segment.ReadUnsigned(2, relocation.source);
		return true;
	}
	else if(super_type == SUPER_RELOC3)
	{
		relocation.target = segment.ReadUnsigned(2, relocation.source);
		return true;
	}
	else if(SUPER_INTERSEG1 <= super_type && super_type < SUPER_INTERSEG13)
	{
		relocation.target = segment.ReadUnsigned(2, relocation.source);
		relocation.segment_number = segment.ReadUnsigned(1, relocation.source + 2);
		return true;
	}
	else if(SUPER_INTERSEG13 <= super_type && super_type < SUPER_INTERSEG25)
	{
		relocation.target = segment.ReadUnsigned(2, relocation.source);
		return true;
	}
	else if(SUPER_INTERSEG25 <= super_type && super_type <= SUPER_INTERSEG36)
	{
		relocation.target = segment.ReadUnsigned(2, relocation.source);
		return true;
	}
	else
	{
		return false;
	}
}

std::unique_ptr<OMFFormat::Segment::Expression> OMFFormat::Segment::ReadExpression(Linker::Reader& rd)
{
	// TODO
	return nullptr;
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::ReadRecord(Linker::Reader& rd)
{
	uint8_t type = rd.ReadUnsigned(1);
	std::unique_ptr<Record> record = nullptr;
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
			break;
		}
	default:
		if(Record::OPC_CONST_FIRST <= type && type <= Record::OPC_CONST_LAST)
		{
			record = makeCONST(type);
		}
		else
		{
			record = std::make_unique<DataRecord>(Record::record_type(type));
		}
	}
	// TODO: nullptr
	record->ReadFile(*this, rd);
	return record;
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makeEND()
{
	return std::make_unique<Record>(Record::OPC_END);
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makeCONST(std::shared_ptr<Linker::Image> image)
{
	size_t length = image->ImageSize();
	if(length >= Record::OPC_CONST_LAST)
	{
		return makeLCONST(image);
	}
	return makeCONST(image);
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makeCONST(size_t length)
{
	if(length >= Record::OPC_CONST_LAST)
	{
		length = Record::OPC_CONST_LAST;
	}
	return std::make_unique<DataRecord>(Record::record_type(Record::OPC_CONST_BASE - 1 + length));
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makeALIGN(offset_t align)
{
	return std::make_unique<ValueRecord>(Record::Record::OPC_ALIGN, align);
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makeORG(offset_t value)
{
	return std::make_unique<ValueRecord>(Record::OPC_ORG, value);
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makeRELOC(uint8_t size, uint8_t shift, offset_t source, offset_t target)
{
	return std::make_unique<RelocationRecord>(Record::OPC_RELOC, size, shift, source, target);
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makeRELOC()
{
	return std::make_unique<RelocationRecord>(Record::OPC_RELOC);
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makeINTERSEG(uint8_t size, uint8_t shift, offset_t source, uint16_t file_number, uint16_t segment_number, offset_t target)
{
	return std::make_unique<IntersegmentRelocationRecord>(Record::OPC_INTERSEG, size, shift, source, file_number, segment_number, target);
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makeINTERSEG()
{
	return std::make_unique<IntersegmentRelocationRecord>(Record::OPC_INTERSEG);
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makeUSING(std::string name)
{
	return std::make_unique<StringRecord>(Record::OPC_USING, name);
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makeSTRONG(std::string name)
{
	return std::make_unique<StringRecord>(Record::OPC_STRONG, name);
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makeGLOBAL()
{
	return std::make_unique<LabelRecord>(Record::OPC_GLOBAL);
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makeGLOBAL(std::string name, uint16_t line_length, int operation, uint16_t private_flag)
{
	return std::make_unique<LabelRecord>(Record::OPC_GLOBAL, name, line_length, operation, private_flag);
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makeGEQU()
{
	return std::make_unique<LabelExpressionRecord>(Record::OPC_GEQU);
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makeGEQU(std::string name, uint16_t line_length, int operation, uint16_t private_flag, std::unique_ptr<Expression> expression)
{
	return std::make_unique<LabelExpressionRecord>(Record::OPC_GEQU, name, line_length, operation, private_flag, std::move(expression));
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makeMEM()
{
	return std::make_unique<RangeRecord>(Record::Record::OPC_MEM);
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makeMEM(offset_t start, offset_t end)
{
	return std::make_unique<RangeRecord>(Record::Record::OPC_MEM, start, end);
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makeEXPR()
{
	return std::make_unique<ExpressionRecord>(Record::Record::OPC_EXPR);
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makeEXPR(uint8_t size, std::unique_ptr<Expression> expression)
{
	return std::make_unique<ExpressionRecord>(Record::Record::OPC_EXPR, size, std::move(expression));
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makeZEXPR()
{
	return std::make_unique<ExpressionRecord>(Record::Record::OPC_ZEXPR);
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makeZEXPR(uint8_t size, std::unique_ptr<Expression> expression)
{
	return std::make_unique<ExpressionRecord>(Record::Record::OPC_ZEXPR, size, std::move(expression));
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makeBEXPR()
{
	return std::make_unique<ExpressionRecord>(Record::Record::OPC_BEXPR);
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makeBEXPR(uint8_t size, std::unique_ptr<Expression> expression)
{
	return std::make_unique<ExpressionRecord>(Record::Record::OPC_BEXPR, size, std::move(expression));
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makeRELEXPR()
{
	return std::make_unique<RelativeExpressionRecord>(Record::Record::OPC_RELEXPR);
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makeRELEXPR(uint8_t size, offset_t origin, std::unique_ptr<Expression> expression)
{
	return std::make_unique<RelativeExpressionRecord>(Record::Record::OPC_RELEXPR, size, origin, std::move(expression));
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makeLOCAL()
{
	return std::make_unique<LabelRecord>(Record::OPC_LOCAL);
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makeLOCAL(std::string name, uint16_t line_length, int operation, uint16_t private_flag)
{
	return std::make_unique<LabelRecord>(Record::OPC_LOCAL, name, line_length, operation, private_flag);
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makeEQU()
{
	return std::make_unique<LabelExpressionRecord>(Record::OPC_EQU);
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makeEQU(std::string name, uint16_t line_length, int operation, uint16_t private_flag, std::unique_ptr<Expression> expression)
{
	return std::make_unique<LabelExpressionRecord>(Record::OPC_EQU, name, line_length, operation, private_flag, std::move(expression));
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makeDS(offset_t count)
{
	return std::make_unique<ValueRecord>(Record::OPC_DS, count);
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makeLCONST(std::shared_ptr<Linker::Image> image)
{
	return std::make_unique<DataRecord>(Record::OPC_LCONST, image);
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makeLCONST()
{
	return std::make_unique<DataRecord>(Record::OPC_LCONST);
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makeLEXPR()
{
	return std::make_unique<ExpressionRecord>(Record::Record::OPC_LEXPR);
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makeLEXPR(uint8_t size, std::unique_ptr<Expression> expression)
{
	return std::make_unique<ExpressionRecord>(Record::Record::OPC_LEXPR, size, std::move(expression));
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makeENTRY()
{
	return std::make_unique<EntryRecord>(Record::Record::OPC_ENTRY);
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makeENTRY(uint16_t segment_number, offset_t location, std::string name)
{
	return std::make_unique<EntryRecord>(Record::Record::OPC_ENTRY, segment_number, location, name);
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makecRELOC(uint8_t size, uint8_t shift, uint16_t source, uint16_t target)
{
	return std::make_unique<RelocationRecord>(Record::OPC_C_RELOC, size, shift, source, target);
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makecRELOC()
{
	return std::make_unique<RelocationRecord>(Record::OPC_C_RELOC);
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makecINTERSEG(uint8_t size, uint8_t shift, uint16_t source, uint16_t segment_number, uint16_t target)
{
	return std::make_unique<IntersegmentRelocationRecord>(Record::OPC_C_INTERSEG, size, shift, source, 1, segment_number, target);
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makecINTERSEG()
{
	return std::make_unique<IntersegmentRelocationRecord>(Record::OPC_C_INTERSEG);
}

std::unique_ptr<OMFFormat::Segment::Record> OMFFormat::Segment::makeSUPER(SuperCompactRecord::super_record_type super_type)
{
	return std::make_unique<SuperCompactRecord>(Record::OPC_SUPER, super_type);
}

