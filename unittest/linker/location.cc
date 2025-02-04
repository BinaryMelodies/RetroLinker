
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>

#include "../../src/linker/location.h"
#include "../../src/linker/position.h"
#include "../../src/linker/section.h"
#include "../../src/linker/segment.h"

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
	std::shared_ptr<Section> test_section;
	void testDefaultLocations();
	void testLocationArithmetic();
	void testLocationToPosition();
	void testLocationDisplacement();
public:
	void setUp() override;
	void tearDown() override;
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
	std::shared_ptr<Segment> the_segment = std::make_shared<Segment>(".test");
	test_section->segment = the_segment;
	test_section->SetAddress(0x1234);
	test_section->bias = 0x123;

	Location location = Location(test_section, 123);

	CPPUNIT_ASSERT_EQUAL(Position(0x1234 + 123, test_section->segment.lock()), location.GetPosition());
	CPPUNIT_ASSERT_EQUAL(Position(0x1234 - 0x123, test_section->segment.lock()), location.GetPosition(true));

	location = Location(123);

	CPPUNIT_ASSERT_EQUAL(Position(123, nullptr), location.GetPosition());
	CPPUNIT_ASSERT_EQUAL(Position(0, nullptr), location.GetPosition(true));
}

void TestLocation::testLocationDisplacement()
{
	Displacement displacement;
	Location location = Location(test_section, 123);
	CPPUNIT_ASSERT(!location.Displace(displacement));
	CPPUNIT_ASSERT_EQUAL(location, Location(test_section, 123));

	std::shared_ptr<Section> unrelated_section = std::make_shared<Section>(".unrelated");
	displacement[unrelated_section] = 456;
	CPPUNIT_ASSERT(!location.Displace(displacement));
	CPPUNIT_ASSERT_EQUAL(location, Location(test_section, 123));

	displacement[test_section] = Location(unrelated_section, 789);
	CPPUNIT_ASSERT(location.Displace(displacement));
	CPPUNIT_ASSERT_EQUAL(location, Location(unrelated_section, 123 + 789));

	location = Location(nullptr, 123);
	CPPUNIT_ASSERT(!location.Displace(displacement));
	CPPUNIT_ASSERT_EQUAL(location, Location(nullptr, 123));
}

void TestLocation::setUp()
{
	test_section = std::make_shared<Section>(".test");
}

void TestLocation::tearDown()
{
	test_section = nullptr;
}

}

