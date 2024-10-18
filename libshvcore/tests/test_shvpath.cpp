#include <shv/core/utils/shvpath.h>
#include <shv/core/stringview.h>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <array>

using namespace shv::core::utils;
using namespace shv::core;
using namespace std;

DOCTEST_TEST_CASE("ShvPath")
{
	DOCTEST_SUBCASE("ShvPath::startsWithPath()")
	{
		{
			string path = "status/errorCommunication";
			array paths = {
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
			string path = "foo/bar";
			array paths = {
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
		string dir = "'a/b/c'/d";
		string dir1 = "a/b/c";
		REQUIRE(ShvPath::firstDir(dir) == StringView(dir1));
	}
	DOCTEST_SUBCASE("ShvPath::takeFirsDir()")
	{
		string dir = "'a/b/c'/d";
		StringView dir_view(dir);
		string dir1 = "a/b/c";
		string dir2 = "d";
		auto dir_view1 = ShvPath::takeFirsDir(dir_view);
		REQUIRE(dir_view1 == StringView(dir1));
		REQUIRE(dir_view == StringView(dir2));
	}
	DOCTEST_SUBCASE("ShvPath join & split")
	{
		string dir1 = "foo";
		string dir2 = "bar/baz";
		string joined = "foo/'bar/baz'";

		DOCTEST_SUBCASE("ShvPath::joinDirs()")
		{
			REQUIRE(ShvPath::joinDirs(dir1, dir2) == joined);
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
			REQUIRE(ShvPath::startsWithPath(it->first, string("a/b")));
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
			Test{"", "aa", false},
			Test{"aa", "", false},
			Test{"", "", true},
			Test{"", "**", true},
			Test{"aa/bb/cc/dd", "aa/*/cc/dd", true},
			Test{"aa/bb/cc/dd", "aa/bb/**/cc/dd", true},
			Test{"aa/bb/cc/dd", "aa/bb/*/**/dd", true},
			Test{"aa/bb/cc/dd", "*/*/cc/dd", true},
			Test{"aa/bb/cc/dd", "*/cc/dd/**", false},
			Test{"aa/bb/cc/dd", "*/*/*/*", true},
			Test{"aa/bb/cc/dd", "aa/*/**", true},
			Test{"aa/bb/cc/dd", "aa/**/dd", true},
			Test{"aa/bb/cc/dd", "**", true},
			Test{"aa/bb/cc/dd", "**/dd", true},
			Test{"aa/bb/cc/dd", "**/*/**", false},
			Test{"aa/bb/cc/dd", "**/**", false},
			Test{"aa/bb/cc/dd", "**/ddd", false},
			Test{"aa/bb/cc/dd", "aa1/bb/cc/dd", false},
			Test{"aa/bb/cc/dd", "aa/bb/cc/dd1", false},
			Test{"aa/bb/cc/dd", "aa/bb/cc1/dd", false},
			Test{"aa/bb/cc/dd", "bb/cc/dd", false},
			Test{"aa/bb/cc/dd", "aa/bb/cc", false},
			Test{"aa/bb/cc/dd", "aa/*/bb/cc", false},
			Test{"aa/bb/cc/dd", "aa/bb/cc/dd/*", false},
			Test{"aa/bb/cc/dd", "*/aa/bb/cc/dd", false},
			Test{"aa/bb/cc/dd", "*/aa/bb/cc/dd/*", false},
			Test{"aa/bb/cc/dd", "aa/bb/cc/dd/ee", false},
			Test{"aa/bb/cc/dd", "aa/bb/cc", false},
			Test{"aa/bb/cc/dd", "*/*/*", false},
			Test{"aa/bb/cc/dd", "*/*/*/*/*", false},
			Test{"aa/bb/cc/dd", "*/**/*/*/*", false},
		};
		for(const Test &t : cases) {
			REQUIRE(shv::core::utils::ShvPath(t.path).matchWild(t.pattern) == t.result);
		}
	}

}
