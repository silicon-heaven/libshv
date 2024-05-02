#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <shv/chainpack/cponreader.h>
#include <shv/chainpack/rpcvalue.h>

#include <necrolog.h>

#include <doctest/doctest.h>

using namespace shv::chainpack;
using namespace std;
using namespace std::string_literals;

// Check that ChainPack has the properties we want.
#define CHECK_TRAIT(x) static_assert(std::x, #x)
CHECK_TRAIT(is_nothrow_constructible_v<RpcValue>);
CHECK_TRAIT(is_nothrow_default_constructible_v<RpcValue>);
CHECK_TRAIT(is_copy_constructible_v<RpcValue>);
CHECK_TRAIT(is_move_constructible_v<RpcValue>);
CHECK_TRAIT(is_copy_assignable_v<RpcValue>);
CHECK_TRAIT(is_move_assignable_v<RpcValue>);
CHECK_TRAIT(is_nothrow_destructible_v<RpcValue>);

namespace {

template< typename T >
std::string int_to_hex( T i )
{
	std::stringstream stream;
	stream << "0x"
		   << std::hex << i;
	return stream.str();
}

}

namespace shv::chainpack {
doctest::String toString(const RpcValue& value) {
	return value.toCpon().c_str();
}

doctest::String toString(const RpcValue::DateTime& value) {
	return value.toIsoString().c_str();
}
}

DOCTEST_TEST_CASE("RpcValue")
{
	DOCTEST_SUBCASE("take Meta test")
	{
		nDebug() << "================================= takeMeta Test =====================================";
		auto rpcval = RpcValue::fromCpon(R"(<1:2,2:12,8:"foo",9:[1,2,3],"bar":"baz",>{"META":17,"18":19})");
		auto rv1 = rpcval;
		auto rv2 = rv1;
		auto rv3 = rv1;
		rv3.takeMeta();
		REQUIRE(rv1.metaData().isEmpty() == false);
		REQUIRE(rv3.metaData().isEmpty() == true);
		REQUIRE(rv3.at("18") == rpcval.at("18"));
	}
}

DOCTEST_TEST_CASE("RpcValue::DateTime")
{
	using Parts = RpcValue::DateTime::Parts;
	DOCTEST_SUBCASE("RpcValue::DateTime::toParts()")
	{
		for(const auto &[dt_str, dt_parts] : {
			std::make_tuple("2022-01-01T00:00:00Z"s, Parts(2022, 1, 1)),
			std::make_tuple("2023-01-23T01:02:03Z"s, Parts(2023, 1, 23, 1, 2, 3)),
			std::make_tuple("2024-02-29T01:02:03Z"s, Parts(2024, 2, 29, 1, 2, 3)),
		}) {
			auto dt1 = RpcValue::DateTime::fromUtcString(dt_str);
			auto dt2 = RpcValue::DateTime::fromParts(dt_parts);
			CAPTURE(dt1);
			CAPTURE(dt2);
			REQUIRE(dt1 == dt2);
			REQUIRE(dt1.toParts() == dt2.toParts());
			REQUIRE(dt2.toIsoString() == dt_str);
			REQUIRE(dt1.toParts() == dt_parts);
		}
	}
}

DOCTEST_TEST_CASE("RpcValue::Decimal")
{
	REQUIRE(RpcValue(RpcValue::Decimal(45, 2186)).toInt() == std::numeric_limits<RpcValue::Int>::max());
	REQUIRE(RpcValue(RpcValue(-2.41785e+306)).toInt() == std::numeric_limits<RpcValue::Int>::min());
}
