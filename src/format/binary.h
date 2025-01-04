#ifndef BINARY_H
#define BINARY_H

#include "../common.h"
#include "../linker/linker.h"
#include "../linker/module.h"
#include "../linker/segment.h"
#include "../linker/writer.h"
#include "mzexe.h"

/* TODO:
	refactor, use a Writable for writing, introduce base_address and zero_fill fields? maybe only for specific subclasses (such as PRL)
*/

namespace Binary
{
	/**
	 * @brief A template for flat binary formats
	 */
	class GenericBinaryFormat : public Linker::OutputFormat, public Linker::LinkerManager
	{
	public:
		/* * * General members * * */

		/** @brief The actual stored image */
		std::shared_ptr<Linker::Writable> image = nullptr;

		void Clear() override;

		GenericBinaryFormat(uint64_t default_base_address = 0, std::string default_extension = "")
			: position_independent(false), base_address(default_base_address), extension(default_extension)
		{
		}

		GenericBinaryFormat(std::string default_extension)
			: position_independent(true), extension(default_extension)
		{
		}

		~GenericBinaryFormat()
		{
			Clear();
		}

		void ReadFile(Linker::Reader& rd) override;

		void WriteFile(Linker::Writer& wr) override;

		void Dump(Dumper::Dumper& dump) override;

		/* * * Writer members * * */
		/** @brief Set when the generated code must not reference absolute references */
		bool position_independent;
		/** @brief Address at which image is stored, it can be format specific or provided as a parameter */
		uint64_t base_address = 0;
		/** @brief Default filename extension for executables (such as .com for MS-DOS, .r for Human68k) */
		std::string extension;

		using LinkerManager::SetLinkScript;

		void SetOptions(std::map<std::string, std::string>& options) override;

		void OnNewSegment(std::shared_ptr<Linker::Segment> segment) override;

		void CreateDefaultSegments();

		virtual Script::List * GetScript(Linker::Module& module);

		/**
		 * @brief Callback function to process relocations
		 */
		virtual bool ProcessRelocation(Linker::Module& module, Linker::Relocation& rel, Linker::Resolution resolution);

		void Link(Linker::Module& module);

		void ProcessModule(Linker::Module& module) override;

		void CalculateValues() override;

		void GenerateFile(std::string filename, Linker::Module& module) override;

		std::string GetDefaultExtension(Linker::Module& module, std::string filename) override;
	};

	/**
	 * @brief A flat binary format, with no header, loaded directly into memory
	 *
	 * Most often used with disk images or MS-DOS .COM files.
	 * They are also used as executable file formats for several operating systems, including:
	 * - CP/M-80 and MSX-DOS .com files (loaded at 0x0100)
	 * - MS-DOS .com files (loaded at offset 0x0100 in an arbitrary segment)
	 * - Human68k .r files (relocatable)
	 * - Sinclair QL QDOS files (loaded at 0x00038000)
	 * - RISC OS ,ff8 files (loaded at 0x00008000)
	 * - other CP/M inspired systems, such as DOS/65 (loaded at a port specific address) and DX-DOS .com files (loaded at an address stored in the directory entry)
	 */
	class BinaryFormat : public GenericBinaryFormat
	{
	public:
		/* * * General members * * */

		/** @brief Concurrent DOS program information entry, allocated only if present */
		Microsoft::MZFormat::PIF * pif = nullptr;

		void Clear() override;

		BinaryFormat(uint64_t default_base_address = 0, std::string default_extension = "")
			: GenericBinaryFormat(default_base_address, default_extension)
		{
		}

		BinaryFormat(std::string default_extension)
			: GenericBinaryFormat(default_extension)
		{
		}

		~BinaryFormat()
		{
			Clear();
		}

		void ReadFile(Linker::Reader& rd) override;

		void WriteFile(Linker::Writer& wr) override;

		void Dump(Dumper::Dumper& dump) override;

		/* * * Writer members * * */
		bool FormatSupportsSegmentation() const override;

		bool FormatIs16bit() const override;

		unsigned FormatAdditionalSectionFlags(std::string section_name) const override;

		/** @brief (x86 only) Represents the memory model of the running executable, which is the way in which the segments are set up during execution */
		enum memory_model_t
		{
			/** @brief Default model, for x86, same as tiny, for other platforms the only possible option */
			MODEL_DEFAULT,
			/** @brief Tiny model, code and data segment are the same */
			MODEL_TINY,
			/** @brief Small model, separate code and data segments */
			MODEL_SMALL,
			/** @brief Compact model, separate code and multiple data segments */
			MODEL_COMPACT,
			/** @brief Large model, every section is a separate segment */
			MODEL_LARGE,
		};
		/** @brief Memory model of generated executable, must be MODEL_DEFAULT for all non-x86 platforms */
		memory_model_t memory_model = MODEL_DEFAULT;

		using LinkerManager::SetLinkScript;

		void SetModel(std::string model) override;

		Script::List * GetScript(Linker::Module& module) override;

		void ProcessModule(Linker::Module& module) override;
	};

}

#endif /* BINARY_H */
