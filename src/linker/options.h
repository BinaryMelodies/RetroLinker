#ifndef OPTIONS_H
#define OPTIONS_H

#include <optional>
#include <sstream>
#include <string>
#include <vector>
#include "../common.h"

namespace Linker
{
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

#endif /* OPTIONS_H */
