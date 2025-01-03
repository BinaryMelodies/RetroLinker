
#include <set>
#include "linker.h"
#include "module.h"
#include "position.h"
#include "reference.h"
#include "relocation.h"
#include "target.h"
#include "writer.h"

using namespace Linker;

Location Reference::ToLocation(Module& module) const
{
	Section * l_section;
	offset_t l_offset = 0;
	if(segment)
		l_section = module.FindSection(*segment);
	else
		l_section = nullptr;
	if(const std::string * stringp = std::get_if<std::string>(&offset))
	{
		Location location;
		if(!module.FindGlobalSymbol(*stringp, location))
		{
			Linker::Error << "Fatal error: Undefined symbol " << *stringp << ", aborting" << std::endl;
			exit(1);
		}
		if(l_section != nullptr && location.section != nullptr && l_section != location.section)
		{
			Linker::Error << "Error: symbol is defined as an offset to a symbol in a different segment, silently ignoring" << std::endl;
		}
		return location;
	}
	else if(const offset_t * offsetp = std::get_if<offset_t>(&offset))
		l_offset = *offsetp;
	return Location(l_section, l_offset);
}

std::ostream& Linker::operator<<(std::ostream& out, const Reference& ref)
{
	if(ref.segment)
		out << *ref.segment << ":";
	if(const std::string * stringp = std::get_if<std::string>(&ref.offset))
		out << *stringp;
	else if(const offset_t * offsetp = std::get_if<offset_t>(&ref.offset))
		out << *offsetp;
	return out;
}

