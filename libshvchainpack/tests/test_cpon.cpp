#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <shv/chainpack/cponreader.h>
#include <shv/chainpack/rpcvalue.h>

#include <necrolog.h>

#include <doctest/doctest.h>

#include <array>
#include <sstream>

using namespace shv::chainpack;
using std::string;

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
doctest::String toString(const RpcDecimal& value) {
	std::ostringstream sb;
	sb << "RpcDecimal(" << value.mantissa() << ',' << value.exponent() << ')';
	return sb.str().c_str();
}

}

DOCTEST_TEST_CASE("Cpon")
{
	DOCTEST_SUBCASE("Typical CPON strings test")
	{
		DOCTEST_SUBCASE("Null")
		{
			const string cpon = "null";
			string err;
			RpcValue rv1 = RpcValue(nullptr);
			RpcValue rv2 = RpcValue::fromCpon(cpon, &err);
			REQUIRE(err.empty());
			REQUIRE(rv1 == rv2);
			REQUIRE(rv1.toCpon() == cpon);
		}
		DOCTEST_SUBCASE("String")
		{
			string err;
			std::array cpons = {
				std::array{"\"a0728a532288587d76617976985992eb3830e259577c7a6d2e2d0758215e2b6f3a399b4d34e12cd91a96d288751ce29d1a8306dfa074bdff86f08c6fa15a54404cbce5ccd7aeb19042e3df52a5e8f6cc9f9bd7153b13530c5665ecafc1be7f9ad30a62d93870e26277b1d9c2d9a7aff6f397157e5248e2b97e126b65ec8f887246767e344ce152c34846e649ea0a7a9a2b79dbe40d55ec231c4104ccc0c3b7b48895c4ca7ab7cf89a1302ed7a3bf2a6cf3862f05d200488c37d8e98f0369d844078e63fced87df6195ecb2fb2f9b532fc3880d6b6e6d880bc79c0e3fbaf387ebbc020cd46675537252aff233cb205ca627fa803b8af672d8cb5bdbfbf75526f\"", ""},
				std::array{ R"(b"\a1fooBARab")", ""},
				std::array{ "x\"6131d2\"", R"(b"a1\d2")"},
			};
			for(auto test : cpons) {
				const char *cpon1 = test[0];
				const char *cpon2 = test[1];
				if(cpon2[0] == 0)
					cpon2 = cpon1;
				const RpcValue cp = RpcValue::fromCpon(cpon1, &err);
				string cpon = cp.toCpon();
				REQUIRE(err.empty());
				REQUIRE(cpon == cpon2);
			}
		}
		DOCTEST_SUBCASE("Numbers")
		{
			RpcValue v(static_cast<int64_t>(1526303051038LL));
			auto s = v.toCpon();
			nDebug() << s;
			nDebug() << RpcValue::fromCpon("1526303051038").toCpon();
			string err;
			REQUIRE(RpcValue::fromCpon("0", &err) == RpcValue(0));
			REQUIRE(err.empty());
			REQUIRE(RpcValue::fromCpon("123", &err) == RpcValue(123));
			REQUIRE(err.empty());
			REQUIRE(RpcValue::fromCpon("1526303051038", &err) == RpcValue(static_cast<int64_t>(1526303051038LL)));
			REQUIRE(err.empty());
			REQUIRE(RpcValue::fromCpon("-123", &err) == RpcValue(-123));
			REQUIRE(err.empty());
			REQUIRE(RpcValue::fromCpon("3u", &err) == RpcValue(static_cast<RpcValue::UInt>(3)));
			REQUIRE(err.empty());
			REQUIRE(RpcValue::fromCpon("223.", &err) == RpcValue(223.));
			REQUIRE(err.empty());
			REQUIRE(RpcValue::fromCpon("0.123", &err) == RpcValue(0.123));
			REQUIRE(err.empty());
			REQUIRE(RpcValue::fromCpon("0.123", &err) == RpcValue(.123));
			REQUIRE(err.empty());
			{
				auto rv = RpcValue::fromCpon("1e23", &err);
				REQUIRE(err.empty());
				REQUIRE(rv == RpcValue(1e23));
			}
			REQUIRE(RpcValue::fromCpon("1e-3", &err) == RpcValue(1e-3));
			REQUIRE(err.empty());
			REQUIRE(RpcValue::fromCpon("12.3e-10", &err) == RpcValue(12.3e-10));
			REQUIRE(err.empty());
			REQUIRE(RpcValue::fromCpon("0.0123", &err) == RpcValue(RpcValue::Decimal(123, -4)));
			REQUIRE(err.empty());
		}
		DOCTEST_SUBCASE("List")
		{
			for(auto test : {"[]", "[ ]", " [ ]", "[ 1, 2,3  , ]"}) {
				string err;
				const RpcValue cp = RpcValue::fromCpon(test, &err);
				nDebug() << test << "--->" << cp.toCpon();
				REQUIRE(err.empty());
			}
			{
				string err;
				const string test = R"(
									[
										"foo bar",
										123,
										true,
										false,
										null,
										456u,
										0.123,
										123.456,
										d"2018-01-14T01:17:33.256-1045"
										]
									)";
				const auto cp = RpcValue::fromCpon(test, &err);
				REQUIRE(err.empty());
				nDebug() << "List test:" << test << "==" << cp.toCpon();
				nDebug() << "Cpon writer:" << cp.toPrettyString();
				REQUIRE(cp.at(0) == "foo bar");
				REQUIRE(cp.at(1) == 123);
				REQUIRE(cp.at(2) == true);
				REQUIRE(cp.at(3) == false);
				REQUIRE(cp.at(4) == RpcValue(nullptr));
				REQUIRE(cp.at(5) == RpcValue::UInt(456));
				REQUIRE(cp.at(6) == 0.123);
				REQUIRE(cp.at(7) == RpcValue::Decimal(123456, -3));
				RpcValue::DateTime dt = cp.at(8).toDateTime();
				REQUIRE(dt.msecsSinceEpoch() % 1000 == 256);
				REQUIRE(dt.utcOffsetMin() == -(10*60+45));
			}
		}
		DOCTEST_SUBCASE("Map")
		{
			string err;
			for(auto test : {"{}", "{ }", " { }", R"({ "1": 2,"3": 45u  , })"}) {
				const RpcValue cp = RpcValue::fromCpon(test, &err);
				nDebug() << test << "--->" << cp.toCpon();
				REQUIRE(err.empty());
			}
		}
		DOCTEST_SUBCASE("IMap")
		{
			string err;
			for(auto test : {
					"i{}",
					"i{ }",
					" i{ }",
					"i{ 1: 2,3: 45  , }",
				}) {
				const RpcValue cp = RpcValue::fromCpon(test, &err);
				nDebug() << test << "--->" << cp.toCpon();
				REQUIRE(err.empty());
			}
		}
		DOCTEST_SUBCASE("Meta")
		{
			{
				string err;
				for(auto test : {" <>[1]", "<>[]", "<1:2>[ ]", "<\"foo\":1>[ 1, 2,3  , ]"}) {
					const RpcValue cp = RpcValue::fromCpon(test, &err);
					nDebug() << test << "--->" << cp.toCpon();
					REQUIRE(err.empty());
				}
			}
			{
				string err;
				for(auto test : {
					"<1:2>i{3:<4:5>6}",
					"<1:2>[3,<4:5>6]",
					"[[]]", "[[[]]]",
					"{\"a\":{}}", "i{1:i{}}",
					"<8:3u>i{2:[[\".broker\",true]]}",
					R"(<4:"svete">i{2:<4:"svete">[0,1]})",
				}) {
					const RpcValue cp = RpcValue::fromCpon(test, &err);
					nDebug() << test << "--->" << cp.toCpon();
					REQUIRE(err.empty());
					REQUIRE(cp.toCpon() == test);
				}
			}
		}
		DOCTEST_SUBCASE("Complex CPON")
		{
			const string simple_test = R"(
									   {
										 "k1":"v1",
										 "k2":42,
										 "k3":["a",123,true,false,null],
									   }
									   )";

			string err;
			const auto cpon = RpcValue::fromCpon(simple_test, &err);
			REQUIRE(err.empty());
			REQUIRE(cpon.asMap().value("k2") == 42);
			REQUIRE(cpon.asMap().value("k3").asList().value(1) == 123);
		}
		DOCTEST_SUBCASE("Complex CPON")
		{
			string err;
			const string imap_test = R"(
									 <1:1, 2u: "bar", "pi-value":3.14>
									 i{
									   1:"v1",
									   2:42,
									   3:[
										 "a",
										 {"foo":1,"bar":2,"baz":3},
										 123,
										 true,
										 false,
										 null,
										 123, 456u, 0.123, 123.456
									   ]
									 }
									 )";
			{
				std::istringstream in(imap_test);
				CponReader rd(in);
				RpcValue::MetaData md;
				rd.read(md);
				nDebug().nospace() << "metadata test: \n" << md.toPrettyString();
				REQUIRE(!md.isEmpty());
			}
			{
				const auto cp = RpcValue::fromCpon(imap_test, &err);
				nDebug().nospace() << "imap_test: \n" << cp.toPrettyString();
				nDebug() << "err: " << err;
				REQUIRE(err.empty());
			}
		}

		DOCTEST_SUBCASE("Comments")
		{
			{
				const string comment_test = R"({
									  // comment /* with nested comment */
									  "a": 1,
									  // comment
									  // continued
									  "b": "text",
									  /* multi
									  line
									  comment
									  // line-comment-inside-multiline-comment
									  */
									  // and single-line comment
									  // and single-line comment /* multiline inside single line */
									  "c": [1, 2, 3]
									  // and single-line comment at end of object
									  })";

				string err;
				auto json_comment = RpcValue::fromCpon(comment_test, &err);
				REQUIRE(err.empty());
				REQUIRE(json_comment.asMap().size() == 3);
			}
			{
				string err;
				const string comment_test = "{\"a\": 1}//trailing line comment";
				auto json_comment = RpcValue::fromCpon(comment_test, &err);
				REQUIRE(err.empty());
				REQUIRE(json_comment.asMap().size() == 1);
			}
			{
				string err;
				const string comment_test = "/*leading multi-line comment*/{\"a\": 1}";
				auto json_comment = RpcValue::fromCpon( comment_test, &err);
				REQUIRE(err.empty());
				REQUIRE(json_comment.asMap().size() == 1);
			}
			{
				const string failing_comment_test = "{\n/* unterminated comment\n\"a\": 1,\n}";
				string err_failing_comment;
				RpcValue json_failing_comment = RpcValue::fromCpon( failing_comment_test, &err_failing_comment);
				REQUIRE(!json_failing_comment.isValid());
				REQUIRE(!err_failing_comment.empty());
			}
			{
				const string failing_comment_test = "{\n/* unterminated trailing comment }";
				string err;
				auto rv = RpcValue::fromCpon( failing_comment_test, &err);
				REQUIRE(!rv.isValid());
				REQUIRE(!err.empty());
			}
			{
				const string failing_comment_test = "{\n/ / bad comment }";
				string err;
				auto rv = RpcValue::fromCpon( failing_comment_test, &err);
				REQUIRE(!rv.isValid());
				REQUIRE(!err.empty());
			}
			{
				const string failing_comment_test = "{// bad comment }";
				string err;
				auto rv = RpcValue::fromCpon( failing_comment_test, &err);
				REQUIRE(!rv.isValid());
				REQUIRE(!err.empty());
			}
			{
				const string failing_comment_test = "{/* bad\ncomment *}";
				string err;
				auto rv = RpcValue::fromCpon( failing_comment_test, &err);
				REQUIRE(!rv.isValid());
				REQUIRE(!err.empty());
			}
		}

		DOCTEST_SUBCASE("List and map constructors")
		{
			{
				std::vector<int> l1 { 1, 2, 3 };
				std::vector<int> l2 { 1, 2, 3 };
				std::set<int> l3 { 1, 2, 3 };
				REQUIRE(RpcValue(l1) == RpcValue(l2));
				REQUIRE(RpcValue(l2) == RpcValue(l3));
				std::map<string, string> m1 { { "k1", "v1" }, { "k2", "v2" } };
				std::unordered_map<string, string> m2 { { "k1", "v1" }, { "k2", "v2" } };
				REQUIRE(RpcValue(m1) == RpcValue(m2));
			}
			{
				// ChainPack literals
				const RpcValue obj = RpcValue::Map({
													   { "k1", "v1" },
													   { "k2", 42.0 },
													   { "k3", RpcList({ "a", 123.0, true, false, nullptr }) },
												   });

				nDebug() << "obj: " << obj.toCpon();
				REQUIRE(obj.toCpon() == "{\"k1\":\"v1\",\"k2\":42.,\"k3\":[\"a\",123.,true,false,null]}");

				REQUIRE(RpcValue("a").toDouble() == 0.);
				REQUIRE(RpcValue("a").asString() == "a");
				REQUIRE(RpcValue().toDouble() == 0.);
			}
			{
				RpcValue my_json = RpcValue::Map {
					{ "key1", "value1" },
					{ "key2", false },
					{ "key3", RpcList { 1, 2, 3 } },
				};
				std::string json_obj_str = my_json.toCpon();
				nDebug() << "json_obj_str: " << json_obj_str.c_str();
				REQUIRE(json_obj_str == "{\"key1\":\"value1\",\"key2\":false,\"key3\":[1,2,3]}");
			}
			{
				class Point {
				public:
					int x;
					int y;
					Point (int xx, int yy) : x(xx), y(yy) {}
					RpcValue toRpcValue() const { return RpcList { x, y }; }
				};

				{
					string err;
					auto rpcval = RpcValue::fromCpon(R"(<1:2,2:12,8:"foo",9:[1,2,3],"bar":"baz",>["META",17,18,19])", &err);
					REQUIRE(rpcval.isValid());
					REQUIRE(err.empty());
					REQUIRE(rpcval.metaValue("bar") == "baz");
					REQUIRE(rpcval.metaValue(1) == 2);
					REQUIRE(rpcval.metaValue(8) == "foo");
					REQUIRE(rpcval.at(3) == 19);
				}
			}
		}
	}

	DOCTEST_SUBCASE("fromCpon invalid inputs")
	{
		const string input = "invalid input";
		REQUIRE_THROWS_AS(shv::chainpack::RpcValue::fromCpon(input), shv::chainpack::ParseException);
	}

	DOCTEST_SUBCASE("Very long decimals")
	{
		// read very long decimal without overflow error, value is capped
		REQUIRE(RpcValue::fromCpon("123456789012345678901234567890123456789012345678901234567890").toInt64() == std::numeric_limits<int64_t>::max());
		REQUIRE(RpcValue::fromCpon("9223372036854775806").toInt64() == 9223372036854775806LL);
		REQUIRE(RpcValue::fromCpon("9223372036854775807").toInt64() == std::numeric_limits<int64_t>::max());
		REQUIRE(RpcValue::fromCpon("9223372036854775808").toInt64() == std::numeric_limits<int64_t>::max());
		REQUIRE(RpcValue::fromCpon("0x7FFFFFFFFFFFFFFE").toInt64() == 0x7FFFFFFFFFFFFFFELL);
		REQUIRE(RpcValue::fromCpon("0x7FFFFFFFFFFFFFFF").toInt64() == std::numeric_limits<int64_t>::max());
		REQUIRE(RpcValue::fromCpon("0x8000000000000000").toInt64() == std::numeric_limits<int64_t>::max());
		REQUIRE(RpcValue::fromCpon("-123456789012345678901234567890123456789012345678901234567890").toInt64() == std::numeric_limits<int64_t>::min());
		REQUIRE(RpcValue::fromCpon("-9223372036854775807").toInt64() == -9223372036854775807LL);
		REQUIRE(RpcValue::fromCpon("-9223372036854775808").toInt64() == std::numeric_limits<int64_t>::min());
		REQUIRE(RpcValue::fromCpon("-9223372036854775809").toInt64() == std::numeric_limits<int64_t>::min());
		REQUIRE(RpcValue::fromCpon("-0x7FFFFFFFFFFFFFFF").toInt64() == -0x7FFFFFFFFFFFFFFFLL);
		REQUIRE(RpcValue::fromCpon("-0x8000000000000000").toInt64() == std::numeric_limits<int64_t>::min());
		REQUIRE(RpcValue::fromCpon("-0x8000000000000001").toInt64() == std::numeric_limits<int64_t>::min());
		REQUIRE(RpcValue::fromCpon("1.23456789012345678901234567890123456789012345678901234567890").toDecimal() == RpcDecimal(1234567890123456789LL, -18));
		REQUIRE(RpcValue::fromCpon("12345678901234567890123456789012345678901234567890123456.7890").toDecimal() == RpcDecimal(std::numeric_limits<int64_t>::max(), 0));
		REQUIRE(RpcValue::fromCpon("123456789012345678901234567890123456789012345678901234567890.").toDecimal() == RpcDecimal(std::numeric_limits<int64_t>::max(), 0));
	}
}
