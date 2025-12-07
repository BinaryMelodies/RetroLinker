#ifndef BUFFER_H
#define BUFFER_H

#include <algorithm>
#include <iostream>
#include <string>
#include <variant>
#include <vector>
#include "../common.h"
#include "image.h"

namespace Linker
{
	class Reader;
	class Segment;
	class Writer;

	/**
	 * @brief A buffer that can be used to read and store data from a file
	 */
	class Buffer : public ActualImage
	{
	protected:
		std::vector<uint8_t> data;

	public:
		Buffer() = default;

		Buffer(size_t size)
		{
			data.resize(size);
		}

		Buffer(const std::vector<uint8_t>& data)
			: data(data)
		{
		}

		offset_t ImageSize() const override;
		virtual void ReadFile(Reader& rd);
		void ReadFile(Reader& rd, offset_t count);
		static std::shared_ptr<Buffer> ReadFromFile(Reader& rd);
		static std::shared_ptr<Buffer> ReadFromFile(Reader& rd, offset_t count);
		using Image::WriteFile;
		offset_t WriteFile(Writer& wr, offset_t count, offset_t offset = 0) const override;
		size_t ReadData(size_t bytes, offset_t offset, void * buffer) const override;

		friend class Section;
		friend class Image;
	};
}

#endif /* BUFFER_H */
