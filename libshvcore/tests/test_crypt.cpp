#include <shv/core/utils/shvpath.h>
#include <shv/core/utils/crypt.h>
#include <necrolog.h>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

using namespace shv::core::utils;
using namespace shv::core;
using namespace std;

DOCTEST_TEST_CASE("Crypt")
{
	{
		shv::core::utils::Crypt crypt;
		std::string s1 = "Hello Dolly";
		std::string s2 = crypt.encrypt(s1);
		std::string s3 = crypt.decrypt(s2);
		nDebug() << s1 << "->" << s2 << "->" << s3;
		REQUIRE(s1 == s3);
	}
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105651
#ifndef _WIN32
	{
		shv::core::utils::Crypt crypt;
		std::string s1 = "Using QtTest library 5.9.1, Qt 5.9.1 (x86_64-little_endian-lp64 shared (dynamic) debug build; by Clang 3.5.0 (tags/RELEASE_350/final))";
		std::string s2 = crypt.encrypt(s1);
		s2.insert(10, "\n")
				.insert(20, "\t")
				.insert(30, "\r\n")
				.insert(40, "\r")
				.insert(50, "\n\n")
				.insert(60, "  \t\t  ");
		std::string s3 = crypt.decrypt(s2);
		nDebug() << s1 << "->" << s2 << "->" << s3;
		REQUIRE(s1 == s3);
	}
	{
		shv::core::utils::Crypt crypt;
		std::string s1 = "H";
		std::string s2 = crypt.encrypt(s1);
		std::string s3 = crypt.decrypt(s2);
		nDebug() << s1 << "->" << s2 << "->" << s3;
		REQUIRE(s1 == s3);
	}
	{
		shv::core::utils::Crypt crypt;
		std::string s1;
		std::string s2 = crypt.encrypt(s1);
		std::string s3 = crypt.decrypt(s2);
		nDebug() << s1 << "->" << s2 << "->" << s3;
		REQUIRE(s1 == s3);
	}
#endif
}
