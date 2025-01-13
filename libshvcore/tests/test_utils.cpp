#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <shv/core/utils.h>

#include <doctest/doctest.h>

namespace shv::chainpack {
// NOLINTBEGIN(misc-use-internal-linkage)
doctest::String toString(const RpcValue& value) {
	return value.toCpon().c_str();
}

doctest::String toString(const RpcValue::Map& value) {
	return RpcValue(value).toCpon().c_str();
}

doctest::String toString(const RpcValue::DateTime& value) {
	return value.toIsoString().c_str();
}
// NOLINTEND(misc-use-internal-linkage)
}


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
	REQUIRE(joinPath(std::string("a"), std::string("b")) == "a/b");
	REQUIRE(joinPath(std::string("a"), std::string("b"), std::string("c")) == "a/b/c");
	REQUIRE(joinPath(std::string("a"), std::string("b"), std::string("c"), std::string("d")) == "a/b/c/d");
	REQUIRE(joinPath(std::string("a"), std::string("b"), std::string("c"), std::string("d"), std::string("e")) == "a/b/c/d/e");

	// The function supports character literals.
	REQUIRE(joinPath("a", "b") == "a/b");
	REQUIRE(joinPath(std::string("a"), "b") == "a/b");
	REQUIRE(joinPath("a", std::string("b")) == "a/b");
}

DOCTEST_TEST_CASE("findLongestPrefix")
{
	std::map<std::string, int> map = {
		{"shv/a", 0},
		{"shv/b", 0},
		{"shv/bbb", 0},
		{"shv/bbb/c", 0},
		{"shv/bbb/cc", 0},
	};

	for (const auto& it : std::map<std::string, decltype(map)::const_iterator> {
		{"shv/a/dwada",      map.find("shv/a")},
		{"shv/a/dwada/ahoj", map.find("shv/a")},
		{"shv/bbb/c/ahoj",   map.find("shv/bbb/c")},
		{"shv/bb/c/ahoj",    map.end()},
		{"notfound",         map.end()},
		{"shv/bbbb",         map.end()},
	}) {
		CAPTURE(it.first);
		CAPTURE(it.second == map.end() ? "<nothing>" : it.second->first);
		REQUIRE(findLongestPrefix(map, it.first) == it.second);
	}
}

DOCTEST_TEST_CASE("Utils::foldMap")
{
	using shv::chainpack::RpcValue;
	RpcValue::Map input = {
		{"shv.a", 0},
		{"shv.b", 0},
		{"shv.bbb", 0},
		{"shv.bbb.c", 0},
		{"shv.bbb.cc", 0},
	};

	REQUIRE(shv::core::Utils::foldMap(input, '.') == shv::chainpack::RpcValue::Map {
		{
			"shv", RpcValue::Map {
				{
					{"a", 0},
					{"b", 0},
					{"bbb", RpcValue::Map {
						{"c", 0 },
						{"cc", 0 },
					}}
				}
			}
		}
	});
}
