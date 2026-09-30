#ifndef PMODE_H
#define PMODE_H

#include "../common.h"
#include "../linker/reader.h"
#include "../linker/segment_manager.h"
#include "../linker/writer.h"

/* TODO: unimplemented */

namespace PMODE
{
	/**
	 * @brief PMODE/W linear executable format (https://github.com/amindlost/pmodew/blob/main/docs/pmw1fmt.txt)
	 */
	class PMW1Format : public virtual Linker::SegmentManager
	{
	public:
		/* * * General members * * */
		offset_t file_offset = 0;
		struct version_type
		{
			uint8_t major;
			uint8_t minor;
		};
		version_type version = { };
		static constexpr uint16_t FLAG_COMPRESSED = 0x0001;
		uint16_t flags = 0;
		uint32_t eip_object = 0;
		uint32_t eip = 0;
		uint32_t esp_object = 0;
		uint32_t esp = 0;
		uint32_t object_table_offset = 0;
		uint32_t relocation_table_offset = 0;
		uint32_t data_offset = 0;

		class Object
		{
		public:
			class Relocation
			{
			public:
				uint8_t type = 0;
				uint32_t source = 0;
				uint8_t target_object = 0;
				uint32_t target_offset = 0;
			};

			uint32_t memory_size = 0;
			uint32_t file_size = 0; // compressed
			uint32_t flags = 0;
			uint32_t relocation_offset = 0;
			uint32_t relocation_count = 0; // only needed during reading
			uint32_t image_size = 0; // without compression
			std::shared_ptr<Linker::Contents> image;
			std::shared_ptr<Linker::Contents> decompressed_image;
			std::vector<Relocation> relocations;
		};
		std::vector<Object> objects;

		void ReadFile(Linker::Reader& rd) override;
		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;

		void Dump(Dumper::Dumper& dump) const override;

		void CalculateValues() override;

		/* * * Reader members * * */

		class CompressedReader
		{
		public:
			std::shared_ptr<Linker::Image> source_image;
			offset_t image_offset = 0;
			uint32_t control_word_buffer = 0;
			uint8_t control_word_size = 0;

			static constexpr size_t DataBufferSize = 4096;
			std::array<uint8_t, DataBufferSize> data_buffer;
			// first valid data byte
			size_t data_buffer_offset = 0;
			// nubmer of valid bytes in buffer
			size_t data_buffer_length = 0;
			// reading position in buffer (should be between first and last valid bytes)
			size_t data_buffer_position = 0;

			void Start(std::shared_ptr<Linker::Image> image);

			bool IsDataBufferEmpty();
			void ReadBytes(size_t offset, size_t count, uint8_t * data);
			void AddBytes(size_t count, uint8_t * data);

			uint32_t ReadNextControlBits(size_t count);
			uint8_t ReadNextDataByte();
			void GenerateNextBytes();
			bool GetNextByte(uint8_t& result);
		};

		/* * * Writer members * * */
		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module, std::string filename) const override;
	};
}

#endif /* PMODE_H */
