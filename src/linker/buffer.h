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
	class Buffer : public Image
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
		 * @brief Increases the size of the buffer by the specified amount
		 *
		 * @param new_size The new size for the buffer. If it is smaller than the current size, nothing is changed.
		 * @return The actual amount of bytes the buffer was increased by.
		 */
		virtual offset_t Expand(offset_t new_size);
		/**
		 * @brief Writes data into the buffer image
		 *
		 * @return The amount of bytes the buffer was increased by.
		 */
		virtual offset_t WriteData(size_t bytes, offset_t offset, const void * buffer);
		/**
		 * @brief Append data to buffer
		 */

		/**
		 * @brief Writes a value into the buffer image
		 *
		 * @return The amount of bytes the buffer was increased by.
		 */
		virtual offset_t WriteWord(size_t bytes, offset_t offset, uint64_t value, EndianType endiantype);

		/**
		 * @brief Writes a value into the buffer image
		 *
		 * @return The amount of bytes the buffer was increased by.
		 */
		offset_t WriteWord(size_t bytes, offset_t offset, uint64_t value);

		/**
		 * @brief Writes a value at the current end of the buffer
		 *
		 * @return The amount of bytes the buffer was increased by.
		 */
		offset_t WriteWord(size_t bytes, uint64_t value, EndianType endiantype);

		/**
		 * @brief Writes value at the current end of the buffer
		 *
		 * @return The amount of bytes the buffer was increased by.
		 */
		offset_t WriteWord(size_t bytes, uint64_t value);

		/**
		 * @brief Appends data at the end of a buffer
		 *
		 * @param new_data Pointer to data to append
		 * @param length Number of bytes of data to load from pointer and insert into buffer
		 * @return The offset of the newly written data within the buffer
		 */
		virtual offset_t Append(const void * new_data, size_t length);

		/**
		 * @brief Appends data at the end of a buffer
		 */
		offset_t Append(const char * new_data);

		/**
		 * @brief Appends data at the end of a buffer
		 */
		offset_t Append(std::vector<uint8_t>& additional_data);

		/**
		 * @brief Overwrites buffer data with contents of reader
		 *
		 * Note that only as many bytes are read in as the size of the buffer.
		 */
		virtual void ReadFile(const std::shared_ptr<Reader>& rd);
		/**
		 * @brief Overwrites buffer data with contents of reader
		 *
		 * All the remaining bytes are read, the buffer is expanded if needed but not shrank
		 */
		void ReadFileRemaining(const std::shared_ptr<Reader>& rd);
		/**
		 * @brief Overwrites buffer data with contents of reader
		 *
		 * Exactly the specified amount is read, the buffer is expanded if needed but not shrank
		 */
		void ReadFile(const std::shared_ptr<Reader>& rd, offset_t count);
		/**
		 * @brief Creates a buffer containing the remaining data in the reader
		 */
		static std::shared_ptr<Buffer> ReadFromFile(const std::shared_ptr<Reader>& rd);
		/**
		 * @brief Creates a buffer containing the specified amount of bytes from the reader
		 *
		 * If less data is available, the buffer will be shorter
		 */
		static std::shared_ptr<Buffer> ReadFromFile(const std::shared_ptr<Reader>& rd, offset_t count);
		using Contents::WriteFile;
		offset_t WriteFile(Writer& wr, offset_t count, offset_t offset = 0) const override;
		size_t ReadData(size_t bytes, offset_t offset, void * buffer) const override;

		friend class Section;
		friend class Contents;
	};
}

#endif /* BUFFER_H */
