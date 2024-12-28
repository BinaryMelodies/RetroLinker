#ifndef GEOS_H
#define GEOS_H

#include "../common.h"
#include "../linker/linker.h"
#include "../linker/reader.h"
#include "../linker/writer.h"

/* TODO: unimplemented */

namespace GEOS
{
	/**
	 * @brief Berkeley Softworks GEOS or GeoWorks Ensemble or NewDeal Office or Breadbox Ensemble Geode file format
	 */
	class GeodeFormat : public virtual Linker::OutputFormat, public Linker::LinkerManager
	{
	public:
		void ReadFile(Linker::Reader& in) override;
		void WriteFile(Linker::Writer& out) override;
		/* TODO */

		std::string GetDefaultExtension(Linker::Module& module, std::string filename) override;
	};
}

#endif /* GEOS_H */
