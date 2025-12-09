#ifndef READER_H
#define READER_H

#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include "../common.h"

namespace Linker
{
	/**
	 * @brief A helper class, encapsulating functionality needed to import binary data
	 */
	class Reader
	{
	public:
		/**
		 * @brief The default endianness of the binary format, used for reading multibyte numeric data
		 */
		EndianType endiantype;
		/**
		 * @brief The input stream
		 */
		std::istream * in;

		const offset_t start_offset;
		const offset_t maximum_size;

		Reader(EndianType endiantype, std::istream * in = nullptr)
			: endiantype(endiantype), in(in), start_offset(0), maximum_size(offset_t(-1))
		{
		}

		Reader(offset_t start_offset, offset_t maximum_size, EndianType endiantype, std::istream * in = nullptr)
			: endiantype(endiantype), in(in), start_offset(start_offset), maximum_size(maximum_size)
		{
		}

		Reader CreateWindow(offset_t new_start_offset, offset_t new_maximum_size = offset_t(-1));

		/**
		 * @brief Read in a sequence of bytes
		 */
		void ReadData(size_t count, void * data);

		/**
		 * @brief Read in a sequence of bytes, resize the vector
		 */
		void ReadData(size_t count, std::vector<uint8_t>& data, size_t offset = 0);

		/**
		 * @brief Read in a sequence of bytes, filling the vector
		 */
		void ReadData(std::vector<uint8_t>& data, size_t offset = 0);

		/**
		 * @brief Read in a sequence of bytes
		 */
		std::string ReadData(size_t count, bool terminate_at_null = false);

		/**
		 * @brief Read in a sequence of 16-bit words
		 */
		std::string ReadUTF16Data(size_t count, bool terminate_at_null = false);

		/**
		 * @brief Read in a sequence of bytes
		 */
		template <class T, std::size_t N>
			void ReadData(std::array<T, N>& data, size_t offset = 0)
		{
			if(offset >= N)
				return;
			ReadData(N - offset, reinterpret_cast<char *>(data.data()) + offset);
		}

		/**
		 * @brief Read an ASCII string up to a terminator
		 */
		std::string ReadASCII(char terminator, size_t maximum = size_t(-1));

		/**
		 * @brief Read a zero terminated ASCII string
		 */
		std::string ReadASCIIZ(size_t maximum = size_t(-1));

		/**
		 * @brief Read a string of 16-bit words up to a terminator
		 */
		std::string ReadUTF16Data(const char terminator[2], size_t maximum = size_t(-1));

		/**
		 * @brief Read a string of 16-bit words up to a terminator
		 */
		std::string ReadUTF16Data(char16_t terminator, size_t maximum, EndianType endiantype);

		/**
		 * @brief Read a string of 16-bit words up to a terminator
		 */
		std::string ReadUTF16Data(char16_t terminator, size_t maximum = size_t(-1));

		/**
		 * @brief Read a string of 16-bit words up to a terminator
		 */
		std::string ReadUTF16Data(char16_t terminator, EndianType endiantype);

		/**
		 * @brief Read a zero terminated string of 16-bit words
		 */
		std::string ReadUTF16ZData(size_t maximum = size_t(-1));

		/**
		 * @brief Read an unsigned word
		 */
		uint64_t ReadUnsigned(size_t bytes, EndianType endiantype);

		/**
		 * @brief Read an unsigned word
		 */
		uint64_t ReadUnsigned(size_t bytes);

		/**
		 * @brief Read a signed word
		 */
		uint64_t ReadSigned(size_t bytes, EndianType endiantype);

		/**
		 * @brief Read a signed word
		 */
		uint64_t ReadSigned(size_t bytes);

		/**
		 * @brief Jump to a specific location in the input stream
		 */
		void Seek(offset_t offset);

		/**
		 * @brief Jump to a distance in the input stream
		 */
		void Skip(offset_t offset);

		/**
		 * @brief Jump to end of the input stream
		 */
		void SeekEnd(relative_offset_t offset = 0);

		/**
		 * @brief Retrieve the current location
		 */
		offset_t Tell();
	};
}

#endif /* READER_H */
