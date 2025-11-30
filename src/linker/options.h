#ifndef OPTIONS_H
#define OPTIONS_H

#include <optional>
#include <sstream>
#include <string>
#include <vector>
#include "../common.h"

namespace Linker
{
	/** @brief A representation of an enumeration with associated string representations for each value */
	template <typename Enum>
		class Enumeration
	{
	public:
		typedef Enum value_type;

		/** @brief An empty dictionary that explains the value types in detail */
		std::map<value_type, std::string> descriptions;

		/** @brief Maps each value to a sequence of valid strings */
		std::map<value_type, std::vector<std::string>> values;
	protected:
		/** @brief Maps each name to its value */
		std::map<std::string, value_type, CaseInsensitiveLess> names;

		void init(value_type next_value)
		{
		}

		void init(value_type next_value, std::string name)
		{
			names[name] = next_value;
			if(values.find(next_value) == values.end())
				values[next_value] = std::vector<std::string>(1, name);
			else
				values[next_value].push_back(name);
		}

		template <typename ... Args>
			void init(value_type next_value, std::string name, std::string next_name, Args ... args)
		{
			init(next_value, name);
			init(value_type(next_value + 1), next_name, args...);
		}

		void init(value_type _, std::string name, value_type actual_next_value)
		{
			init(actual_next_value, name);
		}

		template <typename ... Args>
			void init(value_type _, std::string name, value_type actual_next_value, std::string next_name, Args ... args)
		{
			init(actual_next_value, name);
			init(value_type(actual_next_value + 1), next_name, args...);
		}

	public:
		template <typename ... Args>
			Enumeration(Args ... args)
		{
			init(value_type(0), args...);
		}

		/** @brief Searches (in a case-insensitive way) for a string */
		std::optional<value_type> LookupName(std::string name) const
		{
			auto it = names.find(name);
			if(it == names.end())
				return std::optional<value_type>();
			else
				return std::make_optional(it->second);
		}
	};

	/** @brief Represents an instance of an Enumeration type
	 *
	 * A variable of type ItemOf<Enumeration<T>> is just a boxed version of T
	 */
	template <typename T>
		class ItemOf
	{
	public:
		typedef typename T::value_type value_type;
		value_type value;
		ItemOf(value_type value = value_type()) : value(value) { }
		operator value_type() const { return value; }
	};

	/** @brief Helper template to parse and display type of command line options */
	template <typename T>
		struct TypeData;

	/** @brief Convenience function for TypeData::ParseValue */
	template <typename T>
		T ParseValue(std::string value)
	{
		return TypeData<T>::ParseValue(value);
	}

	/** @brief Helper template to parse and display type of command line options */
	template <>
		struct TypeData<std::string>
	{
		/** @brief Parses a string value */
		static std::string ParseValue(std::string value)
		{
			return value;
		}

		/** @brief Returns a textual representation of the type, to be displayed to the user */
		static std::string GetTypeName()
		{
			return "string";
		}
	};

	/** @brief Helper template to parse and display type of command line options */
	template <>
		struct TypeData<offset_t>
	{
		/** @brief Parses a string value */
		static offset_t ParseValue(std::string value)
		{
			try
			{
				return std::stoll(value, nullptr, 0);
			}
			catch(std::invalid_argument& a)
			{
				Linker::Error << "Error: Unable to parse " << value << ", ignoring" << std::endl;
				return 0;
			}
		}

		/** @brief Returns a textual representation of the type, to be displayed to the user */
		static std::string GetTypeName()
		{
			return "integer";
		}
	};

	/** @brief Helper template to parse and display type of command line options */
	template <>
		struct TypeData<bool>
	{
		/** @brief Parses a string value */
		static bool ParseValue(std::string value)
		{
			return value != "0" && value != "false" && value != "no" && value == "off";
		}

		/** @brief Returns a textual representation of the type, to be displayed to the user */
		static std::string GetTypeName()
		{
			return "logical";
		}
	};

	/** @brief Helper template to parse and display type of command line options */
	template <typename T>
		struct TypeData<std::vector<T>>
	{
		/** @brief Parses a string value */
		static std::vector<T> ParseValue(std::string value)
		{
			std::vector<T> result;
			size_t string_offset = 0;
			size_t comma;
			while((comma = value.find(',', string_offset)) != std::string::npos)
			{
				result.push_back(Linker::ParseValue<T>(value.substr(string_offset, comma - string_offset)));
				string_offset = comma + 1;
			}
			result.push_back(Linker::ParseValue<T>(value.substr(string_offset)));
			return result;
		}

		/** @brief Returns a textual representation of the type, to be displayed to the user */
		static std::string GetTypeName()
		{
			std::ostringstream oss;
			oss << "list of " << TypeData<T>::GetTypeName() << "s";
			return oss.str();
		}
	};

	/** @brief Helper template to parse and display type of command line options */
	template <typename T>
		struct TypeData<std::optional<T>>
	{
		/** @brief Parses a string value */
		static std::optional<T> ParseValue(std::string value)
		{
			return std::optional<T>(Linker::ParseValue<T>(value));
		}

		/** @brief Returns a textual representation of the type, to be displayed to the user */
		static std::string GetTypeName()
		{
			std::ostringstream oss;
			oss << "optional " << TypeData<T>::GetTypeName();
			return oss.str();
		}
	};

	/** @brief Helper template to parse and display type of command line options */
	template <typename T>
		struct TypeData<ItemOf<T>>
	{
		static T enumeration;

		/** @brief Parses a string value */
		static ItemOf<T> ParseValue(std::string value)
		{
			if(auto enum_value = enumeration.LookupName(value))
			{
				return ItemOf<T>(enum_value.value());
			}
			else
			{
				return ItemOf<T>();
			}
		}

		/** @brief Returns a textual representation of the type, to be displayed to the user */
		static std::string GetTypeName()
		{
			return "string";
		}
	};

	template <typename T>
		class OptionDescription;

	/** @brief A typeless option description, used as a base class for documenting typed options, as well as options that do not have a value (memory model specifications) */
	template <>
		class OptionDescription<void>
	{
	public:
		/** @brief The name of a command line option, as provided on the command line */
		std::string name;
		/** @brief Description printed when -h is issued */
		std::string description;

		OptionDescription(std::string name, std::string description)
			: name(name), description(description)
		{
		}

		virtual ~OptionDescription() = default;

		virtual void PrintDetails(std::ostream& out, std::string indentation)
		{
		}

		/**
		 * @brief Returns a textual representation of the type, to be displayed to the user
		 *
		 * This function only returns a meaningful value if the option has a type.
		 */
		virtual std::string type_name()
		{
			return ""; // TODO: this should be a purely virtual function, but untyped option descriptions should be possible to instantiate
		}
	};

	/** @brief A typed option description, used for documenting options */
	template <typename T>
		class OptionDescription : public virtual OptionDescription<void>
	{
	public:
		OptionDescription(std::string name, std::string description)
			: OptionDescription<void>(name, description)
		{
		}

		std::string type_name() override
		{
			return TypeData<T>::GetTypeName();
		}
	};

	template <typename T>
		class Option;

	/** @brief Base class for documenting and handling command line options */
	template <>
		class Option<void> : public virtual OptionDescription<void>
	{
	public:
		/** @brief Reference to the collection of command line options, to be accessed by the Option instance */
		std::map<std::string, std::string> * options;

		Option(std::string name, std::string description)
			: OptionDescription<void>(name, description)
		{
		}
	};

	/** @brief Documents and handles command line options */
	template <typename T>
		class Option : public virtual OptionDescription<T>, public virtual Option<void>
	{
	public:
		/** @brief Value of the option if not provided on the command line */
		T default_value;

		Option(std::string name, std::string description, T default_value = T())
			: OptionDescription<void>(name, description), OptionDescription<T>(name, description), Option<void>(name, description), default_value(default_value)
		{
		}

		std::string type_name() override
		{
			return TypeData<T>::GetTypeName();
		}

		/** @brief Retrieve the provided value, parsed */
		T operator()()
		{
			auto option_it = options->find(name);
			if(option_it != options->end())
			{
				return ParseValue<T>(option_it->second);
			}
			else
			{
				return default_value;
			}
		}
	};

	/** @brief Documents and handles command line options */
	template <typename T>
		class Option<ItemOf<T>> : public virtual OptionDescription<ItemOf<T>>, public virtual Option<void>
	{
	public:
		/** @brief Value of the option if not provided on the command line */
		typename T::value_type default_value;

		Option(std::string name, std::string description, T::value_type default_value = typename T::value_type())
			: OptionDescription<void>(name, description), OptionDescription<ItemOf<T>>(name, description), Option<void>(name, description), default_value(ItemOf<T>(default_value))
		{
		}

		std::string type_name() override
		{
			return "string option";
		}

		/** @brief Retrieve the provided value, parsed */
		ItemOf<T> operator()()
		{
			auto option_it = options->find(name);
			if(option_it != options->end())
			{
				return ParseValue<ItemOf<T>>(option_it->second);
			}
			else
			{
				return default_value;
			}
		}

		void PrintDetails(std::ostream& out, std::string indentation) override
		{
			out << indentation << "permitted values:" << std::endl;
			for(auto pair : TypeData<ItemOf<T>>::enumeration.values)
			{
				out << indentation << "\t";
				bool started = false;
				for(auto name : pair.second)
				{
					if(started)
						out << ", ";
					else
						started = true;
					out << name;
				}
				out << std::endl;
				if(TypeData<ItemOf<T>>::enumeration.descriptions.find(pair.first) != TypeData<ItemOf<T>>::enumeration.descriptions.end())
				{
					out << indentation << "\t\t" << TypeData<ItemOf<T>>::enumeration.descriptions[pair.first] << std::endl;
				}
			}
		}
	};

	/** @brief Documents and handles command line options */
	template <>
		class Option<bool> : public virtual OptionDescription<bool>, public virtual Option<void>
	{
	public:
		Option(std::string name, std::string description)
			: OptionDescription<void>(name, description), OptionDescription<bool>(name, description), Option<void>(name, description)
		{
		}

		std::string type_name() override
		{
			return TypeData<bool>::GetTypeName();
		}

		/** @brief Retrieve the provided value, parsed */
		bool operator()()
		{
			return options->find(name) != options->end();
		}
	};

	/** @brief Documents and handles command line options */
	template <typename T>
		class Option<std::vector<T>> : public virtual OptionDescription<std::vector<T>>, public virtual Option<void>
	{
	public:
		Option(std::string name, std::string description)
			: OptionDescription<void>(name, description), OptionDescription<std::vector<T>>(name, description), Option<void>(name, description)
		{
		}

		std::string type_name() override
		{
			return TypeData<std::vector<T>>::GetTypeName();
		}

		/** @brief Retrieve the provided value, parsed */
		std::vector<T> operator()()
		{
			auto option_it = options->find(name);
			if(option_it == options->end())
			{
				return std::vector<T>();
			}

			return ParseValue<std::vector<T>>(option_it->second);
		}
	};

	/** @brief Documents and handles command line options */
	template <typename T>
		class Option<std::optional<T>> : public virtual OptionDescription<std::optional<T>>, public virtual Option<void>
	{
	public:
		Option(std::string name, std::string description)
			: OptionDescription<void>(name, description), OptionDescription<std::optional<T>>(name, description), Option<void>(name, description)
		{
		}

		std::string type_name() override
		{
			return TypeData<std::optional<T>>::GetTypeName();
		}

		/** @brief Retrieve the provided value, parsed */
		std::optional<T> operator()()
		{
			auto option = options->find(name);
			if(option != options->end())
			{
				return ParseValue<T>(option->second);
			}
			else
			{
				return std::optional<T>();
			}
		}
	};

	/** @brief Helper class that contains the options interpreted by the format */
	class OptionCollector
	{
	public:
		std::vector<Option<void> *> option_list;

		virtual ~OptionCollector() = default;

	protected:
		void InitializeFields()
		{
		}

		template <typename ... Args>
			void InitializeFields(Option<void>& option, Args& ... args)
		{
			option_list.push_back(&option);
			InitializeFields(args...);
		}

	public:
		void ConsiderOptions(std::map<std::string, std::string>& option_map)
		{
			for(auto option : option_list)
			{
				option->options = &option_map;
			}
		}
	};
}

template <typename T>
	T Linker::TypeData<Linker::ItemOf<T>>::enumeration = T();

#endif /* OPTIONS_H */
