
#include "dumper.h"

using namespace Dumper;

bool ChoiceDisplay::IsMissing(std::tuple<offset_t>& values)
{
	if(missing_on_value)
	{
		return std::get<0>(values) == missing_value;
	}
	else
	{
		auto it = names.find(std::get<0>(values));
		return it == names.end();
	}
}

void ChoiceDisplay::DisplayValue(Dumper& dump, std::tuple<offset_t> values)
{
	auto it = names.find(std::get<0>(values));
	if(it == names.end())
	{
		dump.out << default_name;
	}
	else
	{
		dump.out << it->second;
	}
}

void HexDisplay::DisplayValue(Dumper& dump, std::tuple<offset_t> values)
{
	dump.PrintHex(std::get<0>(values), width);
}

void DecDisplay::DisplayValue(Dumper& dump, std::tuple<offset_t> values)
{
	dump.PrintDec(std::get<0>(values));
	dump.out << suffix;
}

void SegmentedDisplay::DisplayValue(Dumper& dump, std::tuple<offset_t, offset_t> values)
{
	dump.PrintHex(std::get<0>(values), 4);
	dump.out << ':';
	dump.PrintHex(std::get<1>(values), width);
}

void VersionDisplay::DisplayValue(Dumper& dump, std::tuple<offset_t, offset_t> values)
{
	dump.PrintDec(std::get<0>(values), "");
	dump.out << separator;
	dump.PrintDec(std::get<1>(values), "");
}

bool StringDisplay::IsMissing(std::tuple<std::string>& values)
{
	if(width == offset_t(-1))
	{
		for(offset_t i = 0; i < width; i++)
		{
			if(std::get<0>(values)[i] == '\0')
				return true;
			else if(std::get<0>(values)[i] != ' ')
				return false;
		}
		return true;
	}
	else
	{
		for(offset_t i = 0; i < width; i++)
		{
			if(std::get<0>(values)[i] != '\0' && std::get<0>(values)[i] != ' ')
				return false;
		}
		return true;
	}
}

void StringDisplay::DisplayValue(Dumper& dump, std::tuple<std::string> values)
{
	dump.out << open_quote;
	if(width == offset_t(-1))
	{
		for(offset_t i = 0; i < std::get<0>(values).size() && std::get<0>(values)[i] != '\0'; i++)
		{
			dump.PutChar((*dump.encoding)[std::get<0>(values)[i] & 0xFF]);
		}
	}
	else
	{
		offset_t i;
		for(i = 0; i < width; i++)
		{
			if(i >= std::get<0>(values).size())
				break;
			dump.PutChar((*dump.encoding)[std::get<0>(values)[i] & 0xFF]);
		}
		for(; i < width; i++)
		{
			dump.PutChar((*dump.encoding)[0]);
		}
	}
	dump.out << close_quote;
}

bool StringDisplay::IsMissing(std::tuple<offset_t>& values)
{
	char * text = reinterpret_cast<char *>(std::get<0>(values));
	if(width == offset_t(-1))
	{
		for(offset_t i = 0; i < width; i++)
		{
			if(text[i] == '\0')
				return true;
			else if(text[i] != ' ')
				return false;
		}
		return true;
	}
	else
	{
		for(offset_t i = 0; i < width; i++)
		{
			if(text[i] != '\0' && text[i] != ' ')
				return false;
		}
		return true;
	}
}

void StringDisplay::DisplayValue(Dumper& dump, std::tuple<offset_t> values)
{
	char * text = reinterpret_cast<char *>(std::get<0>(values));
	dump.out << open_quote;
	for(offset_t i = 0; width == offset_t(-1) ? text[i] != '\0' : i < width; i++)
	{
		dump.PutChar((*dump.encoding)[text[i] & 0xFF]);
	}
	dump.out << close_quote;
}

BitFieldDisplay::~BitFieldDisplay()
{
	for(auto it : bitfields)
	{
		delete it.second;
	}
}

void BitFieldDisplay::DisplayValue(Dumper& dump, std::tuple<offset_t> values)
{
	HexDisplay::DisplayValue(dump, values);
	for(auto it : bitfields)
	{
		std::tuple<offset_t> bit_value;
		std::get<0>(bit_value) = (std::get<0>(values) >> it.second->offset) & ((1 << it.second->length) - 1);
		if(it.second->ShouldDisplay(bit_value))
		{
			dump.out << ", ";
			it.second->display->DisplayValue(dump, bit_value);
		}
	}
}

Field::~Field()
{
}

Container::~Container()
{
	for(auto field : fields)
	{
		delete field;
	}
}

void Container::Display(Dumper& dump)
{
	dump.out << "== " << name;
	Field * number_field;
	if((number_field = FindField("number")) && number_field->internal)
	{
		dump.out << " ";
		number_field->DisplayValue(dump);
	}
	dump.out << std::endl;
	for(auto field : fields)
	{
		if(field->ShouldDisplay())
		{
			dump.out << "\t";
			dump.out << field->label << ":\t";
			field->DisplayValue(dump);
			dump.out << std::endl;
		}
	}
}

/*void Region::Display(Dumper& dump)
{
	dump.out << "== " << name;
	Field * number_field;
	if((number_field = FindField("number")) && number_field->internal)
	{
		dump.out << " ";
		number_field->display->DisplayValue(dump, number_field->values);
	}
	dump.out << std::endl;
	for(auto field : fields)
	{
		if(field->ShouldDisplay())
		{
			dump.out << "\t";
			dump.out << field->label << ":\t";
			field->DisplayValue(dump);
			dump.out << std::endl;
		}
	}
}*/

void Entry::Display(Dumper& dump)
{
	dump.out << "* ";
	if(offset != offset_t(-1))
	{
		dump.out << "[";
		dump.PrintHex(offset, display_width);
		dump.out << "] ";
	}
	dump.out << name << " ";
	dump.PrintDec(number);
	bool started = false;
	for(auto field : fields)
	{
		if(field->ShouldDisplay())
		{
			if(started)
				dump.out << ", ";
			else
				dump.out << "\t";
			started = true;
			dump.out << field->label << ": ";
			field->DisplayValue(dump);
		}
	}
	dump.out << std::endl;
}

void Block::Display(Dumper& dump)
{
	Region::Display(dump);
	if(!image || image->ActualDataSize() == 0)
		return;
	size_t block_offset = GetField<offset_t>("Offset", 0);
	size_t block_address = GetField<offset_t>("Address", 0);
	size_t image_offset = block_address & 0xF;
	bool last_underlined = false;

	if(block_offset != 0)
	{
		dump.out << "[";
		for(unsigned i = 0; i < offset_display_width; i++)
		{
			dump.out << (i < sizeof "FILE" - 1 ? "FILE"[i] : ' ');
		}
		dump.out << "]\t";
	}
	if(block_address != 0)
	{
		dump.out << "(";
		for(unsigned i = 0; i < position_display_width; i++)
		{
			dump.out << (i < sizeof "SEGMENT" - 1 ? "SEGMENT"[i] : ' ');
		}
		dump.out << ")\t";
	}
	for(unsigned i = 0; i < address_display_width; i++)
	{
		dump.out << (i < sizeof "MEMORY" - 1 ? "MEMORY"[i] : ' ');
	}
	dump.out << "\tDATA" << std::endl;

	for(size_t off = 0; off < (image_offset & 0xF) + image->ActualDataSize(); off += 16)
	{
		size_t address = block_address - image_offset + off;
		bool current_underlined = false;
		if(block_offset != 0)
		{
			dump.out << "[";
			/* display the start of the block on the first line, but the start of the line on following lines */
			dump.PrintHex(off == 0 ? block_offset : block_offset - image_offset + off, offset_display_width, "");
			dump.out << "]\t";
		}
		if(block_address != 0)
		{
			dump.out << "(";
			dump.PrintHex(off == 0 ? 0 : off - image_offset, position_display_width, "");
			dump.out << ")\t";
		}
		dump.PrintHex(off == 0 ? block_address : address, address_display_width, "");
		dump.out << "\t";
		if(last_underlined)
			dump.BeginUnderline();

		for(int i = 0; i < 16; i++)
		{
			if(dump.use_ansi)
			{
				if(i == 8)
					dump.out << "   ";
				else if(i != 0)
					dump.out << " ";
			}
			else
			{
				if(i == 8)
				{
					if(signal_ends.find(off + i - 1) == signal_ends.end())
						dump.out << " ";
					else
						dump.out << "]";
					dump.out << " ";
					if(signal_starts.find(off + i) == signal_starts.end())
						dump.out << " ";
					else
						dump.out << "[";
				}
				else
				{
					if(signal_ends.find(off + i - 1) == signal_ends.end())
					{
						if(signal_starts.find(off + i) == signal_starts.end())
							dump.out << " ";
						else
							dump.out << "[";
					}
					else
					{
						if(signal_starts.find(off + i) == signal_starts.end())
							dump.out << "]";
						else
							dump.out << "I";
					}
				}
			}

			if(signal_starts.find(off + i) != signal_starts.end())
			{
				dump.BeginUnderline();
				current_underlined = true;
			}
			int byte;
			if(block_address <= address + i
			&& off - image_offset + i < image->ActualDataSize()
			&& ((byte = image->GetByte(off - image_offset + i)) != -1))
			{
				dump.PrintHex(byte, 2, "");
			}
			else
			{
				dump.out << "  ";
			}
			if(signal_ends.find(off + i) != signal_ends.end())
			{
				dump.EndUnderline();
				current_underlined = false;
			}
		}
		if(current_underlined)
			dump.EndUnderline();
		dump.out << "\t";
		if(last_underlined)
			dump.BeginUnderline();
		for(int i = 0; i < 16; i++)
		{
			int byte;
			if(i == 8)
				dump.out << " ";
			if(signal_starts.find(off + i) != signal_starts.end())
			{
				dump.BeginUnderline();
				current_underlined = true;
			}
			if(block_address <= address + i
			&& off - image_offset + i < image->ActualDataSize()
			&& ((byte = image->GetByte(off - image_offset + i)) != -1))
			{
				dump.PutChar((*dump.encoding)[byte]);
			}
			else
			{
				dump.PutChar(' ');
			}
			if(signal_ends.find(off + i) != signal_ends.end())
			{
				dump.EndUnderline();
				current_underlined = false;
			}
		}
		if(current_underlined)
			dump.EndUnderline();
		dump.out << std::endl;
		last_underlined = current_underlined;
	}
}

#if 0
char32_t Block::encoding_default[256] =
{
	0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E,
	0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E,
	0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E,
	0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E,
	0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
	0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,
	0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
	0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,
	0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
	0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
	0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
	0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,
	0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
	0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,
	0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
	0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x002E,
	0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E,
	0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E,
	0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E,
	0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E,
	0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E,
	0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E,
	0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E,
	0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E,
	0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E,
	0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E,
	0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E,
	0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E,
	0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E,
	0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E,
	0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E,
	0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E, 0x002E,
};
#endif

char32_t Block::encoding_default[256] =
{
	0x2400, 0x2401, 0x2402, 0x2403, 0x2404, 0x2405, 0x2406, 0x2407,
	0x2400, 0x2401, 0x2402, 0x2403, 0x2404, 0x2405, 0x2406, 0x2407,
	0x2418, 0x2419, 0x241A, 0x241B, 0x241C, 0x241D, 0x241E, 0x241F,
	0x2418, 0x2419, 0x241A, 0x241B, 0x241C, 0x241D, 0x241E, 0x241F,
	0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
	0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,
	0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
	0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,
	0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
	0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
	0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
	0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,
	0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
	0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,
	0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
	0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x2421,
	0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD,
	0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD,
	0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD,
	0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD,
	0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD,
	0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD,
	0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD,
	0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD,
	0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD,
	0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD,
	0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD,
	0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD,
	0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD,
	0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD,
	0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD,
	0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD,
};

char32_t Block::encoding_cp437[256] =
{
	0x2400, 0x263A, 0x263B, 0x2665, 0x2666, 0x2663, 0x2660, 0x2022,
	0x25D8, 0x25CB, 0x25D9, 0x2642, 0x2640, 0x266A, 0x266B, 0x263C,
	0x25BA, 0x25C4, 0x2195, 0x203C, 0x00B6, 0x00A7, 0x25AC, 0x21A8,
	0x2191, 0x2193, 0x2192, 0x2190, 0x221F, 0x2194, 0x25B2, 0x25BC,
	0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
	0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,
	0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
	0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,
	0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
	0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
	0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
	0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,
	0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
	0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,
	0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
	0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x2302,
	0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7,
	0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
	0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9,
	0x00FF, 0x00D6, 0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x20A7, 0x0192,
	0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA,
	0x00BF, 0x2310, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB,
	0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556,
	0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510,
	0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F,
	0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567,
	0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B,
	0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580,
	0x03B1, 0x00DF, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x00B5, 0x03C4,
	0x03A6, 0x0398, 0x03A9, 0x03B4, 0x221E, 0x03C6, 0x03B5, 0x2229,
	0x2261, 0x00B1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00F7, 0x2248,
	0x00B0, 0x2219, 0x00B7, 0x221A, 0x207F, 0x00B2, 0x25A0, 0xFFFD,
};

char32_t Block::encoding_st[256] =
{
	0x2400, 0x21E7, 0x21E9, 0x21E8, 0x21E6, 0xFFFD, 0xFFFD, 0xFFFD,
	0x2713, 0xFFFD, 0xFFFD, 0x266A, 0x240C, 0x240D, 0xFFFD, 0xFFFD,
	0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD,
	0xFFFD, 0xFFFD, 0x0259, 0x241B, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD,
	0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
	0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,
	0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
	0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,
	0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
	0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
	0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
	0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,
	0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
	0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,
	0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
	0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x2302,
	0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7,
	0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
	0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9,
	0x00FF, 0x00D6, 0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x00DF, 0x0192,
	0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA,
	0x00BF, 0x2310, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB,
	0x00E3, 0x00F5, 0x00D8, 0x00F8, 0x0153, 0x0152, 0x00C0, 0x00C3,
	0x00D5, 0x00A8, 0x00B4, 0x2020, 0x00B6, 0x00A9, 0x00AE, 0x2122,
	0x0133, 0x0132, 0x05D0, 0x05D1, 0x05D2, 0x05D3, 0x05D4, 0x05D5,
	0x05D6, 0x05D7, 0x05D8, 0x05D9, 0x05DB, 0x05DC, 0x05DE, 0x05E0,
	0x05E1, 0x05E2, 0x05E4, 0x05E6, 0x05E7, 0x05E8, 0x05E9, 0x05EA,
	0x05DF, 0x05DA, 0x05DD, 0x05E3, 0x05E5, 0x00A7, 0x2227, 0x221E,
	0x03B1, 0x03B2, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x00B5, 0x03C4,
	0x03A6, 0x0398, 0x03A9, 0x03B4, 0x222E, 0x03D5, 0x2208, 0x2229,
	0x2261, 0x00B1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00F7, 0x2248,
	0x00B0, 0x2022, 0x00B7, 0x221A, 0x207F, 0x00B2, 0x00B3, 0x00AF,
};
