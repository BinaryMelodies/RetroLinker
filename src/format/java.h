#ifndef JAVA_H
#define JAVA_H

#include "../dumper/dumper.h"
#include "../linker/format.h"

namespace Java
{
	class ClassFormat : public Linker::Format
	{
	public:
		void ReadFile(Linker::Reader& rd) override;
		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		offset_t ImageSize() const override;
		void Dump(Dumper::Dumper& dump) const override;
	};
}

#endif /* JAVA_H */
