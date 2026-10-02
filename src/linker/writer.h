#ifndef WRITER_H
#define WRITER_H

#include <iostream>
#include <string>
#include <vector>
#include "../common.h"

namespace Linker
{
	/**
	 * @brief Abstract base class that encapsulates functionality needed to export binary data
	 */
	class Writer
	{
	public:
		/**
		 * @brief The default endianness of the binary format, used for reading multibyte numeric data
		 */
		EndianType endiantype;

		Writer(EndianType endiantype)
			: endiantype(endiantype)
		{
		}

		virtual ~Writer() = default;

		/**
		 * @brief Write out a sequence of bytes
		 */
		virtual void WriteData(size_t count, const void * data) = 0;

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
		 * @brief Write a word
		 */
		void WriteWord(size_t bytes, uint64_t value, EndianType endiantype);

		/**
		 * @brief Write a word
		 */
		void WriteWord(size_t bytes, uint64_t value);

		/**
		 * @brief Write a date-time value according to some epoch
		 */
		template <typename Clock>
			void WriteTimestamp(Timestamp<Clock> timestamp, EndianType endiantype)
		{
			WriteWord(sizeof(typename Clock::rep), Clock::to_ticks(timestamp), endiantype);
		}

		/**
		 * @brief Write a date-time value according to some epoch
		 */
		template <typename Clock>
			void WriteTimestamp(Timestamp<Clock> timestamp)
		{
			WriteWord(sizeof(typename Clock::rep), Clock::to_ticks(timestamp));
		}

		/**
		 * @brief Jump to a specific location in the ouput stream
		 */
		virtual void Seek(offset_t offset) = 0;

		/**
		 * @brief Jump to a distance in the output stream
		 */
		virtual void Skip(offset_t offset) = 0;

		/**
		 * @brief Jump to a specific offset from the end
		 */
		virtual void SeekEnd(offset_t offset = 0) = 0;

		/**
		 * @brief Retrieve the current location
		 */
		virtual offset_t Tell() = 0;

		/**
		 * @brief Move to a specific offset, fill with zeroes if needed
		 */
		void FillTo(offset_t position);

		/**
		 * @brief Align the current pointer
		 */
		void AlignTo(offset_t align);
	};

	/**
	 * @brief Writer subclass whose target is a C++ output stream
	 */
	class StreamWriter : public Writer
	{
	public:
		/**
		 * @brief The output stream
		 */
		std::ostream * out;

		StreamWriter(EndianType endiantype, std::ostream * out = nullptr)
			: Writer(endiantype), out(out)
		{
		}

		StreamWriter(EndianType endiantype, std::ostream& out)
			: Writer(endiantype), out(&out)
		{
		}

	protected:
		void _FillNulls(size_t count);

	public:
		void WriteData(size_t count, const void * data) override;
		void Seek(offset_t offset) override;
		void Skip(offset_t offset) override;
		void SeekEnd(offset_t offset = 0) override;
		offset_t Tell() override;
	};

	class Buffer;

	// TODO: untested
	/**
	 * @brief Writer subclass that stores data into an internal buffer
	 */
	class BufferWriter : public Writer
	{
	public:
		std::shared_ptr<Linker::Buffer> buffer;
		offset_t position = 0;

		// TODO: allocate buffer if parameter not provided

		BufferWriter(EndianType endiantype, std::shared_ptr<Linker::Buffer> buffer)
			: Writer(endiantype), buffer(buffer)
		{
		}

		void WriteData(size_t count, const void * data) override;
		void Seek(offset_t offset) override;
		void Skip(offset_t offset) override;
		void SeekEnd(offset_t offset = 0) override;
		offset_t Tell() override;
	};
}

#endif /* WRITER_H */
