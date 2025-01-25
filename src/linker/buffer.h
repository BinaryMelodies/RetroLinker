#ifndef BUFFER_H
#define BUFFER_H

#include <algorithm>
#include <iostream>
#include <string>
#include <variant>
#include <vector>
#include "../common.h"
#include "image.h"
#include "position.h"
#include "reader.h"

namespace Linker
{
	class Segment;

	/**
	 * @brief A buffer that can be used to read and store data from a file
	 */
	class Buffer : public Image
	{
	protected:
		std::vector<uint8_t> data;

	public:
		Buffer()
		{
		}

		Buffer(size_t size)
		{
			data.resize(size);
		}

		offset_t ActualDataSize() override;
		void ReadFile(Reader& rd);
		void ReadFile(Reader& rd, offset_t count);
		static std::shared_ptr<Buffer> ReadFromFile(Reader& rd);
		static std::shared_ptr<Buffer> ReadFromFile(Reader& rd, offset_t count);
		using Image::WriteFile;
		offset_t WriteFile(Writer& wr, offset_t count, offset_t offset = 0) override;
		int GetByte(offset_t offset) override;

		friend class Section;
	};
}

#endif /* BUFFER_H */
