#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <shv/chainpack/rpcvalue.h>
#include <doctest/doctest.h>
#include <optional>


DOCTEST_TEST_CASE("RpcValue")
{
	std::string input;
	std::optional<std::string> expected_output;

	DOCTEST_SUBCASE("simple inputs")
	{
		DOCTEST_SUBCASE("numeric value")
		{
			input = "1";
		}

		DOCTEST_SUBCASE("string value")
		{
			input = R"("ahoj")";
		}

		DOCTEST_SUBCASE("datetime value")
		{
			DOCTEST_SUBCASE("simple case") // TODO give me a better name
			{
				input = R"(d"2018-02-02 0:00:00.001")";
				expected_output = R"(d"2018-02-02T00:00:00.001Z")";
			}

			DOCTEST_SUBCASE("weird timezone")
			{
				input = R"(d"2041-03-04 0:00:00-1015")";
				expected_output = R"(d"2041-03-04T00:00:00-1015")";
			}
		}
	}

	if (!expected_output.has_value()) {
		expected_output = input;
	}

	std::string err;
	auto rpc_value = shv::chainpack::RpcValue::fromCpon(input, &err);
	auto output = rpc_value.toCpon();
	REQUIRE(output == *expected_output);
	REQUIRE(err.empty());
}
