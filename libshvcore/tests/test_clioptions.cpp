#include <shv/core/utils/clioptions.h>
#include <shv/core/utils/shvpath.h>
#include <shv/core/utils.h>
#include <shv/core/stringview.h>
#include <necrolog.h>

#include <filesystem>

#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

using namespace shv::core::utils;
using namespace shv::core;

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::vector<std::string> cmdline;

int main(int argc, char** argv)
{
	cmdline = NecroLog::setCLIOptions(argc, argv);

	return doctest::Context(argc, argv).run();
}

DOCTEST_TEST_CASE("CliOptions")
{
	using namespace std::string_literals;
	DOCTEST_SUBCASE("Make config dir and config file absolute")
	{
		std::string req_abs_config_dir;
		std::string req_abs_config_file;
		std::string config_dir;
		std::string config_file;
		const auto cwd = std::filesystem::current_path().string();
		const auto default_config_file_name = "test_core_clioptions.conf"s;
		ConfigCLIOptions cliopts;
		cliopts.parse(cmdline);
		DOCTEST_SUBCASE("Empty config dir")
		{
			config_dir = {};
			DOCTEST_SUBCASE("Empty config file")
			{
				config_file = {};
				req_abs_config_dir = cwd;
				req_abs_config_file = joinPath(cwd, default_config_file_name);
			}
			DOCTEST_SUBCASE("Relative config file")
			{
				config_file = "foo/bar/baz.conf"s;
				req_abs_config_dir = cwd;
				req_abs_config_file = joinPath(cwd, config_file);
			}
			DOCTEST_SUBCASE("Absolute config file")
			{
				config_file = "/foo/bar/baz.conf"s;
				req_abs_config_dir = "/foo/bar"s;
				req_abs_config_file = config_file;
			}
		}
		DOCTEST_SUBCASE("Relative config dir")
		{
			config_dir = "foo/bar"s;
			DOCTEST_SUBCASE("Empty config file")
			{
				config_file = {};
				req_abs_config_dir = joinPath(cwd, config_dir);
				req_abs_config_file = joinPath(cwd, config_dir, default_config_file_name);
			}
			DOCTEST_SUBCASE("Relative config file")
			{
				config_file = "foo/bar/baz.conf"s;
				req_abs_config_dir = joinPath(cwd, config_dir);
				req_abs_config_file = joinPath(cwd, config_dir, config_file);
			}
			DOCTEST_SUBCASE("Absolute config file")
			{
				config_file = "/abs/foo/bar/baz.conf"s;
				req_abs_config_dir = joinPath(cwd, config_dir);
				req_abs_config_file = config_file;
			}
		}
		DOCTEST_SUBCASE("Absolute config dir")
		{
			config_dir = "/a/foo/bar"s;
			DOCTEST_SUBCASE("Empty config file")
			{
				config_file = {};
				req_abs_config_dir = config_dir;
				req_abs_config_file = joinPath(config_dir, default_config_file_name);
			}
			DOCTEST_SUBCASE("Relative config file")
			{
				config_file = "foo/bar/baz.conf"s;
				req_abs_config_dir = config_dir;
				req_abs_config_file = joinPath(config_dir, config_file);
			}
			DOCTEST_SUBCASE("Absolute config file")
			{
				config_file = "/abs/foo/bar/baz.conf"s;
				req_abs_config_dir = config_dir;
				req_abs_config_file = config_file;
			}
		}
		auto paths = cliopts.absoluteConfigPaths(config_dir, config_file);
		REQUIRE(std::get<0>(paths) == req_abs_config_dir);
		REQUIRE(std::get<1>(paths) == req_abs_config_file);
	}
}
