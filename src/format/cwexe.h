#ifndef CWEXE_H
#define CWEXE_H

#include "../common.h"
#include "../linker/reader.h"
#include "../linker/segment_manager.h"
#include "../linker/writer.h"

/* TODO: unimplemented */

namespace CauseWay
{
	/**
	 * @brief CauseWay format (signature `3P`)
	 */
	class CauseWayFormat : public virtual Linker::SegmentManager
	{
	public:
		/* * * General members * * */
		// TODO

		void ReadFile(const std::shared_ptr<Linker::Reader>& rd) override;
		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;

		void Dump(Dumper::Dumper& dump) const override;

		void CalculateValues() override;

		/* * * Reader members * * */

		/* * * Writer members * * */
		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module, std::string filename) const override;
	};
}

#endif /* CWEXE_H */
