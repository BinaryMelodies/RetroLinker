
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>

#include "../src/unicode.h"

namespace UnitTests
{

class TestUnicode : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TestUnicode);
	CPPUNIT_TEST(testUTF8ToUTF32);
	CPPUNIT_TEST(testUTF32ToUTF8);
	CPPUNIT_TEST(testUTF16ToUTF32);
	CPPUNIT_TEST(testUTF32ToUTF16);
	CPPUNIT_TEST_SUITE_END();
private:

	void testUTF8ToUTF32();
	void testUTF32ToUTF8();
	void testUTF16ToUTF32();
	void testUTF32ToUTF16();
};

static const char initial_bytes[8] = { 0, 0, '\xC0', '\xE0', '\xF0', '\xF8', '\xFC', '\xFE' };

void TestUnicode::testUTF8ToUTF32()
{
	UnicodeConversion::Result result;
	char const * input_pointer;
	char32_t * output_pointer;

	// check single byte input sequences

	for(uint16_t i = 0; i < 0x100; i++)
	{
		char input_buffer[1];
		char32_t output_buffer[1];

		input_buffer[0] = i;

		// strict
		input_pointer = input_buffer;
		output_pointer = output_buffer;

		result = UTF8ToUTF32(input_pointer, output_pointer, 1, false);

		if(i == 0)
			CPPUNIT_ASSERT_EQUAL(UnicodeConversion::NullCharacter, result);
		else if(i < 0x80)
			CPPUNIT_ASSERT_EQUAL(UnicodeConversion::Success, result);
		else if(i < 0xC0)
			CPPUNIT_ASSERT_EQUAL(UnicodeConversion::InvalidValue, result);
		else if(i < 0xF5)
			CPPUNIT_ASSERT_EQUAL(UnicodeConversion::InputTruncated, result);
		else
			CPPUNIT_ASSERT_EQUAL(UnicodeConversion::InvalidValue, result);

		CPPUNIT_ASSERT_EQUAL(ptrdiff_t(UnicodeConversion::InputAdvanced(result) ? 1 : 0), const_cast<char *>(input_pointer) - input_buffer);
		CPPUNIT_ASSERT_EQUAL(ptrdiff_t(UnicodeConversion::InputAdvanced(result) ? 1 : 0), output_pointer - output_buffer);

		if(UnicodeConversion::InputAdvanced(result) && i < 0x80)
		{
			CPPUNIT_ASSERT_EQUAL(uint32_t(i), uint32_t(output_buffer[0]));
		}

		// permissive
		input_pointer = input_buffer;
		output_pointer = output_buffer;

		result = UTF8ToUTF32(input_pointer, output_pointer, 1, true);

		if(i == 0)
			CPPUNIT_ASSERT_EQUAL(UnicodeConversion::NullCharacter, result);
		else if(i < 0x80)
			CPPUNIT_ASSERT_EQUAL(UnicodeConversion::Success, result);
		else if(i < 0xC0)
			CPPUNIT_ASSERT_EQUAL(UnicodeConversion::InvalidValue, result);
		else if(i < 0xFF)
			CPPUNIT_ASSERT_EQUAL(UnicodeConversion::InputTruncated, result);
		else
			CPPUNIT_ASSERT_EQUAL(UnicodeConversion::InvalidValue, result);

		CPPUNIT_ASSERT_EQUAL(ptrdiff_t(UnicodeConversion::InputAdvanced(result) ? 1 : 0), const_cast<char *>(input_pointer) - input_buffer);
		CPPUNIT_ASSERT_EQUAL(ptrdiff_t(UnicodeConversion::InputAdvanced(result) ? 1 : 0), output_pointer - output_buffer);

		if(UnicodeConversion::InputAdvanced(result) && i < 0x80)
		{
			CPPUNIT_ASSERT_EQUAL(uint32_t(i), uint32_t(output_buffer[0]));
		}
	}

	for(size_t sequence_length = 2; sequence_length < 8; sequence_length++)
	{
		char input_buffer[7];
		char32_t output_buffer[1];

		for(int32_t bit_position = -1; uint32_t(bit_position + 1) <= 1 + sequence_length * 5 && uint32_t(bit_position + 1) <= 32; bit_position ++)
		{
			input_buffer[0] = initial_bytes[sequence_length];
			for(size_t i = 1; i < sequence_length; i++)
			{
				input_buffer[i] = 0x80;
			}

			if(bit_position >= 0)
				input_buffer[sequence_length - 1 - bit_position / 6] |= 1 << (bit_position % 6);

			// permissive
			input_pointer = input_buffer;
			output_pointer = output_buffer;

			result = UTF8ToUTF32(input_pointer, output_pointer, sequence_length, true);
			if(bit_position == -1)
				CPPUNIT_ASSERT_EQUAL(UnicodeConversion::NullCharacter, result);
			else if(bit_position < 21)
				CPPUNIT_ASSERT_EQUAL(UnicodeConversion::Success, result);
			else
				CPPUNIT_ASSERT_EQUAL(UnicodeConversion::OutOfRangeCharacter, result);

			CPPUNIT_ASSERT_EQUAL(ptrdiff_t(sequence_length), const_cast<char *>(input_pointer) - input_buffer);
			CPPUNIT_ASSERT_EQUAL(ptrdiff_t(1), output_pointer - output_buffer);

			if(bit_position < 0)
				CPPUNIT_ASSERT_EQUAL(uint32_t(0), uint32_t(output_buffer[0]));
			else
				CPPUNIT_ASSERT_EQUAL(uint32_t(1 << bit_position), uint32_t(output_buffer[0]));

			// strict
			input_pointer = input_buffer;
			output_pointer = output_buffer;

			result = UTF8ToUTF32(input_pointer, output_pointer, sequence_length, false);
			if(sequence_length >= 5)
				CPPUNIT_ASSERT_EQUAL(UnicodeConversion::InvalidValue, result);
			else if(bit_position == -1)
				CPPUNIT_ASSERT_EQUAL(UnicodeConversion::NullCharacter, result);
			else
				CPPUNIT_ASSERT_EQUAL(UnicodeConversion::Success, result);

			CPPUNIT_ASSERT_EQUAL(ptrdiff_t(UnicodeConversion::IsSuccess(result) ? sequence_length : 1), const_cast<char *>(input_pointer) - input_buffer);
			CPPUNIT_ASSERT_EQUAL(ptrdiff_t(1), output_pointer - output_buffer);

			if(UnicodeConversion::IsSuccess(result))
			{
				if(bit_position < 0)
					CPPUNIT_ASSERT_EQUAL(uint32_t(0), uint32_t(output_buffer[0]));
				else
					CPPUNIT_ASSERT_EQUAL(uint32_t(1 << bit_position), uint32_t(output_buffer[0]));
			}
			else
			{
				CPPUNIT_ASSERT_EQUAL(uint32_t(input_buffer[0] & 0xFF), uint32_t(output_buffer[0]));
			}
		}
	}
}

void TestUnicode::testUTF32ToUTF8()
{
	UnicodeConversion::Result result;
	char32_t const * input_pointer;
	char * output_pointer;
	size_t required_output;

	for(int32_t bit_position = -1; bit_position < 32; bit_position ++)
	{
		char32_t input_buffer[1];
		char output_buffer[7];

		size_t actual_required_output = bit_position <= 6 ? 1 : (bit_position + 4) / 5;
		if(bit_position < 0)
			input_buffer[0] = 0;
		else
			input_buffer[0] = 1 << bit_position;

		input_pointer = input_buffer;
		output_pointer = output_buffer;
		required_output = size_t(-1);

		result = UTF32ToUTF8(input_pointer, output_pointer, actual_required_output, required_output);
		if(bit_position == -1)
			CPPUNIT_ASSERT_EQUAL(UnicodeConversion::NullCharacter, result);
		else if(bit_position < 21)
			CPPUNIT_ASSERT_EQUAL(UnicodeConversion::Success, result);
		else
			CPPUNIT_ASSERT_EQUAL(UnicodeConversion::OutOfRangeCharacter, result);

		CPPUNIT_ASSERT_EQUAL(ptrdiff_t(1), const_cast<char32_t *>(input_pointer) - input_buffer);
		CPPUNIT_ASSERT_EQUAL(ptrdiff_t(actual_required_output), output_pointer - output_buffer);
		CPPUNIT_ASSERT_EQUAL(actual_required_output, required_output);

		if(actual_required_output > 1)
		{
			input_pointer = input_buffer;
			output_pointer = output_buffer;
			required_output = size_t(-1);

			result = UTF32ToUTF8(input_pointer, output_pointer, actual_required_output - 1, required_output);
			CPPUNIT_ASSERT_EQUAL(UnicodeConversion::BufferOverflow, result);

			CPPUNIT_ASSERT_EQUAL(ptrdiff_t(0), const_cast<char32_t *>(input_pointer) - input_buffer);
			CPPUNIT_ASSERT_EQUAL(ptrdiff_t(0), output_pointer - output_buffer);
			CPPUNIT_ASSERT_EQUAL(actual_required_output, required_output);
		}
	}
}

void TestUnicode::testUTF16ToUTF32()
{
	UnicodeConversion::Result result;
	char16_t const * input_pointer;
	char32_t * output_pointer;

	for(uint16_t i = 0; i < 256; i++)
	{
		char16_t input_buffer[1];
		char32_t output_buffer[1];

		input_buffer[0] = i;

		input_pointer = input_buffer;
		output_pointer = output_buffer;

		result = UTF16ToUTF32(input_pointer, output_pointer, 1);

		if(i == 0)
			CPPUNIT_ASSERT_EQUAL(UnicodeConversion::NullCharacter, result);
		else
			CPPUNIT_ASSERT_EQUAL(UnicodeConversion::Success, result);

		CPPUNIT_ASSERT_EQUAL(ptrdiff_t(1), const_cast<char16_t *>(input_pointer) - input_buffer);
		CPPUNIT_ASSERT_EQUAL(ptrdiff_t(1), output_pointer - output_buffer);
		CPPUNIT_ASSERT_EQUAL(uint32_t(i), uint32_t(output_buffer[0]));


		input_buffer[0] = i << 8;

		input_pointer = input_buffer;
		output_pointer = output_buffer;

		result = UTF16ToUTF32(input_pointer, output_pointer, 1);

		if(0xD8 <= i && i <= 0xDB)
			CPPUNIT_ASSERT_EQUAL(UnicodeConversion::InputTruncated, result);
		else if(0xDC <= i && i <= 0xDF)
			CPPUNIT_ASSERT_EQUAL(UnicodeConversion::InvalidValue, result);
		else if(i == 0)
			CPPUNIT_ASSERT_EQUAL(UnicodeConversion::NullCharacter, result);
		else
			CPPUNIT_ASSERT_EQUAL(UnicodeConversion::Success, result);

		CPPUNIT_ASSERT_EQUAL(ptrdiff_t(1), const_cast<char16_t *>(input_pointer) - input_buffer);
		CPPUNIT_ASSERT_EQUAL(ptrdiff_t(1), output_pointer - output_buffer);
		if(!(0xD8 <= i && i <= 0xDF))
			CPPUNIT_ASSERT_EQUAL(uint32_t(i << 8), uint32_t(output_buffer[0]));
	}

	for(int32_t bit_position = -1; bit_position < 20; bit_position ++)
	{
		char16_t input_buffer[2];
		char32_t output_buffer[1];

		input_buffer[0] = 0xD800;
		input_buffer[1] = 0xDC00;

		if(bit_position >= 0)
			input_buffer[1 - bit_position / 10] |= 1 << (bit_position % 10);

		input_pointer = input_buffer;
		output_pointer = output_buffer;

		result = UTF16ToUTF32(input_pointer, output_pointer, 2);

		CPPUNIT_ASSERT_EQUAL(UnicodeConversion::Success, result);
		CPPUNIT_ASSERT_EQUAL(ptrdiff_t(2), const_cast<char16_t *>(input_pointer) - input_buffer);
		CPPUNIT_ASSERT_EQUAL(ptrdiff_t(1), output_pointer - output_buffer);
		CPPUNIT_ASSERT_EQUAL(uint32_t(0x00100000 | (bit_position >= 0 ? 1 << bit_position : 0)), uint32_t(output_buffer[0]));


		input_pointer = input_buffer;
		output_pointer = output_buffer;

		result = UTF16ToUTF32(input_pointer, output_pointer, 1);

		CPPUNIT_ASSERT_EQUAL(UnicodeConversion::InputTruncated, result);
		CPPUNIT_ASSERT_EQUAL(ptrdiff_t(1), const_cast<char16_t *>(input_pointer) - input_buffer);
		CPPUNIT_ASSERT_EQUAL(ptrdiff_t(1), output_pointer - output_buffer);
		CPPUNIT_ASSERT_EQUAL(uint32_t(0x00100000 | (bit_position >= 10 ? 1 << bit_position : 0)), uint32_t(output_buffer[0]));
	}
}

void TestUnicode::testUTF32ToUTF16()
{
	UnicodeConversion::Result result;
	char32_t const * input_pointer;
	char16_t * output_pointer;
	size_t required_output;

	for(uint16_t i = 0; i < 256; i++)
	{
		char32_t input_buffer[1];
		char16_t output_buffer[1];

		input_buffer[0] = i;

		input_pointer = input_buffer;
		output_pointer = output_buffer;
		required_output = size_t(-1);

		result = UTF32ToUTF16(input_pointer, output_pointer, 1, required_output);

		if(i == 0)
			CPPUNIT_ASSERT_EQUAL(UnicodeConversion::NullCharacter, result);
		else
			CPPUNIT_ASSERT_EQUAL(UnicodeConversion::Success, result);

		CPPUNIT_ASSERT_EQUAL(ptrdiff_t(1), const_cast<char32_t *>(input_pointer) - input_buffer);
		CPPUNIT_ASSERT_EQUAL(ptrdiff_t(1), output_pointer - output_buffer);
		CPPUNIT_ASSERT_EQUAL(uint32_t(i), uint32_t(output_buffer[0]));


		input_buffer[0] = i << 8;

		input_pointer = input_buffer;
		output_pointer = output_buffer;
		required_output = size_t(-1);

		result = UTF32ToUTF16(input_pointer, output_pointer, 1, required_output);

		if(0xD8 <= i && i <= 0xDF)
			CPPUNIT_ASSERT_EQUAL(UnicodeConversion::InvalidCharacter, result);
		else if(i == 0)
			CPPUNIT_ASSERT_EQUAL(UnicodeConversion::NullCharacter, result);
		else
			CPPUNIT_ASSERT_EQUAL(UnicodeConversion::Success, result);

		CPPUNIT_ASSERT_EQUAL(ptrdiff_t(1), const_cast<char32_t *>(input_pointer) - input_buffer);
		CPPUNIT_ASSERT_EQUAL(ptrdiff_t(1), output_pointer - output_buffer);
		if(!(0xD8 <= i && i <= 0xDF))
			CPPUNIT_ASSERT_EQUAL(uint32_t(i << 8), uint32_t(output_buffer[0]));
	}

	for(int32_t bit_position = -1; bit_position < 16; bit_position ++)
	{
		char32_t input_buffer[1];
		char16_t output_buffer[2];

		input_buffer[0] = 0x00100000;
		if(bit_position >= 0)
			input_buffer[0] |= 1 << bit_position;

		input_pointer = input_buffer;
		output_pointer = output_buffer;
		required_output = size_t(-1);

		result = UTF32ToUTF16(input_pointer, output_pointer, 2, required_output);
		CPPUNIT_ASSERT_EQUAL(UnicodeConversion::Success, result);

		CPPUNIT_ASSERT_EQUAL(ptrdiff_t(1), const_cast<char32_t *>(input_pointer) - input_buffer);
		CPPUNIT_ASSERT_EQUAL(ptrdiff_t(2), output_pointer - output_buffer);
		CPPUNIT_ASSERT_EQUAL(size_t(2), required_output);


		input_pointer = input_buffer;
		output_pointer = output_buffer;
		required_output = size_t(-1);

		result = UTF32ToUTF16(input_pointer, output_pointer, 1, required_output);
		CPPUNIT_ASSERT_EQUAL(UnicodeConversion::BufferOverflow, result);

		CPPUNIT_ASSERT_EQUAL(ptrdiff_t(0), const_cast<char32_t *>(input_pointer) - input_buffer);
		CPPUNIT_ASSERT_EQUAL(ptrdiff_t(0), output_pointer - output_buffer);
	}

	for(int32_t bit_position = 16; bit_position < 32; bit_position ++)
	{
		char32_t input_buffer[1];
		char16_t output_buffer[2];

		input_buffer[0] = 0x00100000 + (1 << bit_position);

		input_pointer = input_buffer;
		output_pointer = output_buffer;
		required_output = size_t(-1);

		result = UTF32ToUTF16(input_pointer, output_pointer, 2, required_output);
		CPPUNIT_ASSERT_EQUAL(UnicodeConversion::OutOfRangeCharacter, result);

		CPPUNIT_ASSERT_EQUAL(ptrdiff_t(1), const_cast<char32_t *>(input_pointer) - input_buffer);
		CPPUNIT_ASSERT_EQUAL(ptrdiff_t(0), output_pointer - output_buffer);
	}
}

}


