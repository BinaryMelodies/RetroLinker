
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>

#include "../../src/linker/symbol_name.h"

using namespace Linker;

namespace UnitTests
{

class TestSymbolName : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TestSymbolName);
	CPPUNIT_TEST(testLoadName);
	CPPUNIT_TEST(testLoadLibraryName);
	CPPUNIT_TEST(testLoadOrdinalOrHint);
	CPPUNIT_TEST(testGetLocalName);
	CPPUNIT_TEST(testGetImportedName);
	CPPUNIT_TEST(testGetImportedOrdinal);
	CPPUNIT_TEST(testEquality);
	CPPUNIT_TEST_SUITE_END();
private:
	SymbolName * internalSymbol;
	SymbolName * importedSymbolByName;
	SymbolName * importedSymbolWithHint;
	SymbolName * importedSymbolByOrdinal;

	void testLoadName();
	void testLoadLibraryName();
	void testLoadOrdinalOrHint();

	void testGetLocalName();
	void testGetImportedName();
	void testGetImportedOrdinal();
	void testEquality();
public:
	void setUp() override;
	void tearDown() override;
};

void TestSymbolName::testLoadName()
{
	std::string result;

	CPPUNIT_ASSERT(internalSymbol->LoadName(result));
	CPPUNIT_ASSERT_EQUAL(std::string("InternalName"), result);

	CPPUNIT_ASSERT(importedSymbolByName->LoadName(result));
	CPPUNIT_ASSERT_EQUAL(std::string("ImportedName"), result);

	CPPUNIT_ASSERT(importedSymbolWithHint->LoadName(result));
	CPPUNIT_ASSERT_EQUAL(std::string("ImportedName"), result);

	CPPUNIT_ASSERT(!importedSymbolByOrdinal->LoadName(result));
}

void TestSymbolName::testLoadLibraryName()
{
	std::string result;

	CPPUNIT_ASSERT(!internalSymbol->LoadLibraryName(result));

	CPPUNIT_ASSERT(importedSymbolByName->LoadLibraryName(result));
	CPPUNIT_ASSERT_EQUAL(std::string("LibraryName"), result);

	result = "";
	CPPUNIT_ASSERT(importedSymbolWithHint->LoadLibraryName(result));
	CPPUNIT_ASSERT_EQUAL(std::string("LibraryName"), result);

	result = "";
	CPPUNIT_ASSERT(importedSymbolByOrdinal->LoadLibraryName(result));
	CPPUNIT_ASSERT_EQUAL(std::string("LibraryName"), result);
}

void TestSymbolName::testLoadOrdinalOrHint()
{
	uint16_t result;

	CPPUNIT_ASSERT(!internalSymbol->LoadOrdinalOrHint(result));

	CPPUNIT_ASSERT(!importedSymbolByName->LoadOrdinalOrHint(result));

	result = -1;
	CPPUNIT_ASSERT(importedSymbolWithHint->LoadOrdinalOrHint(result));
	CPPUNIT_ASSERT_EQUAL(uint16_t(1234), result);

	result = -1;
	CPPUNIT_ASSERT(importedSymbolByOrdinal->LoadOrdinalOrHint(result));
	CPPUNIT_ASSERT_EQUAL(uint16_t(1234), result);
}

void TestSymbolName::testGetLocalName()
{
	std::string result;

	CPPUNIT_ASSERT(internalSymbol->GetLocalName(result));
	CPPUNIT_ASSERT_EQUAL(std::string("InternalName"), result);

	CPPUNIT_ASSERT(!importedSymbolByName->GetLocalName(result));

	CPPUNIT_ASSERT(!importedSymbolWithHint->GetLocalName(result));

	CPPUNIT_ASSERT(!importedSymbolByOrdinal->GetLocalName(result));
}

void TestSymbolName::testGetImportedName()
{
	std::string libraryResult;
	std::string symbolResult;
	uint16_t hintResult;

	CPPUNIT_ASSERT(!internalSymbol->GetImportedName(libraryResult, symbolResult));

	CPPUNIT_ASSERT(importedSymbolByName->GetImportedName(libraryResult, symbolResult));
	CPPUNIT_ASSERT_EQUAL(std::string("ImportedName"), symbolResult);
	CPPUNIT_ASSERT_EQUAL(std::string("LibraryName"), libraryResult);

	symbolResult = libraryResult = "";
	CPPUNIT_ASSERT(importedSymbolWithHint->GetImportedName(libraryResult, symbolResult));
	CPPUNIT_ASSERT_EQUAL(std::string("ImportedName"), symbolResult);
	CPPUNIT_ASSERT_EQUAL(std::string("LibraryName"), libraryResult);

	CPPUNIT_ASSERT(!importedSymbolByOrdinal->GetImportedName(libraryResult, symbolResult));

	//

	CPPUNIT_ASSERT(!internalSymbol->GetImportedName(libraryResult, symbolResult, hintResult));

	symbolResult = libraryResult = "";
	CPPUNIT_ASSERT(importedSymbolByName->GetImportedName(libraryResult, symbolResult, hintResult));
	CPPUNIT_ASSERT_EQUAL(std::string("ImportedName"), symbolResult);
	CPPUNIT_ASSERT_EQUAL(std::string("LibraryName"), libraryResult);

	symbolResult = libraryResult = "";
	hintResult = -1;
	CPPUNIT_ASSERT(importedSymbolWithHint->GetImportedName(libraryResult, symbolResult, hintResult));
	CPPUNIT_ASSERT_EQUAL(std::string("ImportedName"), symbolResult);
	CPPUNIT_ASSERT_EQUAL(std::string("LibraryName"), libraryResult);
	CPPUNIT_ASSERT_EQUAL(uint16_t(1234), hintResult);

	CPPUNIT_ASSERT(!importedSymbolByOrdinal->GetImportedName(libraryResult, symbolResult, hintResult));
}

void TestSymbolName::testGetImportedOrdinal()
{
	std::string libraryResult;
	uint16_t hintResult;

	CPPUNIT_ASSERT(!internalSymbol->GetImportedOrdinal(libraryResult, hintResult));

	CPPUNIT_ASSERT(!importedSymbolByName->GetImportedOrdinal(libraryResult, hintResult));

	CPPUNIT_ASSERT(!importedSymbolWithHint->GetImportedOrdinal(libraryResult, hintResult));

	hintResult = -1;
	CPPUNIT_ASSERT(importedSymbolByOrdinal->GetImportedOrdinal(libraryResult, hintResult));
	CPPUNIT_ASSERT_EQUAL(std::string("LibraryName"), libraryResult);
	CPPUNIT_ASSERT_EQUAL(uint16_t(1234), hintResult);
}

void TestSymbolName::testEquality()
{
	CPPUNIT_ASSERT_EQUAL(*internalSymbol, SymbolName("InternalName"));
	CPPUNIT_ASSERT_EQUAL(*importedSymbolByName, SymbolName("LibraryName", "ImportedName"));
	CPPUNIT_ASSERT_EQUAL(*importedSymbolWithHint, SymbolName("LibraryName", "ImportedName", 1234));
	CPPUNIT_ASSERT_EQUAL(*importedSymbolByOrdinal, SymbolName("LibraryName", 1234));

	CPPUNIT_ASSERT(SymbolName("abc") != SymbolName("def"));
	CPPUNIT_ASSERT(SymbolName("abc") != SymbolName("lib", "abc"));
	CPPUNIT_ASSERT(SymbolName("abc") != SymbolName("lib", "abc", 1234));
	CPPUNIT_ASSERT(SymbolName("abc") != SymbolName("lib", 1234));

	CPPUNIT_ASSERT(SymbolName("lib", "abc") != SymbolName("bil", "abc"));
	CPPUNIT_ASSERT(SymbolName("lib", "abc") != SymbolName("lib", "def"));
	CPPUNIT_ASSERT(SymbolName("lib", "abc") != SymbolName("lib", "abc", 1234));
	CPPUNIT_ASSERT(SymbolName("lib", "abc") != SymbolName("lib", 1234));

	CPPUNIT_ASSERT(SymbolName("lib", "abc", 1234) != SymbolName("bil", "abc", 1234));
	CPPUNIT_ASSERT(SymbolName("lib", "abc", 1234) != SymbolName("lib", "def", 1234));
	CPPUNIT_ASSERT(SymbolName("lib", "abc", 1234) != SymbolName("lib", "abc", 5678));
	CPPUNIT_ASSERT(SymbolName("lib", "abc", 1234) != SymbolName("lib", 1234));

	CPPUNIT_ASSERT(SymbolName("lib", 1234) != SymbolName("bil", 1234));
	CPPUNIT_ASSERT(SymbolName("lib", 1234) != SymbolName("lib", 5678));
}

void TestSymbolName::setUp()
{
	internalSymbol = new SymbolName("InternalName");
	importedSymbolByName = new SymbolName("LibraryName", "ImportedName");
	importedSymbolWithHint = new SymbolName("LibraryName", "ImportedName", 1234);
	importedSymbolByOrdinal = new SymbolName("LibraryName", 1234);
}

void TestSymbolName::tearDown()
{
	delete internalSymbol;
	delete importedSymbolByName;
	delete importedSymbolWithHint;
	delete importedSymbolByOrdinal;
}

class TestExportedSymbol : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TestExportedSymbol);
	CPPUNIT_TEST(testIsExportedByOrdinal);
	CPPUNIT_TEST(testLoadName);
	CPPUNIT_TEST(testLoadOrdinalOrHint);
	CPPUNIT_TEST(testGetExportedByName);
	CPPUNIT_TEST(testGetExportedByOrdinal);
	CPPUNIT_TEST(testEquality);
	CPPUNIT_TEST(testComparison);
	CPPUNIT_TEST_SUITE_END();
private:
	ExportedSymbolName * exportedSymbolByName;
	ExportedSymbolName * exportedSymbolWithHint;
	ExportedSymbolName * exportedSymbolByOrdinal;

	void testIsExportedByOrdinal();
	void testLoadName();
	void testLoadOrdinalOrHint();
	void testGetExportedByName();
	void testGetExportedByOrdinal();
	void testEquality();
	bool equal(const ExportedSymbolName& a, const ExportedSymbolName& b);
	bool unequal(const ExportedSymbolName& a, const ExportedSymbolName& b);
	void testComparison();
public:
	void setUp() override;
	void tearDown() override;
};

void TestExportedSymbol::testIsExportedByOrdinal()
{
	CPPUNIT_ASSERT(!exportedSymbolByName->IsExportedByOrdinal());
	CPPUNIT_ASSERT(!exportedSymbolWithHint->IsExportedByOrdinal());
	CPPUNIT_ASSERT(exportedSymbolByOrdinal->IsExportedByOrdinal());
}

void TestExportedSymbol::testLoadName()
{
	std::string result;

	CPPUNIT_ASSERT(exportedSymbolByName->LoadName(result));
	CPPUNIT_ASSERT_EQUAL(std::string("ExportedName"), result);

	result = "";
	CPPUNIT_ASSERT(exportedSymbolWithHint->LoadName(result));
	CPPUNIT_ASSERT_EQUAL(std::string("ExportedName"), result);

	result = "";
	CPPUNIT_ASSERT(exportedSymbolByOrdinal->LoadName(result));
	CPPUNIT_ASSERT_EQUAL(std::string("InternalName"), result);
}

void TestExportedSymbol::testLoadOrdinalOrHint()
{
	uint16_t result;

	CPPUNIT_ASSERT(!exportedSymbolByName->LoadOrdinalOrHint(result));

	result = -1;
	CPPUNIT_ASSERT(exportedSymbolWithHint->LoadOrdinalOrHint(result));
	CPPUNIT_ASSERT_EQUAL(uint16_t(1234), result);

	result = -1;
	CPPUNIT_ASSERT(exportedSymbolByOrdinal->LoadOrdinalOrHint(result));
	CPPUNIT_ASSERT_EQUAL(uint16_t(1234), result);
}

void TestExportedSymbol::testGetExportedByName()
{
	std::string nameResult;
	uint16_t hintResult;

	CPPUNIT_ASSERT(exportedSymbolByName->GetExportedByName(nameResult));
	CPPUNIT_ASSERT_EQUAL(std::string("ExportedName"), nameResult);

	nameResult = "";
	CPPUNIT_ASSERT(exportedSymbolWithHint->GetExportedByName(nameResult));
	CPPUNIT_ASSERT_EQUAL(std::string("ExportedName"), nameResult);

	CPPUNIT_ASSERT(!exportedSymbolByOrdinal->GetExportedByName(nameResult));

	//

	nameResult = "";
	CPPUNIT_ASSERT(exportedSymbolByName->GetExportedByName(nameResult, hintResult));
	CPPUNIT_ASSERT_EQUAL(std::string("ExportedName"), nameResult);

	nameResult = "";
	hintResult = -1;
	CPPUNIT_ASSERT(exportedSymbolWithHint->GetExportedByName(nameResult, hintResult));
	CPPUNIT_ASSERT_EQUAL(std::string("ExportedName"), nameResult);
	CPPUNIT_ASSERT_EQUAL(uint16_t(1234), hintResult);

	CPPUNIT_ASSERT(!exportedSymbolByOrdinal->GetExportedByName(nameResult, hintResult));
}

void TestExportedSymbol::testGetExportedByOrdinal()
{
	std::string nameResult;
	uint16_t hintResult;

	CPPUNIT_ASSERT(!exportedSymbolByName->GetExportedByOrdinal(hintResult));

	CPPUNIT_ASSERT(!exportedSymbolWithHint->GetExportedByOrdinal(hintResult));

	CPPUNIT_ASSERT(exportedSymbolByOrdinal->GetExportedByOrdinal(hintResult));
	CPPUNIT_ASSERT_EQUAL(uint16_t(1234), hintResult);

	//

	CPPUNIT_ASSERT(!exportedSymbolByName->GetExportedByOrdinal(hintResult, nameResult));

	CPPUNIT_ASSERT(!exportedSymbolWithHint->GetExportedByOrdinal(hintResult, nameResult));

	nameResult = "";
	hintResult = -1;
	CPPUNIT_ASSERT(exportedSymbolByOrdinal->GetExportedByOrdinal(hintResult, nameResult));
	CPPUNIT_ASSERT_EQUAL(uint16_t(1234), hintResult);
	CPPUNIT_ASSERT_EQUAL(std::string("InternalName"), nameResult);
}

void TestExportedSymbol::testEquality()
{
	CPPUNIT_ASSERT_EQUAL(*exportedSymbolByName, ExportedSymbolName("ExportedName"));
	CPPUNIT_ASSERT_EQUAL(*exportedSymbolWithHint, ExportedSymbolName("ExportedName", 1234));
	CPPUNIT_ASSERT_EQUAL(*exportedSymbolByOrdinal, ExportedSymbolName(1234, "InternalName"));

	CPPUNIT_ASSERT(ExportedSymbolName("abc") != ExportedSymbolName("def"));
	CPPUNIT_ASSERT(ExportedSymbolName("abc") != ExportedSymbolName("abc", 1234));
	CPPUNIT_ASSERT(ExportedSymbolName("abc") != ExportedSymbolName(1234, "abc"));

	CPPUNIT_ASSERT(ExportedSymbolName("abc", 1234) != ExportedSymbolName("def", 1234));
	CPPUNIT_ASSERT(ExportedSymbolName("abc", 1234) != ExportedSymbolName("abc", 5678));
	CPPUNIT_ASSERT(ExportedSymbolName("abc", 1234) != ExportedSymbolName(1234, "abc"));

	CPPUNIT_ASSERT(ExportedSymbolName(1234, "abc") != ExportedSymbolName(5678, "abc"));
	CPPUNIT_ASSERT(ExportedSymbolName(1234, "abc") != ExportedSymbolName(1234, "def"));
	CPPUNIT_ASSERT(ExportedSymbolName(1234, "abc") != ExportedSymbolName("abc", 1234));
}

bool TestExportedSymbol::equal(const ExportedSymbolName& a, const ExportedSymbolName& b)
{
	return a <= b && a >= b;
}

bool TestExportedSymbol::unequal(const ExportedSymbolName& a, const ExportedSymbolName& b)
{
	return a < b || a > b;
}

void TestExportedSymbol::testComparison()
{
	CPPUNIT_ASSERT(unequal(ExportedSymbolName("abc"), ExportedSymbolName("def")));
	CPPUNIT_ASSERT(unequal(ExportedSymbolName("abc"), ExportedSymbolName("abc", 1234)));
	CPPUNIT_ASSERT(unequal(ExportedSymbolName("abc"), ExportedSymbolName(1234, "abc")));

	CPPUNIT_ASSERT(unequal(ExportedSymbolName("abc", 1234), ExportedSymbolName("def", 1234)));
	CPPUNIT_ASSERT(unequal(ExportedSymbolName("abc", 1234), ExportedSymbolName("abc", 5678)));
	CPPUNIT_ASSERT(unequal(ExportedSymbolName("abc", 1234), ExportedSymbolName(1234, "abc")));

	CPPUNIT_ASSERT(unequal(ExportedSymbolName(1234, "abc"), ExportedSymbolName(5678, "abc")));
	CPPUNIT_ASSERT(unequal(ExportedSymbolName(1234, "abc"), ExportedSymbolName(1234, "def")));
	CPPUNIT_ASSERT(unequal(ExportedSymbolName(1234, "abc"), ExportedSymbolName("abc", 1234)));
}

void TestExportedSymbol::setUp()
{
	exportedSymbolByName = new ExportedSymbolName("ExportedName");
	exportedSymbolWithHint = new ExportedSymbolName("ExportedName", 1234);
	exportedSymbolByOrdinal = new ExportedSymbolName(1234, "InternalName");
}

void TestExportedSymbol::tearDown()
{
	delete exportedSymbolByName;
	delete exportedSymbolWithHint;
	delete exportedSymbolByOrdinal;
}

}

