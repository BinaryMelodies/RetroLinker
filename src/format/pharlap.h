#ifndef PHARLAP_H
#define PHARLAP_H

#include "../common.h"
#include "../dumper/dumper.h"
#include "../linker/options.h"
#include "../linker/segment_manager.h"
#include "mzexe.h"

namespace PharLap
{
	/**
	 * @brief Phar Lap "MP" .exp and "MQ" .rex file
	 */
	class MPFormat : public virtual Linker::SegmentManager
	{
	public:
		class MPOptionCollector : public Linker::OptionCollector
		{
		public:
			Linker::Option<std::string> stub{"stub", "Filename for stub that gets prepended to executable"};
			Linker::Option<offset_t> stack{"stack", "Specify the stack size"};

			MPOptionCollector()
			{
				InitializeFields(stub, stack);
			}
		};

		void ReadFile(Linker::Reader& rd) override;

		unsigned FormatAdditionalSectionFlags(std::string section_name) const override;

		bool has_relocations;

		std::shared_ptr<Linker::Contents> image;

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

			explicit Relocation(uint32_t offset)
				: value(offset)
			{
			}

			bool operator ==(const Relocation& other) const;

			bool operator <(const Relocation& other) const;
		};

		offset_t image_size = 0;
		std::vector<Relocation> relocations;
		offset_t header_size = 0;
		offset_t min_extra_pages = 0;
		offset_t max_extra_pages = 0;
		uint32_t esp = 0;
		uint32_t eip = 0;
		uint16_t checksum = 0; // TODO
		offset_t relocation_offset = 0;
		uint16_t relocation_count = 0;
		/** @brief Stack size requested at the command line */
		uint32_t stack_size = 0;
		mutable Microsoft::MZStubWriter stub;

		MPFormat(bool has_relocations = false)
			: has_relocations(has_relocations)
		{
		}

		std::shared_ptr<Linker::OptionCollector> GetOptions() override;

		void SetOptions(std::map<std::string, std::string>& options) override;

		void OnNewSegment(std::shared_ptr<Linker::Segment> segment) override;

		std::unique_ptr<Script::List> GetScript(Linker::Module& module);

		void Link(Linker::Module& module);

		void ProcessModule(Linker::Module& module) override;

		void CalculateValues() override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;

		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module, std::string filename) const override;
	};

	/**
	 * @brief Phar Lap "P2"/"P3" .exp file
	 */
	class P3Format : public virtual Linker::SegmentManager
	{
	public:
		class P3OptionCollector : public Linker::OptionCollector
		{
		public:
			Linker::Option<std::string> stub{"stub", "Filename for stub that gets prepended to executable"};
			Linker::Option<offset_t> stack{"stack", "Specify the stack size"};

			P3OptionCollector()
			{
				InitializeFields(stub, stack);
			}
		};

		void ReadFile(Linker::Reader& rd) override;

		bool FormatSupportsSegmentation() const override;

		bool FormatIs16bit() const override;

		bool FormatIsProtectedMode() const override;

#if 0
		bool FormatSupportsStackSection() const override;
#endif

		bool is_multisegmented;
		bool is_32bit;

		uint16_t header_size = 0;
		uint32_t file_size = 0;
		uint16_t checksum16 = 0; // TODO
		uint32_t checksum32 = 0; // TODO

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

		mutable Microsoft::MZStubWriter stub;

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

			void ReadFile(Linker::Reader& rd);

			void CalculateValues();

			void WriteFile(Linker::Writer& wr) const;
		};

		RunTimeParameterBlock runtime_parameters;

		std::shared_ptr<Linker::OptionCollector> GetOptions() override;

		void SetOptions(std::map<std::string, std::string>& options) override;

		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module, std::string filename) const override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;

		class Flat;
		class MultiSegmented;
		class External;
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
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;
	};

	class P3Format::MultiSegmented : public P3Format
	{
	public:
		class AbstractSegment
		{
		public:
			uint32_t address = 0;
			virtual ~AbstractSegment();
			virtual uint32_t GetStoredSize() const = 0;
			virtual uint32_t GetLoadedSize() const = 0;
			virtual void WriteFile(Linker::Writer& wr) const = 0;
		};

		class Descriptor
		{
		public:
			std::weak_ptr<AbstractSegment> image = std::weak_ptr<AbstractSegment>();

			enum
			{
				TSS16  = 0x00008100,
				LDT    = 0x00008200,
				TSS32  = 0x00008900,
				Code16 = 0x00009A00,
				Code32 = 0x00409A00,
				Data16 = 0x00009200,
				Data32 = 0x00409200,

				DESC_X = 0x00000800,
				DESC_S = 0x00001000,
				DESC_G = 0x00800000,
			};
			uint32_t access = 0;

			// only for segments
			uint32_t limit = 0;
			uint32_t base = 0;
			// only for gates
			uint32_t offset = 0;
			uint16_t selector = 0;

			explicit Descriptor() = default;

			static Descriptor FromSegment(uint32_t access, std::weak_ptr<AbstractSegment> image = std::weak_ptr<AbstractSegment>());
			static Descriptor ReadEntry(Linker::Image& image, offset_t offset);

			bool IsSegment() const;
			bool IsGate() const;

			void CalculateValues();

			void WriteEntry(Linker::Writer& wr) const;

			void FillEntry(Dumper::Entry& entry) const;
		};

		class DescriptorTable : public AbstractSegment
		{
		public:
			std::vector<Descriptor> descriptors;

			uint32_t GetStoredSize() const override;

			uint32_t GetLoadedSize() const override;

			void WriteFile(Linker::Writer& wr) const override;

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

			uint32_t GetStoredSize() const override;

			uint32_t GetLoadedSize() const override;

			void WriteFile(Linker::Writer& wr) const override;
		};

		std::shared_ptr<TaskStateSegment> tss;

		class SITEntry : public AbstractSegment
		{
		public:
			uint16_t selector = 0;
			uint16_t flags = 0;
			uint32_t base_offset = 0; /* TODO??? */
			uint32_t zero_fill = 0; /* only used for reading */

			SITEntry(uint16_t selector = 0)
				: selector(selector)
			{
			}

			uint32_t GetStoredSize() const override;

			virtual uint32_t GetZeroSize() const;

			uint32_t GetLoadedSize() const override;

			void WriteSITEntry(Linker::Writer& wr) const;

			void WriteFile(Linker::Writer& wr) const override;

			static std::shared_ptr<SITEntry> ReadSITEntry(Linker::Reader& rd);
		};

		class Segment : public SITEntry
		{
		public:
			/* Segment members */
			std::shared_ptr<Linker::Segment> segment;

			uint32_t access;

			Segment(std::shared_ptr<Linker::Segment> segment, uint32_t access, uint16_t selector)
				: SITEntry(selector), segment(segment), access(access)
			{
			}

			uint32_t GetStoredSize() const override;

			uint32_t GetZeroSize() const override;

			void WriteFile(Linker::Writer& wr) const override;
		};

		MultiSegmented(bool is_32bit = true)
			: P3Format(true, is_32bit)
		{
		}

		class Relocation
		{
		public:
			uint16_t selector;
			uint32_t offset;

			Relocation(uint16_t selector, uint32_t offset)
				: selector(selector), offset(offset)
			{
			}

			bool operator ==(const Relocation& other) const;

			bool operator <(const Relocation& other) const;

			void WriteFile(Linker::Writer& wr) const;
		};

		std::vector<std::shared_ptr<AbstractSegment>> segments;
		std::map<std::shared_ptr<Linker::Segment>, uint16_t> segment_associations;
		std::set<Relocation> relocations;
		std::shared_ptr<Segment> code;
		std::shared_ptr<Segment> data;

		void OnNewSegment(std::shared_ptr<Linker::Segment> linker_segment) override;

		std::unique_ptr<Script::List> GetScript(Linker::Module& module);

		void Link(Linker::Module& module);

		void ProcessModule(Linker::Module& module) override;

		void CalculateValues() override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;
	};

	class P3Format::External : public P3Format
	{
	public:
		std::vector<std::shared_ptr<P3Format::MultiSegmented::SITEntry>> segments;
		std::vector<P3Format::MultiSegmented::Relocation> relocations;
		std::shared_ptr<Linker::Buffer> image;

		std::shared_ptr<P3Format::MultiSegmented::DescriptorTable> gdt;
		std::shared_ptr<P3Format::MultiSegmented::DescriptorTable> idt;
		std::shared_ptr<P3Format::MultiSegmented::DescriptorTable> ldt;
		std::shared_ptr<P3Format::MultiSegmented::TaskStateSegment> tss;

		External()
			: P3Format(true, true)
		{
		}

		void ReadFile(Linker::Reader& rd) override;

		bool FormatSupportsSegmentation() const override;

		bool FormatIs16bit() const override;

		bool FormatIsProtectedMode() const override;

		void SetOptions(std::map<std::string, std::string>& options) override;

		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module, std::string filename) const override;

		void ProcessModule(Linker::Module& module) override;

		void CalculateValues() override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;
	};
}

#endif /* PHARLAP_H */
