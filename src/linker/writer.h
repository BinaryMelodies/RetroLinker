#ifndef WRITER_H
#define WRITER_H

#include <iostream>
#include <string>
#include <vector>
#include "../common.h"

namespace Linker
{
	/**
	 * @brief A helper class, encapsulating functionality needed to export binary data
	 */
	class Writer
	{
	public:
		/**
		 * @brief The default endianness of the binary format, used for reading multibyte numeric data
		 */
		EndianType endiantype;
		/**
		 * @brief The output stream
		 */
		std::ostream * out;

		Writer(EndianType endiantype, std::ostream * out = nullptr)
			: endiantype(endiantype), out(out)
		{
		}

		/**
		 * @brief Write out a sequence of bytes
		 */
		void WriteData(size_t count, const void * data);

		/**
		 * @brief Write out a sequence of bytes
		 */
		size_t WriteData(size_t max_count, const std::vector<uint8_t>& data, size_t offset = 0);

		/**
		 * @brief Write out a sequence of bytes
		 */
		size_t WriteData(const std::vector<uint8_t>& data, size_t offset = 0);

		/**
		 * @brief Write out a sequence of bytes
		 */
		template <class T, std::size_t N>
			void WriteData(const std::array<T, N>& data, size_t offset = 0)
		{
			if(offset >= N)
				return;
			WriteData(N - offset, reinterpret_cast<const char *>(data.data()) + offset);
		}

		/**
		 * @brief Write a string, possibly truncated or zero padded
		 */
		void WriteData(size_t count, std::string text, char padding = '\0');

		/**
		 * @brief Write a string
		 */
		void WriteData(std::string text);

		/**
		 * @brief Write data using an input stream as the source of data
		 */
		void WriteData(size_t count, std::istream& in);

		/**
		 * @brief Read a word
		 */
		void WriteWord(size_t bytes, uint64_t value, EndianType endiantype);

		/**
		 * @brief Read a word
		 */
		void WriteWord(size_t bytes, uint64_t value);

	protected:
		void ForceSeek(offset_t offset);

	public:
		/**
		 * @brief Jump to a specific location in the ouput stream
		 */
		void Seek(offset_t offset);

		/**
		 * @brief Jump to a distance in the output stream
		 */
		void Skip(offset_t offset);

		/**
		 * @brief Jump to a specific offset from the end
		 */
		void SeekEnd(offset_t offset = 0);

		/**
		 * @brief Retrieve the current location
		 */
		offset_t Tell();

		/**
		 * @brief Move to a specific offset, fill with zeroes if needed
		 */
		void FillTo(offset_t position);

		/**
		 * @brief Align the current pointer
		 */
		void AlignTo(offset_t align);
	};
}

#endif /* WRITER_H */
