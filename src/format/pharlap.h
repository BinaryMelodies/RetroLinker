#ifndef PHARLAP_H
#define PHARLAP_H

#include "../common.h"
#include "../dumper/dumper.h"
#include "../linker/linker_manager.h"
#include "mzexe.h"

namespace PharLap
{
	/**
	 * @brief Phar Lap "MP" .exp and "MQ" .rex file
	 */
	class MPFormat : public virtual Linker::LinkerManager, protected Microsoft::MZStubWriter
	{
	public:
		void ReadFile(Linker::Reader& rd) override;

		unsigned FormatAdditionalSectionFlags(std::string section_name) const override;

		bool has_relocations;
		std::string stub_file;

		std::shared_ptr<Linker::Segment> image;

		static const uint32_t REL32 = 0x80000000;
		union Relocation
		{
			struct
			{
				uint32_t offset : 31, rel32 : 1;
			};
			uint32_t value;

			Relocation(uint32_t offset, unsigned rel32)
				: offset(offset), rel32(rel32)
			{
			}

			/*Relocation(uint32_t offset)
				: value(offset)
			{
			}*/

			bool operator ==(const Relocation& other) const;

			bool operator <(const Relocation& other) const;
		};

		offset_t image_size = 0;
		std::set<Relocation> relocations;
		offset_t header_size = 0;
		offset_t bss_pages = 0;
		offset_t extra_pages = 0;
		uint32_t esp = 0;
		uint32_t eip = 0;
		offset_t relocation_offset = 0;

		MPFormat(bool has_relocations = false)
			: has_relocations(has_relocations)
		{
		}

		void SetOptions(std::map<std::string, std::string>& options) override;

		void OnNewSegment(std::shared_ptr<Linker::Segment> segment) override;

		std::unique_ptr<Script::List> GetScript(Linker::Module& module);

		void Link(Linker::Module& module);

		void ProcessModule(Linker::Module& module) override;

		void CalculateValues() override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) override;
		void Dump(Dumper::Dumper& dump) override;

		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module, std::string filename) override;
	};

	/**
	 * @brief Phar Lap "P2"/"P3" .exp file
	 */
	class P3Format : public virtual Linker::LinkerManager, protected Microsoft::MZStubWriter
	{
	public:
		void ReadFile(Linker::Reader& rd) override;

		bool FormatSupportsSegmentation() const override;

#if 0
		bool FormatSupportsStackSection() const override;
#endif

		/*std::string stub_file;*/
		const bool is_multisegmented;
		bool is_32bit;

		uint16_t header_size = 0;
		uint32_t file_size = 0;

		uint32_t runtime_parameters_offset = 0;
		uint32_t runtime_parameters_size = 0;
		uint32_t relocation_table_offset = 0;
		uint32_t relocation_table_size = 0;
		uint32_t segment_information_table_offset = 0;
		uint32_t segment_information_table_size = 0;
		uint16_t segment_information_table_entry_size = 0;
		uint32_t load_image_offset = 0;
		uint32_t load_image_size = 0;
		uint32_t symbol_table_offset = 0;
		uint32_t symbol_table_size = 0;
		uint32_t gdt_address = 0;
		uint32_t gdt_size = 0;
		uint32_t ldt_address = 0;
		uint32_t ldt_size = 0;
		uint32_t idt_address = 0;
		uint32_t idt_size = 0;
		uint32_t tss_address = 0;
		uint32_t tss_size = 0;
		uint32_t minimum_extra = 0;
		uint32_t maximum_extra = 0;
		uint32_t base_load_offset = 0;
		uint32_t esp = 0;
		uint16_t ss = 0;
		uint32_t eip = 0;
		uint16_t cs = 0;
		uint16_t ldtr = 0;
		uint16_t tr = 0;
		uint16_t flags = 0;
		uint32_t memory_requirements = 0;
		uint32_t stack_size = 0;

		P3Format(bool is_multisegmented, bool is_32bit = true)
			: is_multisegmented(is_multisegmented), is_32bit(is_32bit)
		{
		}

		class RunTimeParameterBlock
		{
		public:
			uint16_t min_realmode_param = 0, max_realmode_param = 0, min_int_buffer_size_kb = 0, max_int_buffer_size_kb = 0, int_stack_count = 0, int_stack_size_kb = 0;
			uint32_t realmode_area_end = 0;
			uint16_t call_buffer_size_kb = 0, flags = 0, ring = 0;

			void CalculateValues();

			void WriteFile(Linker::Writer& wr);
		};

		RunTimeParameterBlock runtime_parameters;

		void SetOptions(std::map<std::string, std::string>& options) override;

		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module, std::string filename) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) override;
		void Dump(Dumper::Dumper& dump) override;

		class Flat;
		class MultiSegmented;
	};

	class P3Format::Flat : public P3Format
	{
	public:
		Flat(bool is_32bit = true)
			: P3Format(false, is_32bit)
		{
		}

		std::shared_ptr<Linker::Segment> image;

		void OnNewSegment(std::shared_ptr<Linker::Segment> segment) override;

		std::unique_ptr<Script::List> GetScript(Linker::Module& module);

		void Link(Linker::Module& module);

		void ProcessModule(Linker::Module& module) override;

		void CalculateValues() override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) override;
		void Dump(Dumper::Dumper& dump) override;
	};

	class P3Format::MultiSegmented : public P3Format
	{
	public:
		class AbstractSegment
		{
		public:
			uint32_t address = 0;
			virtual ~AbstractSegment();
			virtual uint32_t GetStoredSize() = 0;
			virtual uint32_t GetLoadedSize() = 0;
			virtual void WriteFile(Linker::Writer& wr) = 0;
		};

		class Descriptor
		{
		public:
			std::weak_ptr<AbstractSegment> image;

			enum
			{
				TSS16  = 0x00008100,
				LDT    = 0x00008200,
				TSS32  = 0x00008900,
				Code16 = 0x00009A00,
				Code32 = 0x00409A00,
				Data16 = 0x00009200,
				Data32 = 0x00409200,

				DESC_G = 0x00800000,
			};

			uint32_t limit = 0;
			uint32_t base = 0;
			uint32_t access;

			Descriptor(uint32_t access, std::weak_ptr<AbstractSegment> image = std::weak_ptr<AbstractSegment>())
				: image(image), access(access)
			{
			}

			void CalculateValues();

			void WriteEntry(Linker::Writer& wr);
		};

		class DescriptorTable : public AbstractSegment
		{
		public:
			std::vector<Descriptor> descriptors;

			uint32_t GetStoredSize() override;

			uint32_t GetLoadedSize() override;

			void WriteFile(Linker::Writer& wr) override;

			void CalculateValues();
		};

		std::shared_ptr<DescriptorTable> gdt;
		std::shared_ptr<DescriptorTable> idt;
		std::shared_ptr<DescriptorTable> ldt;

		class TaskStateSegment : public AbstractSegment
		{
		public:
			bool is_32bit;

			uint32_t esp0 = 0, esp1 = 0, esp2 = 0;
			uint32_t cr3 = 0, eip = 0, eflags = 0, eax = 0, ecx = 0, edx = 0, ebx = 0, esp = 0, ebp = 0, esi = 0, edi = 0;
			uint16_t ss0 = 0, ss1 = 0, ss2 = 0, es = 0, cs = 0, ss = 0, ds = 0, fs = 0, gs = 0, ldtr = 0, iopb = 0;
			uint16_t link = 0;

			TaskStateSegment(bool is_32bit = true)
				: is_32bit(is_32bit)
			{
			}

			uint32_t GetStoredSize() override;

			uint32_t GetLoadedSize() override;

			void WriteFile(Linker::Writer& wr) override;
		};

		std::shared_ptr<TaskStateSegment> tss;

		class Segment : public AbstractSegment
		{
		public:
			/* Segment members */
			std::shared_ptr<Linker::Segment> segment;

			uint32_t access;
			uint16_t selector;
			uint16_t flags = 0;
			uint32_t base_offset = 0; /* TODO??? */

			Segment(std::shared_ptr<Linker::Segment> segment, uint32_t access, uint16_t selector)
				: segment(segment), access(access), selector(selector)
			{
			}

			uint32_t GetStoredSize() override;

			uint32_t GetLoadedSize() override;

			void WriteSITEntry(Linker::Writer& wr);

			void WriteFile(Linker::Writer& wr) override;
		};

		MultiSegmented(bool is_32bit = true)
			: P3Format(true, is_32bit)
		{
		}

		class Relocation
		{
		public:
			std::shared_ptr<Segment> segment;
			uint32_t offset;

			Relocation(std::shared_ptr<Segment> segment, uint32_t offset)
				: segment(segment), offset(offset)
			{
			}

			bool operator ==(const Relocation& other) const;

			bool operator <(const Relocation& other) const;

			void WriteFile(Linker::Writer& wr) const;
		};

		std::vector<std::shared_ptr<AbstractSegment>> segments;
		std::map<std::shared_ptr<Linker::Segment>, std::shared_ptr<Segment>> segment_associations;
		std::set<Relocation> relocations;
		std::shared_ptr<Segment> code;
		std::shared_ptr<Segment> data;

		void OnNewSegment(std::shared_ptr<Linker::Segment> linker_segment) override;

		std::unique_ptr<Script::List> GetScript(Linker::Module& module);

		void Link(Linker::Module& module);

		void ProcessModule(Linker::Module& module) override;

		void CalculateValues() override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) override;
		void Dump(Dumper::Dumper& dump) override;
	};

	/**
	 * @brief Container for Phar Lap "P2"/"P3" .exp files, used when reading in a P2/P3 file
	 */
	class P3FormatContainer : public virtual Linker::OutputFormat
	{
	public:
		std::unique_ptr<P3Format> contents;

		void ReadFile(Linker::Reader& rd) override;
		bool FormatSupportsSegmentation() const override;

		void SetOptions(std::map<std::string, std::string>& options) override;

		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module, std::string filename) override;

		void ProcessModule(Linker::Module& module) override;

		void CalculateValues() override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) override;
		void Dump(Dumper::Dumper& dump) override;
	};
}

#endif /* PHARLAP_H */
