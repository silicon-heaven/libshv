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
// NOLINTBEGIN(misc-use-internal-linkage)
doctest::String toString(const RpcValue& value) {
	return value.toCpon().c_str();
}

doctest::String toString(const RpcValue::DateTime& value) {
	return value.toIsoString().c_str();
}
// NOLINTEND(misc-use-internal-linkage)
}

DOCTEST_TEST_CASE("RpcValue")
{
	DOCTEST_SUBCASE("take Meta test")
	{
		nDebug() << "================================= takeMeta Test =====================================";
		auto rpcval = RpcValue::fromCpon(R"(<1:2,2:12,8:"foo",9:[1,2,3],"bar":"baz",>{"META":17,"18":19})");
		const auto& rv1 = rpcval;
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

	DOCTEST_SUBCASE("RpcDecimal normalization")
	{
		SUBCASE("Removes trailing zeros and adjusts exponent") {
			RpcDecimal d1(1000, -3); // 1000 * 10^-3 = 1.0
			RpcDecimal n1 = RpcDecimal::normalize(d1);
			REQUIRE(n1.mantissa() == 1);
			REQUIRE(n1.exponent() == 0);

			RpcDecimal d2(1200, -3); // 1200 * 10^-3 = 1.2
			RpcDecimal n2 = RpcDecimal::normalize(d2);
			REQUIRE(n2.mantissa() == 12);   // 12 * 10^-1 = 1.2
			REQUIRE(n2.exponent() == -1);

			RpcDecimal d3(500, -1); // 500 * 10^-1 = 50
			RpcDecimal n3 = RpcDecimal::normalize(d3);
			REQUIRE(n3.mantissa() == 5);    // 5 * 10^1 = 50
			REQUIRE(n3.exponent() == 1);
		}

		SUBCASE("Zero normalization keeps exponent 0") {
			RpcDecimal zero1(0, -10);
			RpcDecimal zero2 = RpcDecimal::normalize(zero1);
			REQUIRE(zero2.mantissa() == 0);
			REQUIRE(zero2.exponent() == 0);
		}

		SUBCASE("Negative values are normalized correctly") {
			RpcDecimal d1(-5000, -3); // -5000 * 10^-3 = -5.0
			RpcDecimal n1 = RpcDecimal::normalize(d1);
			REQUIRE(n1.mantissa() == -5);
			REQUIRE(n1.exponent() == 0);
		}

		SUBCASE("Normalization preserves numeric value") {
			RpcDecimal d1(1200, -3);
			RpcDecimal n1 = RpcDecimal::normalize(d1);
			REQUIRE(d1.toDouble() == doctest::Approx(n1.toDouble()));
		}
	}


	DOCTEST_SUBCASE("Decimal comparison operators")
	{
		RpcDecimal d1(100, -2);   // 1.00
		RpcDecimal d2(1, 0);      // 1
		RpcDecimal d3(10, -1);    // 1.0

		SUBCASE("Equality and normalization") {
			REQUIRE(d1 == d2);
			REQUIRE(d2 == d3);
			REQUIRE_FALSE(d1 != d2);
			REQUIRE(d1 <= d3);
			REQUIRE(d1 >= d3);
		}

		SUBCASE("Less and greater comparisons") {
			RpcDecimal smaller(5, -1); // 0.5
			RpcDecimal larger(2, 0);   // 2
			REQUIRE(smaller < d1);
			REQUIRE(d1 > smaller);
			REQUIRE(d1 < larger);
			REQUIRE(larger > d1);
			REQUIRE(d1 <= larger);
			REQUIRE(larger >= d1);
		}

		SUBCASE("Negative numbers") {
			RpcDecimal neg1(-5, 0);
			RpcDecimal neg2(-50, -1); // -5.0
			RpcDecimal pos(5, 0);
			REQUIRE(neg1 == neg2);
			REQUIRE(neg1 < pos);
			REQUIRE(pos > neg2);
		}

		SUBCASE("Zero edge cases") {
			RpcDecimal zero1(0, 0);
			RpcDecimal zero2(0, 5); // 0 * 10^5 == 0
			REQUIRE(zero1 == zero2);
			REQUIRE_FALSE(zero1 < zero2);
			REQUIRE_FALSE(zero1 > zero2);
			REQUIRE(zero1 <= zero2);
			REQUIRE(zero1 >= zero2);
		}

		SUBCASE("Large exponents") {
			RpcDecimal big1(1, 3);    // 1000
			RpcDecimal big2(1000, 0); // 1000
			RpcDecimal big3(1, 4);    // 10000
			REQUIRE(big1 == big2);
			REQUIRE(big1 < big3);
			REQUIRE(big3 > big2);
		}
	}
}
