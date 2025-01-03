#ifndef SECTION_H
#define SECTION_H

#include <algorithm>
#include <iostream>
#include <string>
#include <variant>
#include <vector>
#include "../common.h"
#include "buffer.h"
#include "position.h"
#include "reader.h"
#include "writable.h"

namespace Linker
{
	class Segment;

	/**
	 * @brief A section of data as read from an object file
	 *
	 * A section represents a series of bytes within an object file, executable or memory image. For segmented architectures such as the 8086, it also represents a segment group at the final stages of linking (once all similar sections have been combined), where each symbol belonging to the same segment will have the same preferred segment base.
	 */
	class Section : public Buffer
	{
	public:
		/**
		 * @brief Name of the section
		 */
		std::string name;
	private:
		/* depends on whether it is zero filled or not */
		//std::vector<uint8_t> data; /* inherited */
		offset_t size; /* only use if ZeroFilled */
	public:
		/**
		 * @brief The type of the section
		 */
		enum section_flags
		{
			/**
			 * @brief The data in the section can be read at runtime
			 */
			Readable = 1 << 0,
			/**
			 * @brief The section can be written to at runtime
			 */
			Writable = 1 << 1,
			/**
			 * @brief The section data can be used as instruction
			 */
			Execable = 1 << 2,
			/**
			 * @brief Sections of the same name with this flag are overlayed instead of appended
			 *
			 * When the sections are zero filled, merged sections will have a size that is the maximum of the merged sections instead of their sum.
			 *
			 * For sections with data, only the longer one is kept.
			 * They are assumed to have identical data.
			 *
			 * TODO: unimplemented
			 */
			Mergeable = 1 << 3,
			/**
			 * @brief Section is filled with zeros
			 *
			 * Most formats do not require sections that are zero filled to be stored in the binary image.
			 * We keep track of which sections are zero filled using this flag.
			 */
			ZeroFilled = 1 << 4, /* note: data should be empty if ZeroFilled is set */
			/**
			 * @brief Section resides at a fixed address and cannot be moved
			 *
			 * When generating code, all sections are assigned starting addresses and are converted to fixed sections.
			 */
			Fixed = 1 << 5,
			/**
			 * @brief Section data represents a resource that has to be handled differently
			 *
			 * The NE, LE/LX, PE and classic Macintosh file formats support storing resources within their binary images.
			 * To ease usage, these resources can be stored in the object file instead of requiring a separate resource file.
			 */
			Resource = 1 << 6,
			/**
			 * @brief Section data may be unallocated if necessary
			 *
			 * Reserved for file formats such as MZFormat and CPM86Format where additional memory can be requested.
			 */
			Optional = 1 << 7,
			/**
			 * @brief Stack section
			 */
			Stack = 1 << 8,
			/**
			 * @brief Heap section
			 */
			Heap = 1 << 9,
			/**
			 * @brief Other flags
			 */
			CustomFlag = 1 << 10,
		} flags;
	private:
		union
		{
			offset_t address; /* only use if fixed is true */
			offset_t align; /* only use if fixed is false */
		};
	public:
		/**
		 * @brief Difference between the first byte of the section and the zero address associated with the section
		 *
		 * On the Intel 8086, addresses consist of two parts: a segment address and an offset within that segment.
		 * The linker collects sections and assigns them to segments, which often map to hardware segments.
		 * On some platforms, the address of the first byte of a section will not be the same as the address of the beginning of the hardware segment it belongs to.
		 * The bias field stores this difference.
		 *
		 * For example, the first byte of a .com file will have an address 0x100.
		 * This is represented by setting the bias to -0x100.
		 */
		offset_t bias;
		/**
		 * @brief The resource type and ID for a resource section
		 *
		 * All resources can have a resource type and a resource ID that can usually be a string or an integer.
		 * Some platforms specify which one is which (for example, Macintosh resources have a 4-character type but a 16-bit ID).
		 * For others (for example, the NE format), they can be either.
		 */
		std::variant<std::string, uint16_t> resource_type, resource_id;

		/**
		 * @brief The segment a section belongs to
		 *
		 * This field is assigned as part of the linking process.
		 */
		Segment * segment;
		/**
		 * @brief Section name that collects sections
		 */
		std::string collection_name;

		Section(std::string name, int flags = Readable)
			:
				name(name),
				size(0),
				flags((section_flags)flags),
				align(1), bias(0),
				resource_type("    "), resource_id(uint16_t(0)),
				segment(nullptr)
		{
		}

	private:
		void AlterFlags(bool state, unsigned flags_mask);

	public:
		/**
		 * @brief Sets the flags of the section
		 *
		 * Certain flags require special handling.
		 */
		void SetFlag(unsigned newflags);

		unsigned GetFlags() const;

		bool IsReadable() const;

		void SetReadable(bool state);

		bool IsWritable() const;

		void SetWritable(bool state);

		bool IsExecable() const;

		void SetExecable(bool state);

		bool IsMergeable() const;

		void SetMergeable(bool state);

		bool IsFixed() const;

		bool IsZeroFilled() const;

		offset_t SetZeroFilled(bool is_zero_filled);

		offset_t GetAlign() const;

		void SetAlign(offset_t new_align);

		offset_t GetStartAddress() const;

		/**
		 * @brief For non-fixed segments, sets the starting address and makes the fixed
		 *
		 * If the segment is already fixed, the address is not changed.
		 * Alignment requirements might cause the new address to be incremented.
		 *
		 * @param new_address Attempted starting address
		 * @return The new starting address
		 */
		offset_t SetAddress(offset_t new_address);

		/**
		 * @brief Forcibly alters the starting address
		 */
		void ResetAddress(offset_t new_address);

		offset_t Size() const;

		/**
		 * @brief Increases the size of the section by the specified amount
		 *
		 * @param new_size The new size for the section. If it is smaller than the current size, nothing is changed.
		 * @return The actual amount of bytes the section was increased by.
		 */
		offset_t Expand(offset_t new_size);

		/**
		 * @brief Expands the section to a size such that its end is at a specified alignment
		 */
		offset_t RealignEnd(offset_t align);

		int GetByte(offset_t offset) override;

		uint64_t ReadUnsigned(size_t bytes, offset_t offset, EndianType endiantype) const;

		uint64_t ReadUnsigned(size_t bytes, offset_t offset) const;

		int64_t ReadSigned(size_t bytes, offset_t offset, EndianType endiantype) const;

		uint64_t ReadSigned(size_t bytes, offset_t offset) const;

		void WriteWord(size_t bytes, offset_t offset, uint64_t value, EndianType endiantype);

		void WriteWord(size_t bytes, offset_t offset, uint64_t value);

		/**
		 * @brief Writes value at the current end of the section
		 */
		void WriteWord(size_t bytes, uint64_t value, EndianType endiantype);

		/**
		 * @brief Writes value at the current end of the section
		 */
		void WriteWord(size_t bytes, uint64_t value);

		/**
		 * @brief Appends data at the end of a section
		 */
		offset_t Append(const void * new_data, size_t length);

		/**
		 * @brief Appends data at the end of a section
		 */
		offset_t Append(const char * new_data);

		/**
		 * @brief Appends (or merges) another section
		 */
		offset_t Append(const Section& other);

		/**
		 * @brief Appends (or merges) another writable buffer
		 */
		offset_t Append(Buffer& buffer);

		/**
		 * @brief Retrieves the address of the first byte of the section
		 */
		Position Start() const;

		/**
		 * @brief Retrieves the address of the start of the segment of the section
		 *
		 * For Intel 8086, it is expected that the data in a segment does not begin at the same location as the base of the segment.
		 */
		Position Base() const;

		/**
		 * @brief Overwrites section data with contents of input stream
		 *
		 * Note that only as many bytes are read in as the size of the section.
		 */
		void ReadFile(std::istream& in);

		/**
		 * @brief Overwrites section data with contents of input stream
		 *
		 * Note that only as many bytes are read in as the size of the section.
		 */
		void ReadFile(Reader& in);

		/**
		 * @brief Writes data into file
		 *
		 * Note that zero filled sections do not write anything.
		 *
		 * @param out Output stream
		 * @param size Maximum number of bytes to write
		 * @return Count of actual number of bytes written
		 */
		offset_t WriteFile(std::ostream& out, offset_t size, offset_t offset = 0) const;

		/**
		 * @brief Writes data into file
		 *
		 * Note that zero filled sections do not write anything.
		 *
		 * @param out Output stream
		 * @return Count of actual number of bytes written
		 */
		offset_t WriteFile(std::ostream& out) const;

		/**
		 * @brief Clear the section
		 */
		void Reset();
	};

	std::ostream& operator<<(std::ostream& out, const Section& section);
}

#endif /* SECTION_H */
