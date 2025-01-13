#include <shv/core/utils/shvpath.h>
#include <shv/core/stringview.h>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <array>

using namespace shv::core::utils;
using namespace shv::core;

DOCTEST_TEST_CASE("ShvPath")
{
	using namespace std::string_literals;
	DOCTEST_SUBCASE("ShvPath::startsWithPath()")
	{
		{
			std::string path = "status/errorCommunication";
			std::array paths = {
				"status/errorCommunication"s,
				"status/errorCommunication/"s,
				"status/errorCommunication/foo"s,
				"status/errorCommunication/foo/"s,
			};
			for(const auto &p : paths) {
				REQUIRE(ShvPath::startsWithPath(p, path));
			}
		}
		{
			std::string path = "foo/bar";
			std::array paths = {
				"bar/baz"s,
				"foo/barbaz"s,
			};
			for(const auto &p : paths) {
				REQUIRE(!ShvPath::startsWithPath(p, path));
			}
		}
	}
	DOCTEST_SUBCASE("ShvPath::firstDir()")
	{
		std::string dir = "'a/b/c'/d";
		std::string dir1 = "a/b/c";
		REQUIRE(ShvPath::firstDir(dir) == StringView(dir1));
	}
	DOCTEST_SUBCASE("ShvPath::takeFirsDir()")
	{
		std::string dir = "'a/b/c'/d";
		StringView dir_view(dir);
		std::string dir1 = "a/b/c";
		std::string dir2 = "d";
		auto dir_view1 = ShvPath::takeFirsDir(dir_view);
		REQUIRE(dir_view1 == StringView(dir1));
		REQUIRE(dir_view == StringView(dir2));
	}
	DOCTEST_SUBCASE("ShvPath join & split")
	{
		std::string dir1 = "foo";
		std::string dir2 = "bar/baz";
		std::string joined = "foo/'bar/baz'";

		DOCTEST_SUBCASE("ShvPath::joinDirs()")
		{
			REQUIRE(ShvPath::joinDirs(dir1, dir2).asString() == joined);
		}
		DOCTEST_SUBCASE("ShvPath::splitPath()")
		{
			StringViewList split{dir1, dir2};
			REQUIRE(ShvPath::splitPath(joined) == split);
		}
	}
	DOCTEST_SUBCASE("ShvPath::forEachDirAndSubdirs()")
	{
		using Map = std::map<std::string, std::string>;
		Map m {
			{"A/b", "a"},
			{"a/b", "a"},
			{"a/b-c", "a"},
			{"a/b/c", "a"},
			{"a/b/c/d", "a"},
			{"a/b/d", "a"},
			{"a/c/d", "a"},
		};
		ShvPath::forEachDirAndSubdirs(m, "a/b", [](Map::const_iterator it) {
			REQUIRE(ShvPath::startsWithPath(it->first, std::string("a/b")));
		});
	}
	DOCTEST_SUBCASE("pattern match")
	{
		struct Test {
			std::string path;
			std::string pattern;
			bool result;
		};
		std::array cases = {
			Test{.path = "", .pattern = "aa", .result = false},
			Test{.path = "aa", .pattern = "", .result = false},
			Test{.path = "", .pattern = "", .result = true},
			Test{.path = "", .pattern = "**", .result = true},
			Test{.path = "aa/bb/cc/dd", .pattern = "aa/*/cc/dd", .result = true},
			Test{.path = "aa/bb/cc/dd", .pattern = "aa/bb/**/cc/dd", .result = true},
			Test{.path = "aa/bb/cc/dd", .pattern = "aa/bb/*/**/dd", .result = true},
			Test{.path = "aa/bb/cc/dd", .pattern = "*/*/cc/dd", .result = true},
			Test{.path = "aa/bb/cc/dd", .pattern = "*/cc/dd/**", .result = false},
			Test{.path = "aa/bb/cc/dd", .pattern = "*/*/*/*", .result = true},
			Test{.path = "aa/bb/cc/dd", .pattern = "aa/*/**", .result = true},
			Test{.path = "aa/bb/cc/dd", .pattern = "aa/**/dd", .result = true},
			Test{.path = "aa/bb/cc/dd", .pattern = "**", .result = true},
			Test{.path = "aa/bb/cc/dd", .pattern = "**/dd", .result = true},
			Test{.path = "aa/bb/cc/dd", .pattern = "**/*/**", .result = false},
			Test{.path = "aa/bb/cc/dd", .pattern = "**/**", .result = false},
			Test{.path = "aa/bb/cc/dd", .pattern = "**/ddd", .result = false},
			Test{.path = "aa/bb/cc/dd", .pattern = "aa1/bb/cc/dd", .result = false},
			Test{.path = "aa/bb/cc/dd", .pattern = "aa/bb/cc/dd1", .result = false},
			Test{.path = "aa/bb/cc/dd", .pattern = "aa/bb/cc1/dd", .result = false},
			Test{.path = "aa/bb/cc/dd", .pattern = "bb/cc/dd", .result = false},
			Test{.path = "aa/bb/cc/dd", .pattern = "aa/bb/cc", .result = false},
			Test{.path = "aa/bb/cc/dd", .pattern = "aa/*/bb/cc", .result = false},
			Test{.path = "aa/bb/cc/dd", .pattern = "aa/bb/cc/dd/*", .result = false},
			Test{.path = "aa/bb/cc/dd", .pattern = "*/aa/bb/cc/dd", .result = false},
			Test{.path = "aa/bb/cc/dd", .pattern = "*/aa/bb/cc/dd/*", .result = false},
			Test{.path = "aa/bb/cc/dd", .pattern = "aa/bb/cc/dd/ee", .result = false},
			Test{.path = "aa/bb/cc/dd", .pattern = "aa/bb/cc", .result = false},
			Test{.path = "aa/bb/cc/dd", .pattern = "*/*/*", .result = false},
			Test{.path = "aa/bb/cc/dd", .pattern = "*/*/*/*/*", .result = false},
			Test{.path = "aa/bb/cc/dd", .pattern = "*/**/*/*/*", .result = false},
		};
		for(const Test &t : cases) {
			REQUIRE(shv::core::utils::ShvPath(t.path).matchWild(t.pattern) == t.result);
		}
	}

}
