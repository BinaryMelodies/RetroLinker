
#include "bflt.h"
#include "../linker/location.h"

using namespace BFLT;

void BFLTFormat::Clear()
{
	/* TODO */
}

void BFLTFormat::ReadFile(Linker::Reader& rd)
{
	/* TODO */
}

offset_t BFLTFormat::WriteFile(Linker::Writer& wr) const
{
	/* TODO */
	return 0;
}

void BFLTFormat::Dump(Dumper::Dumper& dump) const
{
	/* TODO */
}

void BFLTFormat::CalculateValues()
{
	/* TODO */
}

/* * * Writer members * * */

void BFLTFormat::SetModel(std::string model)
{
	/* TODO */
}

void BFLTFormat::SetOptions(std::map<std::string, std::string>& options)
{
	/* TODO */
}

void BFLTFormat::OnNewSegment(std::shared_ptr<Linker::Segment> segment)
{
	/* TODO */
}

void BFLTFormat::ProcessModule(Linker::Module& module)
{
	/* TODO */
}

void BFLTFormat::GenerateFile(std::string filename, Linker::Module& module)
{
	/* TODO */
}

std::string BFLTFormat::GetDefaultExtension(Linker::Module& module, std::string filename) const
{
	/* TODO */
	return "";
}

