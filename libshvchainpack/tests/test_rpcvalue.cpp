#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <shv/chainpack/cponreader.h>
#include <shv/chainpack/rpcvalue.h>

#include <necrolog.h>

#include <doctest/doctest.h>

using namespace shv::chainpack;
using namespace std;
using namespace std::string_literals;

// Check that ChainPack has the properties we want.
#define CHECK_TRAIT(x) static_assert(std::x::value, #x)
CHECK_TRAIT(is_nothrow_constructible<RpcValue>);
CHECK_TRAIT(is_nothrow_default_constructible<RpcValue>);
CHECK_TRAIT(is_copy_constructible<RpcValue>);
CHECK_TRAIT(is_move_constructible<RpcValue>);
CHECK_TRAIT(is_copy_assignable<RpcValue>);
CHECK_TRAIT(is_move_assignable<RpcValue>);
CHECK_TRAIT(is_nothrow_destructible<RpcValue>);

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

	DOCTEST_SUBCASE("refcnt test")
	{
		nDebug() << "================================= RefCnt Test =====================================";
		auto rpcval = RpcValue::fromCpon(R"(<1:2,2:12,8:"foo",9:[1,2,3],"bar":"baz",>{"META":17,"18":19})");
		auto rv1 = rpcval;
		REQUIRE(rpcval.refCnt() == 2);
		{
			auto rv2 = rpcval;
			REQUIRE(rpcval.refCnt() == 3);
			REQUIRE(rpcval == rv2);
		}
		REQUIRE(rpcval.refCnt() == 2);
		rv1.set("foo", "bar");
		REQUIRE(rv1.refCnt() == 1);
		REQUIRE(rpcval.refCnt() == 1);
		REQUIRE(rv1.at("foo") == RpcValue("bar"));
		rv1 = rpcval;
		REQUIRE(rpcval.refCnt() == 2);
		rpcval.setMetaValue(2, 42);
		REQUIRE(rv1.refCnt() == 1);
		REQUIRE(rpcval.refCnt() == 1);
		REQUIRE(rpcval.metaValue(2) == RpcValue(42));
		REQUIRE(rv1.metaValue(2) == RpcValue(12));
	}
	DOCTEST_SUBCASE("take Meta test")
	{
		nDebug() << "================================= takeMeta Test =====================================";
		auto rpcval = RpcValue::fromCpon(R"(<1:2,2:12,8:"foo",9:[1,2,3],"bar":"baz",>{"META":17,"18":19})");
		auto rv1 = rpcval;
		auto rv2 = rv1;
		auto rv3 = rv1;
		rv3.takeMeta();
		REQUIRE(rpcval.refCnt() == 3);
		REQUIRE(rv3.refCnt() == 1);
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
