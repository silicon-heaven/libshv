#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <shv/core/utils.h>

#include <doctest/doctest.h>

using namespace shv::core::utils;
DOCTEST_TEST_CASE("joinPath")
{
	std::string prefix;
	std::string suffix;
	std::string result;

	DOCTEST_SUBCASE("regular")
	{
		prefix = "foo";
		suffix = "bar";
		result = "foo/bar";
	}

	DOCTEST_SUBCASE("trailing slash left operand")
	{
		prefix = "foo/";
		suffix = "bar";
		result = "foo/bar";
	}

	DOCTEST_SUBCASE("trailing slash right operand")
	{
		prefix = "foo";
		suffix = "bar/";
		result = "foo/bar/";
	}

	DOCTEST_SUBCASE("trailing slash both operands")
	{
		prefix = "foo/";
		suffix = "bar/";
		result = "foo/bar/";
	}

	DOCTEST_SUBCASE("left operand empty")
	{
		prefix = "";
		suffix = "bar";
		result = "bar";
	}

	DOCTEST_SUBCASE("right operand empty")
	{
		prefix = "foo";
		suffix = "";
		result = "foo";
	}

	DOCTEST_SUBCASE("leading slash left operand")
	{
		prefix = "/foo";
		suffix = "bar";
		result = "/foo/bar";
	}

	DOCTEST_SUBCASE("leading slash right operand")
	{
		prefix = "foo";
		suffix = "/bar";
		result = "foo/bar";
	}

	REQUIRE(joinPath(prefix, suffix) == result);
}

DOCTEST_TEST_CASE("joinPath - variadic arguments")
{
	// The function takes any number of args.
	REQUIRE(joinPath(std::string("a")) == "a");
	REQUIRE(joinPath(std::string("a"), std::string("b")) == "a/b");
	REQUIRE(joinPath(std::string("a"), std::string("b"), std::string("c")) == "a/b/c");
	REQUIRE(joinPath(std::string("a"), std::string("b"), std::string("c"), std::string("d")) == "a/b/c/d");
	REQUIRE(joinPath(std::string("a"), std::string("b"), std::string("c"), std::string("d"), std::string("e")) == "a/b/c/d/e");

	// The function supports character literals.
	REQUIRE(joinPath("a") == "a");
	REQUIRE(joinPath("a", "b") == "a/b");
	REQUIRE(joinPath(std::string("a"), "b") == "a/b");
	REQUIRE(joinPath("a", std::string("b")) == "a/b");
}
