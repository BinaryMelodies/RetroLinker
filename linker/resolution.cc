
#include "resolution.h"

using namespace Linker;

std::ostream& Linker::operator<<(std::ostream& out, const Resolution& resolution)
{
	out << "resolution(" << std::hex << resolution.value << std::dec;
	if(resolution.target)
	{
		out << " to target " << *resolution.target;
	}
	if(resolution.reference)
	{
		out << " wrt reference " << *resolution.reference;
	}
	out << ")";
	return out;
}

