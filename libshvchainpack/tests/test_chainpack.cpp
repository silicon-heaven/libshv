#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <shv/chainpack/chainpack.h>
#include <shv/chainpack/chainpackreader.h>
#include <shv/chainpack/chainpackwriter.h>
#include <shv/chainpack/rpcvalue.h>
#include <shv/chainpack/utils.h>

#include <necrolog.h>

#include <doctest/doctest.h>

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

std::string hex_dump(const RpcValue::String &out)
{
	std::string ret;
	for (char i : out) {
		char h = i / 16;
		char l = i % 16;
		ret += utils::hexNibble(h);
		ret += utils::hexNibble(l);
	}
	return ret;
}

std::string binary_dump(const RpcValue::String &out)
{
	std::string ret;
	for (size_t i = 0; i < out.size(); ++i) {
		auto u = static_cast<uint8_t>(out[i]);
		if(i > 0)
			ret += '|';
		for (size_t j = 0; j < 8*sizeof(u); ++j) {
			ret += (u & ((static_cast<uint8_t>(128)) >> j))? '1': '0';
		}
	}
	return ret;
}
}

namespace shv::chainpack {

// NOLINTBEGIN(misc-use-internal-linkage)
doctest::String toString(const RpcValue& value) {
	return value.toCpon().c_str();
}
// NOLINTEND(misc-use-internal-linkage)
}

DOCTEST_TEST_CASE("ChainPack")
{
	DOCTEST_SUBCASE("Dump packing schema")
	{
		nDebug() << "============= chainpack binary test ============";
		for (int i = ChainPack::PackingSchema::Null; i <= ChainPack::PackingSchema::CString; ++i) {
			RpcValue::String out;
			out += static_cast<char>(i);
			auto e = static_cast<ChainPack::PackingSchema::Enum>(i);
			std::ostringstream str;
			str << std::setw(3) << i << " " << std::hex << i << " " << binary_dump(out).c_str() << " "  << ChainPack::PackingSchema::name(e);
			nInfo() << str.str();
		}
		for (int i = ChainPack::PackingSchema::FALSE_SCHEMA; i <= ChainPack::PackingSchema::TERM; ++i) {
			RpcValue::String out;
			out += static_cast<char>(i);
			auto e = static_cast<ChainPack::PackingSchema::Enum>(i);
			std::ostringstream str;
			str << std::setw(3) << i << " " << std::hex << i << " " << binary_dump(out).c_str() << " "  << ChainPack::PackingSchema::name(e);
			nInfo() << str.str();
		}
	}
	DOCTEST_SUBCASE("NULL")
	{
		RpcValue cp1{nullptr};
		std::stringstream out;
		{ ChainPackWriter wr(out);  wr.write(cp1); }
		REQUIRE(out.str().size() == 1);
		ChainPackReader rd(out);
		RpcValue cp2 = rd.read();
		nDebug() << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str()).c_str();
		REQUIRE(cp1.type() == cp2.type());
	}
	DOCTEST_SUBCASE("tiny uint")
	{
		for (RpcValue::UInt n = 0; n < 64; ++n) {
			RpcValue cp1{n};
			std::stringstream out;
			{ ChainPackWriter wr(out);  wr.write(cp1); }
			if(n < 10)
				nDebug() << n << " " << cp1.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str()).c_str();
			REQUIRE(out.str().size() == 1);
			ChainPackReader rd(out); RpcValue cp2 = rd.read();
			REQUIRE(cp1.type() == cp2.type());
			REQUIRE(cp1.toUInt() == cp2.toUInt());
		}
	}
	DOCTEST_SUBCASE("uint")
	{
		for (unsigned i = 0; i < sizeof(RpcValue::UInt); ++i) {
			for (unsigned j = 0; j < 3; ++j) {
				RpcValue::UInt n = RpcValue::UInt{1} << (i*8 + j*3+1);
				RpcValue cp1{n};
				std::stringstream out;
				{ ChainPackWriter wr(out);  wr.write(cp1); }
				ChainPackReader rd(out);
				RpcValue cp2 = rd.read();
				nDebug() << n << int_to_hex(n) << "..." << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str()).c_str();
				REQUIRE(cp1.type() == cp2.type());
				REQUIRE(cp1.toUInt() == cp2.toUInt());
			}
		}
	}
	DOCTEST_SUBCASE("tiny int")
	for (RpcValue::Int n = 0; n < 64; ++n) {
		RpcValue cp1{n};
		std::stringstream out;
		{ ChainPackWriter wr(out);  wr.write(cp1); }
		if(out.str().size() < 10)
			nDebug() << n << " " << cp1.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str()).c_str();
		REQUIRE(out.str().size() == 1);
		ChainPackReader rd(out); RpcValue cp2 = rd.read();
		REQUIRE(cp1.type() == cp2.type());
		REQUIRE(cp1.toInt() == cp2.toInt());
	}
	DOCTEST_SUBCASE("int")
	{
		for (int sig = 1; sig >= -1; sig-=2) {
			for (unsigned i = 0; i < sizeof(RpcValue::Int); ++i) {
				for (unsigned j = 0; j < 3; ++j) {
					RpcValue::Int n = sig * (RpcValue::Int{1} << (i*8 + j*2+2));
					RpcValue cp1{n};
					std::stringstream out;
					{ ChainPackWriter wr(out);  wr.write(cp1); }
					ChainPackReader rd(out); RpcValue cp2 = rd.read();
					nDebug() << n << int_to_hex(n) << "..." << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str()).c_str();
					REQUIRE(cp1.type() == cp2.type());
					REQUIRE(cp1.toUInt() == cp2.toUInt());
				}
			}
		}
	}
	DOCTEST_SUBCASE("double")
	{
		{
			auto n_max = 1000000.;
			auto n_min = -1000000.;
			auto step = (n_max - n_min) / 100;
			for (auto n = n_min; n < n_max; n += step) {
				RpcValue cp1{n};
				std::stringstream out;
				{ ChainPackWriter wr(out);  wr.write(cp1); }
				REQUIRE(out.str().size() > 1);
				ChainPackReader rd(out); RpcValue cp2 = rd.read();
				if(n > -3*step && n < 3*step)
					nDebug() << n << " " << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str()).c_str();
				REQUIRE(cp1.type() == cp2.type());
				REQUIRE(cp1.toDouble() == cp2.toDouble());
			}
		}
		{
			double n_max = std::numeric_limits<double>::max();
			double n_min = std::numeric_limits<double>::min();
			double step = -1.23456789e10;
			for (double n = n_min; n < n_max / -step / 10; n *= step) {
				RpcValue cp1{n};
				std::stringstream out;
				{ ChainPackWriter wr(out);  wr.write(cp1); }
				REQUIRE(out.str().size() > 1);
				ChainPackReader rd(out); RpcValue cp2 = rd.read();
				if(n > -100 && n < 100)
					nDebug() << n << " - " << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str()).c_str();
				REQUIRE(cp1.type() == cp2.type());
				REQUIRE(cp1.toDouble() == cp2.toDouble());
			}
		}
	}
	DOCTEST_SUBCASE("Decimal")
	{
		RpcValue::Int mant = 123456789;
		int prec_max = 16;
		int prec_min = -16;
		int step = 1;
		for (int prec = prec_min; prec <= prec_max; prec += step) {
			RpcValue cp1{RpcValue::Decimal(mant, prec)};
			std::stringstream out;
			{ ChainPackWriter wr(out);  wr.write(cp1); }
			REQUIRE(out.str().size() > 1);
			ChainPackReader rd(out); RpcValue cp2 = rd.read();
			nDebug() << mant << prec << " - " << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str()).c_str();
			REQUIRE(cp1.type() == cp2.type());
			REQUIRE(cp1 == cp2);
		}
	}
	DOCTEST_SUBCASE("bool")
	{
		for(bool b : {false, true}) {
			RpcValue cp1{b};
			std::stringstream out;
			{ ChainPackWriter wr(out);  wr.write(cp1); }
			REQUIRE(out.str().size() == 1);
			ChainPackReader rd(out); RpcValue cp2 = rd.read();
			nDebug() << b << " " << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str()).c_str();
			REQUIRE(cp1.type() == cp2.type());
			REQUIRE(cp1.toBool() == cp2.toBool());
		}
	}
	DOCTEST_SUBCASE("Blob")
	{
		std::string s = "blob containing zero character";
		RpcValue::Blob blob{s.begin(), s.end()};
		blob[blob.size() - 9] = 0;
		RpcValue cp1{blob};
		std::stringstream out;
		{
			ChainPackWriter wr(out);
			wr.write(cp1);
		}
		ChainPackReader rd(out);
		RpcValue cp2 = rd.read();
		REQUIRE(cp1.type() == cp2.type());
		REQUIRE(cp1.asBlob() == cp2.asBlob());
	}
	DOCTEST_SUBCASE("string")
	{
		{
			RpcValue::String str{"string containing zero character"};
			str[str.size() - 10] = 0;
			RpcValue cp1{str};
			std::stringstream out;
			{ ChainPackWriter wr(out);  wr.write(cp1); }
			ChainPackReader rd(out); RpcValue cp2 = rd.read();
			nDebug() << str << " " << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str());
			REQUIRE(cp1.type() == cp2.type());
			REQUIRE(cp1.asString() == cp2.asString());
		}
		{
			// long string
			RpcValue::String str;
			for (int i = 0; i < 1000; ++i)
				str += std::to_string(i % 10);
			RpcValue cp1{str};
			std::stringstream out;
			{ ChainPackWriter wr(out);  wr.write(cp1); }
			ChainPackReader rd(out); RpcValue cp2 = rd.read();
			REQUIRE(cp1.type() == cp2.type());
			REQUIRE(cp2.asString().size() == str.size());
			REQUIRE(cp1 == cp2);
		}
	}
	DOCTEST_SUBCASE("DateTime")
	{
		REQUIRE(RpcValue::DateTime::fromUtcString("") == RpcValue::DateTime::fromMSecsSinceEpoch(0));
		REQUIRE(RpcValue::DateTime() == RpcValue::DateTime::fromMSecsSinceEpoch(0));
		REQUIRE(RpcValue::DateTime::fromMSecsSinceEpoch(0) == RpcValue::DateTime::fromMSecsSinceEpoch(0));
		REQUIRE(RpcValue::DateTime::fromMSecsSinceEpoch(1) == RpcValue::DateTime::fromMSecsSinceEpoch(1, 2));
		REQUIRE(!(RpcValue::DateTime() < RpcValue::DateTime()));
		REQUIRE(RpcValue::DateTime::fromMSecsSinceEpoch(1) < RpcValue::DateTime::fromMSecsSinceEpoch(2));
		REQUIRE(RpcValue::DateTime::fromMSecsSinceEpoch(0) == RpcValue::DateTime::fromUtcString("1970-01-01T00:00:00"));
		for(const auto &str : {
			"2018-02-02 0:00:00.001",
			"2018-02-02 01:00:00.001+01",
			"2018-12-02 0:00:00",
			"2018-01-01 0:00:00",
			"2019-01-01 0:00:00",
			"2020-01-01 0:00:00",
			"2021-01-01 0:00:00",
			"2031-01-01 0:00:00",
			"2041-01-01 0:00:00",
			"2041-03-04 0:00:00-1015",
			"2041-03-04 0:00:00.123-1015",
			"1970-01-01 0:00:00",
			"2017-05-03 5:52:03",
			"2017-05-03T15:52:03.923Z",
			"2017-05-03T15:52:31.123+10",
			"2017-05-03T15:52:03Z",
			"2017-05-03T15:52:03.000-0130",
			"2017-05-03T15:52:03.923+00",
		}) {
			RpcValue::DateTime dt = RpcValue::DateTime::fromUtcString(str);
			RpcValue cp1{dt};
			std::stringstream out;
			{ ChainPackWriter wr(out);  wr.write(cp1); }
			std::string pack = out.str();
			ChainPackReader rd(out); RpcValue cp2 = rd.read();
			nDebug() << str << " " << dt.toIsoString().c_str() << " " << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(pack);
			REQUIRE(cp1.type() == cp2.type());
			REQUIRE(cp1.toDateTime() == cp2.toDateTime());
		}
	}
	DOCTEST_SUBCASE("List")
	{
		{
			for(const auto &s : {"[]", "[[]]", R"(["a",123,true,[1,2,3],null])"}) {
				string err;
				RpcValue cp1 = RpcValue::fromCpon(s, &err);
				std::stringstream out;
				{ ChainPackWriter wr(out);  wr.write(cp1); }
				ChainPackReader rd(out); RpcValue cp2 = rd.read();
				nDebug() << s << " " << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str()).c_str();
				REQUIRE(cp1.type() == cp2.type());
				REQUIRE(cp1.asList() == cp2.asList());
			}
		}
		{
			RpcValue cp1{RpcList{1,2,3}};
			std::stringstream out;
			{ ChainPackWriter wr(out);  wr.write(cp1); }
			ChainPackReader rd(out); RpcValue cp2 = rd.read();
			nDebug() << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str()).c_str();
			REQUIRE(cp1.type() == cp2.type());
			REQUIRE(cp1.asList() == cp2.asList());
		}
		{
			static constexpr size_t N = 10;
			std::stringstream out;
			{
				ChainPackWriter wr(out);
				wr.writeContainerBegin(RpcValue::Type::List);
				std::string s("foo-bar");
				for (size_t i = 0; i < N; ++i) {
					wr.writeListElement(RpcValue(s + std::to_string(i)));
				}
				wr.writeContainerEnd();
			}
			ChainPackReader rd(out); RpcValue cp2 = rd.read();
			nDebug() << cp2.toCpon() << " dump: " << binary_dump(out.str()).c_str();
			const RpcList& list = cp2.asList();
			REQUIRE(list.size() == N);
			for (size_t i = 0; i < list.size(); ++i) {
				std::string s("foo-bar");
				REQUIRE(RpcValue(s + std::to_string(i)) == list.at(i));
			}
		}
	}
	DOCTEST_SUBCASE("Map")
	{
		{
			RpcValue cp1{{
					{"foo", RpcList{11,12,13}},
					{"bar", 2},
					{"baz", 3},
						 }};
			std::stringstream out;
			{ ChainPackWriter wr(out);  wr.write(cp1); }
			ChainPackReader rd(out); RpcValue cp2 = rd.read();
			nDebug() << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str());
			REQUIRE(cp1.type() == cp2.type());
			REQUIRE(cp1.asMap() == cp2.asMap());
		}
		{
			RpcValue::Map m{
					{"foo", RpcList{11,12,13}},
					{"bar", 2},
					{"baz", 3},
						 };
			std::stringstream out;
			{
				ChainPackWriter wr(out);
				wr.writeContainerBegin(RpcValue::Type::Map);
				for(const auto &it : m)
					wr.writeMapElement(it.first, it.second);
				wr.writeContainerEnd();
			}
			ChainPackReader rd(out);
			RpcValue::Map m2 = rd.read().asMap();
			for(const auto &it : m2) {
				REQUIRE(it.second == m[it.first]);
			}
		}
		for(const auto &s : {"{}", R"({"a":{}})", R"({"foo":{"bar":"baz"}})"}) {
			string err;
			RpcValue cp1 = RpcValue::fromCpon(s, &err);
			std::stringstream out;
			{ ChainPackWriter wr(out);  wr.write(cp1); }
			ChainPackReader rd(out); RpcValue cp2 = rd.read();
			nDebug() << s << " " << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str()).c_str();
			REQUIRE(cp1.type() == cp2.type());
			REQUIRE(cp1 == cp2);
		}
	}
	DOCTEST_SUBCASE("IMap")
	{
		{
			RpcValue::IMap map {
				{1, "foo"},
				{2, "bar"},
				{333, 15U},
			};
			RpcValue cp1{map};
			std::stringstream out;
			{ ChainPackWriter wr(out);  wr.write(cp1); }
			ChainPackReader rd(out); RpcValue cp2 = rd.read();
			nDebug() << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str());
			REQUIRE(cp1.type() == cp2.type());
			REQUIRE(cp1.asIMap() == cp2.asIMap());
		}
		{
			RpcValue cp1{{
					{127, RpcList{11,12,13}},
					{128, 2},
					{129, 3},
						 }};
			std::stringstream out;
			{ ChainPackWriter wr(out);  wr.write(cp1); }
			ChainPackReader rd(out); RpcValue cp2 = rd.read();
			nDebug() << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str());
			REQUIRE(cp1.type() == cp2.type());
			REQUIRE(cp1.asIMap() == cp2.asIMap());
		}
	}
	DOCTEST_SUBCASE("Meta")
	{
		{
			nDebug() << "------------- Meta1";
			RpcValue cp1{RpcValue::String{"META1"}};
			cp1.setMetaValue(meta::Tag::MetaTypeId, 11);
			std::stringstream out;
			{ ChainPackWriter wr(out);  wr.write(cp1); }
			ChainPackReader rd(out); RpcValue cp2 = rd.read();
			std::ostream::pos_type consumed = out.tellg();
			nDebug() << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " consumed: " << consumed << " dump: " << binary_dump(out.str());
			nDebug() << "hex:" << hex_dump(out.str());
			REQUIRE(out.str().size() == static_cast<size_t>(consumed));
			REQUIRE(cp1.type() == cp2.type());
			REQUIRE(cp1.metaData() == cp2.metaData());
		}
		{
			nDebug() << "------------- Meta2";
			RpcValue cp1{RpcList{"META2", 17, 18, 19}};
			cp1.setMetaValue(meta::Tag::MetaTypeNameSpaceId, 12);
			cp1.setMetaValue(meta::Tag::MetaTypeId, 2);
			cp1.setMetaValue(meta::Tag::USER, "foo");
			cp1.setMetaValue(meta::Tag::USER+1, RpcList{1,2,3});
			cp1.setMetaValue("bar", 3);
			std::stringstream out;
			{ ChainPackWriter wr(out);  wr.write(cp1); }
			ChainPackReader rd(out); RpcValue cp2 = rd.read();
			std::ostream::pos_type consumed = out.tellg();
			nDebug() << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " consumed: " << consumed << " dump: " << binary_dump(out.str());
			nDebug() << "hex:" << hex_dump(out.str());
			REQUIRE(out.str().size() == static_cast<size_t>(consumed));
			REQUIRE(cp1.type() == cp2.type());
			REQUIRE(cp1.metaData() == cp2.metaData());
		}
	}
	DOCTEST_SUBCASE("RpcValue::typeForName")
	{
		REQUIRE(RpcValue::typeForName("Null") == RpcValue::Type::Null);
		REQUIRE(RpcValue::typeForName("UInt") == RpcValue::Type::UInt);
		REQUIRE(RpcValue::typeForName("Int") == RpcValue::Type::Int);
		REQUIRE(RpcValue::typeForName("Double") == RpcValue::Type::Double);
		REQUIRE(RpcValue::typeForName("Bool") == RpcValue::Type::Bool);
		REQUIRE(RpcValue::typeForName("String") == RpcValue::Type::String);
		REQUIRE(RpcValue::typeForName("List") == RpcValue::Type::List);
		REQUIRE(RpcValue::typeForName("Map") == RpcValue::Type::Map);
		REQUIRE(RpcValue::typeForName("IMap") == RpcValue::Type::IMap);
		REQUIRE(RpcValue::typeForName("DateTime") == RpcValue::Type::DateTime);
		REQUIRE(RpcValue::typeForName("Decimal") == RpcValue::Type::Decimal);

		REQUIRE(RpcValue::typeForName("Int bla bla", 3) == RpcValue::Type::Int);
		REQUIRE(RpcValue::typeForName("Int bla bla", 2) == RpcValue::Type::Invalid);
		REQUIRE(RpcValue::typeForName("Int bla bla", 4) == RpcValue::Type::Invalid);
	}

	DOCTEST_SUBCASE("fuzzing tests")
	{
		std::array<uint8_t, 15> input{0x8b, 0x8b, 0x0, 0x8d, 0xf6, 0xff, 0xff, 0xff, 0xff, 0x0, 0x8b, 0x0, 0x8e, 0x0, 0x0};
		REQUIRE_THROWS_WITH_AS(shv::chainpack::RpcValue::fromChainPack({reinterpret_cast<const char*>(input.data()), input.size()}), "Parse error: 4 MALFORMED_INPUT at pos: 15 - DateTime msec value overflow.", ParseException);
		std::array<uint8_t, 24> input2{0x8a, 0x83, 0x89, 0x89, 0x8a, 0x89, 0x8a, 0x8a, 0xff, 0xff, 0xff, 0xff, 0x89, 0x8a, 0x89, 0xec, 0xec, 0xec, 0xec, 0xec, 0xec, 0x8a, 0x8b};
		REQUIRE_THROWS_WITH_AS(shv::chainpack::RpcValue::fromChainPack({reinterpret_cast<const char*>(input2.data()), input2.size()}), "Can't convert NaN to int", std::runtime_error);
	}
}
