#ifndef COMMON_H
#define COMMON_H

#include <cassert>
#include <cstdint>
#include <cstddef>
#include <map>
#include <memory>
#include <iostream>
#include <string>

typedef uint64_t offset_t;
typedef int64_t relative_offset_t;
typedef size_t number_t;

enum EndianType
{
	UndefinedEndian,
	LittleEndian,
	BigEndian,
	PDP11Endian, /* little endian within a 16-bit word, big endian between words */
	AntiPDP11Endian,
};

extern EndianType DefaultEndianType;

/**
 * @brief Evaluates the offset of a byte within a word, according to the endianness
 */
size_t GetOffset(EndianType endiantype, size_t bytes, size_t index);

/**
 * @brief Rounds value up to closest value with specified alignment, which must be a power of 2
 */
offset_t AlignTo(offset_t value, offset_t align);

/**
 * @brief Swaps bytes in 16-bit word
 */
uint16_t Swap16(uint16_t value);

/**
 * @brief Converts between native and little endian encoding
 */
uint16_t FromLittleEndian16(uint16_t value);

/**
 * @brief Converts between native and big endian encoding
 */
uint16_t FromBigEndian16(uint16_t value);

/**
 * @brief Swaps bytes in 32-bit word
 */
uint32_t Swap32(uint32_t value);

/**
 * @brief Swaps 16-bit words in 32-bit word
 */
uint32_t Swap32words(uint32_t value);

/**
 * @brief Converts between native and little endian encoding
 */
uint32_t FromLittleEndian32(uint32_t value);

/**
 * @brief Converts between native and little endian encoding
 */
uint32_t FromBigEndian32(uint32_t value);

/**
 * @brief Converts between native and little endian encoding
 */
uint32_t FromPDP11Endian32(uint32_t value);

/**
 * @brief Swaps bytes in 64-bit word
 */
uint64_t Swap64(uint64_t value);

/**
 * @brief Swaps 16-bit words in 64-bit word
 */
uint64_t Swap64words(uint64_t value);

/**
 * @brief Converts between native and little endian encoding
 */
uint64_t FromLittleEndian64(uint64_t value);

uint64_t FromBigEndian64(uint64_t value);

uint64_t FromPDP11Endian64(uint64_t value);

/**
 * @brief Accesses unsigned word within byte stream
 */
uint64_t ReadUnsigned(size_t bytes, size_t maximum, uint8_t const * data, EndianType endiantype);

/**
 * @brief Extends the sign of the value
 */
int64_t SignExtend(size_t bytes, int64_t value);

/**
 * @brief Accesses signed word within byte stream
 */
int64_t ReadSigned(size_t bytes, size_t maximum, uint8_t const * data, EndianType endiantype);

/**
 * @brief Stores word within byte stream
 */
void WriteWord(size_t bytes, size_t maximum, uint8_t * data, uint64_t value, EndianType endiantype);

bool LookupOption(std::map<std::string, std::string>& options, std::string key, std::string& value);

struct CaseInsensitiveLess
{
	inline bool operator()(const std::string& first, const std::string& second) const
	{
		return std::lexicographical_compare(first.begin(), first.end(), second.begin(), second.end(),
			[](char c1, char c2) -> bool { return std::tolower(static_cast<unsigned char>(c1)) < std::tolower(static_cast<unsigned char>(c2)); });
	}
};

namespace Linker
{
	class Exception
	{
	public:
		std::string message;
		Exception(std::string message) : message(message)
		{
		}
	};

	/* TODO: implement these properly */
	extern std::ostream Debug;
	extern std::ostream Warning;
	extern std::ostream Error;

	[[noreturn]] void FatalError(std::string message);

	class Section;
	class Location;
	typedef std::map<std::shared_ptr<Section>, Location> Displacement;
}

#endif /* COMMON_H */
