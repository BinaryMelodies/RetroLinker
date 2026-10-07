
// attempt for a resource extractor

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "common.h"
#include "format/apple.h"
#include "format/elf.h"
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
	std::cerr << "Usage: " << argv0 << " <expression>" << std::endl;
	std::cerr << "\tBasic operations:" << std::endl;
	std::cerr << "\t-h" << std::endl << "\t\tDisplay this help page" << std::endl;
	std::cerr << "\t-l <filename>" << std::endl << "\t\tRead specified file" << std::endl;
	std::cerr << "\tUnary operators:" << std::endl;
	std::cerr << "\t-f <format> <command>" << std::endl << "\t\tSelect input format for command" << std::endl;
	std::cerr << "\tnot <predicate>" << std::endl << "\t\tNegate predicate" << std::endl;
	std::cerr << "\tBinary operators:" << std::endl;
	std::cerr << "\t<selector>@<identifier|integer|string>" << std::endl << "\t\tSelect named tag in selector" << std::endl;
	std::cerr << "\t<file|predicate> and <predicate>" << std::endl << "\t\tFilter resources where predicate holds" << std::endl;
	std::cerr << "\t<file|predicate> except <predicate>" << std::endl << "\t\tFilter resources where predicate does not hold" << std::endl;
	std::cerr << "\t<predicate> or <predicate>" << std::endl << "\t\tLogical 'or' between predicates" << std::endl;
	std::cerr << "\t<file> or <file>" << std::endl << "\t\tInclude resources from second file that are missing from the first file" << std::endl;
	std::cerr << "\t<file> add <file>" << std::endl << "\t\tOverwrite resources in first file with those from the second file" << std::endl;
}

enum system_type
{
	System_Unspecified,
	/** @brief Refers to formats found in a resource file, which includes OS/2 and 16-bit Windows, but not Windows NT */
	System_UnknownMicrosoft,
	/*System_Windows1x,
	System_Windows3x,*/
	System_Windows,
	System_WindowsNT,
	System_OS2,
	System_OS2_PPC,
	System_BeOS, // TODO: not implemented
	System_Macintosh,
	System_GSOS,
};
system_type SystemType = System_Unspecified;

static inline bool IsMicrosoftResource(system_type type)
{
	switch(type)
	{
	case System_UnknownMicrosoft:
	/*case System_Windows1x:
	case System_Windows3x:*/
	case System_Windows:
	case System_OS2:
	//case System_OS2_PPC: // TODO: not sure
		return true;
	default:
		return false;
	}
}

void SetSystem(system_type type)
{
	if(SystemType == type
	|| (type == System_UnknownMicrosoft && IsMicrosoftResource(SystemType)))
		return;


	if(SystemType == System_Unspecified
	|| (SystemType == System_UnknownMicrosoft && IsMicrosoftResource(type)))
	{
		SystemType = type;
	}
	else
	{
		Linker::FatalError("Fatal error: incompatible systems");
	}
}

/** @brief A YAML/JSON-like data format */
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

// TODO: use virtual methods

void extract_resources(const std::shared_ptr<Apple::MacintoshOutputDriver>& format);
void extract_resources(const std::shared_ptr<Apple::GSOutputDriver>& format);
void extract_resources(const std::shared_ptr<Apple::AppleSingleDouble>& format);
void extract_resources(const std::shared_ptr<Apple::MacBinary>& format);
void extract_resources(const std::shared_ptr<Apple::MacintoshResourceFileFormat>& format);
void extract_resources(const std::shared_ptr<Apple::GSOSResourceFileFormat>& format);
void extract_resources(const std::shared_ptr<Microsoft::NEFormat>& format);
void extract_resources(const std::shared_ptr<Microsoft::LEFormat>& format);
void extract_resources(const std::shared_ptr<Microsoft::PEFormat>& format);
void extract_resources(const std::shared_ptr<ELF::ELFFormat>& format);
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
	SetSystem(System_Macintosh);
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

static std::map<offset_t, std::string> gs_os_resource_type_names =
{
	{ 0x8001, "rIcon" },
	{ 0x8002, "rPicture" },
	{ 0x8003, "rControlList" },
	{ 0x8004, "rControlTemplate" },
	{ 0x8005, "rC1InputString" },
	{ 0x8006, "rPString" },
	{ 0x8007, "rStringList" },
	{ 0x8008, "rMenuBar" },
	{ 0x8009, "rMenu" },
	{ 0x800A, "rMenuItem" },
	{ 0x800B, "rTextForLETextBox2" },
	// { 0x800C, "" },
	{ 0x800D, "rCtlColorTbl" },
	{ 0x800E, "rWindParam1" },
	{ 0x800F, "rWindParam2" },
	{ 0x8010, "rWindColor" },
	{ 0x8011, "rTextBlock" },
	{ 0x8012, "rStyleBlock" },
	{ 0x8013, "rToolStartup" },
	{ 0x8014, "rResName" },
	{ 0x8015, "rAlertString" },
	{ 0x8016, "rText" },
	// { 0x8017, "" },
	// { 0x8018, "" },
	// { 0x8019, "" },
	{ 0x801A, "rTwoRects" },
	// { 0x801B, "" },
	{ 0x801C, "rListRef" },
	{ 0x801D, "rCString" },
	// { 0x801E, "" },
	// { 0x801F, "" },
	{ 0x8020, "rErrorString" },
	{ 0x8021, "rKTransTable" },
	// { 0x8022, "" },
	{ 0x8023, "rC1OutputString" },
	// { 0x8024, "" },
	{ 0x8025, "rTERuler" },
};

void extract_resources(const std::shared_ptr<Apple::GSOSResourceFileFormat>& format)
{
	std::cout << "GS/OS" << std::endl;
	SetSystem(System_GSOS);
	for(auto reference_record : format->map_index)
	{
		Value res = Value::MakeTable();
		res["type"] = offset_t(reference_record->type);
		auto type_name = gs_os_resource_type_names.find(reference_record->type);
		if(type_name != gs_os_resource_type_names.end())
		{
			res["type-desc"] = type_name->second;
		}
		res["id"] = offset_t(reference_record->id);
		res["flags"] = offset_t(reference_record->attributes);

		std::cout << res << std::endl;
	}
}

void extract_resources(const std::shared_ptr<Microsoft::NEFormat>& format)
{
	if(format->IsOS2())
	{
		std::cout << "NE for OS/2" << std::endl;
		SetSystem(System_OS2);

		// TODO: needs testing

		for(auto resource : format->resources)
		{
			Value res = Value::MakeTable();
			res["type"] = offset_t(resource->type_id);
			auto type_name = Microsoft::OS2::resource_type_id_descriptions.find(resource->type_id);
			if(type_name != Microsoft::OS2::resource_type_id_descriptions.end())
			{
				res["type-desc"] = type_name->second;
			}

			res["id"] = offset_t(resource->id);
			res["name"] = res["id"]; // duplicate
			res["flags"] = offset_t(resource->flags);

			std::cout << res << std::endl;
		}
	}
	else
	{
		/*if(format->windows_version.major >= 3)
		{
			std::cout << "NE for Windows 3.x or later" << std::endl;
			SetSystem(System_Windows3x);
		}
		else
		{
			std::cout << "NE for Windows 1.x/2.x or unknown system" << std::endl;
			SetSystem(System_Windows1x);
		}*/
		std::cout << "NE for Windows or unknown system" << std::endl;
		SetSystem(System_Windows);

		for(auto resource_type : format->resource_types)
		{
			for(auto resource : resource_type->resources)
			{
				Value res = Value::MakeTable();

				if(resource->type_id_name.has_value())
				{
					res["type"] = resource->type_id_name.value();
				}
				else
				{
					res["type"] = offset_t(resource->type_id);
					auto type_name = Microsoft::Windows::resource_type_id_descriptions.find(resource->type_id);
					if(type_name != Microsoft::Windows::resource_type_id_descriptions.end())
					{
						res["type-desc"] = type_name->second;
					}
				}

				if(resource->id_name.has_value())
				{
					res["id"] = resource->id_name.value();
				}
				else
				{
					res["id"] = offset_t(resource->id);
				}
				res["name"] = res["id"]; // duplicate
				res["flags"] = offset_t(resource->flags);

				std::cout << res << std::endl;
			}
		}
	}
}

void extract_resources(const std::shared_ptr<Microsoft::LEFormat>& format)
{
	std::cout << "LE/LX" << std::endl;
	SetSystem(System_OS2);
	// TODO
}

void extract_pe_resources(const std::shared_ptr<Microsoft::PEFormat::Resource>& resource, std::vector<Value> levels)
{
	Value res = Value::MakeTable();
	res["levels"] = levels;
	if(levels.size() == 3)
	{
		res["type"] = levels[0];
		if(auto integer = levels[0].GetInteger())
		{
			auto type_name = Microsoft::Windows::resource_type_id_descriptions.find(*integer);
			if(type_name != Microsoft::Windows::resource_type_id_descriptions.end())
			{
				res["type-desc"] = type_name->second;
			}
		}
		res["name"] = res["id"] = levels[1];
		res["language"] = levels[2];
	}
	res["codepage"] = resource->codepage;

	std::cout << res << std::endl;
}

void extract_pe_resources(const std::shared_ptr<Microsoft::PEFormat::ResourceDirectory>& resource_directory, std::vector<Value>& levels)
{
	for(auto& entry : resource_directory->name_entries)
	{
		std::visit([&entry, levels](auto&& content)
		{
			auto levels_ = levels;
			levels_.push_back(entry.identifier.name);
			extract_pe_resources(content, levels_);
		}, entry.content);
	}

	for(auto& entry : resource_directory->id_entries)
	{
		std::visit([&entry, levels](auto&& content)
		{
			auto levels_ = levels;
			levels_.push_back(offset_t(entry.identifier));
			extract_pe_resources(content, levels_);
		}, entry.content);
	}
}

void extract_resources(const std::shared_ptr<Microsoft::PEFormat>& format)
{
	std::cout << "PE" << std::endl;
	SetSystem(System_WindowsNT);
	if(format->resources != nullptr)
	{
		std::vector<Value> levels;
		extract_pe_resources(format->resources, levels);
	}
}

void extract_resources(const std::shared_ptr<ELF::ELFFormat>& format)
{
	// TODO: check if it contains SHT_RES (SHT_OLD_RES is not parsed yet) or PT_RES
	std::cout << "ELF" << std::endl;
	SetSystem(System_OS2_PPC);
	// TODO

	// TODO: also for BeOS?
}

void extract_resources(const std::shared_ptr<Microsoft::ResourceFile>& format)
{
	std::cout << "resources" << std::endl;
	SetSystem(System_UnknownMicrosoft);
	// TODO
}

void extract_resources(const std::shared_ptr<Microsoft::NTResourceFile>& format)
{
	std::cout << "NT resources" << std::endl;
	SetSystem(System_WindowsNT);
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
	else if(auto elf_format = std::dynamic_pointer_cast<ELF::ELFFormat>(format))
	{
		extract_resources(elf_format);
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

class AST;

void add_resources(std::shared_ptr<Format> base_format, std::shared_ptr<Format> source_format, bool overwrite);
void apply_filter(std::shared_ptr<Format> format, std::shared_ptr<AST> predicate, bool positive_filter);

struct OptionSet
{
	char * argv0;
	std::optional<std::string> format;

	OptionSet(char * argv0)
		: argv0(argv0)
	{
	}
};

class AST
{
public:
	virtual ~AST() = default;

	int execute(char * argv0)
	{
		return execute(OptionSet(argv0));
	}

	virtual int execute(const OptionSet& options)
	{
		extract_resources(evaluate(options)); // TODO
		return 0;
	}

	virtual std::shared_ptr<Format> evaluate(const OptionSet& options)
	{
		Linker::FatalError("Internal error");
	}
};

class Literal : public AST
{
public:
	std::string literal;

	Literal(std::string literal)
		: literal(literal)
	{
	}
};

class Selector : public AST
{
public:
	std::shared_ptr<AST> structure;
	std::string name;

	Selector(std::shared_ptr<AST> structure, std::string name)
		: structure(structure), name(name)
	{
	}
};

class Command : public AST
{
public:
	enum operator_type
	{
		Load,
		Help,
	};
	operator_type op;
	std::string parameter;

	Command(operator_type op, std::string parameter)
		: op(op), parameter(parameter)
	{
	}

	int execute(const OptionSet& options) override
	{
		if(op == Help)
		{
			usage(options.argv0);
		}
		else
		{
			evaluate(options);
		}
		return 0;
	}

	std::shared_ptr<Format> evaluate(const OptionSet& options) override
	{
		switch(op)
		{
		case Help:
			usage(options.argv0);
			Linker::FatalError("Fatal error: HELP operator in evaluation context");
		case Load:
			{
				std::ifstream in;
				in.open(parameter, std::ios_base::in | std::ios_base::binary);
				if(!in.is_open())
				{
					std::ostringstream message;
					message << "Fatal error: Unable to open file " << parameter;
					Linker::FatalError(message.str());
				}
				auto rd = std::make_shared<StreamReader>(LittleEndian, in);
				//int status = 0;

				std::shared_ptr<Format> format;

				if(options.format.has_value())
				{
					format = FetchFormat(options.format.value());
					read(format, rd);
				}
				else
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
							//status = 1;
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
				return format;
			}
		default:
			assert(false);
		}
	}
};

class Unary : public AST
{
public:
	enum operator_type
	{
		Not,
		Format,
	};
	operator_type op;
	std::string parameter;
	std::shared_ptr<AST> operand;

	Unary(operator_type op, std::string parameter, std::shared_ptr<AST> operand)
		: op(op), parameter(parameter), operand(operand)
	{
	}

	Unary(operator_type op, std::shared_ptr<AST> operand)
		: op(op), operand(operand)
	{
	}

	std::shared_ptr<Linker::Format> evaluate(const OptionSet& options) override
	{
		switch(op)
		{
		case Not:
			Linker::FatalError("Fatal error: NOT operator in evaluation context");
		case Format:
			{
				OptionSet modified = options;
				modified.format = parameter;
				return operand->evaluate(modified);
			}
		default:
			assert(false);
		}
	}
};

class Binary : public AST
{
public:
	enum operator_type
	{
		And,
		Or,
		Except,
		Add,
	};
	operator_type op;
	std::shared_ptr<AST> lhs, rhs;

	Binary(operator_type op, std::shared_ptr<AST> lhs, std::shared_ptr<AST> rhs)
		: op(op), lhs(lhs), rhs(rhs)
	{
	}

	std::shared_ptr<Format> evaluate(const OptionSet& options) override
	{
		std::shared_ptr<Format> format = lhs->evaluate(options);
		std::shared_ptr<Format> format2;
		switch(op)
		{
		case And:
			apply_filter(format, rhs, true);
			return format;
		case Except:
			apply_filter(format, rhs, false);
			return format;
		case Or:
			format2 = lhs->evaluate(options);
			add_resources(format, format2, false);
			return format;
		case Add:
			format2 = lhs->evaluate(options);
			add_resources(format, format2, true);
			return format;
		default:
			assert(false);
		}
	}
};

class Parser
{
public:
	int argc;
	char * arg;
	char ** argv;

	Parser(int argc, char ** argv)
		: argc(argc), arg(*argv), argv(argv + 1)
	{
	}

	int peek_char()
	{
		if(argc == 0)
		{
			return -1;
		}
		else
		{
			return *arg;
		}
	}

	int next_char()
	{
		if(argc == 0)
		{
			return -1;
		}
		else if(*arg == '\0')
		{
			arg = *argv++;
			argc--;
			return 0;
		}
		else
		{
			return *arg++;
		}
	}

	enum token_type
	{
		token_empty = 0, // not a real token
		token_eof,
		token_error,
		token_string,
		token_integer,
		token_identifier,
	};

	token_type parse_token(std::string& result)
	{
		int c;
		std::string string;
		while((c = next_char()) == ' ' || c == '\0')
			;
		if(c == -1)
		{
			return token_eof;
		}
		else if(c == '"' || c == '\'')
		{
			int q = c;
			while((c = next_char()) != -1 && c != q)
			{
				string += c;
			}
			if(c == -1)
			{
				Linker::FatalError("Fatal error: unterminated string literal");
				return token_error;
			}
			result = string;
			return token_string;
		}
		else if(isalnum(c) || c == '_' || c == '.' || c == '-')
		{
			token_type type = token_integer;
			int base = 10;
			string += c;
			while(isalnum(c = peek_char()) || c == '_' || c == '.' || c == '-')
			{
				string += next_char();
				switch(base)
				{
				case 10:
					if(string == "0x")
					{
						base = 16;
						type = token_integer;
					}
					else if(string == "0o")
					{
						base = 8;
						type = token_integer;
					}
					else if(string == "0b")
					{
						base = 2;
						type = token_integer;
					}
					else if(!('0' <= c && c <= '9'))
					{
						type = token_identifier;
					}
					break;
				case 16:
					if(!(('0' <= c && c <= '9') || ('A' <= c && c <= 'F') || ('a' <= c && c <= 'f')))
					{
						type = token_identifier;
					}
					break;
				case 8:
					if(!('0' <= c && c <= '7'))
					{
						type = token_identifier;
					}
					break;
				case 2:
					if(!('0' <= c && c <= '1'))
					{
						type = token_identifier;
					}
					break;
				}
			}
			result = string;
			return type;
		}
		else
		{
			return token_type(c);
		}
	}

	token_type last_token = token_empty;
	std::string last_token_string;

	token_type peek_token(std::string& result)
	{
		if(last_token == token_empty)
		{
			last_token = parse_token(last_token_string);
		}
		result = last_token_string;
		return last_token;
	}

	token_type next_token(std::string& result)
	{
		token_type type = peek_token(result);
		last_token = token_empty;
		last_token_string.clear();
		return type;
	}

	std::shared_ptr<AST> parse_primary()
	{
		std::shared_ptr<AST> result;

		std::string text;
		token_type next = next_token(text);

		switch(int(next))
		{
		case '(':
			result = parse_expression();
			if(next_token(text) != ')')
			{
				Linker::FatalError("Fatal error: Syntax error in expression");
			}
			return result;
		case token_identifier:
			if(text == "not")
			{
				return std::make_shared<Unary>(Unary::Not, parse_primary());
			}
			else if(text == "-h")
			{
				return std::make_shared<Command>(Command::Help, "");
			}
			// TODO: create command
			else if(text == "-f")
			{
				switch(next_token(text))
				{
				case token_identifier:
				case token_string:
				case token_integer:
					return std::make_shared<Unary>(Unary::Format, text, parse_primary());
				default:
					Linker::FatalError("Fatal error: Syntax error in expression");
				}
			}
			else if(text == "-l")
			{
				switch(next_token(text))
				{
				case token_identifier:
				case token_string:
				case token_integer:
					return std::make_shared<Command>(Command::Load, text);
				default:
					Linker::FatalError("Fatal error: Syntax error in expression");
				}
			}
		case token_string:
		case token_integer:
			result = std::make_shared<Literal>(text);
			while(peek_token(text) == '@')
			{
				next = next_token(text);
				switch(next)
				{
				case token_string:
				case token_integer:
				case token_identifier:
					result = std::make_shared<Selector>(result, text);
					break;
				default:
					Linker::FatalError("Fatal error: Syntax error in expression");
				}
			}
			return result;
		default:
			Linker::FatalError("Fatal error: Syntax error in expression");
		}
	}

	std::shared_ptr<AST> parse_conjunction()
	{
		std::shared_ptr<AST> result;

		std::string text;
		token_type next;

		result = parse_primary();

		while(true)
		{
			next = peek_token(text);
			switch(next)
			{
			case token_identifier:
				if(text == "and")
				{
					next_token(text);
					result = std::make_shared<Binary>(Binary::And, result, parse_primary());
				}
				else if(text == "except")
				{
					next_token(text);
					result = std::make_shared<Binary>(Binary::Except, result, parse_primary());
				}
				else
				{
					return result;
				}
			default:
				return result;
			}
		}
	}

	std::shared_ptr<AST> parse_expression()
	{
		std::shared_ptr<AST> result;

		std::string text;
		token_type next;

		result = parse_conjunction();

		while(true)
		{
			next = peek_token(text);
			switch(next)
			{
			case token_identifier:
				if(text == "or")
				{
					next_token(text);
					result = std::make_shared<Binary>(Binary::Or, result, parse_conjunction());
				}
				else if(text == "add")
				{
					next_token(text);
					result = std::make_shared<Binary>(Binary::Add, result, parse_conjunction());
				}
				else
				{
					return result;
				}
			default:
				return result;
			}
		}
	}

	std::shared_ptr<AST> parse_command()
	{
		auto result = parse_expression();

		std::string text;

		int i;

		if((i = next_token(text)) != token_eof)
		{
			Linker::FatalError("Fatal error: Superfluous tokens at end of expression");
		}

		return result;
	}
};

void add_resources(std::shared_ptr<Format> base_format, std::shared_ptr<Format> source_format, bool overwrite)
{
	// TODO
}

void apply_filter(std::shared_ptr<Format> format, std::shared_ptr<AST> predicate, bool positive_filter)
{
	// TODO
}

int main(int argc, char * argv[])
{
	if(argc == 1)
	{
		usage(argv[0]);
		return 0;
	}

	Parser parser(argc - 1, argv + 1);

	auto command = parser.parse_command();

	return command->execute(argv[0]);
}

