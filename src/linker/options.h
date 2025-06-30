#ifndef OPTIONS_H
#define OPTIONS_H

#include <sstream>

namespace Linker
{
	template <typename T>
		struct TypeData;

	template <typename T>
		T ParseValue(std::string value)
	{
		return TypeData<T>::ParseValue(value);
	}

	template <>
		struct TypeData<std::string>
	{
		static std::string ParseValue(std::string value)
		{
			return value;
		}

		static std::string GetTypeName()
		{
			return "string";
		}
	};

	template <>
		struct TypeData<offset_t>
	{
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

		static std::string GetTypeName()
		{
			return "integer";
		}
	};

	template <>
		struct TypeData<bool>
	{
		static bool ParseValue(std::string value)
		{
			return value != "0" && value != "false" && value != "no" && value == "off";
		}

		static std::string GetTypeName()
		{
			return "logical";
		}
	};

	template <typename T>
		struct TypeData<std::vector<T>>
	{
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

		static std::string GetTypeName()
		{
			std::ostringstream oss;
			oss << "list of " << TypeData<T>::GetTypeName() << "s";
			return oss.str();
		}
	};

	template <typename T>
		struct TypeData<std::optional<T>>
	{
		static std::optional<T> ParseValue(std::string value)
		{
			return std::optional<T>(Linker::ParseValue<T>(value));
		}

		static std::string GetTypeName()
		{
			std::ostringstream oss;
			oss << "optional " << TypeData<T>::GetTypeName();
			return oss.str();
		}
	};

	template <typename T>
		class Option;

	template <>
		class Option<void>
	{
	public:
		std::map<std::string, std::string> * options;

		std::string name;
		std::string description;

		Option(std::string name, std::string description)
			: name(name), description(description)
		{
		}

		virtual std::string type_name() = 0;

		virtual ~Option() = default;
	};

	template <typename T>
		class Option : public Option<void>
	{
	public:
		T default_value;

		Option(std::string name, std::string description, T default_value = T())
			: Option<void>(name, description), default_value(default_value)
		{
		}

		std::string type_name() override
		{
			return TypeData<T>::GetTypeName();
		}

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

	template <>
		class Option<bool> : public Option<void>
	{
	public:
		Option(std::string name, std::string description)
			: Option<void>(name, description)
		{
		}

		std::string type_name() override
		{
			return TypeData<bool>::GetTypeName();
		}

		bool operator()()
		{
			return options->find(name) != options->end();
		}
	};

	template <typename T>
		class Option<std::vector<T>> : public Option<void>
	{
	public:
		Option(std::string name, std::string description)
			: Option<void>(name, description)
		{
		}

		std::string type_name() override
		{
			return TypeData<std::vector<T>>::GetTypeName();
		}

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

	template <typename T>
		class Option<std::optional<T>> : public Option<void>
	{
	public:
		Option(std::string name, std::string description)
			: Option<void>(name, description)
		{
		}

		std::string type_name() override
		{
			return TypeData<std::optional<T>>::GetTypeName();
		}

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
			void InitializeFields(Option<void>& option, Args ... args)
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
