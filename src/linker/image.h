#ifndef IMAGE_H
#define IMAGE_H

#include <vector>
#include "../common.h"
#include "reader.h"
#include "writer.h"

namespace Linker
{
	class ActualImage;

	/**
	 * @brief Represents an abstract data image whose data can be written to a file
	 */
	class Image
	{
	public:
		/**
		 * @brief Retrieves size of stored data
		 */
		virtual offset_t ImageSize() = 0;
		/**
		 * @brief Writes data of non-zero filled sections
		 */
		virtual offset_t WriteFile(Writer& wr, offset_t count, offset_t offset = 0) = 0;
		/**
		 * @brief Writes data of non-zero filled sections
		 */
		virtual offset_t WriteFile(Writer& wr);
		/**
		 * @brief Retrieves a randomly accessible image
		 */
		virtual std::shared_ptr<ActualImage> AsImage();
		virtual ~Image() = default;
	};

	class ActualImage : public Image, public std::enable_shared_from_this<ActualImage>
	{
	public:
		std::shared_ptr<ActualImage> AsImage() override;

		/**
		 * @brief Attempts to fill a buffer with data
		 *
		 * @param bytes The (maximum) number of bytes to place in buffer
		 * @param offset The offset in the image to start reading the data
		 * @param buffer A byte buffer
		 * @result Actual number of bytes copied into buffer
		 */
		virtual size_t ReadData(size_t bytes, offset_t offset, void * buffer) const = 0;

		/** @brief Reads an unsigned number at a specific offset */
		uint64_t ReadUnsigned(size_t bytes, offset_t offset, EndianType endiantype) const;

		/** @brief Reads an unsigned number at a specific offset with the default endianness */
		uint64_t ReadUnsigned(size_t bytes, offset_t offset) const;

		/** @brief Reads a signed number at a specific offset */
		int64_t ReadSigned(size_t bytes, offset_t offset, EndianType endiantype) const;

		/** @brief Reads a signed number at a specific offset with the default endianness */
		int64_t ReadSigned(size_t bytes, offset_t offset) const;

		/**
		 * @brief Retrieve byte at a certain offset (optional, might not be defined)
		 */
		int GetByte(offset_t offset) const;
	};
}

#endif /* IMAGE_H */
