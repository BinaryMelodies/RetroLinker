#ifndef DUMPER_H
#define DUMPER_H

#include <iostream>
#include <iomanip>
#include <set>
#include <vector>
#include "../common.h"
#include "../linker/writable.h"

namespace Dumper
{

class Dumper;

/**
 * @brief This class represents an entry that can be displayed in a file dump
 */
template <typename ... Ts>
	class Display
{
public:
	virtual ~Display() { }

	/** @brief Returns true if the specified value is such that it should not be displayed */
	virtual bool IsMissing(std::tuple<Ts...>& values)
	{
		//return is_missing(values);
		return values == std::tuple<Ts...>();
	}

	/** @brief Prints the value through the Dumper, different types of fields can be displayed in different ways */
	virtual void DisplayValue(Dumper& dump, std::tuple<Ts...> values) = 0;
};

/**
 * @brief Represents an enumerated value, with named options
 */
class ChoiceDisplay : public Display<offset_t>
{
public:
	/**
	 * @brief Maps values to names
	 */
	std::map<offset_t, std::string> names;
	/**
	 * @brief Name for values not contained in names
	 */
	std::string default_name;

	/**
	 * @brief If false, any value not listed in names is missing, otherwise only missing_value is missing
	 */
	bool missing_on_value;
	/**
	 * @brief The single missing value, only used for missing_on_value true
	 */
	offset_t missing_value;

	ChoiceDisplay(std::map<offset_t, std::string> names, std::string default_name = "unknown")
		: names(names), default_name(default_name), missing_on_value(false), missing_value(0)
	{
	}

	ChoiceDisplay(std::map<offset_t, std::string> names, offset_t missing_value, std::string default_name = "unknown")
		: names(names), default_name(default_name), missing_on_value(true), missing_value(missing_value)
	{
	}

	/**
	 * @brief Creates a boolean choice
	 */
	ChoiceDisplay(std::string on_true, std::string on_false)
		: missing_on_value(false), missing_value(0)
	{
		/* TODO: alternative?
			default_name = on_true;
			names[0] = on_false;
		*/
		names[0] = on_false;
		names[1] = on_true;
	}

	/**
	 * @brief Creates a boolean choice that is either present with name or not present at all
	 */
	ChoiceDisplay(std::string on_true)
		: default_name("unknown"), missing_on_value(false), missing_value(0)
	{
		/* TODO: alternative?
			default_name = on_true;
			missing_on_value = true;
			missing_value = 0;
		*/
		names[1] = on_true;
	}

	bool IsMissing(std::tuple<offset_t>& values) override;
	void DisplayValue(Dumper& dump, std::tuple<offset_t> values) override;
};

/**
 * @brief Represents a field with a hexadecimal display, typically bitfields, addresses, sizes, etc.
 */
class HexDisplay : public Display<offset_t>
{
public:
	unsigned width;
	HexDisplay(unsigned width = 8)
		: width(width)
	{
	}

	void DisplayValue(Dumper& dump, std::tuple<offset_t> values) override;
};

/**
 * @brief Represents a field with a decimal display, usually indices into an array or similar, such as segment numbers
 */
class DecDisplay : public Display<offset_t>
{
public:
	std::string suffix;
	DecDisplay(std::string suffix = "")
		: suffix(suffix)
	{
	}

	void DisplayValue(Dumper& dump, std::tuple<offset_t> values) override;
};

/**
 * @brief A value displayed as a colon-separated pair, typically 8086 segmented addresses
 */
class SegmentedDisplay : public Display<offset_t, offset_t>
{
public:
	unsigned width;
	SegmentedDisplay(unsigned width = 4)
		: width(width)
	{
	}

	void DisplayValue(Dumper& dump, std::tuple<offset_t, offset_t> values) override;
};

/**
 * @brief A value displayed as a separated pair, such as a version number
 */
class VersionDisplay : public Display<offset_t, offset_t>
{
public:
	std::string separator;
	VersionDisplay(std::string separator = ".")
		: separator(separator)
	{
	}

	void DisplayValue(Dumper& dump, std::tuple<offset_t, offset_t> values) override;
};

/**
 * @brief A display with a prefix for a section
 */
template <typename ... Ts>
	class SectionedDisplay : public Display<offset_t, Ts...>
{
public:
	std::string suffix;
	Display<Ts...> * offset_display;

	SectionedDisplay(Display<Ts...> * offset_display)
		: suffix(""), offset_display(offset_display)
	{
	}

	SectionedDisplay(std::string suffix, Display<Ts...> * offset_display)
		: suffix(suffix), offset_display(offset_display)
	{
	}

	~SectionedDisplay()
	{
		delete offset_display;
	}

	void DisplayValue(Dumper& dump, std::tuple<offset_t, Ts...> values) override;
};

/**
 * @brief A value that is separated into bitfields, typically bit flags
 */
class BitFieldDisplay : public HexDisplay
{
public:
	class BitField
	{
	public:
		unsigned offset, length;
		Display<offset_t> * display;
		bool optional_field;
		BitField(unsigned offset, unsigned length, Display<offset_t> * display, bool optional_field)
			: offset(offset), length(length), display(display), optional_field(optional_field)
		{
		}

		~BitField()
		{
			delete display;
		}

		bool ShouldDisplay(std::tuple<offset_t>& values)
		{
			return !optional_field || !display->IsMissing(values);
		}
	};

	std::map<unsigned, BitField *> bitfields;

	BitFieldDisplay(unsigned width = 8)
		: HexDisplay(width)
	{
	}

	static BitFieldDisplay& Make(unsigned width = 8)
	{
		return *new BitFieldDisplay(width);
	}

	~BitFieldDisplay();

	BitFieldDisplay& AddBitField(unsigned offset, unsigned length, Display<offset_t> * display, bool optional_field)
	{
		bitfields[offset] = new BitField(offset, length, display, optional_field);
		return *this;
	}

	void DisplayValue(Dumper& dump, std::tuple<offset_t> values) override;
};

/**
 * @brief A display for a fixed or variable length string field
 */
class StringDisplay : public Display<std::string>
{
public:
	/**
	 * @brief The width of the string field, exactly this many characters will be shown, unless it is offset_t(-1), then it prints until the first null byte
	 */
	offset_t width;
	std::string open_quote, close_quote;

	StringDisplay(size_t width, std::string open_quote, std::string close_quote)
		: width(width), open_quote(open_quote), close_quote(close_quote)
	{
	}

	StringDisplay(size_t width, std::string quote = "")
		: width(width), open_quote(quote), close_quote(quote)
	{
	}

	StringDisplay(std::string quote = "")
		: width(-1), open_quote(quote), close_quote(quote)
	{
	}

	bool IsMissing(std::tuple<std::string>& values) override;
	void DisplayValue(Dumper& dump, std::tuple<std::string> values) override;

	using Display<std::string>::IsMissing;
	bool IsMissing(std::tuple<offset_t>& values);
	using Display<std::string>::DisplayValue;
	void DisplayValue(Dumper& dump, std::tuple<offset_t> values);
};

/**
 * @brief A representation of a named value within a structure
 */
class Field
{
public:
	/** @brief The name to be displayed */
	std::string label;
	/** @brief If the field is optional, it will not be displayed for certain values */
	bool optional_field;
	/** @brief The field should not be displayed, it is for internal use (alternatively, it can be displayed through another method) */
	bool internal;

	Field(std::string label, bool optional_field = false, bool internal = false)
		: label(label), optional_field(optional_field), internal(internal)
	{
	}

	virtual ~Field();

	virtual bool ShouldDisplay() = 0;
	virtual void DisplayValue(Dumper& dump) = 0;
};

/**
 * @brief A typed representation of a named value within a structure
 */
template <typename ... Ts>
	class FieldOf : public Field
{
public:
	/** @brief The method to show it in */
	Display<Ts...> * display;
	std::tuple<Ts...> values;

	FieldOf(std::string label, Display<Ts...> * display, Ts... values, bool optional_field = false, bool internal = false)
		: Field(label, optional_field, internal), display{display}, values{values...}
	{
	}

	~FieldOf()
	{
		if(display)
			delete display;
	}

	bool ShouldDisplay() override
	{
		return !internal && (!optional_field || !display->IsMissing(values));
	}

	void DisplayValue(Dumper& dump) override
	{
		display->DisplayValue(dump, values);
	}
};

/**
 * @brief A record whose values should be displayed together, as a collection
 */
class Container
{
public:
	std::string name;

	std::map<std::string, Field *> field_names;
	std::vector<Field *> fields;

	Container(std::string name = "")
		: name(name)
	{
	}

	virtual ~Container();

	Field * FindField(std::string name)
	{
		auto it = field_names.find(name);
		if(it == field_names.end())
			return nullptr;
		return it->second;
	}

	template <typename T>
		T GetField(std::string name, offset_t default_value = T())
	{
		auto it = field_names.find(name);
		if(it == field_names.end())
			return default_value;
		if(FieldOf<T> * field = dynamic_cast<FieldOf<T> *>(it->second))
		{
			return std::get<0>(field->values);
		}
		return default_value;
	}

#if 0
	offset_t GetField(std::string name, int index, offset_t default_value)
	{
		auto it = field_names.find(name);
		if(it == field_names.end())
			return default_value;
		return it->second->values[index];
	}
#endif

	void AddField(Field * field)
	{
		fields.push_back(field);
		field_names[field->label] = field;
	}

	void AddField(size_t index, Field * field)
	{
		fields.insert(fields.begin() + index, field);
		field_names[field->label] = field;
	}

	template <typename D, typename ... Ts>
//		void AddField(std::string label, Display<Ts...> * display, Ts... values)
		void AddField(std::string label, D * display, Ts... values)
	{
//		AddField(new typename display_arguments<D>::Field(label, display, values..., false, false));
		AddField(new FieldOf<Ts...>(label, display, values..., false, false));
	}

	template <typename D, typename ... Ts>
//		void AddOptionalField(std::string label, Display<Ts...> * display, Ts... values)
		void AddOptionalField(std::string label, D * display, Ts... values)
	{
		AddField(new FieldOf<Ts...>(label, display, values..., true, false));
	}

	template <typename D, typename ... Ts>
//		void AddHiddenField(std::string label, Display<Ts...> * display, Ts... values)
		void AddHiddenField(std::string label, D * display, Ts... values)
	{
		AddField(new FieldOf<Ts...>(label, display, values..., false, true));
	}

	template <typename D, typename ... Ts>
//		void AddField(std::string label, Display<Ts...> * display, Ts... values)
		void InsertField(size_t index, std::string label, D * display, Ts... values)
	{
		AddField(index, new FieldOf<Ts...>(label, display, values..., false, false));
	}

	template <typename D, typename ... Ts>
//		void AddOptionalField(std::string label, Display<Ts...> * display, Ts... values)
		void InsertOptionalField(size_t index, std::string label, D * display, Ts... values)
	{
		AddField(index, new FieldOf<Ts...>(label, display, values..., true, false));
	}

	template <typename D, typename ... Ts>
//		void AddHiddenField(std::string label, Display<Ts...> * display, Ts... values)
		void InsertHiddenField(size_t index, std::string label, D * display, Ts... values)
	{
		AddField(index, new FieldOf<Ts...>(label, display, values..., false, true));
	}

	virtual void Display(Dumper& dump);
};

/**
 * @brief A record that represents a region within the file
 */
class Region : public Container
{
public:
	Region(std::string name, offset_t offset, offset_t length, unsigned display_width)
		: Container(name)
	{
		AddField("Offset", new HexDisplay(display_width), offset);
		AddField("Length", new HexDisplay(display_width), length);
	}

//	void Display(Dumper& dump);
};

/**
 * @brief A brief record, such as a relocation or imported library
 */
class Entry : public Container
{
public:
	offset_t number;
	offset_t offset;
	unsigned display_width;

	Entry(std::string name, offset_t number, offset_t offset = offset_t(-1), unsigned display_width = 8)
		: Container(name), number(number), offset(offset), display_width(display_width)
	{
	}

	void Display(Dumper& dump) override;
};

/**
 * @brief A region within a file that can be dumped, decompiled, and it may contain fixups
 */
class Block : public Region
{
public:
	/** @brief Displaying in-file offsets */
	unsigned offset_display_width;
	/** @brief Displaying in-segment positions */
	unsigned position_display_width;
	/** @brief Displaying in-memory addresses */
	unsigned address_display_width;

	static char32_t encoding_default[256];
	static char32_t encoding_cp437[256];
	static char32_t encoding_st[256];

	Linker::Writable * image;

	std::set<offset_t> signal_starts;
	std::set<offset_t> signal_ends;

	/**
	 * @brief Add a relocation inside the image block
	 *
	 * When displaying a dump of a block, the relocation zones can be signalled, for example via underlining.
	 * This method is named to reflect this signalling.
	 */
	void AddSignal(offset_t off, offset_t len)
	{
		offset_t end = off + len - 1;
		signal_starts.insert(off);
		signal_ends.insert(end);
	}

	Block(std::string name, offset_t offset, Linker::Writable * image, offset_t address, unsigned display_width,
			unsigned offset_display_width = 8, unsigned address_display_width = -1, unsigned position_display_width = -1)
		: Region(name, offset, image ? image->ActualDataSize() : 0, display_width),
			offset_display_width(offset_display_width),
			position_display_width(position_display_width != -1u ? position_display_width
				: address_display_width != -1u ? address_display_width : offset_display_width),
			address_display_width(address_display_width != -1u ? address_display_width : offset_display_width),
			image(image)
	{
		AddField("Address", new HexDisplay(display_width), address);
	}

	void Display(Dumper& dump) override;
};

/**
 * @brief A class to control the output of a file analysis
 */
class Dumper
{
public:
	std::ostream& out;
	bool use_ansi;

	char32_t (* encoding)[256];

	Dumper(std::ostream& out)
		: out(out), use_ansi(true), encoding(nullptr)
	{
	}

	void SetEncoding(char32_t (& encoding)[256], bool force = false)
	{
		if(this->encoding == nullptr || force)
		{
			this->encoding = &encoding;
		}
	}

	void SetTitle(std::string title)
	{
		out << "=== " << title << " ===" << std::endl;
	}

//	std::vector<Field> fields;

	/**
	 * @brief Displays a hexadecimal value (default prefix is "0x")
	 */
	void PrintHex(offset_t value, unsigned width, std::string prefix = "0x")
	{
		out << prefix << std::hex << std::setw(width) << std::setfill('0') << value;
	}

#if 0
	/**
	 * @brief Displays a hexadecimal value (prefix is "0x")
	 */
	void PrintHex(offset_t value, unsigned width, bool prefixed)
	{
		PrintHex(value, width, prefixed ? "0x" : "");
	}
#endif

	/**
	 * @brief Displays a decimal value (default prefix is "#")
	 */
	void PrintDec(offset_t value, std::string prefix = "#")
	{
		out << prefix << std::dec << value;
	}

#if 0
	/**
	 * @brief Displays a decimal value (prefix is "#")
	 */
	void PrintDec(offset_t value, bool prefixed)
	{
		PrintDec(value, prefixed ? "#" : "");
	}
#endif

	/**
	 * @brief Displays a Unicode character as a UTF-8 byte sequence
	 */
	void PutChar(char32_t c)
	{
		/* TODO: this should be customizable */
		if(c < 0x80)
		{
			out.put(c);
		}
		else if(c < 0x800)
		{
			out.put((c >> 6) | 0xC0);
			out.put((c & 0x3F) | 0x80);
		}
		else if(c < 0x10000)
		{
			out.put((c >> 12) | 0xE0);
			out.put(((c >> 6) & 0x3F) | 0x80);
			out.put((c & 0x3F) | 0x80);
		}
		else
		{
			out.put(((c >> 18) & 0x07) | 0xF0);
			out.put(((c >> 12) & 0x3F) | 0x80);
			out.put(((c >> 6) & 0x3F) | 0x80);
			out.put((c & 0x3F) | 0x80);
		}
	}

	/**
	 * @brief ANSI escape sequence to add underline
	 */
	void BeginUnderline()
	{
		if(use_ansi)
			out << "\33[4m";
	}

	/**
	 * @brief ANSI escape sequence to remove all formatting
	 */
	void EndUnderline()
	{
		if(use_ansi)
			out << "\33[m";
	}
};

template <unsigned I, size_t ... Is, typename ... Ts>
	inline auto rest_(std::tuple<Ts...> elements, std::index_sequence<Is...> s)
{
	return std::make_tuple(std::get<I + Is>(elements)...);
}

template <unsigned I, typename ... Ts>
	inline auto rest(std::tuple<Ts...> elements)
{
	return rest_<I>(elements, std::make_index_sequence<sizeof...(Ts) - I>());
}

template <typename ... Ts>
	void SectionedDisplay<Ts...>::DisplayValue(Dumper& dump, std::tuple<offset_t, Ts...> values)
{
	dump.PrintDec(std::get<0>(values), "");
	dump.out << suffix << ':';
	offset_display->DisplayValue(dump, rest<1>(values));
}

}

#endif /* DUMPER_H */
