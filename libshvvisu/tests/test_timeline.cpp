#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <shv/visu/timeline/graphmodel.h>
#include <shv/core/log.h>

#include <doctest/doctest.h>

using namespace shv::visu::timeline;

DOCTEST_TEST_CASE("Graph model")
{
	static constexpr auto CHANNEL = "channel";
	static constexpr auto EMPTY_CHANNEL = "emptyChannel";

	auto graph_model = shv::visu::timeline::GraphModel();

	shv::core::utils::ShvTypeDescr td(shv::core::utils::ShvTypeDescr::Type::Int);
	graph_model.appendChannel(CHANNEL, {}, td);
	size_t ch_ix = graph_model.channelCount() -1;

	/* Data
		time | 1 | 4 | 10|
		ix   | 0 | 1 | 2 |
	*/

	graph_model.appendValueShvPath(CHANNEL, Sample(1, 1));
	graph_model.appendValueShvPath(CHANNEL, Sample(4, 4));
	graph_model.appendValueShvPath(CHANNEL, Sample(10, 3));

	graph_model.appendChannel(EMPTY_CHANNEL, {}, td);
	size_t empty_ch_ix = graph_model.channelCount() -1;
	size_t invalid_ch_ix = graph_model.channelCount();

	DOCTEST_SUBCASE("lessOrEqualTimeIndex")
	{
		DOCTEST_SUBCASE("invalid channel")
		{
			REQUIRE(!graph_model.lessOrEqualTimeIndex(invalid_ch_ix, 1).has_value());
		}

		DOCTEST_SUBCASE("empty channel")
		{
			REQUIRE(!graph_model.lessOrEqualTimeIndex(empty_ch_ix, 1).has_value());
		}

		DOCTEST_SUBCASE("value equals first sample time value")
		{
			REQUIRE(graph_model.lessOrEqualTimeIndex(ch_ix, 1) == 0);
		}

		DOCTEST_SUBCASE("value equals last sample time value")
		{
			REQUIRE(graph_model.lessOrEqualTimeIndex(ch_ix, 10) == 2);
		}

		DOCTEST_SUBCASE("value equals sample time value")
		{
			REQUIRE(graph_model.lessOrEqualTimeIndex(ch_ix, 4) == 1);
		}

		DOCTEST_SUBCASE("value between samples")
		{
			REQUIRE(graph_model.lessOrEqualTimeIndex(ch_ix, 2) == 0);
		}

		DOCTEST_SUBCASE("value less than x range")
		{
			REQUIRE(!graph_model.lessOrEqualTimeIndex(ch_ix, -10).has_value());
		}

		DOCTEST_SUBCASE("value greater than x range")
		{
			REQUIRE(graph_model.lessOrEqualTimeIndex(ch_ix, 20) == 2);
		}

	}

	DOCTEST_SUBCASE("lessTimeIndex")
	{
		DOCTEST_SUBCASE("invalid channel")
		{
			REQUIRE(!graph_model.lessTimeIndex(invalid_ch_ix, 1).has_value());
		}

		DOCTEST_SUBCASE("empty channel")
		{
			REQUIRE(!graph_model.lessTimeIndex(empty_ch_ix, 1).has_value());
		}

		DOCTEST_SUBCASE("value equals first sample time value")
		{
			REQUIRE(!graph_model.lessTimeIndex(ch_ix, 1).has_value());
		}

		DOCTEST_SUBCASE("value equals last sample time value")
		{
			REQUIRE(graph_model.lessTimeIndex(ch_ix, 10) == 1);
		}

		DOCTEST_SUBCASE("value equals sample time value")
		{
			REQUIRE(graph_model.lessTimeIndex(ch_ix, 4) == 0);
		}

		DOCTEST_SUBCASE("value between samples")
		{
			REQUIRE(graph_model.lessTimeIndex(ch_ix, 2) == 0);
		}

		DOCTEST_SUBCASE("value less than x range")
		{
			REQUIRE(!graph_model.lessTimeIndex(ch_ix, -10).has_value());
		}

		DOCTEST_SUBCASE("value greater than x range")
		{
			REQUIRE(graph_model.lessTimeIndex(ch_ix, 20) == 2);
		}
	}

	DOCTEST_SUBCASE("greaterTimeIndex")
	{
		DOCTEST_SUBCASE("invalid channel")
		{
			REQUIRE(!graph_model.greaterTimeIndex(invalid_ch_ix, 1).has_value());
		}

		DOCTEST_SUBCASE("empty channel")
		{
			REQUIRE(!graph_model.greaterTimeIndex(empty_ch_ix, 1).has_value());
		}

		DOCTEST_SUBCASE("value equals first sample time value")
		{
			REQUIRE(graph_model.greaterTimeIndex(ch_ix, 1) == 1);
		}

		DOCTEST_SUBCASE("value equals last sample time value")
		{
			REQUIRE(!graph_model.greaterTimeIndex(ch_ix, 10).has_value());
		}

		DOCTEST_SUBCASE("value equals sample time value")
		{
			REQUIRE(graph_model.greaterTimeIndex(ch_ix, 4) == 2);
		}

		DOCTEST_SUBCASE("value between samples")
		{
			REQUIRE(graph_model.greaterTimeIndex(ch_ix, 2) == 1);
		}

		DOCTEST_SUBCASE("value less than x range")
		{
			REQUIRE(graph_model.greaterTimeIndex(ch_ix, -10) == 0);
		}

		DOCTEST_SUBCASE("value greater than x range")
		{
			REQUIRE(!graph_model.greaterTimeIndex(ch_ix, 20).has_value());
		}
	}

	DOCTEST_SUBCASE("greaterOrEqualTimeIndex")
	{
		DOCTEST_SUBCASE("invalid channel")
		{
			REQUIRE(!graph_model.greaterOrEqualTimeIndex(invalid_ch_ix, 1).has_value());
		}

		DOCTEST_SUBCASE("empty channel")
		{
			REQUIRE(!graph_model.greaterOrEqualTimeIndex(empty_ch_ix, 1).has_value());
		}

		DOCTEST_SUBCASE("value equals first sample time value")
		{
			REQUIRE(graph_model.greaterOrEqualTimeIndex(ch_ix, 1) == 0);
		}

		DOCTEST_SUBCASE("value equals last sample time value")
		{
			REQUIRE(graph_model.greaterOrEqualTimeIndex(ch_ix, 10) == 2);
		}

		DOCTEST_SUBCASE("value equals sample time value")
		{
			REQUIRE(graph_model.greaterOrEqualTimeIndex(ch_ix, 4) == 1);
		}

		DOCTEST_SUBCASE("value between samples")
		{
			REQUIRE(graph_model.greaterOrEqualTimeIndex(ch_ix, 2) == 1);
		}

		DOCTEST_SUBCASE("value less than x range")
		{
			REQUIRE(graph_model.greaterOrEqualTimeIndex(ch_ix, -10) == 0);
		}

		DOCTEST_SUBCASE("value greater than x range")
		{
			REQUIRE(!graph_model.greaterOrEqualTimeIndex(ch_ix, 20).has_value());
		}
	}
}

