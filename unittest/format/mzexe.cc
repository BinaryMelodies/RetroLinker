
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>

#include "../../src/format/mzexe.h"

using namespace Linker;
using namespace Microsoft;

namespace UnitTests
{

class TestMZFormat : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TestMZFormat);
	CPPUNIT_TEST(testCreateEXE);
	CPPUNIT_TEST(testEXEFileSize);
	CPPUNIT_TEST(testEXEHeaderValues);
	CPPUNIT_TEST(testEXERelocations);
	CPPUNIT_TEST(testEXEHeaderSize);
	CPPUNIT_TEST_SUITE_END();
private:
	MZFormat exe;
	Section * code;

	/** @brief Creates and verifies an empty executable */
	void testCreateEXE();
	void testEXEFileSize();
	void testEXEHeaderValues();
	void testEXERelocations();
	void testEXEHeaderSize();

	std::string store();
	void load(std::string data);

	std::string generate_image(size_t size);
	void set_image(std::string data);
	void test_image(std::string data);

	std::vector<MZFormat::Relocation> generate_relocations(size_t count);
	void test_relocations(std::vector<MZFormat::Relocation>& relocations);
public:
	void setUp();
	void tearDown();
};

void TestMZFormat::testCreateEXE()
{
	exe.Initialize();
	exe.CalculateValues();
	std::string image = store();
	load(image);
	CPPUNIT_ASSERT_EQUAL(MZFormat::MAGIC_MZ, exe.GetSignature());
	CPPUNIT_ASSERT(exe.GetHeaderSize() >= 0x20);
	test_image("");
}

void TestMZFormat::testEXEFileSize()
{
	std::string data;

	data = generate_image(1);
	exe.Clear();
	exe.Initialize();
	set_image(data);
	exe.CalculateValues();
	load(store());
	CPPUNIT_ASSERT_EQUAL(MZFormat::MAGIC_MZ, exe.GetSignature());
	CPPUNIT_ASSERT(exe.GetHeaderSize() == 0x20);
	test_image(data);

	/* check roll over */

	data = generate_image(511 - 0x20);
	exe.Clear();
	exe.Initialize();
	set_image(data);
	exe.CalculateValues();
	load(store());
	CPPUNIT_ASSERT_EQUAL(MZFormat::MAGIC_MZ, exe.GetSignature());
	CPPUNIT_ASSERT(exe.GetHeaderSize() == 0x20);
	test_image(data);

	data = generate_image(512 - 0x20);
	exe.Clear();
	exe.Initialize();
	set_image(data);
	exe.CalculateValues();
	load(store());
	CPPUNIT_ASSERT_EQUAL(MZFormat::MAGIC_MZ, exe.GetSignature());
	CPPUNIT_ASSERT(exe.GetHeaderSize() == 0x20);
	test_image(data);

	data = generate_image(513 - 0x20);
	exe.Clear();
	exe.Initialize();
	set_image(data);
	exe.CalculateValues();
	load(store());
	CPPUNIT_ASSERT_EQUAL(MZFormat::MAGIC_MZ, exe.GetSignature());
	CPPUNIT_ASSERT(exe.GetHeaderSize() == 0x20);
	test_image(data);

	data = generate_image(1023 - 0x20);
	exe.Clear();
	exe.Initialize();
	set_image(data);
	exe.CalculateValues();
	load(store());
	CPPUNIT_ASSERT_EQUAL(MZFormat::MAGIC_MZ, exe.GetSignature());
	CPPUNIT_ASSERT(exe.GetHeaderSize() == 0x20);
	test_image(data);

	data = generate_image(1024 - 0x20);
	exe.Clear();
	exe.Initialize();
	set_image(data);
	exe.CalculateValues();
	load(store());
	CPPUNIT_ASSERT_EQUAL(MZFormat::MAGIC_MZ, exe.GetSignature());
	CPPUNIT_ASSERT(exe.GetHeaderSize() == 0x20);
	test_image(data);

	data = generate_image(1025 - 0x20);
	exe.Clear();
	exe.Initialize();
	set_image(data);
	exe.CalculateValues();
	load(store());
	CPPUNIT_ASSERT_EQUAL(MZFormat::MAGIC_MZ, exe.GetSignature());
	CPPUNIT_ASSERT(exe.GetHeaderSize() == 0x20);
	test_image(data);

	/* check largest file size */

	if(true)
		return; /* ignore this test, takes a while to run */

	data = generate_image((0xFFFF << 9) - 0x20);
	exe.Clear();
	exe.Initialize();
	set_image(data);
	exe.CalculateValues();
	load(store());
	CPPUNIT_ASSERT_EQUAL(MZFormat::MAGIC_MZ, exe.GetSignature());
	CPPUNIT_ASSERT(exe.GetHeaderSize() == 0x20);
	test_image(data);
}

void TestMZFormat::testEXEHeaderValues()
{
	std::string data;

	static const uint16_t min_extra_paras = 0x1234;
	static const uint16_t max_extra_paras = 0x5678;
	static const uint16_t cs = 0x0102;
	static const uint16_t ip = 0x0304;
	static const uint16_t ss = 0x0506;
	static const uint16_t sp = 0x0506;
	static const uint16_t overlay_number = 0xABCD;

	/* TODO: also test checksum */

	data = generate_image((cs << 4) + ip + 1 - 0x20);
	exe.Clear();
	exe.Initialize();
	set_image(data);
	exe.SetSignature(MZFormat::MAGIC_ZM);
	exe.min_extra_paras = min_extra_paras;
	exe.extra_paras = max_extra_paras - min_extra_paras;
	exe.cs = cs;
	exe.ip = ip;
	exe.ss = ss;
	exe.sp = sp;
	exe.overlay_number = overlay_number;
	exe.CalculateValues();
	load(store());
	CPPUNIT_ASSERT_EQUAL(MZFormat::MAGIC_ZM, exe.GetSignature());
	CPPUNIT_ASSERT(exe.GetHeaderSize() == 0x20);
	test_image(data);
	CPPUNIT_ASSERT_EQUAL(min_extra_paras, exe.min_extra_paras);
	CPPUNIT_ASSERT_EQUAL(max_extra_paras, exe.max_extra_paras);
	CPPUNIT_ASSERT_EQUAL(overlay_number, exe.overlay_number);
	CPPUNIT_ASSERT_EQUAL(cs, exe.cs);
	CPPUNIT_ASSERT_EQUAL(ip, exe.ip);
	CPPUNIT_ASSERT_EQUAL(ss, exe.ss);
	CPPUNIT_ASSERT_EQUAL(sp, exe.sp);
}

void TestMZFormat::testEXERelocations()
{
	std::vector<MZFormat::Relocation> relocations;
	std::string data;

	relocations = generate_relocations(1);
	data = generate_image(2);
	exe.Clear();
	exe.Initialize();
	set_image(data);
	exe.relocations = relocations;
	exe.CalculateValues();
	load(store());
	CPPUNIT_ASSERT_EQUAL(MZFormat::MAGIC_MZ, exe.GetSignature());
	CPPUNIT_ASSERT(exe.relocation_offset >= 0x1C);
	CPPUNIT_ASSERT(exe.GetHeaderSize() >= exe.relocation_offset + 4 * exe.relocations.size());
	test_relocations(relocations);
	test_image(data);

	/* test repositioned 0 relocations */

	static const uint16_t relocation_offset1 = 0x56;
	exe.Clear();
	exe.Initialize();
	set_image(data);
	exe.relocation_offset = relocation_offset1;
	exe.CalculateValues();
	load(store());
	CPPUNIT_ASSERT_EQUAL(MZFormat::MAGIC_MZ, exe.GetSignature());
	CPPUNIT_ASSERT_EQUAL(relocation_offset1, exe.relocation_offset);
	CPPUNIT_ASSERT_EQUAL(0ul, exe.relocations.size());
	CPPUNIT_ASSERT(exe.GetHeaderSize() >= exe.relocation_offset);
	test_image(data);

	/* test ill position relocation being overridden */

	static const uint16_t relocation_offset2 = 0x03;
	relocations = generate_relocations(1);
	exe.Clear();
	exe.Initialize();
	set_image(data);
	exe.relocation_offset = relocation_offset2;
	exe.relocations = relocations;
	exe.CalculateValues();
	load(store());
	CPPUNIT_ASSERT_EQUAL(MZFormat::MAGIC_MZ, exe.GetSignature());
//	std::cout << exe.relocation_offset << std::endl;
	CPPUNIT_ASSERT(exe.relocation_offset >= 0x1C);
	CPPUNIT_ASSERT(exe.GetHeaderSize() >= exe.relocation_offset + 4 * exe.relocations.size());
	test_relocations(relocations);
	test_image(data);

	/* test too many relocations */

	relocations = generate_relocations(0x3FF8);
	exe.Clear();
	exe.Initialize();
	set_image(data);
	exe.relocations = relocations;
	CPPUNIT_ASSERT_THROW(exe.CalculateValues(), Exception);

	/* test invalid relocation offset */

	static const uint16_t relocation_offset3 = 0xFFFF;
	relocations = generate_relocations(1);
	exe.Clear();
	exe.Initialize();
	exe.relocation_offset = relocation_offset3;
	exe.relocations = relocations;
	CPPUNIT_ASSERT_THROW(exe.CalculateValues(), Exception);
}

void TestMZFormat::testEXEHeaderSize()
{
	std::vector<MZFormat::Relocation> relocations;
	std::string data;

	/* normal */

	offset_t normal_header_size;

	data = generate_image(4);
	relocations = generate_relocations(4);
	exe.Clear();
	exe.Initialize();
	set_image(data);
	exe.relocations = relocations;
	exe.CalculateValues();
	load(store());
	CPPUNIT_ASSERT_EQUAL(MZFormat::MAGIC_MZ, exe.GetSignature());
	CPPUNIT_ASSERT(exe.relocation_offset >= 0x1C);
	CPPUNIT_ASSERT(exe.GetHeaderSize() >= exe.relocation_offset + 4 * exe.relocations.size());
	test_relocations(relocations);
	test_image(data);

	normal_header_size = exe.GetHeaderSize();

	offset_t header_align = 0x10;
	while(header_align <= normal_header_size)
		header_align <<= 1;

	/* higher alignment */

	data = generate_image(4);
	relocations = generate_relocations(4);
	exe.Clear();
	exe.Initialize();
	set_image(data);
	exe.relocations = relocations;
	exe.option_header_align = header_align;
	exe.CalculateValues();
	load(store());
	CPPUNIT_ASSERT_EQUAL(MZFormat::MAGIC_MZ, exe.GetSignature());
	CPPUNIT_ASSERT(exe.relocation_offset >= 0x1C);
	CPPUNIT_ASSERT(exe.GetHeaderSize() >= exe.relocation_offset + 4 * exe.relocations.size());
	CPPUNIT_ASSERT((exe.GetHeaderSize() & (header_align - 1)) == 0);
	test_relocations(relocations);
	test_image(data);
}

void TestMZFormat::setUp()
{
	exe.Clear();
	exe.Initialize();
	code = new Linker::Section(".code");
}

void TestMZFormat::tearDown()
{
	exe.Clear();
	delete code;
	code = nullptr;
}

std::string TestMZFormat::store()
{
	std::string image;
	std::ostringstream out;
	Writer wr(::Undefined, &out);
	exe.WriteFile(wr);
	return out.str();
}

void TestMZFormat::load(std::string data)
{
	std::istringstream in(data);
	Reader rd(::Undefined, &in);
	CPPUNIT_ASSERT_NO_THROW(exe.ReadFile(rd));
}

std::string TestMZFormat::generate_image(size_t size)
{
	/* first it generates a sequence of 256 sequential bytes, but then the next 256 bytes are incremented by 3 each, then 5, then 7 etc. */
	std::ostringstream out;
	for(size_t i = 0; i < size; i++)
	{
		int off = i & 0xFF;
		size_t inc = (i >> 7) | 1;
		out.put(char(inc * off));
	}
	return out.str();
}

void TestMZFormat::set_image(std::string data)
{
	code->Reset();
	code->Append(data.c_str(), data.size());
	Linker::Segment * segment = new Linker::Segment(".code");
	segment->Append(code);
	if(exe.image)
		delete exe.image;
	exe.image = segment;
/*	if(exe.image_segment)
		delete exe.image_segment;
	exe.image_segment = segment;*/
}

void TestMZFormat::test_image(std::string data)
{
	uint32_t file_size = exe.GetHeaderSize() + data.size();
	CPPUNIT_ASSERT_EQUAL(file_size & 0x1FF, (uint32_t)exe.last_block_size);
	CPPUNIT_ASSERT_EQUAL((file_size + 0x1FF) >> 9, (uint32_t)exe.file_size_blocks);
	CPPUNIT_ASSERT_EQUAL(file_size, exe.GetFileSize());
	Linker::Buffer * buffer = dynamic_cast<Linker::Buffer *>(exe.image);
	assert(buffer != nullptr); /* internal check */
	CPPUNIT_ASSERT_EQUAL(data.size(), buffer->ActualDataSize());
	for(uint32_t i = 0; i < data.size(); i++)
	{
		CPPUNIT_ASSERT_EQUAL(data[i] & 0xFF, buffer->GetByte(i) & 0xFF);
	}
}

std::vector<MZFormat::Relocation> TestMZFormat::generate_relocations(size_t count)
{
	std::vector<MZFormat::Relocation> relocations;
	for(size_t i = 0; i < count; i++)
	{
		int off = i & 0xFF;
		size_t inc = (i >> 7) | 1;
		off = (off * inc) & 0xFF;
		relocations.push_back(MZFormat::Relocation(off * 0x0102, off * 0x0304));
	}
	return relocations;
}

void TestMZFormat::test_relocations(std::vector<MZFormat::Relocation>& relocations)
{
	/* TODO: relocations do not have to be exactly the same, in the exact same order */
	CPPUNIT_ASSERT_EQUAL(relocations.size(), exe.relocations.size());
	for(size_t i = 0; i < relocations.size(); i++)
	{
		CPPUNIT_ASSERT(relocations[i] == exe.relocations[i]);
	}
}

}

