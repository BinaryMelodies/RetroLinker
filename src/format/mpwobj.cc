
#include "mpwobj.h"
#include "../linker/buffer.h"

using namespace MPW;

const std::map<offset_t, std::string> ObjectFormat::Record::record_type_names =
{
	{ Pad,               "Pad" },
	{ First,             "First" },
	{ Last,              "Last" },
	{ Comment,           "Comment" },
	{ Dictionary,        "Dictionary" },
	{ Module,            "Module" },
	{ EntryPoint,        "Entry Point" },
	{ Size,              "Size" },
	{ Contents,          "Contents" },
	{ Reference,         "Reference" },
	{ ComputedReference, "Computed Reference" },
	{ Filename,          "Filename" },
	{ SourceStatement,   "Source Statement" },
	{ ModuleBegin,       "Module Begin" },
	{ ModuleEnd,         "Module End" },
	{ BlockBegin,        "Block Begin" },
	{ BlockEnd,          "Block End" },
	{ LocalIdentifier,   "Local Identifier" },
	{ LocalLabel,        "Local Label" },
	{ LocalType,         "Local Type" },
};

std::shared_ptr<ObjectFormat::Record> ObjectFormat::Record::ReadFile(const std::shared_ptr<Linker::Reader>& rd)
{
	std::shared_ptr<Record> record;
	offset_t record_offset = rd->Tell();
	uint8_t type = rd->ReadUnsigned(1);
	Linker::Debug << "Debug: Record type " << int(type) << std::endl;
	switch(type)
	{
	case Pad:
		record = std::make_shared<Record>(record_type(type), record_offset);
		break;
	case First:
		record = std::make_shared<FirstRecord>(record_type(type), record_offset);
		break;
	case Last:
		record = std::make_shared<Record>(record_type(type), record_offset);
		break;
	case Comment:
		record = std::make_shared<CommentRecord>(record_type(type), record_offset);
		break;
	case Dictionary:
		record = std::make_shared<DictionaryRecord>(record_type(type), record_offset);
		break;
	case Module:
		record = std::make_shared<ModuleRecord>(record_type(type), record_offset);
		break;
	case EntryPoint:
		record = std::make_shared<EntryPointRecord>(record_type(type), record_offset);
		break;
	case Size:
		record = std::make_shared<SizeRecord>(record_type(type), record_offset);
		break;
	case Contents:
		record = std::make_shared<ContentsRecord>(record_type(type), record_offset);
		break;
	case Reference:
		record = std::make_shared<ReferenceRecord>(record_type(type), record_offset);
		break;
	case ComputedReference:
		record = std::make_shared<ReferenceRecord>(record_type(type), record_offset);
		break;
	case Filename:
		record = std::make_shared<FilenameRecord>(record_type(type), record_offset);
		break;
	case SourceStatement:
		record = std::make_shared<SourceStatementRecord>(record_type(type), record_offset);
		break;
	case ModuleBegin:
		record = std::make_shared<ModuleBeginRecord>(record_type(type), record_offset);
		break;
	case ModuleEnd:
		record = std::make_shared<ModuleEndRecord>(record_type(type), record_offset);
		break;
	case BlockBegin:
		record = std::make_shared<BlockBeginRecord>(record_type(type), record_offset);
		break;
	case BlockEnd:
		record = std::make_shared<BlockEndRecord>(record_type(type), record_offset);
		break;
	case LocalIdentifier:
		record = std::make_shared<LocalIdentifierRecord>(record_type(type), record_offset);
		break;
	case LocalLabel:
		record = std::make_shared<LocalLabelRecord>(record_type(type), record_offset);
		break;
	case LocalType:
		record = std::make_shared<LocalTypeRecord>(record_type(type), record_offset);
		break;
	default:
		record = std::make_shared<Record>(record_type(type), record_offset);
		break;
	}
	record->ReadBody(rd);
	return record;
}

void ObjectFormat::Record::ReadBody(const std::shared_ptr<Linker::Reader>& rd)
{
	switch(type)
	{
	case Pad:
		break;
	case Last:
		rd->Skip(1);
		break;
	default:
		Linker::Error << "Error: Unimplemented record type " << int(type) << std::endl;
		break;
	}
}

offset_t ObjectFormat::Record::ImageSize() const
{
	switch(type)
	{
	case Pad:
		return 1;
	case Last:
		return 2;
	default:
		return offset_t(-1);
	}
}

offset_t ObjectFormat::Record::WriteFile(const std::shared_ptr<Linker::Writer>& wr) const
{
	switch(type)
	{
	case Pad:
		wr->WriteWord(1, type);
		return 1;
	case Last:
		wr->WriteWord(1, type);
		wr->WriteWord(1, 0);
		return 2;
	default:
		return offset_t(-1);
	}
}

void ObjectFormat::Record::Dump(Dumper::Dumper& dump, offset_t record_index) const
{
	std::string record_name;
	switch(type)
	{
	case Pad:
		record_name = "Pad record";
		break;
	case Last:
		record_name = "Last record";
		break;
	default:
		record_name = "Unknown record";
		break;
	}

	Dumper::Region record_region(record_name, record_offset, ImageSize(), 8);
	record_region.InsertField(0, "Index", Dumper::DecDisplay::Make(), offset_t(record_index + 1));
	record_region.AddField("Record type", Dumper::ChoiceDisplay::Make(record_type_names, Dumper::DecDisplay::Make()), offset_t(type));
	if(record_name == "Unknown record")
	{
		record_region.AddField("Record type", Dumper::HexDisplay::Make(2), offset_t(type));
	}
	record_region.Display(dump, Dumper::Header);
}

void ObjectFormat::FirstRecord::ReadBody(const std::shared_ptr<Linker::Reader>& rd)
{
	flags = rd->ReadUnsigned(1);
	version = version_type(rd->ReadUnsigned(2));
}

offset_t ObjectFormat::FirstRecord::ImageSize() const
{
	return 4;
}

offset_t ObjectFormat::FirstRecord::WriteFile(const std::shared_ptr<Linker::Writer>& wr) const
{
	wr->WriteWord(1, type);
	wr->WriteWord(1, flags);
	wr->WriteWord(2, version);
	return ImageSize();
}

void ObjectFormat::FirstRecord::Dump(Dumper::Dumper& dump, offset_t record_index) const
{
	Dumper::Region record_region("First record", record_offset, ImageSize(), 8);
	record_region.InsertField(0, "Index", Dumper::DecDisplay::Make(), offset_t(record_index + 1));
	record_region.AddField("Record type", Dumper::ChoiceDisplay::Make(record_type_names, Dumper::DecDisplay::Make()), offset_t(type));
	record_region.AddField("Flags",
		Dumper::BitFieldDisplay::Make(2)
			->AddBitField(0, 1, Dumper::ChoiceDisplay::Make("nested"), true),
		offset_t(flags));
	static const std::map<offset_t, std::string> version_names =
	{
		{ offset_t(Nonsymbolic_2_0), "Nonsymbolic MPW 2.0 OMF file" },
		{ offset_t(Nonsymbolic_3_0), "Nonsymbolic MPW 3.0 OMF file" },
		{ offset_t(Symbolic),        "OMF file containing symbolic information" },
	};
	record_region.AddField("Version", Dumper::ChoiceDisplay::Make(version_names, Dumper::HexDisplay::Make(4)), offset_t(version));
	record_region.Display(dump, Dumper::Header);
}

void ObjectFormat::CommentRecord::ReadBody(const std::shared_ptr<Linker::Reader>& rd)
{
	rd->Skip(1);
	uint16_t record_size = rd->ReadUnsigned(2);
	if(record_size < 4)
	{
		Linker::Error << "Error: Invalid size for Comment record" << std::endl;
		return;
	}
	comments = Linker::Buffer::ReadFromFile(rd, record_size - 4);
}

offset_t ObjectFormat::CommentRecord::ImageSize() const
{
	return 4 + comments->ImageSize();
}

offset_t ObjectFormat::CommentRecord::WriteFile(const std::shared_ptr<Linker::Writer>& wr) const
{
	wr->WriteWord(1, type);
	wr->WriteWord(1, 0);
	wr->WriteWord(2, comments->ImageSize() + 4);
	return comments->WriteFile(wr) + 4;
}

void ObjectFormat::CommentRecord::Dump(Dumper::Dumper& dump, offset_t record_index) const
{
	Dumper::Block record_block("Comment record", record_offset + 4, comments->AsImage(), 0, 8);
	record_block.InsertField(0, "Index", Dumper::DecDisplay::Make(), offset_t(record_index + 1));
	record_block.AddField("Record type", Dumper::ChoiceDisplay::Make(record_type_names, Dumper::DecDisplay::Make()), offset_t(type));
	record_block.AddField("Record offset", Dumper::HexDisplay::Make(8), offset_t(record_offset));
	record_block.AddField("Record size", Dumper::HexDisplay::Make(8), offset_t(ImageSize()));
	record_block.Display(dump, Dumper::Header);
}

void ObjectFormat::DictionaryRecord::ReadBody(const std::shared_ptr<Linker::Reader>& rd)
{
	flags = rd->ReadUnsigned(1);
	uint16_t record_size = rd->ReadUnsigned(2);
	first_id = rd->ReadUnsigned(2);
	if(record_size < 6)
	{
		Linker::Error << "Error: Invalid size for Dictionary record" << std::endl;
		return;
	}
	record_size -= 6;
	while(record_size > 0)
	{
		uint8_t length = rd->ReadUnsigned(1);
		std::string name = rd->ReadData(std::max(offset_t(record_size - 1), offset_t(length)));
		record_size -= 1 + name.size();
		names.push_back(name);
	}
}

offset_t ObjectFormat::DictionaryRecord::ImageSize() const
{
	offset_t size = 6 + names.size();
	for(auto& name : names)
	{
		size += name.size();
	}
	return size;
}

offset_t ObjectFormat::DictionaryRecord::WriteFile(const std::shared_ptr<Linker::Writer>& wr) const
{
	offset_t size = 6 + names.size();
	wr->WriteWord(1, type);
	wr->WriteWord(1, flags);
	wr->WriteWord(2, ImageSize());
	wr->WriteWord(2, first_id);
	for(auto& name : names)
	{
		wr->WriteWord(1, name.size());
		wr->WriteData(name);
		size += name.size();
	}
	return size;
}

void ObjectFormat::DictionaryRecord::Dump(Dumper::Dumper& dump, offset_t record_index) const
{
	Dumper::Region record_region("Dictionary record", record_offset, ImageSize(), 8);
	record_region.InsertField(0, "Index", Dumper::DecDisplay::Make(), offset_t(record_index + 1));
	record_region.AddField("Record type", Dumper::ChoiceDisplay::Make(record_type_names, Dumper::DecDisplay::Make()), offset_t(type));
	record_region.AddField("Flags",
		Dumper::BitFieldDisplay::Make(2)
			->AddBitField(7, 1, Dumper::ChoiceDisplay::Make("Debug", "Link"), false),
		offset_t(flags));
	record_region.AddField("First ID", Dumper::HexDisplay::Make(4), offset_t(first_id));
	record_region.Display(dump, Dumper::Header);

	offset_t name_index = 0;
	offset_t name_offset = record_offset + 6;
	for(auto& name : names)
	{
		Dumper::Entry name_entry("Name", offset_t(name_index + 1), offset_t(name_offset), 8);
		name_entry.AddField("ID", Dumper::HexDisplay::Make(4), offset_t(first_id + name_index));
		name_entry.AddField("Text", Dumper::StringDisplay::Make("'"), name);
		name_entry.Display(dump, Dumper::Symbol);
		name_offset += 1 + name.size();
		name_index ++;
	}
}

void ObjectFormat::ModuleRecord::ReadBody(const std::shared_ptr<Linker::Reader>& rd)
{
	flags = rd->ReadUnsigned(1);
	module_id = rd->ReadUnsigned(2);
	segment.id = rd->ReadUnsigned(2);
}

offset_t ObjectFormat::ModuleRecord::ImageSize() const
{
	return 6;
}

offset_t ObjectFormat::ModuleRecord::WriteFile(const std::shared_ptr<Linker::Writer>& wr) const
{
	wr->WriteWord(1, type);
	wr->WriteWord(1, flags);
	wr->WriteWord(2, module_id);
	wr->WriteWord(2, segment.id);
	return 6;
}

void ObjectFormat::ModuleRecord::Dump(Dumper::Dumper& dump, offset_t record_index) const
{
	Dumper::Region record_region("Module record", record_offset, ImageSize(), 8);
	record_region.InsertField(0, "Index", Dumper::DecDisplay::Make(), offset_t(record_index + 1));
	record_region.AddField("Record type", Dumper::ChoiceDisplay::Make(record_type_names, Dumper::DecDisplay::Make()), offset_t(type));
	record_region.AddField("Flags",
		Dumper::BitFieldDisplay::Make(2)
			->AddBitField(0, 1, Dumper::ChoiceDisplay::Make("data", "code"), false)
			->AddBitField(3, 1, Dumper::ChoiceDisplay::Make("extern", "local"), false)
			->AddBitField(4, 1, Dumper::ChoiceDisplay::Make("main"), true)
			->AddBitField(7, 1, Dumper::ChoiceDisplay::Make("force active"), true),
		offset_t(flags));
	record_region.AddField("Module ID", Dumper::HexDisplay::Make(4), offset_t(module_id));
	record_region.AddField(flags & DataFlag ? "Segment size" : "Segment ID", Dumper::HexDisplay::Make(4), offset_t(segment.id));
	record_region.Display(dump, Dumper::Header);
}

void ObjectFormat::EntryPointRecord::ReadBody(const std::shared_ptr<Linker::Reader>& rd)
{
	flags = rd->ReadUnsigned(1);
	entry_id = rd->ReadUnsigned(2);
	offset = rd->ReadUnsigned(4);
	// TODO
	Linker::Error << "Error: Unimplemented record type " << int(type) << std::endl;
}

offset_t ObjectFormat::EntryPointRecord::ImageSize() const
{
	// TODO
	return offset_t(-1);
}

offset_t ObjectFormat::EntryPointRecord::WriteFile(const std::shared_ptr<Linker::Writer>& wr) const
{
	// TODO
	return offset_t(-1);
}

void ObjectFormat::EntryPointRecord::Dump(Dumper::Dumper& dump, offset_t record_index) const
{
	// TODO
}

void ObjectFormat::SizeRecord::ReadBody(const std::shared_ptr<Linker::Reader>& rd)
{
	flags = rd->ReadUnsigned(1);
	module_size = rd->ReadUnsigned(4);
	// TODO
	Linker::Error << "Error: Unimplemented record type " << int(type) << std::endl;
}

offset_t ObjectFormat::SizeRecord::ImageSize() const
{
	// TODO
	return offset_t(-1);
}

offset_t ObjectFormat::SizeRecord::WriteFile(const std::shared_ptr<Linker::Writer>& wr) const
{
	// TODO
	return offset_t(-1);
}

void ObjectFormat::SizeRecord::Dump(Dumper::Dumper& dump, offset_t record_index) const
{
	// TODO
}

void ObjectFormat::ContentsRecord::ReadBody(const std::shared_ptr<Linker::Reader>& rd)
{
	flags = rd->ReadUnsigned(1);
	uint16_t record_size = rd->ReadUnsigned(2);
	uint16_t record_bytes = 4;
	if(flags & PartialFlag)
	{
		offset = rd->ReadUnsigned(4);
		record_bytes += 4;
	}
	if(flags & RepeatFlag)
	{
		repeat = rd->ReadUnsigned(2);
		record_bytes += 2;
	}
	if(record_size < record_bytes)
	{
		Linker::Error << "Error: Invalid size for Contents record" << std::endl;
		return;
	}
	contents = Linker::Buffer::ReadFromFile(rd, record_size - record_bytes);
}

offset_t ObjectFormat::ContentsRecord::ImageSize() const
{
	offset_t size = 4 + contents->ImageSize();
	if(flags & PartialFlag)
	{
		size += 4;
	}
	if(flags & RepeatFlag)
	{
		size += 2;
	}
	return size;
}

offset_t ObjectFormat::ContentsRecord::WriteFile(const std::shared_ptr<Linker::Writer>& wr) const
{
	offset_t size = 4;
	wr->WriteWord(1, type);
	wr->WriteWord(1, flags);
	wr->WriteWord(2, ImageSize());
	if(flags & PartialFlag)
	{
		wr->WriteWord(4, offset);
		size += 8;
	}
	if(flags & RepeatFlag)
	{
		wr->WriteWord(2, repeat);
		size += 2;
	}
	return contents->WriteFile(wr) + size;
}

void ObjectFormat::ContentsRecord::Dump(Dumper::Dumper& dump, offset_t record_index) const
{
	uint16_t contents_offset = 4;
	if(flags & PartialFlag)
	{
		contents_offset += 4;
	}
	if(flags & RepeatFlag)
	{
		contents_offset += 2;
	}

	Dumper::Block record_block("Contents", record_offset + contents_offset, contents->AsImage(), offset_t(offset), 8);
	record_block.InsertField(0, "Index", Dumper::DecDisplay::Make(), offset_t(record_index + 1));
	record_block.AddField("Record type", Dumper::ChoiceDisplay::Make(record_type_names, Dumper::DecDisplay::Make()), offset_t(type));
	record_block.AddField("Record offset", Dumper::HexDisplay::Make(8), offset_t(record_offset));
	record_block.AddField("Record size", Dumper::HexDisplay::Make(8), offset_t(ImageSize()));
	record_block.AddField("Flags",
		Dumper::BitFieldDisplay::Make(2)
			->AddBitField(0, 1, Dumper::ChoiceDisplay::Make("data", "code"), false)
			->AddBitField(3, 1, Dumper::ChoiceDisplay::Make("partial", "complete"), false)
			->AddBitField(4, 1, Dumper::ChoiceDisplay::Make("repeat"), true),
		offset_t(flags));
	record_block.AddOptionalField("Offset in module", Dumper::HexDisplay::Make(8), offset_t(offset));
	record_block.AddOptionalField("Repeat", Dumper::DecDisplay::Make(), offset_t(repeat));
	record_block.Display(dump, Dumper::Header);

	// TODO: repeat
	record_block.Display(dump, Dumper::Header);
}

void ObjectFormat::ReferenceRecord::ReadBody(const std::shared_ptr<Linker::Reader>& rd)
{
	flags = rd->ReadUnsigned(1);
	uint16_t record_size = rd->ReadUnsigned(2);
	id = rd->ReadUnsigned(2);
	uint16_t record_bytes = 6;
	if(type == ComputedReference)
	{
		id2 = rd->ReadUnsigned(2);
		record_bytes += 2;
	}
	if(record_size < record_bytes)
	{
		if(type == ComputedReference)
			Linker::Error << "Error: Invalid size for Computed Reference record" << std::endl;
		else
			Linker::Error << "Error: Invalid size for Reference record" << std::endl;
		return;
	}
	record_size -= record_bytes;
	size_t offset_size = flags & LongFlag ? 4 : 2;
	while(record_size >= offset_size)
	{
		offsets.push_back(rd->ReadUnsigned(offset_size));
		record_size -= offset_size;
	}
	if(record_size)
	{
		Linker::Warning << "Warning: Remaining bytes in record, skipping" << std::endl;
		rd->Skip(record_size);
	}
}

offset_t ObjectFormat::ReferenceRecord::ImageSize() const
{
	return (type == ComputedReference ? 8 : 6) + (flags & LongFlag ? 4 : 2) * offsets.size();
}

offset_t ObjectFormat::ReferenceRecord::WriteFile(const std::shared_ptr<Linker::Writer>& wr) const
{
	wr->WriteWord(1, type);
	wr->WriteWord(1, flags);
	wr->WriteWord(2, ImageSize());
	wr->WriteWord(2, id);
	if(type == ComputedReference)
	{
		wr->WriteWord(1, id2);
	}
	size_t offset_size = flags & LongFlag ? 4 : 2;
	for(auto offset : offsets)
	{
		wr->WriteWord(offset_size, offset);
	}
	return ImageSize();
}

void ObjectFormat::ReferenceRecord::Dump(Dumper::Dumper& dump, offset_t record_index) const
{
	std::string record_name;
	switch(type)
	{
	case Reference:
		record_name = "Reference record";
		break;
	case ComputedReference:
		record_name = "Computed Reference record";
		break;
	default:
		record_name = "Unknown record";
		break;
	}

	Dumper::Region record_region(record_name, record_offset, ImageSize(), 8);
	record_region.InsertField(0, "Index", Dumper::DecDisplay::Make(), offset_t(record_index + 1));
	record_region.AddField("Record type", Dumper::ChoiceDisplay::Make(record_type_names, Dumper::DecDisplay::Make()), offset_t(type));
	static const std::map<offset_t, std::string> reference_size_names =
	{
		{ 0, "32-bit" },
		{ 1, "16-bit" },
		{ 2, "8-bit" },
	};
	record_region.AddField("Flags",
		Dumper::BitFieldDisplay::Make(2)
			->AddBitField(0, 1, Dumper::ChoiceDisplay::Make("data", "code"), false)
			->AddBitField(3, 1, Dumper::ChoiceDisplay::Make("long", "short"), false)
			->AddBitField(4, type == ComputedReference ? 2 : 1, "location size", Dumper::ChoiceDisplay::Make(reference_size_names), false)
			->AddBitField(7, 1, Dumper::ChoiceDisplay::Make(type == ComputedReference ? "difference" : "A5-relative"), true),
		offset_t(flags));
	record_region.AddField("Reference ID", Dumper::HexDisplay::Make(4), offset_t(id));
	if(type == ComputedReference)
	{
		record_region.AddField("Frame ID", Dumper::HexDisplay::Make(4), offset_t(id2));
	}
	record_region.Display(dump, Dumper::Header);
	offset_t offset_index = 0;
	offset_t offset_offset = record_offset + (type == ComputedReference ? 8 : 6);
	for(auto offset : offsets)
	{
		Dumper::Entry offset_entry("Offset", offset_t(offset_index + 1), offset_t(offset_offset), 8);
		offset_entry.AddField("Offset", Dumper::HexDisplay::Make(flags & LongFlag ? 8 : 4), offset_t(offset));
		offset_entry.Display(dump, Dumper::Relocation);
		offset_offset += flags & LongFlag ? 4 : 2;
		offset_index ++;
	}
}

void ObjectFormat::FilenameRecord::ReadBody(const std::shared_ptr<Linker::Reader>& rd)
{
	rd->Skip(1);
	file_id = rd->ReadUnsigned(2);
	modification = rd->ReadTimestamp<Macintosh_clock>();
}

offset_t ObjectFormat::FilenameRecord::ImageSize() const
{
	return 6;
}

offset_t ObjectFormat::FilenameRecord::WriteFile(const std::shared_ptr<Linker::Writer>& wr) const
{
	wr->WriteWord(1, type);
	wr->WriteWord(1, 0);
	wr->WriteWord(2, file_id);
	wr->WriteTimestamp(modification);
	return 6;
}

void ObjectFormat::FilenameRecord::Dump(Dumper::Dumper& dump, offset_t record_index) const
{
	Dumper::Region record_region("Filename record", record_offset, ImageSize(), 8);
	record_region.InsertField(0, "Index", Dumper::DecDisplay::Make(), offset_t(record_index + 1));
	record_region.AddField("Record type", Dumper::ChoiceDisplay::Make(record_type_names, Dumper::DecDisplay::Make()), offset_t(type));
	record_region.AddField("File ID", Dumper::HexDisplay::Make(4), offset_t(file_id));
	record_region.AddField("Modification", Dumper::TimestampDisplay<Macintosh_clock>::Make(), modification);
	record_region.Display(dump, Dumper::Header);
}

void ObjectFormat::SourceStatementRecord::ReadBody(const std::shared_ptr<Linker::Reader>& rd)
{
	rd->ReadUnsigned(1);
	uint16_t record_size = rd->ReadUnsigned(2);
	// TODO
	Linker::Error << "Error: Unimplemented record type " << int(type) << std::endl;
	rd->Skip(record_size - 4);
}

offset_t ObjectFormat::SourceStatementRecord::ImageSize() const
{
	// TODO
	return offset_t(-1);
}

offset_t ObjectFormat::SourceStatementRecord::WriteFile(const std::shared_ptr<Linker::Writer>& wr) const
{
	// TODO
	return offset_t(-1);
}

void ObjectFormat::SourceStatementRecord::Dump(Dumper::Dumper& dump, offset_t record_index) const
{
	// TODO
}

void ObjectFormat::ModuleBeginRecord::ReadBody(const std::shared_ptr<Linker::Reader>& rd)
{
	flags = rd->ReadUnsigned(1);
	/*uint16_t record_size =*/ rd->ReadUnsigned(2);
	module_id = rd->ReadUnsigned(2);
	parent_id = rd->ReadUnsigned(2);
	file_id = rd->ReadUnsigned(2);
	file_offset = rd->ReadUnsigned(flags & FileOffset16Flag ? 2 : 4);
	module_kind = module_kind_type(rd->ReadUnsigned(1));
	rd->Skip(1);
}

offset_t ObjectFormat::ModuleBeginRecord::ImageSize() const
{
	return flags & FileOffset16Flag ? 14 : 16;
}

offset_t ObjectFormat::ModuleBeginRecord::WriteFile(const std::shared_ptr<Linker::Writer>& wr) const
{
	wr->WriteWord(1, type);
	wr->WriteWord(1, flags);
	wr->WriteWord(2, ImageSize());
	wr->WriteWord(2, module_id);
	wr->WriteWord(2, parent_id);
	wr->WriteWord(2, file_id);
	wr->WriteWord(flags & FileOffset16Flag ? 2 : 4, file_offset);
	wr->WriteWord(1, module_kind);
	wr->WriteWord(1, 0);
	return ImageSize();
}

void ObjectFormat::ModuleBeginRecord::Dump(Dumper::Dumper& dump, offset_t record_index) const
{
	Dumper::Region record_region("Module Begin record", record_offset, ImageSize(), 8);
	record_region.InsertField(0, "Index", Dumper::DecDisplay::Make(), offset_t(record_index + 1));
	record_region.AddField("Record type", Dumper::ChoiceDisplay::Make(record_type_names, Dumper::DecDisplay::Make()), offset_t(type));
	record_region.AddField("Flags",
		Dumper::BitFieldDisplay::Make(2)
			->AddBitField(2, 1, Dumper::ChoiceDisplay::Make("declaration"), true)
			->AddBitField(7, 1, "file offset size", Dumper::ChoiceDisplay::Make("16-bit", "32-bit"), false),
		offset_t(flags));
	record_region.AddField("Module ID", Dumper::HexDisplay::Make(4), offset_t(module_id));
	record_region.AddField("Parent ID", Dumper::HexDisplay::Make(4), offset_t(parent_id));
	record_region.AddField("File ID", Dumper::HexDisplay::Make(4), offset_t(file_id));
	record_region.AddField("File offset", Dumper::HexDisplay::Make(flags & FileOffset16Flag ? 4 : 8), offset_t(file_offset));
	static const std::map<offset_t, std::string> module_kind_names =
	{
		{ None,       "none" },
		{ Unit,       "unit" },
		{ Procedure,  "procedure" },
		{ Function,   "function" },
		{ DataModule, "data module" },
	};
	record_region.AddField("Module kind", Dumper::ChoiceDisplay::Make(module_kind_names, Dumper::HexDisplay::Make(4)), offset_t(module_kind));
	record_region.Display(dump, Dumper::Header);
}

void ObjectFormat::ModuleEndRecord::ReadBody(const std::shared_ptr<Linker::Reader>& rd)
{
	flags = rd->ReadUnsigned(1);
	module_id = rd->ReadUnsigned(2);
	file_offset = rd->ReadUnsigned(4);
	// TODO
	Linker::Error << "Error: Unimplemented record type " << int(type) << std::endl;
}

offset_t ObjectFormat::ModuleEndRecord::ImageSize() const
{
	// TODO
	return offset_t(-1);
}

offset_t ObjectFormat::ModuleEndRecord::WriteFile(const std::shared_ptr<Linker::Writer>& wr) const
{
	// TODO
	return offset_t(-1);
}

void ObjectFormat::ModuleEndRecord::Dump(Dumper::Dumper& dump, offset_t record_index) const
{
	// TODO
}

void ObjectFormat::BlockBeginRecord::ReadBody(const std::shared_ptr<Linker::Reader>& rd)
{
	rd->ReadUnsigned(1);
	uint16_t record_size = rd->ReadUnsigned(2);
	// TODO
	Linker::Error << "Error: Unimplemented record type " << int(type) << std::endl;
	rd->Skip(record_size - 4);
}

offset_t ObjectFormat::BlockBeginRecord::ImageSize() const
{
	// TODO
	return offset_t(-1);
}

offset_t ObjectFormat::BlockBeginRecord::WriteFile(const std::shared_ptr<Linker::Writer>& wr) const
{
	// TODO
	return offset_t(-1);
}

void ObjectFormat::BlockBeginRecord::Dump(Dumper::Dumper& dump, offset_t record_index) const
{
	// TODO
}

void ObjectFormat::BlockEndRecord::ReadBody(const std::shared_ptr<Linker::Reader>& rd)
{
	rd->Skip(1);
	block_id = rd->ReadUnsigned(2);
	file_offset = rd->ReadUnsigned(4);
	code_offset = rd->ReadUnsigned(4);
	// TODO
	Linker::Error << "Error: Unimplemented record type " << int(type) << std::endl;
}

offset_t ObjectFormat::BlockEndRecord::ImageSize() const
{
	// TODO
	return offset_t(-1);
}

offset_t ObjectFormat::BlockEndRecord::WriteFile(const std::shared_ptr<Linker::Writer>& wr) const
{
	// TODO
	return offset_t(-1);
}

void ObjectFormat::BlockEndRecord::Dump(Dumper::Dumper& dump, offset_t record_index) const
{
	// TODO
}

void ObjectFormat::LocalIdentifierRecord::ReadBody(const std::shared_ptr<Linker::Reader>& rd)
{
	flags = rd->ReadUnsigned(1);
	uint16_t record_size = rd->ReadUnsigned(2);
	parent_id = rd->ReadUnsigned(2);
	file_id = rd->ReadUnsigned(2);
	file_offset = rd->ReadUnsigned(flags & FileOffset16Flag ? 2 : 4);
	if(record_size < (flags & FileOffset16Flag ? 10 : 12))
	{
		Linker::Error << "Error: Invalid size for Comment record" << std::endl;
		return;
	}
	record_size -= (flags & FileOffset16Flag ? 10 : 12);
	while(record_size > 0)
	{
		Identifier identifier;
		identifier.id = rd->ReadUnsigned(2);
		identifier.type = rd->ReadUnsigned(2);
		identifier.file_delta = rd->ReadUnsigned(1);
//		std::cerr << "[" << std::hex << int(identifier.file_delta) << "]" << std::endl;
		uint8_t kind = rd->ReadUnsigned(1);
		identifier.storage_class = Identifier::storage_class_type(kind & 0xF);
		identifier.value = (kind & 0x10) != 0;
		identifier.reference = (kind & 0x20) != 0;
		identifier.offset_size = Identifier::offset_size_type(kind >> 6);
		record_size -= 6;
		switch(identifier.offset_size)
		{
		case Identifier::NoOffset:
			break;
		case Identifier::Offset16:
			rd->ReadData(2, identifier.offset_data);
			record_size -= 2;
			break;
		case Identifier::Offset32:
			rd->ReadData(4, identifier.offset_data);
			record_size -= 4;
			break;
		case Identifier::VariableOffset:
			{
				uint16_t size = rd->ReadUnsigned(2);
				rd->ReadData(size, identifier.offset_data);
				record_size -= size;
			}
			break;
		}
		identifiers.push_back(identifier);
	}
}

offset_t ObjectFormat::LocalIdentifierRecord::ImageSize() const
{
	offset_t size = flags & FileOffset16Flag ? 10 : 12;
	for(auto& identifier : identifiers)
	{
		size += 6 + identifier.offset_data.size();
		if(identifier.offset_size == Identifier::VariableOffset)
		{
			size += 2;
		}
	}
	return size;
}

offset_t ObjectFormat::LocalIdentifierRecord::WriteFile(const std::shared_ptr<Linker::Writer>& wr) const
{
	wr->WriteWord(1, type);
	wr->WriteWord(1, flags);
	wr->WriteWord(2, ImageSize());
	wr->WriteWord(2, parent_id);
	wr->WriteWord(2, file_id);
	wr->WriteWord(flags & FileOffset16Flag ? 2 : 4, file_offset);
	for(auto& identifier : identifiers)
	{
		wr->WriteWord(2, identifier.id);
		wr->WriteWord(2, identifier.type);
		wr->WriteWord(1, identifier.file_delta);
		uint8_t kind =
			identifier.storage_class
			| (identifier.value ? 0x10 : 0)
			| (identifier.reference ? 0x20 : 0)
			| (identifier.offset_size << 6);
		wr->WriteWord(1, kind);
		if(identifier.offset_size == Identifier::VariableOffset)
		{
			wr->WriteWord(2, identifier.offset_data.size());
		}
		wr->WriteData(identifier.offset_data);
	}
	return ImageSize();
}

void ObjectFormat::LocalIdentifierRecord::Dump(Dumper::Dumper& dump, offset_t record_index) const
{
	Dumper::Region record_region("Local Identifier record", record_offset, ImageSize(), 8);
	record_region.InsertField(0, "Index", Dumper::DecDisplay::Make(), offset_t(record_index + 1));
	record_region.AddField("Record type", Dumper::ChoiceDisplay::Make(record_type_names, Dumper::DecDisplay::Make()), offset_t(type));
	record_region.AddField("Flags",
		Dumper::BitFieldDisplay::Make(2)
			->AddBitField(7, 1, "file offset size", Dumper::ChoiceDisplay::Make("16-bit", "32-bit"), false),
		offset_t(flags));
	record_region.AddField("Parent ID", Dumper::HexDisplay::Make(4), offset_t(parent_id));
	record_region.AddField("File ID", Dumper::HexDisplay::Make(4), offset_t(file_id));
	record_region.AddField("File offset", Dumper::HexDisplay::Make(flags & FileOffset16Flag ? 4 : 8), offset_t(file_offset));
	record_region.Display(dump, Dumper::Header);

	offset_t identifier_index = 0;
	offset_t identifier_offset = record_offset + (flags & FileOffset16Flag ? 10 : 12);
	for(auto& identifier : identifiers)
	{
		Dumper::Entry identifier_entry("Identifier", offset_t(identifier_index + 1), offset_t(identifier_offset), 8);
		identifier_entry.AddField("ID", Dumper::HexDisplay::Make(4), offset_t(identifier.id));
		identifier_entry.AddField("Type", Dumper::HexDisplay::Make(4), offset_t(identifier.type));
		identifier_entry.AddOptionalField("File delta", Dumper::HexDisplay::Make(2), offset_t(identifier.file_delta));
		static const std::map<offset_t, std::string> storage_class_names =
		{
			{ Identifier::Register,   "register" },
			{ Identifier::A5Relative, "A5-relative" },
			{ Identifier::A6Relative, "A6-relative" },
			{ Identifier::A7Relative, "A7-relative" },
			{ Identifier::Absolute,   "absolute" },
			{ Identifier::Constant,   "constant" },
		};
		static const std::map<offset_t, std::string> offset_size_names =
		{
			{ Identifier::NoOffset,       "no offset" },
			{ Identifier::Offset16,       "2-byte" },
			{ Identifier::Offset32,       "4-byte" },
			{ Identifier::VariableOffset, "variable size" },
		};
		identifier_entry.AddField("Kind",
			Dumper::BitFieldDisplay::Make(4)
				->AddBitField(0, 4, "storage class", Dumper::ChoiceDisplay::Make(storage_class_names, Dumper::HexDisplay::Make(1)), false)
				->AddBitField(4, 1, Dumper::ChoiceDisplay::Make("value"), true)
				->AddBitField(5, 1, Dumper::ChoiceDisplay::Make("reference"), true)
				->AddBitField(6, 2, "offset size", Dumper::ChoiceDisplay::Make(offset_size_names), true),
			offset_t(identifier.storage_class
				| (identifier.value ? 0x10 : 0)
				| (identifier.reference ? 0x20 : 0)
				| (identifier.offset_size << 6)));
		identifier_entry.AddField("Offset data", Dumper::BlobDisplay::Make(identifier.offset_data.size(), Dumper::StringDisplay::Make("'")), identifier.offset_data);
		identifier_entry.Display(dump, Dumper::Symbol);

		identifier_offset += 6 + identifier.offset_data.size();
		if(identifier.offset_size == Identifier::VariableOffset)
		{
			identifier_offset += 2;
		}
		identifier_index ++;
	}
}

void ObjectFormat::LocalLabelRecord::ReadBody(const std::shared_ptr<Linker::Reader>& rd)
{
	rd->ReadUnsigned(1);
	uint16_t record_size = rd->ReadUnsigned(2);
	// TODO
	Linker::Error << "Error: Unimplemented record type " << int(type) << std::endl;
	rd->Skip(record_size - 4);
}

offset_t ObjectFormat::LocalLabelRecord::ImageSize() const
{
	// TODO
	return offset_t(-1);
}

offset_t ObjectFormat::LocalLabelRecord::WriteFile(const std::shared_ptr<Linker::Writer>& wr) const
{
	// TODO
	return offset_t(-1);
}

void ObjectFormat::LocalLabelRecord::Dump(Dumper::Dumper& dump, offset_t record_index) const
{
	// TODO
}

void ObjectFormat::LocalTypeRecord::ReadBody(const std::shared_ptr<Linker::Reader>& rd)
{
	rd->ReadUnsigned(1);
	uint16_t record_size = rd->ReadUnsigned(2);
	// TODO
	Linker::Error << "Error: Unimplemented record type " << int(type) << std::endl;
	rd->Skip(record_size - 4);
}

offset_t ObjectFormat::LocalTypeRecord::ImageSize() const
{
	// TODO
	return offset_t(-1);
}

offset_t ObjectFormat::LocalTypeRecord::WriteFile(const std::shared_ptr<Linker::Writer>& wr) const
{
	// TODO
	return offset_t(-1);
}

void ObjectFormat::LocalTypeRecord::Dump(Dumper::Dumper& dump, offset_t record_index) const
{
	// TODO
}

void ObjectFormat::ReadFile(const std::shared_ptr<Linker::Reader>& rd)
{
	rd->endiantype = ::BigEndian;

	file_offset = rd->Tell();
	offset_t image_end = rd->GetImageEnd();

	while(rd->Tell() < image_end)
	{
		records.push_back(Record::ReadFile(rd));
	}
}

offset_t ObjectFormat::ImageSize() const
{
	offset_t size = 0;
	for(auto record : records)
	{
		size += record->ImageSize();
	}
	return size;
}

offset_t ObjectFormat::WriteFile(const std::shared_ptr<Linker::Writer>& wr) const
{
	offset_t size = 0;
	for(auto record : records)
	{
		size += record->WriteFile(wr);
	}
	return size;
}

void ObjectFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_macroman);

	dump.SetTitle("Apple MPW Object Format format");
	Dumper::Region file_region("File", file_offset, ImageSize(), 8);
	file_region.Display(dump, Dumper::Header);

	size_t record_index = 0;
	for(auto record : records)
	{
		record->Dump(dump, record_index);
		record_index++;
	}
}

