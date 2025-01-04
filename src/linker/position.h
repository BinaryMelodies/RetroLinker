#ifndef POSITION_H
#define POSITION_H

#include "../common.h"

namespace Linker
{
	class Segment;

	/**
	 * @brief Stores an absolute address along with the containing segment or address space
	 *
	 * When linking in large models, it is important to track if symbols belong to different segments/address spaces, so that a corresponding relocation can be generated for them.
	 * This class makes it easier to check whether the target and reference frame of a relocation is in the same address space.
	 */
	class Position
	{
	public:
		/**
		 * @brief The address of the position, independent of segment it belongs to
		 */
		offset_t address;
		/**
		 * @brief The segment or address space of the position
		 */
		std::shared_ptr<Segment> segment;

		Position(offset_t address = 0, std::shared_ptr<Segment> segment = nullptr)
			: address(address), segment(segment)
		{
		}

		/**
		 * @brief Returns the offset from the start of the segment
		 */
		offset_t GetSegmentOffset() const;

		/**
		 * @brief Arithmetic on the address
		 */
		Position& operator+=(offset_t value);

		/**
		 * @brief Arithmetic on the address
		 */
		Position& operator-=(offset_t value);
	};

	/**
	 * @brief Arithmetic on positions
	 */
	Position operator+(Position a, offset_t b);

	/**
	 * @brief Arithmetic on positions
	 */
	Position operator-(Position a, offset_t b);

	/**
	 * @brief Arithmetic on positions
	 */
	bool operator ==(const Position& a, const Position& b);

	/**
	 * @brief Arithmetic on positions
	 */
	bool operator !=(const Position& a, const Position& b);

	std::ostream& operator<<(std::ostream& out, const Position& position); /* implemented separately to avoid circular references */
}

#endif /* POSITION_H */
