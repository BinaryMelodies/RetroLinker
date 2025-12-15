#ifndef BUFFER_H
#define BUFFER_H

#include <algorithm>
#include <iostream>
#include <string>
#include <variant>
#include <vector>
#include "../common.h"
#include "image.h"

namespace Linker
{
	class Reader;
	class Segment;
	class Writer;

	/**
	 * @brief A buffer that can be used to read and store data from a file
	 */
	class Buffer : public ActualImage
	{
	protected:
		std::vector<uint8_t> data;

	public:
		Buffer() = default;

		Buffer(size_t size)
		{
			data.resize(size);
		}

		Buffer(const std::vector<uint8_t>& data)
			: data(data)
		{
		}

		offset_t ImageSize() const override;
		/**
		 * @brief Resize buffer
		 */
		void Resize(offset_t new_size);
		/**
		 * @brief Append data to buffer
		 */
		void Append(std::vector<uint8_t>& additional_data);
		/**
		 * @brief Overwrites buffer data with contents of reader
		 *
		 * Note that only as many bytes are read in as the size of the buffer.
		 */
		virtual void ReadFile(Reader& rd);
		/**
		 * @brief Overwrites buffer data with contents of reader
		 *
		 * All the remaining bytes are read, the buffer is expanded if needed but not shrank
		 */
		void ReadFileRemaining(Reader& rd);
		/**
		 * @brief Overwrites buffer data with contents of reader
		 *
		 * Exactly the specified amount is read, the buffer is expanded if needed but not shrank
		 */
		void ReadFile(Reader& rd, offset_t count);
		/**
		 * @brief Creates a buffer containing the remaining data in the reader
		 */
		static std::shared_ptr<Buffer> ReadFromFile(Reader& rd);
		/**
		 * @brief Creates a buffer containing the specified amount of bytes from the reader
		 *
		 * If less data is available, the buffer will be shorter
		 */
		static std::shared_ptr<Buffer> ReadFromFile(Reader& rd, offset_t count);
		using Contents::WriteFile;
		offset_t WriteFile(Writer& wr, offset_t count, offset_t offset = 0) const override;
		size_t ReadData(size_t bytes, offset_t offset, void * buffer) const override;

		friend class Section;
		friend class Contents;
	};
}

#endif /* BUFFER_H */
