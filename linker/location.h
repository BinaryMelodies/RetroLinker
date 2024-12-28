#ifndef LOCATION_H
#define LOCATION_H

#include <iostream>
#include "../common.h"
#include "position.h"

namespace Linker
{
	class Section;

	/**
	 * @brief Represents a single offset within a section, or an absolute location in memory if the section is null
	 */
	class Location
	{
	public:
		/**
		 * @brief The section the symbol is located in, or null for an absolute location
		 */
		Section * section;
		/**
		 * @brief The offset of the symbol within a section, or the absolute address
		 */
		offset_t offset;

		/**
		 * @brief Creates a location within a section
		 */
		Location(Section * section, offset_t offset = 0)
			: section(section), offset(offset)
		{
		}

		/**
		 * @brief Creates an absolute location
		 */
		Location(offset_t offset = 0)
			: section(nullptr), offset(offset)
		{
		}

		/**
		 * @brief Recalculates location after a section has moved
		 *
		 * @param displacement A map from sections to locations, specifying the new starting place of the section.
		 * This can also indicate when a section has been appended to another one, and the location will be updated to reference the new section.
		 * @return True if location changed due to displacement.
		 */
		bool Displace(const Displacement& displacement);

		/**
		 * @brief Calculates the address
		 *
		 * Using the starting addresses of sections, calculates the address of the location, as well as the segment it belongs to.
		 *
		 * On some platforms, most prominently on the Intel 8086, addresses are composed of a preferred segment and an offset within that segment.
		 * Symbols will have an associated hardware segment, which is usually, but not always, mapped onto the linker's idea of a Segment.
		 */
		Position GetPosition(bool segment_of = false) const;

		bool operator==(const Location& other) const;

		Location& operator+=(offset_t value);

		Location& operator-=(offset_t value);
	};

	Location operator+(Location a, offset_t b);
	Location operator-(Location a, offset_t b);
	std::ostream& operator<<(std::ostream& out, const Location& location);
}

#endif /* LOCATION_H */
