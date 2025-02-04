
#include "linker/buffer.cc"
#include "linker/location.cc"
#include "linker/reader.cc"
#include "linker/section.cc"
#include "linker/symbol_name.cc"
#include "format/mzexe.cc"

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/XmlOutputter.h>

CPPUNIT_TEST_SUITE_REGISTRATION(UnitTests::TestBuffer);
CPPUNIT_TEST_SUITE_REGISTRATION(UnitTests::TestLocation);
CPPUNIT_TEST_SUITE_REGISTRATION(UnitTests::TestReader);
CPPUNIT_TEST_SUITE_REGISTRATION(UnitTests::TestSection);
CPPUNIT_TEST_SUITE_REGISTRATION(UnitTests::TestSymbolName);
CPPUNIT_TEST_SUITE_REGISTRATION(UnitTests::TestExportedSymbol);

CPPUNIT_TEST_SUITE_REGISTRATION(UnitTests::TestMZFormat);

int main()
{
	CPPUNIT_NS::TestResult result;
	CPPUNIT_NS::TestResultCollector collected;
	result.addListener(&collected);
	CPPUNIT_NS::BriefTestProgressListener progress;
	result.addListener(&progress);
	CPPUNIT_NS::TestRunner runner;
	runner.addTest(CPPUNIT_NS::TestFactoryRegistry::getRegistry().makeTest());
	runner.run(result);
	CPPUNIT_NS::CompilerOutputter outputter(&collected, std::cerr);
	outputter.write();

	std::ofstream xmlfile("results.xml");
	CPPUNIT_NS::XmlOutputter xml(&collected, xmlfile);
	xml.write();

	return collected.wasSuccessful() ? 0 : 1;
}

