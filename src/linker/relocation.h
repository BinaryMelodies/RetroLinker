#ifndef RELOCATION_H
#define RELOCATION_H

#include "../common.h"
#include "location.h"
#include "target.h"

namespace Linker
{
	class Resolution;

	/**
	 * @brief A representation of a value within some binary data that has to be fixed up once the exact position of certain symbols within memory is known.
	 *
	 * One of the tasks of a linker is to associate symbols in different object files and replace references to them with their values.
	 * These references to outside symbols are called relocations or fix-ups.
	 * When a linker is unable to resolve a relocation that has to be deferred to run time, such relocations must be stored in the resulting executable file.
	 * Different executable file formats have differring levels of support for relocations.
	 *
	 * Since RetroLinker supports many very different targets, the Relocation class is designed to be as general as possible.
	 * At its most basic, a relocation must store the symbol referenced, the *reference*, and the location where it is to be stored, the *source*.
	 * The Relocation class also stores the size in bytes of the value expected, a value that needs to be added to the resolved value, and the endianness of the stored value.
	 * Relative values can also be represented, which are often used for relative branches.
	 * To support Intel 8086 segmentation, the value of the segment selector corresponding to the segment of a symbol may also be used, and resolutions can be made with respect to another reference, usually the start of a segment.
	 */
	class Relocation
	{
	public:
		enum reference_kind
		{
			/** @brief The value of the target is used */
			Direct,
			/**
			 * @brief The paragraph of value is used, obtained via shifting it by 4 to the right
			 *
			 * This is intended for real mode x86 platforms.
			 */
			ParagraphAddress,
			/**
			 * @brief The 8-bit segment of value is used, obtained via shifting it by 16 to the right
			 *
			 * This is intended for segmented Z8000 platforms.
			 */
			SegmentIndex,
			/**
			 * @brief A selector index is used to reference this target
			 *
			 * This is intended for protected mode x86 platforms.
			 */
			SelectorIndex, // TODO: unimplemented
			/**
			 * @brief An entry in the Global Offset Table is used instead of the actual value
			 */
			GOTEntry,
			/**
			 * @brief An entry in the Procedure Linkage Table is used instead of the actual value
			 */
			PLTEntry, // TODO: unimplemented
			/**
			 * @brief Size of the target
			 */
			SizeOf, // TODO: unimplemented
		}
		/**
		 * @brief Specifies the way the target is used to derive the resolved value, typically Direct
		 */
		kind;
		/**
		 * @brief The size of the value when stored at the source, in bytes (for example, 2 for 16-bit, 4 for 32-bit)
		 */
		size_t size;
		/**
		 * @brief The location where the value of the symbol should be stored
		 */
		Location source;
		/**
		 * @brief The symbol or location referenced by the relocation
		 */
		Target target;
		/**
		 * @brief The symbol or location whose value is subtracted from the final value, used for self-relative and segment-relative addresses
		 */
		Target reference;
		/**
		 * @brief A value to be added
		 */
		uint64_t addend;
		/**
		 * @brief The endianness of the stored value
		 */
		EndianType endiantype;
		/**
		 * @brief The amount of bits the value should be shifted by
		 */
		int shift;
		/**
		 * @brief The bitmask of the value within the word
		 */
		uint64_t mask;
		/**
		 * @brief Set to true if value must be negated first
		 */
		bool subtract;

		Relocation(reference_kind kind, size_t size, Location source, Target target, Target reference, uint64_t addend, EndianType endiantype)
			: kind(kind), size(size), source(source), target(target), reference(reference), addend(addend), endiantype(endiantype),
				shift(0), mask(-1), subtract(false)
		{
		}

		/**
		 * @brief Creates an empty relocation
		 *
		 * This is used only as a default initializor.
		 * The generated relocation references an absolute location, and it is not expected to be resolved.
		 */
		static Relocation Empty();

		/**
		 * @brief Creates a relocation referencing the absolute address of a target
		 */
		static Relocation Absolute(size_t size, Location source, Target target, uint64_t addend, EndianType endiantype);

		/**
		 * @brief Creates a relocation referencing the absolute address of a target
		 */
		static Relocation Absolute(size_t size, Location source, Target target, uint64_t addend = 0);

		/**
		 * @brief Creates a relocation that references the offset of a target within its preferred segment (Intel 8086 specific)
		 */
		static Relocation Offset(size_t size, Location source, Target target, uint64_t addend, EndianType endiantype);

		/**
		 * @brief Creates a relocation that references the offset of a target within its preferred segment (Intel 8086 specific)
		 */
		static Relocation Offset(size_t size, Location source, Target target, uint64_t addend = 0);

		/**
		 * @brief Creates a relocation that references the offset of a target from a specific reference point
		 */
		static Relocation OffsetFrom(size_t size, Location source, Target target, Target reference, uint64_t addend, EndianType endiantype);

		/**
		 * @brief Creates a relocation that references the offset of a target from a specific reference point
		 */
		static Relocation OffsetFrom(size_t size, Location source, Target target, Target reference, uint64_t addend = 0);

		/**
		 * @brief Creates a relocation that references the offset of a target from the source
		 */
		static Relocation Relative(size_t size, Location source, Target target, uint64_t addend, EndianType endiantype);

		/**
		 * @brief Creates a relocation that references the offset of a target from the source
		 */
		static Relocation Relative(size_t size, Location source, Target target, uint64_t addend = 0);

		/**
		 * @brief Creates a relocation that stores the 16-bit paragraph (shifted right by 4) of the target (Intel 8086 specific)
		 */
		static Relocation Paragraph(Location source, Target target, uint64_t addend = 0);

		/**
		 * @brief Creates a relocation that stores a 16-bit selector value referencing the target (Intel 8086 specific)
		 */
		static Relocation Selector(Location source, Target target, uint64_t addend = 0);

		/**
		 * @brief Creates a relocation that stores the 8-bit segment number of the target (Zilog Z8000 specific)
		 */
		static Relocation Segment(size_t size, Location source, Target target, uint64_t addend = 0);

		/**
		 * @brief Creates a relocation that stores the 16-bit paragraph difference (shifted right by 4) between the target and the reference (Intel 8086 specific)
		 *
		 * This relocation is needed on platforms with no segment relocations to set up the segments as part of an initializing routine by adding the offset between segments to the initial value of one of the segment registers.
		 * For example, ELKS needs it.
		 */
		static Relocation ParagraphDifference(Location source, Target target, Target reference, uint64_t addend = 0);

		/**
		 * @brief Creates a relocation referencing the absolute address of a GOT entry
		 */
		static Relocation GOTEntryAbsolute(size_t size, Location source, SymbolName target, uint64_t addend, EndianType endiantype);

		/**
		 * @brief Creates a relocation that references the offset of a GOT entry from the source
		 */
		static Relocation GOTEntryRelative(size_t size, Location source, SymbolName target, uint64_t addend, EndianType endiantype);

		/**
		 * @brief Creates a relocation that references the offset of a GOT entry from the start of the GOT
		 */
		static Relocation GOTEntryOffset(size_t size, Location source, SymbolName target, uint64_t addend, EndianType endiantype);

		/**
		 * @brief Instead of the full word, only modify the following bits
		 */
		Relocation& SetMask(uint64_t new_mask);

		/**
		 * @brief The value stored in the word must be shifted by this to give the actual value
		 */
		Relocation& SetShift(int new_shift);

		/**
		 * @brief The value stored in the word must be negated before adding the addend and storing
		 */
		Relocation& SetSubtract(bool to_subtract = true);

		/**
		 * @brief Recalculates the source, target and reference locations after a section has moved
		 */
		bool Displace(const Displacement& displacement);

		/**
		 * @brief If the target and reference symbols can be resolved, return the value with some additional information about the segments
		 */
		bool Resolve(Module& object, Resolution& resolution);

		/**
		 * @brief Accesses the value within the section data
		 */
		uint64_t ReadUnsigned();

		/**
		 * @brief Accesses the value within the section data
		 */
		int64_t ReadSigned();

		/**
		 * @brief Accesses the value within the section data
		 */
		void WriteWord(uint64_t value);

		/**
		 * @brief Updates the addend with the value stored in the section data
		 *
		 * Some object formats do not store the addend in the relocation data.
		 * Instead, the value is expected to be added to the value already present in the binary image.
		 * We use this function to load the addend from the image data.
		 */
		void AddCurrentValue();

		/**
		 * @brief Determines if a relocation is self-relative
		 *
		 * The Relocation class represents self-relative relocations using the reference field.
		 * For some output formats, it is important to tell whether a relocation is self-relative.
		 */
		bool IsRelative() const;
	};

	std::ostream& operator<<(std::ostream& out, const Relocation& relocation);
}

#endif /* RELOCATION_H */
