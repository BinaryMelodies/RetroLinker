#ifndef COMMON_H
#define COMMON_H

#include <cassert>
#include <cstdint>
#include <cstddef>
#include <chrono>
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

static constexpr EndianType ByteSwapped(EndianType endian_type)
{
	switch(endian_type)
	{
	case UndefinedEndian:
		return UndefinedEndian;
	case LittleEndian:
		return BigEndian;
	case BigEndian:
		return LittleEndian;
	case PDP11Endian:
		return AntiPDP11Endian;
	case AntiPDP11Endian:
		return PDP11Endian;
	}
	assert(false);
}

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

struct CaseInsensitiveEqual
{
	inline bool operator()(const std::string& first, const std::string& second) const
	{
		return std::ranges::equal(first.begin(), first.end(), second.begin(), second.end(),
			[](char c1, char c2) -> bool { return std::tolower(static_cast<unsigned char>(c1)) == std::tolower(static_cast<unsigned char>(c2)); });
	}
};

struct CaseInsensitiveLess
{
	inline bool operator()(const std::string& first, const std::string& second) const
	{
		return std::lexicographical_compare(first.begin(), first.end(), second.begin(), second.end(),
			[](char c1, char c2) -> bool { return std::tolower(static_cast<unsigned char>(c1)) < std::tolower(static_cast<unsigned char>(c2)); });
	}
};

/** @brief Checks if the string starts with a certain substring, based on the Python function with the same name
 *
 * @param str The string to check
 * @param start The string that must appear in the beginning
 */
bool starts_with(std::string str, std::string start);

/** @brief Checks if the string ends with a certain substring, based on the Python function with the same name
 *
 * @param str The string to check
 * @param start The string that must appear in the end
 */
bool ends_with(std::string str, std::string end);

/** @brief Represents a month value in the Gregorian calendar */
enum class Month : unsigned
{
	January = 1,
	February = 2,
	March = 3,
	April = 4,
	May = 5,
	June = 6,
	July = 7,
	August = 8,
	September = 9,
	October = 10,
	November = 11,
	December = 12,
};

/** @brief Provides a type for compile-time constants representing Gregorian dates */
template <unsigned _Year, Month _Month = Month::January, unsigned _Day = 1>
	struct GregorianCalendarDate
{
	static constexpr unsigned Year = _Year;
	static constexpr ::Month Month = _Month;
	static constexpr unsigned Day = _Day;

	static constexpr std::chrono::sys_days to_days()
	{
		return std::chrono::year(Year) / std::chrono::month(unsigned(Month)) / std::chrono::day(Day);
	}
};

/** @brief A static class that encodes a specific timestamp format
 *
 * This structure satisfies the requirements for a Clock.
 *
 * @tparam Epoch The starting date, corresponding to the zero timestamp value
 * @tparam Rep The binary representation (typically int32_t)
 * @tparam Period The unit of time period corresponding to increasing the binary representation by 1
 */
template <typename Epoch, typename Rep = int32_t, typename Period = std::ratio<1>>
	struct VendorClock
{
	using rep = Rep;
	using period = Period;
	using duration = std::chrono::duration<rep, period>;
	using time_point = std::chrono::time_point<VendorClock>;
	static constexpr bool is_steady = false;

	static constexpr rep to_ticks(duration dur)
	{
		return dur.count();
	}

	static constexpr rep to_ticks(time_point tp)
	{
		return to_ticks(tp.time_since_epoch());
	}

	static constexpr duration to_duration(Rep ticks)
	{
		return duration(ticks);
	}

	static constexpr time_point to_time_point(duration dur)
	{
		return time_point(dur);
	}

	static constexpr time_point to_time_point(Rep ticks)
	{
		return time_point(duration(ticks));
	}

	static constexpr time_point to_time_point(std::chrono::time_point<std::chrono::system_clock> tp)
	{
		return time_point(std::chrono::duration_cast<duration>(tp - Epoch::to_time_point()));
	}

	static constexpr time_point to_time_point(std::chrono::sys_days days, std::chrono::nanoseconds ns)
	{
		return time_point(std::chrono::duration_cast<duration>(days - Epoch::to_days()) + std::chrono::duration_cast<duration>(ns));
	}

	template <typename FromClock>
		static time_point convert(std::chrono::time_point<FromClock> from)
	{
		return to_time_point(FromClock::to_days(from), FromClock::to_nanoseconds(from));
	}

	static constexpr std::chrono::sys_days to_days(time_point tp)
	{
		return Epoch::to_days() + std::chrono::duration_cast<std::chrono::days>(tp.time_since_epoch());
	}

	static constexpr std::chrono::nanoseconds to_nanoseconds(time_point tp)
	{
		const auto dur = tp.time_since_epoch();
		return dur - std::chrono::duration_cast<std::chrono::days>(dur);
	}

	static std::string to_iso_time(time_point tp)
	{
		return std::format("{:%Y-%m-%d} {:%H:%M:%S}", to_days(tp), to_nanoseconds(tp));
	}

	static time_point now() noexcept
	{
		return to_time_point(std::chrono::system_clock::now());
	}
};

/** @brief Represents the clock for POSIX timestamps */
using POSIX_clock = VendorClock<GregorianCalendarDate<1970, Month::January, 1>>;

/** @brief Convenience name for a time_point corresponding to a clock */
template <typename Clock>
	using Timestamp = std::chrono::time_point<Clock>;

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
