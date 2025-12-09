
#include "common.h"
#include "unicode.h"

UnicodeConversion::Result UTF32ToUTF8(char32_t const *& input, char *& output, size_t max_output, size_t& required_output)
{
	char32_t value = *input;
	if(value < 0x0080)
	{
		required_output = 1;
		if(max_output < 1)
			return UnicodeConversion::BufferOverflow;
		input++;

		*output++ = value;

		if(value == '\0')
			return UnicodeConversion::NullCharacter;
		else
			return UnicodeConversion::Success;
	}
	else if(value < 0x0800)
	{
		required_output = 2;
		if(max_output < 2)
			return UnicodeConversion::BufferOverflow;
		input++;

		*output++ = 0xC0 | (value >> 6);
		*output++ = 0x80 | (value & 0x3F);
		return UnicodeConversion::Success;
	}
	else if(value < 0x00010000)
	{
		required_output = 3;
		if(max_output < 3)
			return UnicodeConversion::BufferOverflow;
		input++;

		*output++ = 0xE0 | (value >> 12);
		*output++ = 0x80 | ((value >> 6) & 0x3F);
		*output++ = 0x80 | (value & 0x3F);
		if((value & 0xF800) == 0xD800)
			return UnicodeConversion::InvalidCharacter;
		else
			return UnicodeConversion::Success;
	}
	else if(value < 0x00200000)
	{
		required_output = 4;
		if(max_output < 4)
			return UnicodeConversion::BufferOverflow;
		input++;

		*output++ = 0xF0 | (value >> 18);
		*output++ = 0x80 | ((value >> 12) & 0x3F);
		*output++ = 0x80 | ((value >> 6) & 0x3F);
		*output++ = 0x80 | (value & 0x3F);
		if(value < 0x00110000)
			return UnicodeConversion::Success;
		else
			return UnicodeConversion::OutOfRangeCharacter;
	}
	else if(value < 0x04000000)
	{
		required_output = 5;
		if(max_output < 5)
			return UnicodeConversion::BufferOverflow;
		input++;

		*output++ = 0xF8 | (value >> 24);
		*output++ = 0x80 | ((value >> 18) & 0x3F);
		*output++ = 0x80 | ((value >> 12) & 0x3F);
		*output++ = 0x80 | ((value >> 6) & 0x3F);
		*output++ = 0x80 | (value & 0x3F);
		return UnicodeConversion::OutOfRangeCharacter;
	}
	else if(value < 0x80000000)
	{
		required_output = 6;
		if(max_output < 6)
			return UnicodeConversion::BufferOverflow;
		input++;

		*output++ = 0xFC | (value >> 30);
		*output++ = 0x80 | ((value >> 24) & 0x3F);
		*output++ = 0x80 | ((value >> 18) & 0x3F);
		*output++ = 0x80 | ((value >> 12) & 0x3F);
		*output++ = 0x80 | ((value >> 6) & 0x3F);
		*output++ = 0x80 | (value & 0x3F);
		return UnicodeConversion::OutOfRangeCharacter;
	}
	else
	{
		required_output = 7;
		if(max_output < 7)
			return UnicodeConversion::BufferOverflow;
		input++;

		*output++ = 0xFE;
		*output++ = 0x80 | ((value >> 30) & 0x3F);
		*output++ = 0x80 | ((value >> 24) & 0x3F);
		*output++ = 0x80 | ((value >> 18) & 0x3F);
		*output++ = 0x80 | ((value >> 12) & 0x3F);
		*output++ = 0x80 | ((value >> 6) & 0x3F);
		*output++ = 0x80 | (value & 0x3F);
		return UnicodeConversion::OutOfRangeCharacter;
	}
}

UnicodeConversion::Result UTF8ToUTF32(char const *& input, char32_t *& output, size_t max_input, bool force_out_of_range_decoding)
{
	if(max_input < 1)
		return UnicodeConversion::InputTruncated;

	uint8_t byte = *input++;
	char32_t codepoint;
	size_t sequence_length;
	size_t shift;

	if(byte < 0x80)
	{
		*output++ = byte;
		if(byte == '\0')
			return UnicodeConversion::NullCharacter;
		else
			return UnicodeConversion::Success;
	}
	else if(byte < 0xC0)
	{
		*output++ = byte;
		return UnicodeConversion::InvalidValue;
	}
	else if(byte < 0xE0)
	{
		byte &= 0x1F;
		sequence_length = 2;
	}
	else if(byte < 0xF0)
	{
		byte &= 0x0F;
		sequence_length = 3;
	}
	else if(byte < 0xF5)
	{
		byte &= 0x07;
		sequence_length = 4;
	}
	else if(!force_out_of_range_decoding)
	{
		*output++ = byte;
		return UnicodeConversion::InvalidValue;
	}
	else if(byte < 0xF8)
	{
		byte &= 0x07;
		sequence_length = 4;
	}
	else if(byte < 0xFC)
	{
		byte &= 0x03;
		sequence_length = 5;
	}
	else if(byte < 0xFE)
	{
		byte &= 0x01;
		sequence_length = 6;
	}
	else if(byte < 0xFF)
	{
		byte = 0;
		sequence_length = 7;
	}
	else
	{
		*output++ = byte;
		return UnicodeConversion::InvalidValue;
	}

	shift = 6 * (sequence_length - 1);
	codepoint = char32_t(byte) << shift;
	shift -= 6;

	for(size_t byte_position = 1; byte_position < sequence_length; byte_position++)
	{
		if(max_input <= byte_position)
		{
			*output++ = codepoint;
			return UnicodeConversion::InputTruncated;
		}
		byte = *input;
		if((byte & 0xC0) != 0x80)
		{
			*output++ = codepoint;
			return UnicodeConversion::InterruptedEncoding;
		}
		// TODO: What if an 0xFE is followed by a value greater than 0x01? This is very theoretical
		input++;
		codepoint |= (byte & 0x3F) << shift;
		shift -= 6;
	}

	*output++ = codepoint;

	if(codepoint == 0)
		return UnicodeConversion::NullCharacter;
	else if((codepoint & 0xF800) == 0xD800)
		return UnicodeConversion::InvalidCharacter;
	else if(codepoint >= 0x00110000)
		return UnicodeConversion::OutOfRangeCharacter;
	else
		return UnicodeConversion::Success;
}

UnicodeConversion::Result UTF32ToUTF16(char32_t const *& input, char16_t *& output, size_t max_output, size_t& required_output)
{
	char32_t value = *input;
	if(value < 0x00010000)
	{
		required_output = 1;
		if(max_output < 1)
			return UnicodeConversion::BufferOverflow;
		input++;

		*output++ = value;

		if(value == '\0')
			return UnicodeConversion::NullCharacter;
		else if((value & 0xF800) == 0xD800)
			return UnicodeConversion::InvalidCharacter;
		else
			return UnicodeConversion::Success;
	}
	else if(value < 0x00110000)
	{
		required_output = 2;
		if(max_output < 2)
			return UnicodeConversion::BufferOverflow;
		input++;

		*output++ = 0xD800 | (value >> 10);
		*output++ = 0xDC00 | (value & 0x03FF);
		return UnicodeConversion::Success;
	}
	else
	{
		required_output = 0;
		input++;
		return UnicodeConversion::OutOfRangeCharacter;
	}
}

UnicodeConversion::Result UTF16ToUTF32(char16_t const *& input, char32_t *& output, size_t max_input)
{
	if(max_input < 1)
		return UnicodeConversion::InputTruncated;

	char16_t value = *input++;
	if((value & 0xFC00) == 0xD800)
	{
		char32_t codepoint = 0x00100000 | (char32_t(value & 0x03FF) << 10);
		if(max_input < 2)
		{
			*output++ = codepoint;
			return UnicodeConversion::InputTruncated;
		}
		value = *input;
		if((value & 0xFC00) != 0xDC00)
		{
			*output++ = codepoint;
			return UnicodeConversion::InterruptedEncoding;
		}
		input++;
		codepoint |= value & 0x03FF;
		*output++ = codepoint;
		return UnicodeConversion::Success;
	}
	else if((value & 0xFC00) == 0xDC00)
	{
		*output++ = value;
		return UnicodeConversion::InvalidValue;
	}
	else
	{
		*output++ = value;
		if(value == '\0')
			return UnicodeConversion::NullCharacter;
		else
			return UnicodeConversion::Success;
	}
}

UnicodeConversion::Result UTF16ToUTF8(char16_t const *& input, char *& output, size_t max_input, size_t max_output, size_t& required_output)
{
	char32_t codepoint;
	char16_t const * input16 = input;
	char32_t * output32 = &codepoint;
	UnicodeConversion::Result result = UTF16ToUTF32(input16, output32, max_input);
	char32_t const * input32 = &codepoint;
	switch(UTF32ToUTF8(input32, output, max_output, required_output))
	{
	case UnicodeConversion::Success:
	case UnicodeConversion::NullCharacter:
		input = input16;
		return result;
	case UnicodeConversion::InvalidCharacter:
		// this should not appear
		Linker::Debug << "Internal error: UTF16ToUTF8 generated InvalidCharacter" << std::endl;
		input = input16;
		return result;
	case UnicodeConversion::OutOfRangeCharacter:
		// this should not appear
		Linker::Debug << "Internal error: UTF16ToUTF8 generated OutOfRangeCharacter" << std::endl;
		input = input16;
		return result;
	case UnicodeConversion::InvalidValue:
	case UnicodeConversion::InterruptedEncoding:
	case UnicodeConversion::InputTruncated:
	default:
		// not generated
		assert(false);
	case UnicodeConversion::BufferOverflow:
		return UnicodeConversion::BufferOverflow;
	}
}

UnicodeConversion::Result UTF8ToUTF16(char const *& input, char16_t *& output, size_t max_input, size_t max_output, size_t& required_output)
{
	char32_t codepoint;
	char const * input8 = input;
	char32_t * output32 = &codepoint;
	UnicodeConversion::Result result = UTF8ToUTF32(input8, output32, max_input, false);
	char32_t const * input32 = &codepoint;
	switch(UTF32ToUTF16(input32, output, max_output, required_output))
	{
	case UnicodeConversion::Success:
	case UnicodeConversion::NullCharacter:
	case UnicodeConversion::InvalidCharacter:
	case UnicodeConversion::OutOfRangeCharacter:
		input = input8;
		return result;
	case UnicodeConversion::InvalidValue:
	case UnicodeConversion::InterruptedEncoding:
	case UnicodeConversion::InputTruncated:
	default:
		// not generated
		assert(false);
	case UnicodeConversion::BufferOverflow:
		return UnicodeConversion::BufferOverflow;
	}
}

