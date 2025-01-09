#ifndef MODULE_H
#define MODULE_H

#include <map>
#include <memory>
#include <vector>
#include <string>
#include "../common.h"
#include "location.h"
#include "relocation.h"

namespace Linker
{
	class Section;

	/**
	 * @brief Encodes an object module file as a collection of sections, symbols and relocations
	 */
	class Module
	{
	public:
		/**
		 * @brief Supported CPU types
		 */
		enum cpu_type
		{
			NONE,
			I80, /* also Z80 */
			I86,
			I386,
			X86_64,
			M6800,
			M6809,
			M68K,
			MOS6502,
			W65K,
			Z8K,
			PPC,
			PPC64,
			ARM,
			ARM64,
			PDP11,
			MIPS,
			SPARC,
			SH,
		};

		/**
		 * @brief Encodes the CPU for the target
		 */
		cpu_type cpu = NONE;
	private:
		std::vector<std::shared_ptr<Section>> sections;
		std::map<std::string, std::shared_ptr<Section>> section_names;
		std::map<std::string, Location> symbols;
		std::map<std::string, Location> local_symbols;
		std::map<std::string, CommonSymbol> unallocated_symbols;
		std::vector<SymbolName> imported_symbols;
		std::map<ExportedSymbol, Location> exported_symbols;
	public:
		/**
		 * @brief List of relocations within the module
		 */
		std::vector<Relocation> relocations;

		/**
		 * @brief Initializes the reader for linking purposes
		 * @param special_char Most input formats do not provide support for the special requirements of the output format (such as segmentation for ELF). We work around this by introducing special name prefixes $$SEGOF$ where $ is the value of special_char.
		 * @param output_format The output format that will be used. This is required to know which extra special features need to be implemented (such as segmentation).
		 * @param input_format The input format that will be used. This is required to know which extra special features need to be implemented (such as segmentation).
		 */
		void SetupOptions(char special_char, std::shared_ptr<OutputFormat> output_format, std::shared_ptr<InputFormat> input_format);

	private:
		/* GNU assembler can use '$', NASM must use '?' */
		char special_prefix_char = '$';
		std::weak_ptr<Linker::OutputFormat> output_format;
		std::weak_ptr<Linker::InputFormat> input_format;

		std::string export_prefix();

		bool parse_exported_name(std::string reference_name, Linker::ExportedSymbol& symbol);

	public:
		/**
		 * @brief Adds an internal symbol
		 */
		void AddLocalSymbol(std::string name, Location location);

		/**
		 * @brief Adds an exported symbol
		 */
		void AddGlobalSymbol(std::string name, Location location);
	private:
		void _AddGlobalSymbol(std::string name, Location location);

	public:
		/**
		 * @brief Adds a common symbol
		 */
		void AddCommonSymbol(std::string name, CommonSymbol symbol);

		/**
		 * @brief Adds an imported symbol
		 */
		void AddImportedSymbol(SymbolName name);

		/**
		 * @brief Adds an exported symbol
		 */
		void AddExportedSymbol(ExportedSymbol name, Location symbol);

		/**
		 * @brief Retrieves list of all imported symbols
		 */
		const std::vector<SymbolName>& GetImportedSymbols() const;

		/**
		 * @brief Retrieves map of all exported symbols and their locations
		 */
		const std::map<ExportedSymbol, Location>& GetExportedSymbols() const;

		/**
		 * @brief Searches for a local symbol
		 */
		bool FindLocalSymbol(std::string name, Location& location);

		/**
		 * @brief Searches for a global symbol
		 */
		bool FindGlobalSymbol(std::string name, Location& location);

		/**
		 * @brief Adds a new section
		 */
		void AddSection(std::shared_ptr<Section> section);

		/**
		 * @brief Retrieves list of all sections
		 */
		const std::vector<std::shared_ptr<Section>>& Sections() const;

		/**
		 * @brief Deletes a specific sections
		 */
		void DeleteSection(size_t index);

		/**
		 * @brief Removes all sections from internal list, without deleting them
		 */
		void RemoveSections();

		/**
		 * @brief Searches for a section with a specific name
		 */
		std::shared_ptr<Section> FindSection(std::string name);

		/**
		 * @brief Searches or creates a section with a specific name, with the assigned flags
		 */
		std::shared_ptr<Section> FetchSection(std::string name, unsigned default_flags);

		/**
		 * @brief Attempts to resolve the targets of all relocations
		 */
		void ResolveRelocations();

		/**
		 * @brief Appends two of its sections
		 */
		void Append(std::shared_ptr<Section> dst, std::shared_ptr<Section> src);

		/**
		 * @brief Appends another module object, merging identically named sections
		 */
		void Append(Module& other);

		/**
		 * @brief All common symbols are converted to global symbols and assigned addresses within a section
		 */
		void AllocateSymbols(std::shared_ptr<Section> section);

		/**
		 * @brief All common symbols are converted to global symbols and assigned addresses within a ".comm" section
		 */
		void AllocateSymbols();
	};
}

#endif /* MODULE_H */
