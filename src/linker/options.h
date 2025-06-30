#ifndef OPTIONS_H
#define OPTIONS_H

namespace Linker
{
	template <typename T>
		struct structParseValue;

	template <typename T>
		T ParseValue(std::string value)
	{
		return structParseValue<T>::ParseValue(value);
	}

	template <>
		struct structParseValue<std::string>
	{
		static std::string ParseValue(std::string value)
		{
			return value;
		}
	};

	template <>
		struct structParseValue<offset_t>
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
	};

	template <>
		struct structParseValue<bool>
	{
		static bool ParseValue(std::string value)
		{
			return value != "0" && value != "false" && value != "no" && value == "off";
		}
	};

	template <typename T>
		struct structParseValue<std::vector<T>>
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
	};

	template <typename T>
		struct structParseValue<std::optional<T>>
	{
		static std::optional<T> ParseValue(std::string value)
		{
			return std::optional<T>(Linker::ParseValue<T>(value));
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
