#ifndef GEOS_H
#define GEOS_H

#include "../common.h"
#include "../linker/linker_manager.h"
#include "../linker/reader.h"
#include "../linker/writer.h"

/* TODO: unimplemented */

namespace GEOS
{
	/**
	 * @brief Berkeley Softworks GEOS or GeoWorks Ensemble or NewDeal Office or Breadbox Ensemble Geode file format
	 */
	class GeodeFormat : public virtual Linker::LinkerManager
	{
	public:
		void ReadFile(Linker::Reader& rd) override;
		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) override;
		/* TODO */

		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module, std::string filename) override;
	};
}

#endif /* GEOS_H */
