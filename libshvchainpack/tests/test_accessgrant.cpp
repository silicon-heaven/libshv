#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <shv/chainpack/accessgrant.h>

#include <necrolog.h>

#include <doctest/doctest.h>


using namespace shv::chainpack;
using namespace std;
using namespace std::string_literals;

DOCTEST_TEST_CASE("Parsing SHV API 2 access string")
{
	auto make_grant = [](const std::string &shv2_access, AccessLevel level, const std::string &access, const std::string &shv2_access_reconstructed) {
		return std::make_tuple(shv2_access, AccessGrant(level, access), shv2_access_reconstructed);
	};
	for (const auto &[access2, acg3, access2_reco] : {
		 make_grant("", AccessLevel::None, {}, ""),
		 make_grant(",", AccessLevel::None, {}, ""),
		 make_grant("rd", AccessLevel::Read, {}, "rd"),
		 make_grant("wr", AccessLevel::Write, {}, "wr"),
		 make_grant("dev", AccessLevel::Devel, {}, "dev"),
		 make_grant("foo,rd", AccessLevel::Read, "foo", "rd,foo"),
		 make_grant("rd,foo", AccessLevel::Read, "foo", "rd,foo"),
		 make_grant("rd,foo,rd", AccessLevel::Read, "foo", "rd,foo"),
		 make_grant("wr,foo,rd", AccessLevel::Write, "foo", "wr,foo"),
		 make_grant("foo,su,bar", AccessLevel::Admin, "foo,bar", "su,foo,bar"),
		 make_grant(",foo,,su,bar,", AccessLevel::Admin, "foo,bar", "su,foo,bar"),
	})
	{
		auto acg2 = AccessGrant::fromShv2Access(access2);
		CAPTURE(access2);
		CAPTURE(acg2.toPrettyString());
		REQUIRE(acg2.accessLevel == acg3.accessLevel);
		REQUIRE(acg2.access == acg3.access);
		auto reco = acg2.toShv2Access();
		REQUIRE(access2_reco == reco);
	}
}



