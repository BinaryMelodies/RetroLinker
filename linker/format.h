#ifndef FORMAT_H
#define FORMAT_H

#include <map>
#include <optional>
#include <string>
#include "../common.h"
#include "reader.h"
#include "writer.h"

namespace Dumper
{
	class Dumper;
}

namespace Linker
{
	class Module;

	/**
	 * @brief A class to encode a general file format
	 */
	class Format
	{
	public:
		offset_t file_offset;

		Format(offset_t file_offset = 0)
			: file_offset(file_offset)
		{
		}

		virtual ~Format();
		/**
		 * @brief Sets all fields to their default values
		 */
		virtual void Initialize();
		/**
		 * @brief Resets all fields to their default values, deallocate memory
		 */
		virtual void Clear();
		/**
		 * @brief Loads file into memory
		 */
		virtual void ReadFile(Reader& in) = 0;
		/**
		 * @brief Stores data in memory to file
		 */
		virtual void WriteFile(Writer& out) = 0;
		/**
		 * @brief Display file contents in a nice manner
		 */
		virtual void Dump(Dumper::Dumper& dump);
	};

	/**
	 * @brief A class that provides a general interface to setting up generation for a format
	 */
	class OutputFormat : public virtual Format
	{
	public:
		/**
		 * @brief If the output format actually drives multiple output formats (resource file, apple double, etc.), specify multiple types, return false if unknown
		 */
		virtual bool AddSupplementaryOutputFormat(std::string subformat);
		/**
		 * @brief Passes command line parameters as settings over to format object
		 */
		virtual void SetOptions(std::map<std::string, std::string>& options);

		/**
		 * @brief Convenience method to look up option by name
		 */
		std::optional<std::string> FetchOption(std::map<std::string, std::string>& options, std::string name);

		/**
		 * @brief Convenience method to look up option by name, returning default value if name is missing
		 */
		std::string FetchOption(std::map<std::string, std::string>& options, std::string name, std::string default_value);

		/**
		 * @brief Convenience method to look up option by name and convert it to integer
		 */
		std::optional<offset_t> FetchIntegerOption(std::map<std::string, std::string>& options, std::string name);

		/**
		 * @brief Sets the way memory is organized, typically modifying a built-in script
		 */
		virtual void SetModel(std::string model);
		/**
		 * @brief Selects a script file to use for linking
		 */
		virtual void SetLinkScript(std::string script_file, std::map<std::string, std::string>& options);
		/**
		 * @brief Processes the module object and initializes format fields
		 */
		virtual void ProcessModule(Linker::Module& object);
		/**
		 * @brief Intermediate step between processing module and generating output file to set up headers and management sections
		 * It is expected that after a module is processed, additional steps are required to evaluate the final values of the fields.
		 */
		virtual void CalculateValues();
		/**
		 * @brief The main function that handles processing, calculating and generating the final image
		 */
		virtual void GenerateFile(std::string filename, Linker::Module& module);
		/**
		 * @brief Appends a default extension to the filename
		 *
		 * A typical behavior would be to append .exe at the end of the filename.
		 * The default action is to leave it intact.
		 */
		virtual std::string GetDefaultExtension(Linker::Module& module, std::string filename);
		/**
		 * @brief Provides a default filename for the output file
		 *
		 * Typically a.out or some other extension, such as a.exe.
		 */
		virtual std::string GetDefaultExtension(Linker::Module& module);
		/* x86 only */
		/**
		 * @brief Whether the format supports multiple segments
		 *
		 * This is typically true for Intel 8086 targets and false for non-Intel targets.
		 * The ELF parser uses this to provide extended relocations, including the following:
		 * - $$SEG$<section name>
		 * - $$SEGOF$<symbol name>
		 * - $$SEGAT$<symbol name>
		 * - $$WRTSEG$<symbol name>$<section name>
		 * - $$SEGDIF$<section name>$<section name>
		 */
		virtual bool FormatSupportsSegmentation() const;
		/**
		 * @brief Whether the format is 16-bit or not
		 *
		 * This is needed for the ELF parser which can not distinguish between the 8086 and 80386 backends.
		 */
		virtual bool FormatIs16bit() const;
		/**
		 * @brief Whether the address space is linear or segmented
		 *
		 * This is needed to determine whether symbol relocations are absolute addresses or offsets within their own segments.
		 * Typically, Intel 8086 backends are non-linear, the others are linear.
		 */
		virtual bool FormatIsLinear() const;
		/* general */
		/**
		 * @brief Whether the format supports resources
		 *
		 * Formats such as NE, LE/LX, PE and the Macintosh classic support including resources in the final binary.
		 * To simplify writing resources, the ELF parser permits incorporating them directly in the binary image as $$RSRC$_<type>$<id>.
		 */
		virtual bool FormatSupportsResources() const;
		/**
		 * @brief Whether the format supports libraries
		 */
		virtual bool FormatSupportsLibraries() const; /* imports/exports, fileformat level support, not system support */
		virtual unsigned FormatAdditionalSectionFlags(std::string section_name) const;
	};

	/**
	 * @brief A class that provides a general interface to loading a module
	 */
	class InputFormat : public virtual Format
	{
	public:
		/**
		 * @brief Initializes the reader for linking purposes
		 * @param special_char Most input formats do not provide support for the special requirements of the output format (such as segmentation for ELF). We work around this by introducing special name prefixes $$SEGOF$ where $ is the value of special_char.
		 * @param format The output format that will be used. This is required to know which extra special features need to be implemented (such as segmentation).
		 */
		virtual void SetupOptions(char special_char, OutputFormat * format);
		/**
		 * @brief Reads a file and loads the information into a module object
		 */
		virtual void ProduceModule(Module& module, Reader& rd) = 0;
	};
}

#endif /* FORMAT_H */
