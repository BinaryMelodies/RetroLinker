
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>

#include "../../linker/location.h"
#include "../../linker/section.h"
#include "../../linker/segment.h"

using namespace Linker;

namespace UnitTests
{

class TestLocation : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TestLocation);
	CPPUNIT_TEST(testDefaultLocations);
	CPPUNIT_TEST(testLocationArithmetic);
	CPPUNIT_TEST(testLocationToPosition);
	CPPUNIT_TEST(testLocationDisplacement);
	CPPUNIT_TEST_SUITE_END();
private:
	Section * test_section;
	void testDefaultLocations();
	void testLocationArithmetic();
	void testLocationToPosition();
	void testLocationDisplacement();
public:
	void setUp();
	void tearDown();
};

void TestLocation::testDefaultLocations()
{
	CPPUNIT_ASSERT_EQUAL(Location(), Location(nullptr, 0));
	CPPUNIT_ASSERT_EQUAL(Location(123), Location(nullptr, 123));
	CPPUNIT_ASSERT_EQUAL(Location(test_section), Location(test_section, 0));
}

void TestLocation::testLocationArithmetic()
{
	Section test(".test");
	Location location = Location(test_section, 123);
	location += 456;
	CPPUNIT_ASSERT_EQUAL(location, Location(test_section, 123 + 456));
	location -= 123;
	CPPUNIT_ASSERT_EQUAL(location, Location(test_section, 456));
	CPPUNIT_ASSERT_EQUAL(Location(test_section, 123) + 456, Location(test_section, 123 + 456));
	CPPUNIT_ASSERT_EQUAL(Location(test_section, 123) - 456, Location(test_section, 123 - 456));
}

void TestLocation::testLocationToPosition()
{
	test_section->segment = new Segment(".test");
	test_section->SetAddress(0x1234);
	test_section->bias = 0x123;

	Location location = Location(test_section, 123);

	CPPUNIT_ASSERT_EQUAL(Position(0x1234 + 123, test_section->segment), location.GetPosition());
	CPPUNIT_ASSERT_EQUAL(Position(0x1234 - 0x123, test_section->segment), location.GetPosition(true));

	location = Location(123);

	CPPUNIT_ASSERT_EQUAL(Position(123, nullptr), location.GetPosition());
	CPPUNIT_ASSERT_EQUAL(Position(0, nullptr), location.GetPosition(true));

	delete test_section->segment;
}

void TestLocation::testLocationDisplacement()
{
	Displacement displacement;
	Location location = Location(test_section, 123);
	CPPUNIT_ASSERT(!location.Displace(displacement));
	CPPUNIT_ASSERT_EQUAL(location, Location(test_section, 123));

	Section * unrelated_section = new Section(".unrelated");
	displacement[unrelated_section] = 456;
	CPPUNIT_ASSERT(!location.Displace(displacement));
	CPPUNIT_ASSERT_EQUAL(location, Location(test_section, 123));

	displacement[test_section] = Location(unrelated_section, 789);
	CPPUNIT_ASSERT(location.Displace(displacement));
	CPPUNIT_ASSERT_EQUAL(location, Location(unrelated_section, 123 + 789));

	location = Location(nullptr, 123);
	CPPUNIT_ASSERT(!location.Displace(displacement));
	CPPUNIT_ASSERT_EQUAL(location, Location(nullptr, 123));

	delete unrelated_section;
}

void TestLocation::setUp()
{
	test_section = new Section(".test");
}

void TestLocation::tearDown()
{
	delete test_section;
}

}

