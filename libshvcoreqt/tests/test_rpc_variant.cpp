#include <shv/coreqt/rpc.h>
#include <shv/coreqt/log.h>
#include <shv/coreqt/utils.h>

#include <QVariant>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

using namespace shv::coreqt::rpc;
using namespace shv::chainpack;
using namespace std;

DOCTEST_TEST_CASE("RpcValue to QVariant conversion and vice versa")
{
	DOCTEST_SUBCASE("Map")
	{
		RpcValue rv1 = RpcValue::Map {
			{"foo", 1},
			{"bar", "baz"},
		};
		QVariant qv = rpcValueToQVariant(rv1);
		auto rv2 = qVariantToRpcValue(qv);
		REQUIRE(rv1 == rv2);
	}
	DOCTEST_SUBCASE("IMap")
	{
		RpcValue rv1 = RpcValue::IMap {
			{2, 1},
			{-1, "baz"},
		};
		QVariant qv = rpcValueToQVariant(rv1);
		auto rv2 = qVariantToRpcValue(qv);
		REQUIRE(rv1 == rv2);
	}
}

using shv::chainpack::RpcValue;
doctest::String toString(const QString& str)
{
	return str.toLatin1().data();
}

DOCTEST_TEST_CASE("shv2mqtt utils")
{
	DOCTEST_SUBCASE("")
	{
		RpcValue v;
		QByteArray expected;

		DOCTEST_SUBCASE("Invalid")
		{
			v = RpcValue();
			expected = "null";
		}

		DOCTEST_SUBCASE("Null")
		{
			v = RpcValue(nullptr);
			expected = "null";
		}

		DOCTEST_SUBCASE("UInt")
		{
			v = 100U;
			expected = "100";
		}

		DOCTEST_SUBCASE("Int")
		{
			v = 100;
			expected = "100";
		}

		DOCTEST_SUBCASE("Double")
		{
			v = RpcValue(100.0);
			expected = "100";
		}

		DOCTEST_SUBCASE("Bool")
		{
			DOCTEST_SUBCASE("true")
			{
				v = RpcValue(true);
				expected = "true";
			}

			DOCTEST_SUBCASE("false")
			{
				v = RpcValue(false);
				expected = "false";
			}
		}

		DOCTEST_SUBCASE("Blob")
		{
			v = RpcValue::Blob {
				2,
				100,
				230
			};
			expected = "\"AmTm\"";
		}

		DOCTEST_SUBCASE("String")
		{
			v = RpcValue::String("AHOJ");
			expected = "\"AHOJ\"";
		}

		DOCTEST_SUBCASE("DateTime")
		{
			v = RpcValue::DateTime::fromMSecsSinceEpoch(1000000);
			expected = "\"1970-01-01T00:16:40.090Z\"";
		}

		DOCTEST_SUBCASE("List")
		{
			v = RpcList {
				100,
				100U,
				RpcValue::String("AHOJ"),
				RpcValue(true),
			};
			expected = R"([
    100,
    100,
    "AHOJ",
    true
]
)";
		}

		DOCTEST_SUBCASE("Map")
		{
			v = RpcValue::Map {
				{"int_key", 100},
				{"uint_key", 100U},
				{"string_key", RpcValue::String("AHOJ")},
				{"datetime_key", RpcValue::DateTime::fromMSecsSinceEpoch(1000000)},
				{"imap_key", RpcValue::IMap{ {1, RpcValue::String("AHOJ")} }},
				{"map_key", RpcValue::Map{ {"some_key", RpcValue::String("AHOJ")} }},
				{"bool_key", RpcValue(true)},
			};
			expected = R"({
    "bool_key": true,
    "datetime_key": "1970-01-01T00:16:40.090Z",
    "imap_key": {
        "1": "AHOJ"
    },
    "int_key": 100,
    "map_key": {
        "some_key": "AHOJ"
    },
    "string_key": "AHOJ",
    "uint_key": 100
}
)";
		}

		DOCTEST_SUBCASE("IMap")
		{
			v = RpcValue::IMap {
				{0, 100},
				{3, 100U},
				{5, RpcValue::String("AHOJ")},
				{7, RpcValue(true)},
			};
			expected = R"({
    "0": 100,
    "3": 100,
    "5": "AHOJ",
    "7": true
}
)";
		}

		DOCTEST_SUBCASE("Decimal")
		{
			v = RpcValue::Decimal(19999, -2);
			expected = "199.99";
		}

		REQUIRE(shv::coreqt::utils::jsonValueToByteArray(shv::coreqt::utils::rpcValueToJson(v)) == expected);
	}
}
