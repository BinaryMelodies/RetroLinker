#ifndef IMAGE_H
#define IMAGE_H

#include <vector>
#include "../common.h"

namespace Linker
{
	class Image;
	class Reader;
	class Writer;

	/**
	 * @brief Represents abstract data contents whose data can be written to a file
	 */
	class Contents
	{
	public:
		virtual ~Contents() = default;
		/**
		 * @brief Retrieves size of stored data
		 */
		virtual offset_t ImageSize() const = 0;
		/**
		 * @brief Writes data of non-zero filled sections
		 */
		virtual offset_t WriteFile(Writer& wr, offset_t count, offset_t offset = 0) const = 0;
		/**
		 * @brief Writes data of non-zero filled sections
		 */
		virtual offset_t WriteFile(Writer& wr) const;
		/**
		 * @brief Retrieves a randomly accessible image
		 */
		virtual std::shared_ptr<const Image> AsImage() const;
		/**
		 * @brief Retrieves a randomly accessible image
		 */
		std::shared_ptr<Image> AsImage();
	};

	class Image : public Contents, public std::enable_shared_from_this<Image>
	{
	public:
		std::shared_ptr<const Image> AsImage() const override;

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
