
#include <set>
#include <sstream>
#include "module.h"
#include "position.h"
#include "reference.h"
#include "relocation.h"
#include "segment_manager.h"
#include "target.h"
#include "writer.h"

using namespace Linker;

Location Reference::ToLocation(Module& module) const
{
	std::shared_ptr<Section> l_section;
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
			std::ostringstream message;
			message << "Fatal error: Undefined symbol " << *stringp << ", aborting";
			Linker::FatalError(message.str());
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

