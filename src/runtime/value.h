#ifndef VALUE_H
#define VALUE_H

#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <variant>
#include <vector>
#include "../common.h"

namespace Runtime
{
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

		bool IsEmpty() const
		{
			return std::holds_alternative<Empty>(content);
		}

		String * GetString()
		{
			return std::get_if<String>(&content);
		}

		const String * GetString() const
		{
			return std::get_if<String>(&content);
		}

		Integer * GetInteger()
		{
			return std::get_if<Integer>(&content);
		}

		const Integer * GetInteger() const
		{
			return std::get_if<Integer>(&content);
		}

		Logical * GetLogical()
		{
			return std::get_if<Logical>(&content);
		}

		const Logical * GetLogical() const
		{
			return std::get_if<Logical>(&content);
		}

		List * GetList()
		{
			return std::get_if<List>(&content);
		}

		const List * GetList() const
		{
			return std::get_if<List>(&content);
		}

		Table * GetTable()
		{
			return std::get_if<Table>(&content);
		}

		const Table * GetTable() const
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

		const Value& operator[](Value value) const
		{
			if(const Table * table = GetTable())
			{
				return table->find(value.ToString())->second;
			}
			else if(const List * list = GetList())
			{
				return (*list)[value.ToInteger()];
			}
			else
			{
				Linker::FatalError("Fatal error: not an aggregate type, cannot call [] operator");
			}
		}
	};

	static inline std::ostream& operator <<(std::ostream& out, const Value& value)
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
}

#endif /* VALUE_H */
