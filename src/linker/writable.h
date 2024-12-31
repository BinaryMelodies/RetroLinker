#ifndef WRITABLE_H
#define WRITABLE_H

#include <vector>
#include "../common.h"
#include "reader.h"
#include "writer.h"

namespace Linker
{
	/**
	 * @brief Represents an abstract interface whose data can be written to a file
	 */
	class Writable
	{
	public:
		/**
		 * @brief Retrieves size of stored data
		 */
		virtual offset_t ActualDataSize() = 0;
		/**
		 * @brief Writes data of non-zero filled sections
		 */
		virtual offset_t WriteFile(Writer& wr, offset_t count, offset_t offset = 0) = 0;
		/**
		 * @brief Writes data of non-zero filled sections
		 */
		offset_t WriteFile(Writer& wr)
		{
			return WriteFile(wr, ActualDataSize());
		}
		/**
		 * @brief Retrieve byte at a certain offset (optional, might not be defined)
		 */
		virtual int GetByte(offset_t offset);
		virtual ~Writable();
	};
}

#endif /* WRITABLE_H */
