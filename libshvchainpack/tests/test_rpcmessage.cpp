#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/chainpackwriter.h>
#include <shv/chainpack/chainpackreader.h>

#include <necrolog.h>

#include <doctest/doctest.h>

#include <optional>

using namespace shv::chainpack;
using std::string;

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

}

namespace shv::chainpack {

doctest::String toString(const RpcValue& value) {
	return value.toCpon().c_str();
}
}

DOCTEST_TEST_CASE("RpcMessage")
{

	DOCTEST_SUBCASE("RpcRequest")
	{
		RpcRequest rq;
		rq.setRequestId(123)
				.setMethod("foo")
				.setParams({{
								{"a", 45},
								{"b", "bar"},
								{"c", RpcValue::List{1,2,3}},
							}});
		rq.setMetaValue(RpcMessage::MetaType::Tag::ShvPath, "aus/mel/pres/A");
		std::stringstream out;
		RpcValue cp1 = rq.value();
		{ ChainPackWriter wr(out); rq.write(wr); }
		ChainPackReader rd(out);
		RpcValue cp2 = rd.read();
		REQUIRE(cp1.type() == cp2.type());
		RpcRequest rq2(cp2);
		REQUIRE(rq2.isRequest());
		REQUIRE(rq2.requestId() == rq.requestId());
		REQUIRE(rq2.method() == rq.method());
		REQUIRE(rq2.params() == rq.params());
	}
	DOCTEST_SUBCASE("RpcResponse result")
	{
		RpcResponse rs;
		rs.setRequestId(123).setResult(42U);
		std::stringstream out;
		RpcValue cp1 = rs.value();
		{ ChainPackWriter wr(out); rs.write(wr); }
		ChainPackReader rd(out);
		RpcValue cp2 = rd.read();
		REQUIRE(cp1.type() == cp2.type());
		RpcResponse rs2(cp2);
		REQUIRE(rs2.isResponse());
		REQUIRE(rs2.requestId() == rs.requestId());
		REQUIRE(rs2.result() == rs.result());
	}
	DOCTEST_SUBCASE("RpcResponse error")
	{
		RpcResponse rs;
		rs.setRequestId(123)
			.setError(RpcResponse::Error("Paramter length should be greater than zero!", RpcResponse::Error::InvalidParams, RpcValue::Map{{"foo", 42}}));
		std::stringstream out;
		RpcValue cp1 = rs.value();
		{ ChainPackWriter wr(out); rs.write(wr); }
		ChainPackReader rd(out);
		RpcValue cp2 = rd.read();
		REQUIRE(cp1.type() == cp2.type());
		RpcResponse rs2(cp2);
		REQUIRE(rs2.isResponse());
		REQUIRE(rs2.requestId() == rs.requestId());
		REQUIRE(rs2.error() == rs.error());
	}
	DOCTEST_SUBCASE("RpcNotify")
	{
		RpcRequest rq;
		rq.setMethod("foo")
				.setParams({{
							   {"a", 45},
							   {"b", "bar"},
							   {"c", RpcValue::List{1,2,3}},
						   }});
		nDebug() << rq.toCpon();
		REQUIRE(rq.isSignal());
		std::stringstream out;
		RpcValue cp1 = rq.value();
		{ ChainPackWriter wr(out); rq.write(wr); }
		ChainPackReader rd(out);
		RpcValue cp2 = rd.read();
		REQUIRE(cp1.type() == cp2.type());
		RpcRequest rq2(cp2);
		REQUIRE(rq2.isSignal());
		REQUIRE(rq2.method() == rq.method());
		REQUIRE(rq2.params() == rq.params());
	}

}
