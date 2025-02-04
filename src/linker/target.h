#ifndef TARGET_H
#define TARGET_H

#include <iostream>
#include <variant>
#include "../common.h"
#include "location.h"
#include "symbol_name.h"

namespace Linker
{
	class Module;

	/**
	 * @brief Represents a possible target or reference frame of a relocation
	 *
	 * When resolving a symbol as part of calculating a relocation, there are many possible outcomes.
	 * A symbol might reference an internal location, or an imported symbol.
	 * For segmented platforms, it might also reference the segment of a symbol, instead of an offset.
	 *
	 * This class provides a representation for this structure.
	 */
	class Target
	{
	public:
		/**
		 * @brief The actual target, either an internal/absolute location, or an imported symbol.
		 */
		std::variant<Location, SymbolName> target;
		/**
		 * @brief Whether the target is the segment, rather than the offset, of the location or symbol.
		 */
		bool segment_of;

		Target(std::variant<Location, SymbolName> target = Location(), bool segment_of = false)
			: target(target), segment_of(segment_of)
		{
		}

		Target(Location location, bool segment_of = false)
			: target(location), segment_of(segment_of)
		{
			/* Note: when resolved, the segment starts at the location section start */
		}

		Target(SymbolName symbol, bool segment_of = false)
			: target(symbol), segment_of(segment_of)
		{
		}

		/**
		 * @brief Creates a new target that references the segment of this target
		 *
		 * For segment targets, it returns an identical Target object.
		 */
		Target GetSegment();

		/**
		 * @brief Recalculates target after a section has moved
		 *
		 * If the target references a location, this function displaces it.
		 * It does nothing for imported symbols.
		 */
		bool Displace(const Displacement& displacement);

		/**
		 * @brief If the target refers to an internal symbol, it gets resolved to a location.
		 *
		 * @return True if target changed, when an internal symbol got resolved.
		 */
		bool ResolveLocals(Module& object);

		/**
		 * @brief Returns a Position object for locations and internal symbols, if possible.
		 *
		 * @param object The Module object where the symbols are looked up
		 * @param position The returned Position object
		 * @return True if lookup succeeded. Targets with undefined symbols return false.
		 */
		bool Lookup(Module& object, Position& position);
	};

	bool operator==(const Target& a, const Target& b);

	bool operator!=(const Target& a, const Target& b);

	std::ostream& operator<<(std::ostream& out, const Target& target);
}

#endif /* TARGET_H */
