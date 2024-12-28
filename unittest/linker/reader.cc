
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>
#include <string>
#include <sstream>

#include "../../linker/reader.h"

using namespace Linker;

namespace UnitTests
{

class TestReader : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TestReader);
	CPPUNIT_TEST(testReadData);
	CPPUNIT_TEST(testReadBinaryData);
	CPPUNIT_TEST(testReadInteger);
	CPPUNIT_TEST(testSeekSkip);
	CPPUNIT_TEST_SUITE_END();
private:
	void testReadData();
	void testReadBinaryData();
	void testReadInteger();
	void testSeekSkip();

	std::string readData(std::string payload);
	uint64_t readInteger(std::string payload, ::EndianType endiantype, bool isSigned = false);
	uint64_t parseInteger(std::string buffer, ::EndianType endiantype, bool isSigned = false);
public:
	void setUp();
	void tearDown();
};

std::string TestReader::readData(std::string payload)
{
	char buffer[payload.size()];
	std::istringstream * iss = new std::istringstream(payload);

	Reader * reader = new Reader(::LittleEndian, iss);
	reader->ReadData(sizeof buffer, buffer);
	std::string result = std::string(buffer, sizeof buffer);
	CPPUNIT_ASSERT_EQUAL(payload, result);
	delete reader;
	delete iss;
	return result;
}

void TestReader::testReadData()
{
	char payload[] = "Test data loaded successfully";
	std::string payload_string(payload, sizeof payload - 1);
	readData(payload_string);
}

void TestReader::testReadBinaryData()
{
	char payload[256];
	for(int i = 0; i < 256; i++)
	{
		payload[i] = 0xD5 ^ i;
	}
	std::string payload_string(payload, sizeof payload - 1);
	readData(payload_string);
}

uint64_t TestReader::readInteger(std::string payload, ::EndianType endiantype, bool isSigned)
{
	std::istringstream * iss = new std::istringstream(payload);
	Reader * reader = new Reader(endiantype, iss);

	uint64_t result =
		isSigned
			? reader->ReadSigned(payload.size(), endiantype)
			: reader->ReadUnsigned(payload.size(), endiantype);
	delete reader;
	delete iss;

	return result;
}

uint64_t TestReader::parseInteger(std::string buffer, ::EndianType endiantype, bool isSigned)
{
	return readInteger(buffer, endiantype, isSigned);
}

void TestReader::testReadInteger()
{
	CPPUNIT_ASSERT_EQUAL((uint64_t)0x12, parseInteger(std::string("\x12", 1), ::LittleEndian));
	CPPUNIT_ASSERT_EQUAL((uint64_t)0x89, parseInteger(std::string("\x89", 1), ::LittleEndian));
	CPPUNIT_ASSERT_EQUAL((uint64_t)0x3412, parseInteger(std::string("\x12\x34", 2), ::LittleEndian));
	CPPUNIT_ASSERT_EQUAL((uint64_t)0x8912, parseInteger(std::string("\x12\x89", 2), ::LittleEndian));
	CPPUNIT_ASSERT_EQUAL((uint64_t)0x563412, parseInteger(std::string("\x12\x34\x56", 3), ::LittleEndian));
	CPPUNIT_ASSERT_EQUAL((uint64_t)0x893412, parseInteger(std::string("\x12\x34\x89", 3), ::LittleEndian));
	CPPUNIT_ASSERT_EQUAL((uint64_t)0x78563412, parseInteger(std::string("\x12\x34\x56\x78", 4), ::LittleEndian));
	CPPUNIT_ASSERT_EQUAL((uint64_t)0x89563412, parseInteger(std::string("\x12\x34\x56\x89", 4), ::LittleEndian));
	CPPUNIT_ASSERT_EQUAL((uint64_t)0xF1DEBC9A78563412, parseInteger(std::string("\x12\x34\x56\x78\x9A\xBC\xDE\xF1", 8), ::LittleEndian));

	CPPUNIT_ASSERT_EQUAL((uint64_t)0x12, parseInteger(std::string("\x12", 1), ::BigEndian));
	CPPUNIT_ASSERT_EQUAL((uint64_t)0x89, parseInteger(std::string("\x89", 1), ::BigEndian));
	CPPUNIT_ASSERT_EQUAL((uint64_t)0x1234, parseInteger(std::string("\x12\x34", 2), ::BigEndian));
	CPPUNIT_ASSERT_EQUAL((uint64_t)0x8934, parseInteger(std::string("\x89\x34", 2), ::BigEndian));
	CPPUNIT_ASSERT_EQUAL((uint64_t)0x123456, parseInteger(std::string("\x12\x34\x56", 3), ::BigEndian));
	CPPUNIT_ASSERT_EQUAL((uint64_t)0x893456, parseInteger(std::string("\x89\x34\x56", 3), ::BigEndian));
	CPPUNIT_ASSERT_EQUAL((uint64_t)0x12345678, parseInteger(std::string("\x12\x34\x56\x78", 4), ::BigEndian));
	CPPUNIT_ASSERT_EQUAL((uint64_t)0x89345678, parseInteger(std::string("\x89\x34\x56\x78", 4), ::BigEndian));
	CPPUNIT_ASSERT_EQUAL((uint64_t)0x123456789ABCDEF1, parseInteger(std::string("\x12\x34\x56\x78\x9A\xBC\xDE\xF1", 8), ::BigEndian));

	CPPUNIT_ASSERT_EQUAL((uint64_t)0x12, parseInteger(std::string("\x12", 1), ::PDP11Endian));
	CPPUNIT_ASSERT_EQUAL((uint64_t)0x89, parseInteger(std::string("\x89", 1), ::PDP11Endian));
	CPPUNIT_ASSERT_EQUAL((uint64_t)0x3412, parseInteger(std::string("\x12\x34", 2), ::PDP11Endian));
	CPPUNIT_ASSERT_EQUAL((uint64_t)0x8912, parseInteger(std::string("\x12\x89", 2), ::PDP11Endian));
	CPPUNIT_ASSERT_EQUAL((uint64_t)0x34127856, parseInteger(std::string("\x12\x34\x56\x78", 4), ::PDP11Endian));
	CPPUNIT_ASSERT_EQUAL((uint64_t)0x89127856, parseInteger(std::string("\x12\x89\x56\x78", 4), ::PDP11Endian));

	CPPUNIT_ASSERT_EQUAL((uint64_t)0x12, parseInteger(std::string("\x12", 1), ::LittleEndian, true));
	CPPUNIT_ASSERT_EQUAL((uint64_t)-0x100 | 0x89, parseInteger(std::string("\x89", 1), ::LittleEndian, true));
	CPPUNIT_ASSERT_EQUAL((uint64_t)0x3412, parseInteger(std::string("\x12\x34", 2), ::LittleEndian, true));
	CPPUNIT_ASSERT_EQUAL((uint64_t)-0x10000 | 0x8912, parseInteger(std::string("\x12\x89", 2), ::LittleEndian, true));
	CPPUNIT_ASSERT_EQUAL((uint64_t)0x563412, parseInteger(std::string("\x12\x34\x56", 3), ::LittleEndian, true));
	CPPUNIT_ASSERT_EQUAL((uint64_t)-0x1000000 | 0x893412, parseInteger(std::string("\x12\x34\x89", 3), ::LittleEndian, true));
	CPPUNIT_ASSERT_EQUAL((uint64_t)0x78563412, parseInteger(std::string("\x12\x34\x56\x78", 4), ::LittleEndian, true));
	CPPUNIT_ASSERT_EQUAL((uint64_t)-0x100000000 | 0x89563412, parseInteger(std::string("\x12\x34\x56\x89", 4), ::LittleEndian, true));
	CPPUNIT_ASSERT_EQUAL((uint64_t)0xF1DEBC9A78563412, parseInteger(std::string("\x12\x34\x56\x78\x9A\xBC\xDE\xF1", 8), ::LittleEndian, true));

	CPPUNIT_ASSERT_EQUAL((uint64_t)0x12, parseInteger(std::string("\x12", 1), ::BigEndian, true));
	CPPUNIT_ASSERT_EQUAL((uint64_t)-0x100 | 0x89, parseInteger(std::string("\x89", 1), ::BigEndian, true));
	CPPUNIT_ASSERT_EQUAL((uint64_t)0x1234, parseInteger(std::string("\x12\x34", 2), ::BigEndian, true));
	CPPUNIT_ASSERT_EQUAL((uint64_t)-0x10000 | 0x8934, parseInteger(std::string("\x89\x34", 2), ::BigEndian, true));
	CPPUNIT_ASSERT_EQUAL((uint64_t)0x123456, parseInteger(std::string("\x12\x34\x56", 3), ::BigEndian, true));
	CPPUNIT_ASSERT_EQUAL((uint64_t)-0x1000000 |0x893456, parseInteger(std::string("\x89\x34\x56", 3), ::BigEndian, true));
	CPPUNIT_ASSERT_EQUAL((uint64_t)0x12345678, parseInteger(std::string("\x12\x34\x56\x78", 4), ::BigEndian, true));
	CPPUNIT_ASSERT_EQUAL((uint64_t)-0x100000000 | 0x89345678, parseInteger(std::string("\x89\x34\x56\x78", 4), ::BigEndian, true));
	CPPUNIT_ASSERT_EQUAL((uint64_t)0x123456789ABCDEF1, parseInteger(std::string("\x12\x34\x56\x78\x9A\xBC\xDE\xF1", 8), ::BigEndian, true));

	CPPUNIT_ASSERT_EQUAL((uint64_t)0x12, parseInteger(std::string("\x12", 1), ::PDP11Endian, true));
	CPPUNIT_ASSERT_EQUAL((uint64_t)-0x100 | 0x89, parseInteger(std::string("\x89", 1), ::PDP11Endian, true));
	CPPUNIT_ASSERT_EQUAL((uint64_t)0x3412, parseInteger(std::string("\x12\x34", 2), ::PDP11Endian, true));
	CPPUNIT_ASSERT_EQUAL((uint64_t)-0x10000 | 0x8912, parseInteger(std::string("\x12\x89", 2), ::PDP11Endian, true));
	CPPUNIT_ASSERT_EQUAL((uint64_t)0x34127856, parseInteger(std::string("\x12\x34\x56\x78", 4), ::PDP11Endian, true));
	CPPUNIT_ASSERT_EQUAL((uint64_t)-0x100000000 | 0x89127856, parseInteger(std::string("\x12\x89\x56\x78", 4), ::PDP11Endian, true));
}

void TestReader::testSeekSkip()
{
	char payload[256];
	char buffer[1];
	for(int i = 0; i < 256; i++)
	{
		payload[i] = i;
	}

	std::string payload_string(payload, sizeof payload);
	std::istringstream * iss = new std::istringstream(payload_string);
	Reader * reader = new Reader(::LittleEndian, iss);

	reader->Seek(123);
	reader->ReadData(1, buffer);
	CPPUNIT_ASSERT_EQUAL(123, (int)(unsigned char)buffer[0]);

	reader->Skip(45);
	reader->ReadData(1, buffer);
	CPPUNIT_ASSERT_EQUAL(123 + 1 + 45, (int)(unsigned char)buffer[0]);

	delete reader;
	delete iss;
}

void TestReader::setUp()
{
}

void TestReader::tearDown()
{
}

}

