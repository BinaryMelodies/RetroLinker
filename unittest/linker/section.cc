
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>

#include "../../linker/section.h"

using namespace Linker;

namespace UnitTests
{

class TestSection : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TestSection);
	CPPUNIT_TEST(testSimpleFlags);
	CPPUNIT_TEST(testZeroFilledFlag);
	CPPUNIT_TEST(testFixedFlag);
	CPPUNIT_TEST_SUITE_END();
private:
	Section * section;
	void testSimpleFlags();
	void testZeroFilledFlag();
	void testFixedFlag();
public:
	void setUp();
	void tearDown();
};

void TestSection::testSimpleFlags()
{
	CPPUNIT_ASSERT_MESSAGE("Default setting should be Readable", section->GetFlags() == Section::Readable);

	section->SetWritable(true);
	CPPUNIT_ASSERT(section->IsWritable());
	section->SetExecable(true);
	CPPUNIT_ASSERT(section->IsExecable());
	section->SetMergeable(true);
	CPPUNIT_ASSERT(section->IsMergeable());

	section->SetReadable(false);
	CPPUNIT_ASSERT(!section->IsReadable());
	section->SetExecable(false);
	CPPUNIT_ASSERT(!section->IsExecable());
	section->SetWritable(false);
	CPPUNIT_ASSERT(!section->IsWritable());
	section->SetMergeable(false);
	CPPUNIT_ASSERT(!section->IsMergeable());
}

void TestSection::testZeroFilledFlag()
{
	section->SetZeroFilled(true);
	section->Expand(123);
	CPPUNIT_ASSERT_EQUAL((offset_t)123, section->Size());
	section->SetZeroFilled(false);
	CPPUNIT_ASSERT_EQUAL((offset_t)123, section->Size());
}

void TestSection::testFixedFlag()
{
	CPPUNIT_ASSERT(!section->IsFixed());
	section->SetAlign(16);
	CPPUNIT_ASSERT_EQUAL((offset_t)16, section->GetAlign());
	CPPUNIT_ASSERT_EQUAL((offset_t)0, section->GetStartAddress());
	section->SetAddress(0x1234);
	CPPUNIT_ASSERT(section->IsFixed());
	CPPUNIT_ASSERT_EQUAL((offset_t)0x1240, section->GetStartAddress());
}

void TestSection::setUp()
{
	section = new Section(".test");
}

void TestSection::tearDown()
{
	delete section;
}

}

