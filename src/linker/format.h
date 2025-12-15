#ifndef FORMAT_H
#define FORMAT_H

#include <map>
#include <optional>
#include <string>
#include "../common.h"
#include "image.h"

namespace Dumper
{
	class Dumper;
}

namespace Linker
{
	class Module;
	class ModuleCollector;
	class OptionCollector;
	template <typename T>
		class OptionDescription;
	class Reader;
	class Writer;

	/**
	 * @brief A class to encode a general file format
	 */
	class Format : public virtual Contents
	{
	public:
		offset_t file_offset;

		Format(offset_t file_offset = 0)
			: file_offset(file_offset)
		{
		}

		virtual ~Format() = default;
		/**
		 * @brief Resets all fields to their default values, deallocate memory
		 */
		virtual void Clear();
		/**
		 * @brief Loads file into memory
		 */
		virtual void ReadFile(Reader& rd) = 0;
		/**
		 * @brief Stores data in memory to file
		 */
		offset_t WriteFile(Writer& wr) const override = 0;
		/**
		 * @brief Display file contents in a nice manner
		 */
		virtual void Dump(Dumper::Dumper& dump) const;

		offset_t ImageSize() const override;
		offset_t WriteFile(Writer& wr, offset_t count, offset_t offset = 0) const override;
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
		 * @brief Returns a list of the supported memory models, used for documentation
		 */
		virtual std::vector<OptionDescription<void>> GetMemoryModelNames();
		/**
		 * @brief Returns a list of the parameters used in the linker scripts, used for documentation
		 */
		virtual std::vector<OptionDescription<void> *> GetLinkerScriptParameterNames();
		/**
		 * @brief Returns a list of special symbol names recognized by the format, used for documentation
		 */
		virtual std::vector<OptionDescription<void>> GetSpecialSymbolNames();
		/**
		 * @brief Returns object containing a sequence of option fields provided with the -S command line flag
		 */
		virtual std::shared_ptr<OptionCollector> GetOptions();
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
		virtual void ProcessModule(Module& object);
		/**
		 * @brief Intermediate step between processing module and generating output file to set up headers and management sections
		 * It is expected that after a module is processed, additional steps are required to evaluate the final values of the fields.
		 */
		virtual void CalculateValues();
		/**
		 * @brief The main function that handles processing, calculating and generating the final image
		 */
		virtual void GenerateFile(std::string filename, Module& module);
		/**
		 * @brief Appends a default extension to the filename
		 *
		 * A typical behavior would be to append .exe at the end of the filename.
		 * The default action is to leave it intact.
		 */
		virtual std::string GetDefaultExtension(Module& module, std::string filename) const;
		/**
		 * @brief Provides a default filename for the output file
		 *
		 * Typically a.out or some other extension, such as a.exe.
		 */
		virtual std::string GetDefaultExtension(Module& module) const;
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
		 * @brief Whether the format is in protected mode or not (x86 only)
		 *
		 * This is needed for the ELF parser to determine whether segment references are paragraph offsets (16-byte units) or selector indexes.
		 *
		 * For non-x86 targets, the result is meaningless.
		 */
		virtual bool FormatIsProtectedMode() const;
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

		/**
		 * @brief Instructs the module to allocate any unallocated local symbols
		 */
		virtual void AllocateSymbols(Module& module) const;
	};

	/**
	 * @brief A class that provides a general interface to loading a module
	 */
	class InputFormat : public virtual Format, public std::enable_shared_from_this<InputFormat>
	{
	public:
		/**
		 * @brief Initializes the reader for linking purposes
		 * @param format The output format that will be used. This is required to know which extra special features need to be implemented (such as segmentation).
		 */
		virtual void SetupOptions(std::shared_ptr<OutputFormat> format);
		/**
		 * @brief Reads a file and loads the information into a module object
		 */
		virtual void ProduceModule(ModuleCollector& linker, Reader& rd, std::string file_name);
		/**
		 * @brief Reads a file and loads the information into a module object, a convenience method when there is a single module generated
		 */
		virtual void ProduceModule(Module& module, Reader& rd);
		/**
		 * @brief Loads the information into a module object
		 */
		virtual void GenerateModule(ModuleCollector& linker, std::string file_name, bool is_library = false) const;
		/**
		 * @brief Loads the information into a module object, a convenience method when there is a single module generated
		 */
		virtual void GenerateModule(Module& module) const;
	public:
		/* x86 only */
		/**
		 * @brief Whether the format enables multiple x86 segments
		 *
		 * This is typically true for Intel 8086 targets and false for non-Intel targets.
		 * The ELF parser uses this to provide extended relocations, including the following:
		 * - $$SEG$<section name>
		 * - $$SEGOF$<symbol name>
		 * - $$SEGAT$<symbol name>
		 * - $$WRTSEG$<symbol name>$<section name>
		 * - $$SEGDIF$<section name>$<section name>
		 */
		virtual bool FormatProvidesSegmentation() const;
		/* w65 only */
		/**
		 * @brief Whether the generated file might contain bugs that require fixing
		 */
		virtual bool FormatRequiresDataStreamFix() const;
		/* general */
		/**
		 * @brief Whether the format supports resources
		 *
		 * Formats such as NE, LE/LX, PE and the Macintosh classic support including resources in the final binary.
		 * To simplify writing resources, the ELF parser permits incorporating them directly in the binary image as $$RSRC$_<type>$<id>.
		 */
		virtual bool FormatProvidesResources() const;
		/**
		 * @brief Whether the format enables importing/exporting libraries
		 */
		virtual bool FormatProvidesLibraries() const;
	};
}

#endif /* FORMAT_H */
