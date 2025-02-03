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
		 * @brief Retrieve byte at a certain offset (optional, might not be defined)
		 */
		virtual int GetByte(offset_t offset) = 0;
	};
}

#endif /* IMAGE_H */
