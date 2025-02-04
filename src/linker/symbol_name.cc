
#include "symbol_name.h"

using namespace Linker;

bool SymbolName::LoadName(std::string& result) const
{
	if(name)
	{
		result = *name;
		return true;
	}
	else
	{
		return false;
	}
}

bool SymbolName::LoadLibraryName(std::string& result) const
{
	if(library)
	{
		result = *library;
		return true;
	}
	else
	{
		return false;
	}
}

bool SymbolName::LoadOrdinalOrHint(uint16_t& result) const
{
	if(hint)
	{
		result = *hint;
		return true;
	}
	else
	{
		return false;
	}
}

bool SymbolName::GetLocalName(std::string& result) const
{
	if(!library)
	{
		assert(name);
		assert(!hint);
		result = *name;
		return true;
	}
	else
	{
		return false;
	}
}

bool SymbolName::GetImportedName(std::string& result_library, std::string& result_name) const
{
	if(library && name)
	{
		result_library = *library;
		result_name = *name;
		return true;
	}
	else
	{
		return false;
	}
}

bool SymbolName::GetImportedName(std::string& result_library, std::string& result_name, uint16_t& result_hint) const
{
	if(library && name)
	{
		result_library = *library;
		result_name = *name;
		if(hint)
			result_hint = *hint;
		else
			result_hint = 0;
		return true;
	}
	else
	{
		return false;
	}
}

bool SymbolName::GetImportedOrdinal(std::string& result_library, uint16_t& result_ordinal) const
{
	if(library && !name)
	{
		assert(hint);
		result_library = *library;
		result_ordinal = *hint;
		return true;
	}
	else
	{
		return false;
	}
}

bool SymbolName::operator ==(const SymbolName& other) const
{
	return library == other.library && name == other.name && hint == other.hint;
}

bool SymbolName::operator !=(const SymbolName& other) const
{
	return !(*this == other);
}

std::ostream& Linker::operator<<(std::ostream& out, const SymbolName& symbol)
{
	std::string name, library;
	uint16_t hint;
	if(symbol.GetLocalName(name))
	{
		out << "symbol " << name;
	}
	else if(symbol.GetImportedName(library, name))
	{
		out << "symbol(library " << library << " name " << name;
		if(symbol.LoadOrdinalOrHint(hint))
			out << " hint " << hint;
		out << ")";
	}
	else if(symbol.GetImportedOrdinal(library, hint))
	{
		out << "symbol(library " << library << " ordinal " << hint << ")";
	}
	else
	{
		Linker::FatalError("Internal error: invalid symbol type");
	}
	return out;
}

bool ExportedSymbolName::IsExportedByOrdinal() const
{
	return by_ordinal;
}

bool ExportedSymbolName::LoadName(std::string& result) const
{
	result = name;
	return true;
}

bool ExportedSymbolName::LoadOrdinalOrHint(uint16_t& result) const
{
	if(ordinal)
	{
		result = *ordinal;
		return true;
	}
	else
	{
		return false;
	}
}

bool ExportedSymbolName::GetExportedByName(std::string& result) const
{
	if(!by_ordinal)
	{
		result = name;
		return true;
	}
	else
	{
		return false;
	}
}

bool ExportedSymbolName::GetExportedByName(std::string& result, uint16_t& hint) const
{
	if(!by_ordinal)
	{
		result = name;
		if(ordinal)
			hint = *ordinal;
		else
			hint = 0;
		return true;
	}
	else
	{
		return false;
	}
}

bool ExportedSymbolName::GetExportedByOrdinal(uint16_t& result) const
{
	if(by_ordinal)
	{
		assert(ordinal);
		result = *ordinal;
		return true;
	}
	else
	{
		return false;
	}
}

bool ExportedSymbolName::GetExportedByOrdinal(uint16_t& result, std::string& result_name) const
{
	if(by_ordinal)
	{
		assert(ordinal);
		result = *ordinal;
		result_name = name;
		return true;
	}
	else
	{
		return false;
	}
}

bool ExportedSymbolName::operator ==(const ExportedSymbolName& other) const
{
	return by_ordinal == other.by_ordinal && name == other.name && ordinal == other.ordinal;
}

bool ExportedSymbolName::operator !=(const ExportedSymbolName& other) const
{
	return !(*this == other);
}

bool ExportedSymbolName::operator <(const ExportedSymbolName& other) const
{
	return (by_ordinal && !other.by_ordinal)
		|| (by_ordinal == other.by_ordinal &&
			(name < other.name
			|| (name == other.name && ordinal < other.ordinal)));
}

bool ExportedSymbolName::operator >=(const ExportedSymbolName& other) const
{
	return !(*this < other);
}

bool ExportedSymbolName::operator >(const ExportedSymbolName& other) const
{
	return other < *this;
}

bool ExportedSymbolName::operator <=(const ExportedSymbolName& other) const
{
	return !(other < *this);
}

std::ostream& Linker::operator<<(std::ostream& out, const ExportedSymbolName& symbol)
{
	std::string name;
	uint16_t hint;
	if(symbol.GetExportedByName(name))
	{
		out << "export name " << name;
		if(symbol.LoadOrdinalOrHint(hint))
			out << " hint " << hint;
	}
	else if(symbol.GetExportedByOrdinal(hint, name))
	{
		out << "export ordinal " << hint << " name " << name;
	}
	else
	{
		Linker::FatalError("Internal error: invalid symbol type");
	}
	return out;
}

