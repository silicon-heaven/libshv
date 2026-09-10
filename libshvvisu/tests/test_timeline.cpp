#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <shv/visu/timeline/graph.h>
#include <shv/visu/timeline/graphmodel.h>
#include <shv/core/log.h>

#include <doctest/doctest.h>
#include <QImage>
#include <QPainter>

using namespace shv::visu::timeline;

class TestGraphChannel : public GraphChannel
{
public:
	using GraphChannel::GraphChannel;

	void setGraphAreaRect(const QRect &rect)
	{
		m_layout.graphAreaRect = rect;
	}
};

class TestGraph : public Graph
{
public:
	using Graph::drawCurrentTime;

	void addChannel(const QRect &graph_area)
	{
		auto *channel = new TestGraphChannel(this);
		channel->setGraphAreaRect(graph_area);
		m_channels.append(channel);
		m_layout.xAxisRect = graph_area;
	}
};

DOCTEST_TEST_CASE("Graph current time visibility")
{
	GraphModel graph_model;
	Graph graph;
	graph.setModel(&graph_model);
	graph.setXRange({0, 100});
	graph.setXRangeZoom({20, 40});

	DOCTEST_SUBCASE("visible time keeps zoom")
	{
		graph.setCurrentTime(30);

		REQUIRE(graph.currentTime() == 30);
		REQUIRE(graph.xRangeZoom() == XRange{20, 40});
	}

	DOCTEST_SUBCASE("hidden time recenters zoom")
	{
		graph.setCurrentTime(60);

		REQUIRE(graph.currentTime() == 60);
		REQUIRE(graph.xRangeZoom() == XRange{50, 70});
	}

	DOCTEST_SUBCASE("time outside full range is rejected")
	{
		graph.setCurrentTime(30);
		graph.setCurrentTime(110);

		REQUIRE(graph.currentTime() == 30);
		REQUIRE(graph.xRangeZoom() == XRange{20, 40});
	}

	DOCTEST_SUBCASE("empty current time hides marker")
	{
		graph.setCurrentTime(30);
		graph.setCurrentTime(std::nullopt);

		REQUIRE(!graph.currentTime());
		REQUIRE(graph.xRangeZoom() == XRange{20, 40});
	}

	DOCTEST_SUBCASE("zero is a valid time")
	{
		graph.setCurrentTime(0);

		REQUIRE(graph.currentTime() == 0);
		REQUIRE(graph.xRangeZoom().contains(0));
	}
}

DOCTEST_TEST_CASE("Graph draws current time at range boundary")
{
	TestGraph graph;
	const QRect graph_area(100, 50, 600, 300);
	graph.addChannel(graph_area);
	graph.setXRange({10, 100});
	graph.setXRangeZoom({10, 100});
	graph.setCurrentTime(10);

	QImage image(800, 400, QImage::Format_ARGB32_Premultiplied);
	image.fill(Qt::transparent);
	QPainter painter(&image);
	graph.drawCurrentTime(&painter, 0);
	painter.end();

	REQUIRE(image.pixelColor(graph_area.left(), graph_area.center().y()).alpha() > 0);
}

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

DOCTEST_TEST_CASE("Graph model duplicate values")
{
	static constexpr auto CHANNEL = "channel";
	shv::core::utils::ShvTypeDescr td(shv::core::utils::ShvTypeDescr::Type::Int);

	DOCTEST_SUBCASE("duplicate values are ignored by default")
	{
		GraphModel graph_model;
		graph_model.appendChannel(CHANNEL, {}, td);
		graph_model.appendValueShvPath(CHANNEL, Sample(1, 1));
		graph_model.appendValueShvPath(CHANNEL, Sample(2, 1));
		graph_model.appendValueShvPath(CHANNEL, Sample(3, QVariant{}));
		graph_model.appendValueShvPath(CHANNEL, Sample(4, QVariant{}));

		REQUIRE(graph_model.count(0) == 2);
	}

	DOCTEST_SUBCASE("duplicate values can be enabled")
	{
		GraphModel graph_model;
		graph_model.setDuplicateValuesEnabled(true);
		graph_model.appendChannel(CHANNEL, {}, td);
		graph_model.appendValueShvPath(CHANNEL, Sample(1, 1));
		graph_model.appendValueShvPath(CHANNEL, Sample(2, 1));
		graph_model.appendValueShvPath(CHANNEL, Sample(3, QVariant{}));
		graph_model.appendValueShvPath(CHANNEL, Sample(4, QVariant{}));

		REQUIRE(graph_model.count(0) == 4);
	}
}
