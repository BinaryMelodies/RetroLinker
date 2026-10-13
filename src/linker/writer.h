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
		virtual void Skip(relative_offset_t offset) = 0;

		/**
		 * @brief Jump to a specific offset from the end
		 */
		virtual void SeekEnd(relative_offset_t offset = 0) = 0;

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
		void Skip(relative_offset_t offset) override;
		void SeekEnd(relative_offset_t offset = 0) override;
		offset_t Tell() override;
	};

	/**
	 * @brief Writer subclass that restricts access of another Writer to fixed window
	 */
	class WindowWriter : public Writer
	{
	public:
		std::shared_ptr<Writer> writer;

		/** @brief The first offset in the base writer that is accessible, 0 meaning all */
		offset_t start_offset;
		/** @brief The maximum number of bytes in the base writer that are accessible, offset_t(-1) is reserved to mean all */
		offset_t maximum_size;
		/** @brief The lowest address that is allowed to be accessed, the offset at which the byte at start_offset is found */
		offset_t window_offset;

		// The WindowWriter offsets [window_offset, window_offset + maximum_size) is mapped to [start_offset, start_offset + maximum_size)
		// everything outside the window is treated as out of bounds

	protected:
		void _ValidateParameters()
		{
			if(maximum_size != offset_t(-1))
			{
				assert(start_offset <= offset_t(-1) - maximum_size);
				assert(window_offset <= offset_t(-1) - maximum_size);
			}
		}

		/** @brief Decrease the window by shifting its lower bound by addend */
		void _ShiftStartOffset(offset_t addend)
		{
			if(window_offset > addend)
			{
				window_offset -= addend;
			}
			else
			{
				if(maximum_size != offset_t(-1) && maximum_size <= addend - window_offset)
				{
					maximum_size = 0;
				}
				else
				{
					maximum_size -= addend - window_offset;
					start_offset += addend - window_offset; // TODO: overflow
				}
				window_offset = 0;
			}

			_ValidateParameters();
		}

		/** @brief Decrease the window by decrementing its upper bound */
		void _RestrictMaximumSize(offset_t new_maximum)
		{
			if(new_maximum < maximum_size)
			{
				maximum_size = new_maximum;
				_ValidateParameters();
			}
		}

		/** @brief Shift the address values at which the window is accessed, so the previous offset O is accessed at O + extra_displacement */
		void _AddDisplacement(offset_t displacement)
		{
			window_offset += displacement; // TODO: overflow
			_ValidateParameters();
		}

		static std::shared_ptr<Writer> _GetWriter(std::shared_ptr<Writer> writer)
		{
			if(auto window_writer = std::dynamic_pointer_cast<WindowWriter>(writer))
			{
				return window_writer->writer;
			}
			else
			{
				return writer;
			}
		}

		static offset_t _GetStartOffset(std::shared_ptr<Writer> writer)
		{
			if(auto window_writer = std::dynamic_pointer_cast<WindowWriter>(writer))
			{
				return window_writer->start_offset;
			}
			else
			{
				return 0;
			}
		}

		static offset_t _GetMaximumSize(std::shared_ptr<Writer> writer)
		{
			if(auto window_writer = std::dynamic_pointer_cast<WindowWriter>(writer))
			{
				return window_writer->maximum_size;
			}
			else
			{
				return offset_t(-1);
			}
		}

		static offset_t _GetWindowOffset(std::shared_ptr<Writer> writer)
		{
			if(auto window_writer = std::dynamic_pointer_cast<WindowWriter>(writer))
			{
				return window_writer->window_offset;
			}
			else
			{
				return 0;
			}
		}

	public:
		WindowWriter(std::shared_ptr<Writer> writer, offset_t start_offset, offset_t maximum_size, offset_t displacement = 0)
			: Writer(writer->endiantype), writer(_GetWriter(writer)), start_offset(_GetStartOffset(writer)), maximum_size(_GetMaximumSize(writer)), window_offset(_GetWindowOffset(writer))
		{
			_ShiftStartOffset(start_offset);
			_RestrictMaximumSize(maximum_size);
			_AddDisplacement(displacement);
		}

		void WriteData(size_t count, const void * data) override;
		void Seek(offset_t offset) override;
		void Skip(relative_offset_t offset) override;
		void SeekEnd(relative_offset_t offset = 0) override;
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
		void Skip(relative_offset_t offset) override;
		void SeekEnd(relative_offset_t offset = 0) override;
		offset_t Tell() override;
	};
}

#endif /* WRITER_H */
