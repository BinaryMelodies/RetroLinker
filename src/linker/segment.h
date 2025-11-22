#ifndef SEGMENT_H
#define SEGMENT_H

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "../common.h"
#include "image.h"

namespace Linker
{
	class Section;
	class Writer;

	/**
	 * @brief A class representing a sequence of sections that must be written to the output file as a group
	 *
	 * A segment represents a collection of sections that appear within the same addressing space.
	 * It also makes it easier to write several sections into the output file consecutively.
	 *
	 * Only sections that appear consecutively in memory (or with known displacements at linking time) may be stored in the same segment.
	 */
	class Segment : public ActualImage
	{
	public:
		/**
		 * @brief The name of the segment
		 */
		std::string name;
		/**
		 * @brief Sequence of sections belonging to the segment
		 */
		std::vector<std::shared_ptr<Section>> sections;
		/**
		 * @brief Address where segment starts
		 */
		offset_t base_address;
		/**
		 * @brief Alignment requirements of segment
		 */
		offset_t align = 1;
		/**
		 * @brief Cached value of the non-filled data for the entire segment
		 *
		 * This should be equal to the sum of all sizes of sections are not zero filled.
		 * Note that a non zero filled section may not follow a zero filled one.
		 */
		offset_t data_size = 0;
		/**
		 * @brief Extra zero filled space following filled data
		 *
		 * This should be equal to the sum of all sizes of sections are zero filled.
		 * Note that a non zero filled section may not follow a zero filled one.
		 */
		offset_t zero_fill = 0;
		/**
		 * @brief Optional extra space after zero filled data
		 */
		offset_t optional_extra = 0;

		Segment(std::string name, offset_t base_address = 0)
			: name(name), base_address(base_address)
		{
		}

		/**
		 * @brief Segment that contains neither non zero filled, nor zero filled, nor optional extra data
		 */
		bool IsMissing();
		/**
		 * @brief Fills zero filled sections with zero data, making them non-zero filled in the sense that they have to be written to disk
		 */
		void Fill();
		/**
		 * @brief Aligns end of segment by increasing size of last section
		 */
		void RealignEnd(offset_t align);
		/**
		 * @brief Appends section to segment
		 *
		 * This action might require aligning end of last section, possibly zero filling it, and setting the base address and bias of the new section.
		 */
		void Append(std::shared_ptr<Section> section);
		/**
		 * @brief Writes data of non-zero filled sections
		 */
		offset_t WriteFile(std::ostream& out, offset_t size, offset_t offset = 0) const;
		/**
		 * @brief Writes data of non-zero filled sections
		 */
		offset_t WriteFile(std::ostream& out) const;
		/**
		 * @brief Writes data of non-zero filled sections
		 */
		offset_t WriteFile(Writer& wr, offset_t count, offset_t offset = 0) const override;
		/**
		 * @brief Writes data of non-zero filled sections
		 */
		offset_t WriteFile(Writer& wr) const override;

		size_t ReadData(size_t bytes, offset_t offset, void * buffer) const override;

		/**
		 * @brief Retrieves total size of segment
		 */
		offset_t TotalSize();
		/**
		 * @brief Retrieves size of all data in segment, as present in the binary image
		 *
		 * This function should always return the same value as data_size.
		 * This is checked by an assert value.
		 */
		offset_t ImageSize() const override;
		/**
		 * @brief Returns starting address (base_address)
		 */
		offset_t GetStartAddress();
		/**
		 * @brief Returns end address (GetStartAddress() + TotalSize())
		 */
		offset_t GetEndAddress();
		/**
		 * @brief Increases final section to end on specified address
		 *
		 * Note that decreasing a segment is not possible
		 */
		void SetEndAddress(offset_t address);
		/**
		 * @brief Aligns the end of the segment
		 */
		void AlignEndAddress(offset_t align);
	protected:
		void ShiftAddress(int64_t amount);
	public:
		/**
		 * @brief Forcibly resets starting address of segment
		 */
		void SetStartAddress(offset_t address);

		/**
		 * @brief Assuming all sections are zero filled, it deletes up to count bytes from the beginning, as much as available
		 *
		 * @param count Number of bytes to delete
		 */
		void DropInitialZeroes(offset_t count);
	protected:
		std::shared_ptr<Segment> shared_from_this();
	};

	std::ostream& operator<<(std::ostream& out, const Segment& segment);
}

#endif /* SEGMENT_H */
