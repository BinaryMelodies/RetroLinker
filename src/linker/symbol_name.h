#ifndef SYMBOL_NAME_H
#define SYMBOL_NAME_H

#include <iostream>
#include <optional>
#include <string>
#include "../common.h"

namespace Linker
{
	/**
	 * @brief Represents an (imported or internal) symbol name, which can be more complex than a string
	 *
	 * When referencing symbols, usually a string identifier is sufficient.
	 * Some output formats, in particular the NE, LE and PE formats, need to store more information with the symbol, including the library it is imported from, and potentially a 16-bit integer ordinal or hint.
	 */
	class SymbolName
	{
	protected:
		std::optional<std::string> library;
			/* internal symbols: empty, imported symbols: refers to the library (NE, LE, PE) */
		std::optional<std::string> name;
			/* symbols imported by ordinal: empty (NE, LE, PE), other symbols: the name of the symbol */
		std::optional<uint16_t> hint;
			/* symbols imported not by ordinal: empty (NE, LE), except PE where it is a hint, other symbols: ordinal */

	public:
		/** @brief Optional value to be added to the symbol location (TODO: not yet fully implemented) */
		offset_t addend = 0;

		/**
		 * @brief Creates an internal symbol with a name
		 */
		SymbolName(std::string name)
			: name(name)
		{
		}

		/**
		 * @brief Creates a symbol imported via name, from a library
		 */
		SymbolName(std::string library, std::string name)
			: library(library), name(name)
		{
		}

		/**
		 * @brief Creates a symbol imported via name and a hint, from a library
		 *
		 * This is expected to be used for the PE format.
		 */
		SymbolName(std::string library, std::string name, uint16_t hint)
			: library(library), name(name), hint(hint)
		{
		}

		/**
		 * @brief Creates a symbol imported via ordinal, from a library
		 *
		 * This is used by formats like NE, LE, PE.
		 */
		SymbolName(std::string library, uint16_t ordinal)
			: library(library), hint(ordinal)
		{
		}

		/**
		 * @brief Retrieves the name of the symbol, if it has one
		 */
		bool LoadName(std::string& result) const;

		/**
		 * @brief Retrieves the name of the library, if it is imported
		 */
		bool LoadLibraryName(std::string& result) const;

		/**
		 * @brief Retrieves the ordinal of symbols imported by ordinal, or the hint for imported symbols with a hint
		 */
		bool LoadOrdinalOrHint(uint16_t& result) const;

		/**
		 * @brief For local symbols, returns the name
		 */
		bool GetLocalName(std::string& result) const;

		/**
		 * @brief For symbols imported by name, returns the library and name
		 */
		bool GetImportedName(std::string& result_library, std::string& result_name) const;

		/**
		 * @brief For symbols imported by name, returns the library, name and hint (or zero if no hint is present)
		 */
		bool GetImportedName(std::string& result_library, std::string& result_name, uint16_t& result_hint) const;

		/**
		 * @brief For symbols imported by ordinal, returns the library and ordinal
		 */
		bool GetImportedOrdinal(std::string& result_library, uint16_t& result_ordinal) const;

		/**
		 * @brief Compares two symbols for equality
		 */
		bool operator ==(const SymbolName& other) const;

		/**
		 * @brief Compares two symbols for inequality
		 */
		bool operator !=(const SymbolName& other) const;

		/**
		 * @brief Symbol representing the global offset table
		 */
		static SymbolName GOT;
	};

	/**
	 * @brief Displays an internal or imported symbol is a nice way
	 */
	std::ostream& operator<<(std::ostream& out, const SymbolName& symbol);

	/**
	 * @brief Represents a symbol to be exported from the module
	 */
	class ExportedSymbolName
	{
	protected:
		bool by_ordinal;
		std::string name;
		std::optional<uint16_t> ordinal;

	public:
		/**
		 * @brief Creates a symbol exported by name
		 */
		ExportedSymbolName(std::string name)
			: by_ordinal(false), name(name), ordinal()
		{
		}

		/**
		 * @brief Creates a symbol exported by name, with a corresponding hint
		 *
		 * This is used exclusively for PE.
		 */
		ExportedSymbolName(std::string name, uint16_t hint)
			: by_ordinal(false), name(name), ordinal(hint)
		{
		}

		/**
		 * @brief Creates a symbol exported by ordinal, with an associated internal name
		 */
		ExportedSymbolName(uint16_t ordinal, std::string internal_name)
			: by_ordinal(true), name(internal_name), ordinal(ordinal)
		{
		}

		bool IsExportedByOrdinal() const;

		/**
		 * @brief Returns the name of the symbol
		 */
		bool LoadName(std::string& result) const;

		/**
		 * @brief Returns the hint or ordinal of the symbol
		 */
		bool LoadOrdinalOrHint(uint16_t& result) const;

		/**
		 * @brief For symbols exported by name, returns the name
		 */
		bool GetExportedByName(std::string& result) const;

		/**
		 * @brief For symbols exported by name, returns the name and hint (or zero)
		 */
		bool GetExportedByName(std::string& result, uint16_t& hint) const;

		/**
		 * @brief For symbols exported by ordinal, returns the ordinal
		 */
		bool GetExportedByOrdinal(uint16_t& result) const;

		/**
		 * @brief For symbols exported by ordinal, returns the ordinal and internal name
		 */
		bool GetExportedByOrdinal(uint16_t& result, std::string& result_name) const;

		/**
		 * @brief Compares two symbols for equality
		 */
		bool operator ==(const ExportedSymbolName& other) const;

		/**
		 * @brief Compares two symbols for inequality
		 */
		bool operator !=(const ExportedSymbolName& other) const;

		/**
		 * @brief Compares two symbols for ordering
		 */
		bool operator <(const ExportedSymbolName& other) const;

		/**
		 * @brief Compares two symbols for ordering
		 */
		bool operator >=(const ExportedSymbolName& other) const;

		/**
		 * @brief Compares two symbols for ordering
		 */
		bool operator >(const ExportedSymbolName& other) const;

		/**
		 * @brief Compares two symbols for ordering
		 */
		bool operator <=(const ExportedSymbolName& other) const;
	};

	/**
	 * @brief Displays an exported symbol is a nice way
	 */
	std::ostream& operator<<(std::ostream& out, const ExportedSymbolName& symbol);
}

#endif /* SYMBOL_NAME_H */
