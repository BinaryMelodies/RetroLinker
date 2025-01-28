
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>

#include "../../src/linker/buffer.h"

using namespace Linker;

namespace UnitTests
{

class TestBuffer : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TestBuffer);
	CPPUNIT_TEST(testCreateBuffer);
	CPPUNIT_TEST(testReadFile);
	CPPUNIT_TEST(testWriteFile);
	CPPUNIT_TEST_SUITE_END();
private:

	void testCreateBuffer();
	void testReadFile();
	void testWriteFile();
};

void TestBuffer::testCreateBuffer()
{
	Buffer buffer (0x1234);
	CPPUNIT_ASSERT_EQUAL(offset_t(0x1234), buffer.ImageSize());
}

void TestBuffer::testReadFile()
{
	Buffer buffer;

	std::string input = "\1\2\3\4\5\6\7\x8\x9\xA\xB\xC\xD\xE\xF";

	{
		std::istringstream iss(input);
		Reader rd(::LittleEndian, &iss);
		buffer.ReadFile(rd);
		CPPUNIT_ASSERT_EQUAL(offset_t(input.size()), buffer.ImageSize());
	}

	{
		std::istringstream iss(input);
		Reader rd(::LittleEndian, &iss);
		buffer.ReadFile(rd, 0);
		CPPUNIT_ASSERT_EQUAL(offset_t(0), buffer.ImageSize());
	}

	{
		std::istringstream iss(input);
		Reader rd(::LittleEndian, &iss);
		buffer.ReadFile(rd, input.size() / 2);
		CPPUNIT_ASSERT_EQUAL(offset_t(input.size() / 2), buffer.ImageSize());
	}

	{
		std::istringstream iss(input);
		Reader rd(::LittleEndian, &iss);
		buffer.ReadFile(rd);
		size_t offset = input.size() / 2;
		CPPUNIT_ASSERT_EQUAL(offset_t(input.size()), buffer.ImageSize());
		CPPUNIT_ASSERT_EQUAL(int(input[offset]), buffer.GetByte(offset));
	}
}

void TestBuffer::testWriteFile()
{
	Buffer buffer;

	std::string input = "\1\2\3\4\5\6\7\x8\x9\xA\xB\xC\xD\xE\xF";

	{
		std::istringstream iss(input);
		Reader rd(::LittleEndian, &iss);
		buffer.ReadFile(rd);
	}

	{
		std::ostringstream oss;
		Writer wr(::LittleEndian, &oss);
		buffer.WriteFile(wr);
		CPPUNIT_ASSERT_EQUAL(oss.str(), input);
	}

	{
		std::ostringstream oss;
		Writer wr(::LittleEndian, &oss);
		offset_t start = input.size() / 5;
		offset_t length = input.size() / 3;
		buffer.WriteFile(wr, length, start);
		CPPUNIT_ASSERT_EQUAL(oss.str(), input.substr(start, length));
	}
}

}

