#ifndef BWEXP_H
#define BWEXP_H

#include "../common.h"
#include "../linker/linker.h"
#include "mzexe.h"

namespace DOS16M
{
	/**
	 * @brief Rational Systems DOS/16M "BW" .exp file
	 */
	class BWFormat : public virtual Linker::OutputFormat, public Linker::LinkerManager, protected Microsoft::MZStubWriter
	{
	public:
		void ReadFile(Linker::Reader& rd) override;

		bool FormatSupportsSegmentation() const override;

		bool FormatIs16bit() const override;

		unsigned FormatAdditionalSectionFlags(std::string section_name) const override;

		class AbstractSegment
		{
		public:
			enum access_type
			{
				TYPE_DATA = 0x92,
				TYPE_CODE = 0x9A,
			} access;

			enum flag_type
			{
				FLAG_EMPTY = 0x2000,
				FLAG_TRANSPARENT = 0x8000,
			} flags;

			uint32_t address;
			uint32_t total_length;

			AbstractSegment(unsigned access = TYPE_DATA, unsigned flags = 0, uint32_t total_length = 0)
				: access((access_type)access), flags((flag_type)flags), address(0), total_length(total_length)
			{
			}

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
			virtual uint32_t GetSize() = 0;
			/**
			 * @brief Produces the binary contents of the segment
			 */
			virtual void WriteContent(Linker::Writer& wr) = 0;

			/**
			 * @brief Produces the GDT entry for the header
			 */
			void WriteHeader(Linker::Writer& wr);
		};

		class Segment : public AbstractSegment
		{
		public:
			Linker::Segment * image;

			Segment(Linker::Segment * segment, unsigned access = TYPE_DATA, unsigned flags = 0)
				: AbstractSegment(access, flags, segment->TotalSize()), image(segment)
			{
			}

			void SetTotalSize(uint32_t new_value) override;

			uint32_t GetSize() override;

			void WriteContent(Linker::Writer& wr) override;
		};

		class DummySegment : public AbstractSegment
		{
		public:
			DummySegment(uint32_t total_length, unsigned access = TYPE_DATA, unsigned flags = 0)
				: AbstractSegment(access, flags, total_length)
			{
			}

			void SetTotalSize(uint32_t new_value) override;

			uint32_t GetSize() override;

			void WriteContent(Linker::Writer& wr) override;
		};

		class RelocationSegment : public AbstractSegment
		{
		public:
			BWFormat * bw;
			uint16_t index;

			RelocationSegment(BWFormat * bw, uint16_t index)
				: AbstractSegment(TYPE_DATA), bw(bw), index(index)
			{
			}

			/**
			 * @brief Invalid call, the size of a relocation segment is calculated dynamically and cannot be changed via a call
			 */
			void SetTotalSize(uint32_t new_value) override;

			uint32_t GetSize() override;

			void WriteContent(Linker::Writer& wr) override;
		};

		/**
		 * @brief BW .exp files support two versions of relocations
		 */
		enum relocations_type
		{
			RelocationsNone,
			RelocationsType1,
			RelocationsType2,
		} option_relocations;
		bool option_force_relocations;

		enum exp_flag_type
		{
			EXP_FLAG_RELOCATABLE = 0x0001,
		} exp_flags;

		enum option_type
		{
			OPTION_RELOCATIONS = 0x1000,
		} options;

		std::map<uint16_t, std::set<uint16_t> > relocations;

		offset_t MeasureRelocations();

		using LinkerManager::SetLinkScript;

		void SetOptions(std::map<std::string, std::string>& options) override;

		offset_t file_size;
		offset_t min_extra;
		offset_t max_extra;
		uint16_t ss, sp, cs, ip, relocsel;
		uint16_t runtime_gdt_length;
		uint16_t version;
		uint32_t next_header_offset;
		uint32_t debug_info_offset; /* TODO: ??? */
		uint16_t first_selector;
		uint32_t private_xm; /* TODO: make parameter */
		uint16_t ext_reserve; /* TODO: ??? */
		uint16_t transparent_stack; /* TODO: ??? */
		uint32_t program_size; /* TODO: ??? */
		uint8_t default_memory_strategy; /* TODO: ??? */
		uint16_t transfer_buffer_size; /* TODO: ??? */
		std::string exp_name; /* TODO: ??? */

		std::vector<AbstractSegment *> segments;
		std::map<Linker::Segment *, size_t> segment_indices;
		int default_data;

		BWFormat()
			: option_relocations(RelocationsType2),
			option_force_relocations(true),
			runtime_gdt_length(0xFFFF),
			first_selector(0x0080) /* TODO: make dynamic */
		{
		}

		void OnNewSegment(Linker::Segment * segment) override;

		Script::List * GetScript(Linker::Module& module);

		void Link(Linker::Module& module);

		size_t GetDefaultDataIndex();

		void ProcessModule(Linker::Module& module) override;

		void CalculateValues() override;

		void WriteFile(Linker::Writer& wr) override;

		std::string GetDefaultExtension(Linker::Module& module, std::string filename) override;
	};
}

#endif /* BWEXP_H */
