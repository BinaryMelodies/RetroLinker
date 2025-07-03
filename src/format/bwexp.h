#ifndef BWEXP_H
#define BWEXP_H

#include "../common.h"
#include "../dumper/dumper.h"
#include "../linker/options.h"
#include "../linker/segment_manager.h"
#include "mzexe.h"

namespace DOS16M
{
	/**
	 * @brief Rational Systems DOS/16M "BW" .exp file
	 */
	class BWFormat : public virtual Linker::SegmentManager
	{
	public:
		class BWOptionCollector : public Linker::OptionCollector
		{
		public:
			Linker::Option<std::string> stub{"stub", "Filename for stub that gets prepended to executable"};

			BWOptionCollector()
			{
				InitializeFields(stub);
			}
		};

		void ReadFile(Linker::Reader& rd) override;

		bool FormatSupportsSegmentation() const override;

		bool FormatIs16bit() const override;

		bool FormatIsProtectedMode() const override;

		unsigned FormatAdditionalSectionFlags(std::string section_name) const override;

		class AbstractSegment
		{
		public:
			enum access_type
			{
				TYPE_DATA = 0x92,
				TYPE_CODE = 0x9A,
			};
			access_type access;

			enum flag_type
			{
				FLAG_EMPTY = 0x2000,
				FLAG_TRANSPARENT = 0x8000,
			};
			flag_type flags;

			uint32_t address = 0;
			uint32_t total_length;

			AbstractSegment(unsigned access = TYPE_DATA, unsigned flags = 0, uint32_t total_length = 0)
				: access(access_type(access)), flags(flag_type(flags)), total_length(total_length)
			{
			}

			virtual ~AbstractSegment();

			/**
			 * @brief Retrieves the total size of the segment, including the bss
			 */
			uint32_t GetTotalSize();

			/**
			 * @brief Modifies the total size of the segment, including the bss. Note that this is not guaranteed to work for all segment types
			 */
			virtual void SetTotalSize(uint32_t new_value);

			/**
			 * @brief Retrieves size of segment. Some subclasses might calculate this dynamically
			 */
			virtual uint32_t GetSize(const BWFormat& bw) const = 0;
			/**
			 * @brief Produces the binary contents of the segment
			 */
			virtual void WriteContent(Linker::Writer& wr, const BWFormat& bw) const = 0;

			/**
			 * @brief Sets up any values before it can be written to file
			 */
			void Prepare(BWFormat& bw);

			/**
			 * @brief Produces the GDT entry for the header
			 */
			void WriteHeader(Linker::Writer& wr, const BWFormat& bw) const;
		};

		class Segment : public AbstractSegment
		{
		public:
			std::shared_ptr<Linker::Segment> image;

			Segment(std::shared_ptr<Linker::Segment> segment, unsigned access = TYPE_DATA, unsigned flags = 0)
				: AbstractSegment(access, flags, segment->TotalSize()), image(segment)
			{
			}

			void SetTotalSize(uint32_t new_value) override;

			uint32_t GetSize(const BWFormat& bw) const override;

			void WriteContent(Linker::Writer& wr, const BWFormat& bw) const override;
		};

		class DummySegment : public AbstractSegment
		{
		public:
			DummySegment(uint32_t total_length, unsigned access = TYPE_DATA, unsigned flags = 0)
				: AbstractSegment(access, flags, total_length)
			{
			}

			void SetTotalSize(uint32_t new_value) override;

			uint32_t GetSize(const BWFormat& bw) const override;

			void WriteContent(Linker::Writer& wr, const BWFormat& bw) const override;
		};

		class RelocationSegment : public AbstractSegment
		{
		public:
			uint16_t index;

			RelocationSegment(uint16_t index)
				: AbstractSegment(TYPE_DATA), index(index)
			{
			}

			/**
			 * @brief Invalid call, the size of a relocation segment is calculated dynamically and cannot be changed via a call
			 */
			void SetTotalSize(uint32_t new_value) override;

			uint32_t GetSize(const BWFormat& bw) const override;

			void WriteContent(Linker::Writer& wr, const BWFormat& bw) const override;
		};

		/**
		 * @brief BW .exp files support two versions of relocations
		 */
		enum relocations_type
		{
			RelocationsNone,
			RelocationsType1,
			RelocationsType2,
		};
		relocations_type option_relocations = RelocationsType2;
		bool option_force_relocations = true;

		enum exp_flag_type
		{
			EXP_FLAG_RELOCATABLE = 0x0001,
		};
		exp_flag_type exp_flags = exp_flag_type(0);

		enum option_type
		{
			OPTION_RELOCATIONS = 0x1000,
		};
		option_type options = option_type(0);

		std::map<uint16_t, std::set<uint16_t> > relocations;

		offset_t MeasureRelocations() const;

		std::shared_ptr<Linker::OptionCollector> GetOptions() override;

		void SetOptions(std::map<std::string, std::string>& options) override;

		offset_t file_size = 0;
		offset_t min_extra = 0;
		offset_t max_extra = 0;
		uint16_t ss = 0, sp = 0, cs = 0, ip = 0, relocsel = 0;
		uint16_t runtime_gdt_length = 0xFFFF;
		uint16_t version = 0;
		uint32_t next_header_offset = 0;
		uint32_t debug_info_offset = 0; /* TODO: ??? */
		uint16_t first_selector = 0x0080; /* TODO: make dynamic */
		uint32_t private_xm = 0; /* TODO: make parameter */
		uint16_t ext_reserve = 0; /* TODO: ??? */
		uint16_t transparent_stack = 0; /* TODO: ??? */
		uint32_t program_size = 0; /* TODO: ??? */
		uint8_t default_memory_strategy = 0; /* TODO: ??? */
		uint16_t transfer_buffer_size = 0; /* TODO: ??? */
		std::string exp_name; /* TODO: ??? */

		mutable Microsoft::MZStubWriter stub;

		std::vector<std::unique_ptr<AbstractSegment>> segments;
		std::map<std::shared_ptr<Linker::Segment>, size_t> segment_indices;
		int default_data = 0;

		void OnNewSegment(std::shared_ptr<Linker::Segment> segment) override;

		std::unique_ptr<Script::List> GetScript(Linker::Module& module);

		void Link(Linker::Module& module);

		size_t GetDefaultDataIndex();

		void ProcessModule(Linker::Module& module) override;

		void CalculateValues() override;

		offset_t ImageSize() const override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;

		void Dump(Dumper::Dumper& dump) const override;

		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module, std::string filename) const override;
	};
}

#endif /* BWEXP_H */
