#ifndef EMXAOUT_H
#define EMXAOUT_H

#include "mzexe.h"
#include "aout.h"
#include "leexe.h"
#include "../common.h"
#include "../dumper/dumper.h"
#include "../linker/module.h"
#include "../linker/options.h"
#include "../linker/segment.h"
#include "../linker/segment_manager.h"
#include "../linker/writer.h"

namespace EMX
{
	class EMXAOutFormat : public virtual AOut::AOutFormat, protected virtual Microsoft::LEFormat
	{
	public:
		EMXAOutFormat()
			: AOutFormat(AOutFormat::EMX, AOutFormat::ZMAGIC), LEFormat(LEFormat::OS2, LEFormat::GUIAware | LEFormat::NoInternalFixup, true)
		{
		}

		void ReadFile(Linker::Reader& rd) override;

		offset_t ImageSize() const override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;

		void Dump(Dumper::Dumper& dump) const override;

		/* * * Reader members * * */

		using Linker::InputFormat::GenerateModule;
		void GenerateModule(Linker::Module& module) const override;

		/* * * Writer members * * */

		using LEFormat::stub;

		static std::vector<Linker::OptionDescription<void> *> ParameterNames;
		std::vector<Linker::OptionDescription<void> *> GetLinkerScriptParameterNames() override;

		std::shared_ptr<Linker::OptionCollector> GetOptions() override;
		void SetOptions(std::map<std::string, std::string>& options) override;

		void OnNewSegment(std::shared_ptr<Linker::Segment> segment) override;

		void ProcessModule(Linker::Module& module) override;

		void CalculateValues() override;

		void GenerateFile(std::string filename, Linker::Module& module) override;

		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module, std::string filename) const override;
		std::string GetDefaultExtension(Linker::Module& module) const override;
	};
}

#endif /* EMXAOUT_H */
