
#include "xpexp.h"

/* TODO: unimplemented */

using namespace Ergo;

void XPFormat::ReadFile(Linker::Reader& rd)
{
	/* TODO */
}

void XPFormat::WriteFile(Linker::Writer& wr)
{
	/* TODO */
}

std::string XPFormat::GetDefaultExtension(Linker::Module& module, std::string filename)
{
	if(stub_file != "")
	{
		return filename + ".exe";
	}
	else
	{
		return filename + ".exp";
	}
}

