#ifndef GEOS_H
#define GEOS_H

#include "../common.h"
#include "../dumper/dumper.h"
#include "../linker/reader.h"
#include "../linker/segment_manager.h"
#include "../linker/writer.h"

/* TODO: unimplemented */

namespace GEOS
{
	/**
	 * @brief Berkeley Softworks GEOS or GeoWorks Ensemble or NewDeal Office or Breadbox Ensemble Geode file format
	 */
	class GeodeFormat : public virtual Linker::SegmentManager
	{
	public:
		void ReadFile(Linker::Reader& rd) override;
		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;
		/* TODO */

		bool FormatSupportsSegmentation() const override;

		bool FormatIs16bit() const override;

		bool FormatIsProtectedMode() const override;

		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module, std::string filename) const override;
	};
}

#endif /* GEOS_H */
