
// attempt for a resource extractor

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "common.h"
#include "format/apple.h"
#include "format/gsos.h"
#include "format/leexe.h"
#include "format/macos.h"
#include "format/neexe.h"
#include "format/peexe.h"
#include "formats.h"

#include <cerrno>
#include <string>
#include <variant>

using namespace Linker;

void usage(char * argv0)
{
	std::cerr << "Usage: " << argv0 << "[options] <input file>" << std::endl;
	std::cerr << "\t-h" << std::endl << "\t\tDisplay this help page" << std::endl;
	std::cerr << "\t-F<format>" << std::endl << "\t\tSelect output format" << std::endl;
}

/** @brief A YAML-like data format */
class Value
{
public:
	typedef struct { } Empty;
	typedef std::string String;
	typedef offset_t Integer;
	typedef bool Logical;
	typedef std::vector<Value> List;
	typedef std::map<std::string, Value> Table;

	std::variant<
		Empty,
		String,
		Integer,
		Logical,
		List,
		Table
	> content;

	Value() : content(Empty {}) { }
	Value(const Empty& content) : content(Empty {}) { }
	Value(const String& content) : content(content) { }
	Value(const char * content) : content(String(content)) { }
	Value(Integer content) : content(content) { }
	explicit Value(Logical content) : content(content) { }
	Value(const List& content) : content(content) { }
	Value(const Table& content) : content(content) { }

	static Value MakeList()
	{
		return Value(List{});
	}

	static Value MakeTable()
	{
		return Value(Table{});
	}

	bool IsEmpty()
	{
		return std::holds_alternative<Empty>(content);
	}

	String * GetString()
	{
		return std::get_if<String>(&content);
	}

	Integer * GetInteger()
	{
		return std::get_if<Integer>(&content);
	}

	Logical * GetLogical()
	{
		return std::get_if<Logical>(&content);
	}

	List * GetList()
	{
		return std::get_if<List>(&content);
	}

	Table * GetTable()
	{
		return std::get_if<Table>(&content);
	}

	String ToString()
	{
		if(IsEmpty())
		{
			return "";
		}
		else if(String * string = GetString())
		{
			return *string;
		}
		else if(Integer * integer = GetInteger())
		{
			std::ostringstream oss;
			oss << *integer;
			return oss.str();
		}
		else if(Logical * logical = GetLogical())
		{
			return *logical ? "true" : "false";
		}
		else
		{
			Linker::FatalError("Fatal error: unable to convert value to string");
		}
	}

	Integer ToInteger()
	{
		if(String * string = GetString())
		{
			const char * text;
			int base;
			if(starts_with(*string, "0x"))
			{
				text = string->c_str() + 2;
				base = 16;
			}
			else if(starts_with(*string, "0o"))
			{
				text = string->c_str() + 2;
				base = 8;
			}
			else if(starts_with(*string, "0b"))
			{
				text = string->c_str() + 2;
				base = 2;
			}
			else if(starts_with(*string, "0"))
			{
				text = string->c_str() + 1;
				base = 8;
			}
			else
			{
				text = string->c_str();
				base = 10;
			}

			if(text[0] != '\0')
			{
				char * tail;
				errno = 0;
				offset_t result = strtoll(text, &tail, base);
				if(tail[0] == '\0' && errno == 0)
				{
					return result;
				}
			}
		}
		else if(Integer * integer = GetInteger())
		{
			return *integer;
		}

		Linker::FatalError("Fatal error: unable to convert value to integer");
	}

	Value& operator[](Value value)
	{
		if(Table * table = GetTable())
		{
			return (*table)[value.ToString()];
		}
		else if(List * list = GetList())
		{
			return (*list)[value.ToInteger()];
		}
		else
		{
			Linker::FatalError("Fatal error: not an aggregate type, cannot call [] operator");
		}
	}
};

std::ostream& operator <<(std::ostream& out, const Value& value)
{
	std::visit([&out](auto&& value)
	{
		using T = std::decay_t<decltype(value)>;
		if constexpr(std::is_same_v<T, Value::Empty>)
		{
			out << "null";
		}
		else if constexpr(std::is_same_v<T, Value::String>)
		{
			out << '"' << value << '"';
		}
		else if constexpr(std::is_same_v<T, Value::Integer>)
		{
			out << value;
		}
		else if constexpr(std::is_same_v<T, Value::Logical>)
		{
			out << (value ? "true" : "false");
		}
		else if constexpr(std::is_same_v<T, Value::List>)
		{
			out << '[';
			bool started = false;
			for(auto& element : value)
			{
				if(started)
					out << ", ";
				else
					started = true;
				out << element;
			}
			out << ']';
		}
		else if constexpr(std::is_same_v<T, Value::Table>)
		{
			out << '{';
			bool started = false;
			for(auto& pair : value)
			{
				if(started)
					out << ", ";
				else
					started = true;
				out << '"' << pair.first << "\": " << pair.second;
			}
			out << '}';
		}
		else
		{
			static_assert(false);
		}
	}, value.content);
	return out;
}

void extract_resources(const std::shared_ptr<Apple::MacintoshOutputDriver>& format);
void extract_resources(const std::shared_ptr<Apple::GSOutputDriver>& format);
void extract_resources(const std::shared_ptr<Apple::AppleSingleDouble>& format);
void extract_resources(const std::shared_ptr<Apple::MacBinary>& format);
void extract_resources(const std::shared_ptr<Apple::MacintoshResourceFileFormat>& format);
void extract_resources(const std::shared_ptr<Apple::GSOSResourceFileFormat>& format);
void extract_resources(const std::shared_ptr<Microsoft::NEFormat>& format);
void extract_resources(const std::shared_ptr<Microsoft::LEFormat>& format);
void extract_resources(const std::shared_ptr<Microsoft::PEFormat>& format);
void extract_resources(const std::shared_ptr<Microsoft::ResourceFile>& format);
void extract_resources(const std::shared_ptr<Microsoft::NTResourceFile>& format);
void extract_resources(const std::shared_ptr<Format>& format);

void extract_resources(const std::shared_ptr<Apple::MacintoshOutputDriver>& format)
{
	extract_resources(format->resource_fork);
}

void extract_resources(const std::shared_ptr<Apple::GSOutputDriver>& format)
{
	if(format->resource_fork != nullptr)
	{
		extract_resources(format->resource_fork);
	}
	else
	{
		Linker::Warning << "Warning: no resource fork found" << std::endl;
	}
}

void extract_resources(const std::shared_ptr<Apple::AppleSingleDouble>& format)
{
	if(auto entry = std::dynamic_pointer_cast<Apple::AppleSingleDouble::GenericEntry>(format->FindEntry(Apple::AppleSingleDouble::ID_ResourceFork)))
	{
		if(auto resource_format = std::dynamic_pointer_cast<Format>(entry->image))
		{
			extract_resources(resource_format);
			return;
		}
		Linker::Warning << "Warning: unrecognized resource file format" << std::endl;
		return;
	}
	Linker::Warning << "Warning: no resource fork found" << std::endl;
}

void extract_resources(const std::shared_ptr<Apple::MacBinary>& format)
{
	extract_resources(format->apple_single);
}

void extract_resources(const std::shared_ptr<Apple::MacintoshResourceFileFormat>& format)
{
	std::cout << "Mac OS" << std::endl;
	for(auto& resource_type : format->resource_types)
	{
		for(auto& resource_reference : resource_type.references)
		{
			Value res = Value::MakeTable();
			res["type"] = std::string(resource_type.type, 4);
			if(resource_reference.name.has_value())
			{
				res["name"] = resource_reference.name.value();
			}
			res["id"] = offset_t(uint16_t(resource_reference.id));
			res["flags"] = offset_t(resource_reference.attributes);
			std::cout << res << std::endl;
		}
	}
}

void extract_resources(const std::shared_ptr<Apple::GSOSResourceFileFormat>& format)
{
	std::cout << "GS/OS" << std::endl;
	// TODO
}

void extract_resources(const std::shared_ptr<Microsoft::NEFormat>& format)
{
	std::cout << "NE" << std::endl;
	// TODO
}

void extract_resources(const std::shared_ptr<Microsoft::LEFormat>& format)
{
	std::cout << "LE" << std::endl;
	// TODO
}

void extract_resources(const std::shared_ptr<Microsoft::PEFormat>& format)
{
	std::cout << "PE" << std::endl;
	// TODO
}

void extract_resources(const std::shared_ptr<Microsoft::ResourceFile>& format)
{
	std::cout << "resources" << std::endl;
	// TODO
}

void extract_resources(const std::shared_ptr<Microsoft::NTResourceFile>& format)
{
	std::cout << "NT resources" << std::endl;
	// TODO
}

void extract_resources(const std::shared_ptr<Format>& format)
{
	// TODO: use virtual methods

	if(auto apple_single_double = std::dynamic_pointer_cast<Apple::AppleSingleDouble>(format))
	{
		extract_resources(apple_single_double);
	}
	else if(auto mac_binary = std::dynamic_pointer_cast<Apple::MacBinary>(format))
	{
		extract_resources(mac_binary);
	}
	else if(auto mac_os_output_driver = std::dynamic_pointer_cast<Apple::MacintoshOutputDriver>(format))
	{
		extract_resources(mac_os_output_driver);
	}
	else if(auto mac_os_resource_file = std::dynamic_pointer_cast<Apple::MacintoshResourceFileFormat>(format))
	{
		extract_resources(mac_os_resource_file);
	}
	else if(auto gs_os_output_driver = std::dynamic_pointer_cast<Apple::GSOutputDriver>(format))
	{
		extract_resources(gs_os_output_driver);
	}
	else if(auto gs_os_resource_file = std::dynamic_pointer_cast<Apple::GSOSResourceFileFormat>(format))
	{
		extract_resources(gs_os_resource_file);
	}
	else if(auto ne_format = std::dynamic_pointer_cast<Microsoft::NEFormat>(format))
	{
		extract_resources(ne_format);
	}
	else if(auto le_format = std::dynamic_pointer_cast<Microsoft::LEFormat>(format))
	{
		extract_resources(le_format);
	}
	else if(auto pe_format = std::dynamic_pointer_cast<Microsoft::PEFormat>(format))
	{
		extract_resources(pe_format);
	}
	else if(auto resource_file = std::dynamic_pointer_cast<Microsoft::ResourceFile>(format))
	{
		extract_resources(resource_file);
	}
	else if(auto nt_resource_file = std::dynamic_pointer_cast<Microsoft::NTResourceFile>(format))
	{
		extract_resources(nt_resource_file);
	}
	else
	{
		Linker::FatalError("Fatal error: unrecognized resource container");
	}
}

void read(std::shared_ptr<Format> format, std::shared_ptr<Reader> rd)
{
	// TODO: do not parse the actual resources
	format->ReadFile(rd);
	extract_resources(format);
}

int main(int argc, char * argv[])
{
	// TODO: multiple input files
	std::string input = "";
	std::shared_ptr<Format> format = nullptr;

	for(int i = 1; i < argc; i++)
	{
		if(argv[i][0] == '-')
		{
			if(argv[i][1] == 'h')
			{
				usage(argv[0]);
				exit(0);
			}
			else if(argv[i][1] == 'F')
			{
				format = FetchFormat(argv[i][2] ? &argv[i][2] : argv[++i]);
			}
		}
		else
		{
			if(input != "")
			{
				Linker::Error << "Error: Multiple input files provided, ignoring" << std::endl;
			}
			input = argv[i];
		}
	}

	if(input == "")
	{
		usage(argv[0]);
		exit(0);
	}

	std::ifstream in;
	in.open(input, std::ios_base::in | std::ios_base::binary);
	if(!in.is_open())
	{
		std::ostringstream message;
		message << "Fatal error: Unable to open file " << input;
		Linker::FatalError(message.str());
	}
	auto rd = std::make_shared<StreamReader>(LittleEndian, in);
	int status = 0;

	if(format == nullptr)
	{
		std::vector<format_description> file_formats;
		DetermineFormat(file_formats, rd);

		if(file_formats.size() == 0)
		{
			Linker::FatalError("Fatal error: Unable to determine file format");
		}

		format_priority highest_priority = PRIORITY_NONE;

		for(auto& file_format : file_formats)
		{
			if(file_format.magic.priority > highest_priority)
				highest_priority = file_format.magic.priority;
		}

		// TODO: there should be only one format

		for(auto& file_format : file_formats)
		{
			if(file_format.magic.priority != highest_priority)
				continue;

			Linker::Debug << "Debug: Reading as " << file_format.magic.description << std::endl;
			format = CreateFormat(rd, file_format);
			if(!format)
			{
				Linker::Error << "Error: Unable to parse file, unimplemented format " << file_format.magic.description << std::endl;
				status = 1;
				continue;
			}
			rd->Seek(file_format.offset);

			try
			{
				read(format, rd);
			}
			catch(Linker::Exception&)
			{
			}
		}

		for(auto& file_format : file_formats)
		{
			if(file_format.magic.priority == highest_priority)
				continue;

			Linker::Debug << "Debug: Other possible format: " << file_format.magic.description << std::endl;
		}
	}
	else
	{
		read(format, rd);
	}

	return status;
}

