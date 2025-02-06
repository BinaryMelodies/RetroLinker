
#include "java.h"
#include "../linker/reader.h"
#include "../linker/writer.h"

using namespace Java;

void ClassFormat::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = ::BigEndian;
	rd.Seek(0);
	// TODO
}

offset_t ClassFormat::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::BigEndian;
	wr.Seek(0);
	// TODO
	return ImageSize();
}

offset_t ClassFormat::ImageSize() const
{
	// TODO
	return offset_t(-1);
}

void ClassFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("Java class format");
	Dumper::Region file_region("File", file_offset, ImageSize(), 8);
	file_region.Display(dump);

	// TODO
}

