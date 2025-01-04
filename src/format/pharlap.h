#ifndef PHARLAP_H
#define PHARLAP_H

#include "../common.h"
#include "../linker/linker.h"
#include "mzexe.h"

namespace PharLap
{
	/**
	 * @brief Phar Lap "MP" .exp and "MQ" .rex file
	 */
	class MPFormat : public virtual Linker::OutputFormat, public Linker::LinkerManager, protected Microsoft::MZStubWriter
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

		offset_t image_size;
		std::set<Relocation> relocations;
		offset_t header_size;
		offset_t bss_pages;
		offset_t extra_pages;
		uint32_t esp;
		uint32_t eip;
		offset_t relocation_offset;

		MPFormat(bool has_relocations = false)
			: has_relocations(has_relocations)
		{
		}

		using LinkerManager::SetLinkScript;

		void SetOptions(std::map<std::string, std::string>& options) override;

		void OnNewSegment(std::shared_ptr<Linker::Segment> segment) override;

		std::unique_ptr<Script::List> GetScript(Linker::Module& module);

		void Link(Linker::Module& module);

		void ProcessModule(Linker::Module& module) override;

		void CalculateValues() override;

		void WriteFile(Linker::Writer& wr) override;

		std::string GetDefaultExtension(Linker::Module& module, std::string filename) override;
	};

	/**
	 * @brief Phar Lap "P2"/"P3" .exp file
	 */
	class P3Format : public Linker::OutputFormat, public Linker::LinkerManager, protected Microsoft::MZStubWriter
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

		uint16_t header_size;
		uint32_t file_size;

		uint32_t runtime_parameters_offset;
		uint32_t runtime_parameters_size;
		uint32_t relocation_table_offset;
		uint32_t relocation_table_size;
		uint32_t segment_information_table_offset;
		uint32_t segment_information_table_size;
		uint16_t segment_information_table_entry_size;
		uint32_t load_image_offset;
		uint32_t load_image_size;
		uint32_t symbol_table_offset;
		uint32_t symbol_table_size;
		uint32_t gdt_address;
		uint32_t gdt_size;
		uint32_t ldt_address;
		uint32_t ldt_size;
		uint32_t idt_address;
		uint32_t idt_size;
		uint32_t tss_address;
		uint32_t tss_size;
		uint32_t minimum_extra;
		uint32_t maximum_extra;
		uint32_t base_load_offset;
		uint32_t esp;
		uint16_t ss;
		uint32_t eip;
		uint16_t cs;
		uint16_t ldtr;
		uint16_t tr;
		uint16_t flags;
		uint32_t memory_requirements;
		uint32_t stack_size;

		P3Format(bool is_multisegmented, bool is_32bit = true)
			: is_multisegmented(is_multisegmented), is_32bit(is_32bit)
		{
		}

		class RunTimeParameterBlock
		{
		public:
			uint16_t min_realmode_param, max_realmode_param, min_int_buffer_size_kb, max_int_buffer_size_kb, int_stack_count, int_stack_size_kb;
			uint32_t realmode_area_end;
			uint16_t call_buffer_size_kb, flags, ring;

			void CalculateValues();

			void WriteFile(Linker::Writer& wr);
		};

		RunTimeParameterBlock runtime_parameters;

		using LinkerManager::SetLinkScript;

		void SetOptions(std::map<std::string, std::string>& options) override;

		std::string GetDefaultExtension(Linker::Module& module, std::string filename) override;

		void WriteFile(Linker::Writer& wr) override;

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

		void WriteFile(Linker::Writer& wr) override;
	};

	class P3Format::MultiSegmented : public P3Format
	{
	public:
		class AbstractSegment
		{
		public:
			uint32_t address;
			virtual ~AbstractSegment();
			virtual uint32_t GetStoredSize() = 0;
			virtual uint32_t GetLoadedSize() = 0;
			virtual void WriteFile(Linker::Writer& wr) = 0;
		};

		class Descriptor
		{
		public:
			AbstractSegment * image;

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

			uint32_t limit;
			uint32_t base;
			uint32_t access;

			Descriptor(uint32_t access, AbstractSegment * image = nullptr)
				: image(image), limit(0), base(0), access(access)
			{
			}

			void CalculateValues();

			void WriteEntry(Linker::Writer& wr);
		};

		class DescriptorTable : public AbstractSegment
		{
		public:
			std::vector<Descriptor *> descriptors;

			uint32_t GetStoredSize() override;

			uint32_t GetLoadedSize() override;

			void WriteFile(Linker::Writer& wr) override;

			void CalculateValues();
		};

		DescriptorTable gdt, idt, ldt;

		class TaskStateSegment : public AbstractSegment
		{
		public:
			bool is_32bit;

			uint32_t esp0, esp1, esp2;
			uint32_t cr3, eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
			uint16_t ss0, ss1, ss2, es, cs, ss, ds, fs, gs, ldtr, iopb;
			uint16_t link;

			TaskStateSegment(bool is_32bit = true)
				: is_32bit(is_32bit)
			{
			}

			uint32_t GetStoredSize() override;

			uint32_t GetLoadedSize() override;

			void WriteFile(Linker::Writer& wr) override;
		};

		TaskStateSegment tss;

		class Segment : public Descriptor, public AbstractSegment
		{
		public:
			std::shared_ptr<Linker::Segment> segment;

			uint16_t selector;
			uint16_t flags;
			uint32_t base_offset; /* TODO??? */

			Segment(std::shared_ptr<Linker::Segment> segment, uint32_t access, uint16_t selector)
				: Descriptor(access, this), segment(segment), selector(selector), base_offset(0)
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
			Segment * segment;
			uint32_t offset;

			Relocation(Segment * segment, uint32_t offset)
				: segment(segment), offset(offset)
			{
			}

			bool operator ==(const Relocation& other) const;

			bool operator <(const Relocation& other) const;

			void WriteFile(Linker::Writer& wr) const;
		};

		std::vector<AbstractSegment *> segments;
		std::map<std::shared_ptr<Linker::Segment>, Segment *> segment_associations;
		std::set<Relocation> relocations;
		Segment * code;
		Segment * data;

		void OnNewSegment(std::shared_ptr<Linker::Segment> linker_segment) override;

		std::unique_ptr<Script::List> GetScript(Linker::Module& module);

		void Link(Linker::Module& module);

		void ProcessModule(Linker::Module& module) override;

		void CalculateValues() override;

		void WriteFile(Linker::Writer& wr) override;
	};
}

#endif /* PHARLAP_H */
