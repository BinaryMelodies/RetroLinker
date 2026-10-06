#ifndef READER_H
#define READER_H

#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "../common.h"

namespace Linker
{
	class ReadOverflow
	{
	};

	/**
	 * @brief Abstract base class that encapsulates functionality needed to import binary data
	 */
	class Reader : public std::enable_shared_from_this<Reader>
	{
	public:
		/**
		 * @brief The default endianness of the binary format, used for reading multibyte numeric data
		 */
		EndianType endiantype;

		/**
		 * @brief Describes how ReadData and all other reading routines should behave when reading exceeds limits (end of file or dynamic boundary)
		 */
		enum OverflowHandlingMode
		{
			/**
			 * @brief Display error message, continue parsing
			 *
			 * This is intended for data structures that have a static size, permitting the user to view contents even in malformed binary files.
			 */
			IgnoreOnOverflow,
			/**
			 * @brief Throw a ReadOverflow exception
			 *
			 * This is useful when the parse routine can manage overflow errors and continue parsing.
			 */
			ReportOnOverflow,
			/**
			 * @brief Execution immediately stops
			 *
			 * Only use when a read error makes it impossible to continue with parsing.
			 * Intended for dynamically sized data structures that are crucial for a complete parsing of the file (such as a section header table).
			 */
			TerminateOnOverflow,
		};
		/** @brief Intended behavior of reading routines on read overflow */
		OverflowHandlingMode on_overflow = TerminateOnOverflow;

		/**
		 * @brief Describes how the user expects the parser to behave on read overflow
		 */
		enum class OverflowHandlingRequest
		{
			/** @brief Let the parser routine decide */
			Default,
			/** @brief Ignore all boundary overflows and keep reading */
			Force,
			/** @brief Terminate program as soon as boundary overflow happens, even if overflow is harmless */
			Report,
		};
		/** @brief Global flag to control how reading overflow should be handled */
		static OverflowHandlingRequest global_overflow_behavior;

		inline OverflowHandlingMode GetActionOnOverflow()
		{
			switch(global_overflow_behavior)
			{
			case OverflowHandlingRequest::Default:
			default:
				return on_overflow;
			case OverflowHandlingRequest::Force:
				return IgnoreOnOverflow;
			case OverflowHandlingRequest::Report:
				return TerminateOnOverflow;
			}
		}

		Reader(EndianType endiantype)
			: endiantype(endiantype)
		{
		}

		std::shared_ptr<Reader> CreateWindow(offset_t new_start_offset, offset_t new_maximum_size = offset_t(-1), offset_t displacement = 0);

		virtual ~Reader() = default;

		/**
		 * @brief Read in as many bytes as possible
		 */
		virtual size_t Read(void * data, size_t max_count = size_t(-1)) = 0;

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
		 * @brief Read a date-time value according to some epoch
		 */
		template <typename Clock>
			Timestamp<Clock> ReadTimestamp(EndianType endiantype)
		{
			if constexpr(std::is_signed_v<typename Clock::rep>)
				return Clock::to_time_point(ReadSigned(sizeof(typename Clock::rep), endiantype));
			else
				return Clock::to_time_point(ReadUnsigned(sizeof(typename Clock::rep), endiantype));
		}

		/**
		 * @brief Read a date-time value according to some epoch
		 */
		template <typename Clock>
			Timestamp<Clock> ReadTimestamp()
		{
			if constexpr(std::is_signed_v<typename Clock::rep>)
				return Clock::to_time_point(ReadSigned(sizeof(typename Clock::rep)));
			else
				return Clock::to_time_point(ReadUnsigned(sizeof(typename Clock::rep)));
		}

		/**
		 * @brief Jump to a specific location in the input stream
		 */
		virtual void Seek(offset_t offset) = 0;

		/**
		 * @brief Jump to a distance in the input stream
		 */
		virtual void Skip(relative_offset_t offset) = 0;

		/**
		 * @brief Jump to end of the input stream
		 */
		virtual void SeekEnd(relative_offset_t offset = 0) = 0;

		/**
		 * @brief Retrieve the current location
		 */
		virtual offset_t Tell() = 0;

		/**
		 * @brief Returns the last location that can be read
		 */
		virtual offset_t GetImageEnd();

		/**
		 * @brief Returns the byte count until the last location that can be read
		 */
		offset_t GetRemainingCount();
	};

	/**
	 * @brief Reader subclass whose source is a C++ input stream
	 */
	class StreamReader : public Reader
	{
	public:
		/**
		 * @brief The input stream
		 */
		std::istream * in;

		StreamReader(EndianType endiantype, std::istream * in = nullptr)
			: Reader(endiantype), in(in)
		{
		}

		StreamReader(EndianType endiantype, std::istream& in)
			: Reader(endiantype), in(&in)
		{
		}

		size_t Read(void * data, size_t max_count = size_t(-1)) override;
		void Seek(offset_t offset) override;
		void Skip(relative_offset_t offset) override;
		void SeekEnd(relative_offset_t offset = 0) override;
		offset_t Tell() override;
	};

	/**
	 * @brief Reader subclass that restricts access of another Reader to fixed window
	 */
	class WindowReader : public Reader
	{
	public:
		std::shared_ptr<Reader> reader;

		/** @brief The first offset in the base reader that is accessible, 0 meaning all */
		offset_t start_offset;
		/** @brief The maximum number of bytes in the base reader that are accessible, offset_t(-1) is reserved to mean all */
		offset_t maximum_size;
		/** @brief The lowest address that is allowed to be accessed, the offset at which the byte at start_offset is found */
		offset_t window_offset;

		// The WindowReader offsets [window_offset, window_offset + maximum_size) is mapped to [start_offset, start_offset + maximum_size)
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

		static std::shared_ptr<Reader> _GetReader(std::shared_ptr<Reader> reader)
		{
			if(auto window_reader = std::dynamic_pointer_cast<WindowReader>(reader))
			{
				return window_reader->reader;
			}
			else
			{
				return reader;
			}
		}

		static offset_t _GetStartOffset(std::shared_ptr<Reader> reader)
		{
			if(auto window_reader = std::dynamic_pointer_cast<WindowReader>(reader))
			{
				return window_reader->start_offset;
			}
			else
			{
				return 0;
			}
		}

		static offset_t _GetMaximumSize(std::shared_ptr<Reader> reader)
		{
			if(auto window_reader = std::dynamic_pointer_cast<WindowReader>(reader))
			{
				return window_reader->maximum_size;
			}
			else
			{
				return offset_t(-1);
			}
		}

		static offset_t _GetWindowOffset(std::shared_ptr<Reader> reader)
		{
			if(auto window_reader = std::dynamic_pointer_cast<WindowReader>(reader))
			{
				return window_reader->window_offset;
			}
			else
			{
				return 0;
			}
		}

	public:
		WindowReader(EndianType endiantype, std::shared_ptr<Reader> reader, offset_t start_offset, offset_t maximum_size, offset_t displacement = 0)
			: Reader(endiantype), reader(_GetReader(reader)), start_offset(_GetStartOffset(reader)), maximum_size(_GetMaximumSize(reader)), window_offset(_GetWindowOffset(reader))
		{
			_ShiftStartOffset(start_offset);
			_RestrictMaximumSize(maximum_size);
			_AddDisplacement(displacement);
		}

		WindowReader(std::shared_ptr<Reader> reader, offset_t start_offset, offset_t maximum_size, offset_t displacement = 0)
			: Reader(reader->endiantype), reader(_GetReader(reader)), start_offset(_GetStartOffset(reader)), maximum_size(_GetMaximumSize(reader)), window_offset(_GetWindowOffset(reader))
		{
			_ShiftStartOffset(start_offset);
			_RestrictMaximumSize(maximum_size);
			_AddDisplacement(displacement);
		}

		size_t Read(void * data, size_t max_count = size_t(-1)) override;
		void Seek(offset_t offset) override;
		void Skip(relative_offset_t offset) override;
		void SeekEnd(relative_offset_t offset = 0) override;
		offset_t Tell() override;

		offset_t GetImageEnd() override;
	};

	class Image;

	// TODO: untested
	/**
	 * @brief Reader subclass that reads data from an internally stored image
	 */
	class ImageReader : public Reader
	{
	public:
		std::shared_ptr<Image> image;
		offset_t position = 0;

		ImageReader(EndianType endiantype, std::shared_ptr<Image> image)
			: Reader(endiantype), image(image)
		{
		}

		size_t Read(void * data, size_t max_count = size_t(-1)) override;
		void Seek(offset_t offset) override;
		void Skip(relative_offset_t offset) override;
		void SeekEnd(relative_offset_t offset = 0) override;
		offset_t Tell() override;

		offset_t GetImageEnd() override;
	};
}

#endif /* READER_H */
