
#include "target.h"
#include "module.h"
#include "position.h"

using namespace Linker;

Target Target::GetSegment()
{
	return Target(target, true);
}

bool Target::Displace(const Displacement& displacement)
{
	if(Location * locationp = std::get_if<Location>(&target))
	{
		return locationp->Displace(displacement);
	}
	return false;
}

bool Target::ResolveLocals(Module& object)
{
	if(SymbolName * symbolp = std::get_if<SymbolName>(&target))
	{
		std::string symbol_name;
		if(symbolp->GetLocalName(symbol_name))
		{
			Location location;
			if(!object.FindLocalSymbol(symbol_name, location))
				return false;
			location.offset += symbolp->addend;
			Linker::Debug << "Debug: Resolved " << target << " to " << location << std::endl;
			target = location;
			return true;
		}
		else
		{
			return false;
		}
	}
	return true;
}

bool Target::Lookup(Module& object, Position& position)
{
	if(Location * locationp = std::get_if<Location>(&target))
	{
		position = locationp->GetPosition(segment_of);
		return true;
	}
	else if(SymbolName * symbolp = std::get_if<SymbolName>(&target))
	{
		std::string symbol_name;
		if(symbolp->GetLocalName(symbol_name))
		{
			Location location;
			if(!object.FindGlobalSymbol(symbol_name, location))
				return false;
			location.offset += symbolp->addend;
			position = location.GetPosition(segment_of);
//			Linker::Debug << "Look up " << *symbolp << ", get " << location << " which is " << position << std::endl;
			return true;
		}
		else
		{
			return false;
		}
	}
	Linker::FatalError("Internal error: invalid target type");
}

bool Linker::operator==(const Target& a, const Target& b)
{
	return a.target == b.target && a.segment_of == b.segment_of;
}

bool Linker::operator!=(const Target& a, const Target& b)
{
	return !(a == b);
}

std::ostream& Linker::operator<<(std::ostream& out, const Target& target)
{
	out << "target ";
	if(target.segment_of)
		out << "segment of ";
	if(const Location * locationp = std::get_if<Location>(&target.target))
	{
		out << *locationp;
	}
	else if(const SymbolName * symbolp = std::get_if<SymbolName>(&target.target))
	{
		out << *symbolp;
	}
	return out;
}

