
#include "cwexe.h"
#include "../dumper/dumper.h"
#include "../linker/location.h"

using namespace CauseWay;

void CauseWayFormat::ReadFile(const std::shared_ptr<Linker::Reader>& rd)
{
	// TODO
}

offset_t CauseWayFormat::WriteFile(const std::shared_ptr<Linker::Writer>& wr) const
{
	// TODO
	return offset_t(-1);
}

void CauseWayFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_cp437);

	dump.SetTitle("3P format");

	Dumper::Region file_region("File", file_offset, 0 /* TODO */, 8);
	// TODO
	file_region.Display(dump, Dumper::Header);
}

void CauseWayFormat::CalculateValues()
{
	// TODO
}

std::string CauseWayFormat::GetDefaultExtension(Linker::Module& module, std::string filename) const
{
	return filename + ".exe";
}

