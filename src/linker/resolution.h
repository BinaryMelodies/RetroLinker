#ifndef RESOLUTION_H
#define RESOLUTION_H

#include <iostream>
#include "../common.h"
#include "target.h"
#include "segment.h"

namespace Linker
{
	/**
	 * @brief Representing a resolved relocation
	 *
	 * When a relocation is resolved, it gives a value, but also the segment (ie. address space) of the target and reference frames.
	 */
	class Resolution
	{
	public:
		/**
		 * @brief The resolved value
		 */
		uint64_t value;
		/**
		 * @brief The segment of the target
		 */
		std::shared_ptr<Segment> target;
		/**
		 * @brief The segment of the reference
		 *
		 * Typical values might have null, indicating an absolute address, or the same segment as target, representing an inter-segment offset
		 */
		std::shared_ptr<Segment> reference;

		Resolution()
		{
		}

		Resolution(uint64_t value, std::shared_ptr<Segment> target, std::shared_ptr<Segment> reference)
			:
				value(value),
				target(target == reference ? nullptr : target),
				reference(target == reference ? nullptr : reference)
		{
		}
	};

	std::ostream& operator<<(std::ostream& out, const Resolution& resolution);
}

#endif /* RESOLUTION_H */
