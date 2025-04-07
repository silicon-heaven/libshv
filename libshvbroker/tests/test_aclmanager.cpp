#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <shv/broker/aclmanager.h>

#include <shv/iotqt/acl/acluser.h>
#include <shv/iotqt/node/shvnode.h>

#include <necrolog.h>

#include <doctest/doctest.h>
#include <doctest/trompeloeil.hpp>

namespace {
// https://stackoverflow.com/a/56766138
template <typename T>
constexpr auto type_name()
{
    std::string_view name, prefix, suffix;
#ifdef __clang__
    name = __PRETTY_FUNCTION__;
    prefix = "auto type_name() [T = ";
    suffix = "]";
#elif defined(__GNUC__)
    name = __PRETTY_FUNCTION__;
    prefix = "constexpr auto type_name() [with T = ";
    suffix = "]";
#elif defined(_MSC_VER)
    name = __FUNCSIG__;
    prefix = "auto __cdecl type_name<";
    suffix = ">(void)";
#endif
    name.remove_prefix(prefix.size());
    name.remove_suffix(suffix.size());
    return name;
}
}

// NOLINTBEGIN(misc-use-internal-linkage)
namespace std {
    template <typename Type>
    doctest::String toString(const std::vector<Type>& values) {
        std::ostringstream res;
        res << "std::vector<" << type_name<Type>() << ">{\n";
        for (const auto& value : values) {
			res << "    " << value << ",\n";
        }
        res << "}";
        return res.str().c_str();
    }
}
// NOLINTEND(misc-use-internal-linkage)

class MockAclManager : public trompeloeil::mock_interface<shv::broker::AclManager> {
public:
	using mock_interface::mock_interface;
protected:
	TROMPELOEIL_MAKE_MOCK0(aclMountDeviceIds, std::vector<std::string>());
	TROMPELOEIL_MAKE_MOCK1(aclMountDef, shv::iotqt::acl::AclMountDef(const std::string &device_id));
	TROMPELOEIL_MAKE_MOCK2(aclSetMountDef, void(const std::string &device_id, const shv::iotqt::acl::AclMountDef &md));

	TROMPELOEIL_MAKE_MOCK0(aclUsers, std::vector<std::string>());
	TROMPELOEIL_MAKE_MOCK1(aclUser, shv::iotqt::acl::AclUser(const std::string &user_name));
	TROMPELOEIL_MAKE_MOCK2(aclSetUser, void(const std::string &user_name, const shv::iotqt::acl::AclUser &u));

	TROMPELOEIL_MAKE_MOCK0(aclRoles, std::vector<std::string>());
	TROMPELOEIL_MAKE_MOCK1(aclRole, shv::iotqt::acl::AclRole(const std::string &role_name));
	TROMPELOEIL_MAKE_MOCK2(aclSetRole, void(const std::string &role_name, const std::optional<shv::iotqt::acl::AclRole> &r));

	TROMPELOEIL_MAKE_MOCK0(aclAccessRoles, std::vector<std::string>());
	TROMPELOEIL_MAKE_MOCK1(aclAccessRoleRules, shv::iotqt::acl::AclRoleAccessRules(const std::string &role_name));
	TROMPELOEIL_MAKE_MOCK2(aclSetAccessRoleRules, void(const std::string &role_name, const shv::iotqt::acl::AclRoleAccessRules &rp));

};

#define EXPECT_aclUsers(...) expectations.emplace_back(NAMED_REQUIRE_CALL(*acl, aclUsers()).RETURN(std::vector<std::string>{__VA_ARGS__}))
#define EXPECT_aclUser(user, ...) expectations.emplace_back(NAMED_REQUIRE_CALL(*acl, aclUser(user)).RETURN(shv::iotqt::acl::AclUser(shv::iotqt::acl::AclPassword("foobar", shv::iotqt::acl::AclPassword::Format::Plain), __VA_ARGS__)))
#define EXPECT_aclRole(role, ...) expectations.emplace_back(NAMED_REQUIRE_CALL(*acl, aclRole(role)).RETURN(shv::iotqt::acl::AclRole(std::vector<std::string> __VA_ARGS__)))
#define EXPECT_aclAccessRoles(...) expectations.emplace_back(NAMED_REQUIRE_CALL(*acl, aclAccessRoles()).RETURN(std::vector<std::string> __VA_ARGS__ ))
#define EXPECT_aclAccessRoleRules(role, rules_cpon) expectations.emplace_back(NAMED_REQUIRE_CALL(*acl, aclAccessRoleRules(role)).RETURN(shv::iotqt::acl::AclRoleAccessRules::fromRpcValue(rules_cpon)))

using namespace shv::chainpack::string_literals;

DOCTEST_TEST_CASE("userFlattenRoles")
{
	NecroLog::setTopicsLogThresholds("AclManager:D,AclResolve:D");
	auto acl = std::make_unique<MockAclManager>(nullptr);
	std::vector<std::string> input_roles;
	std::vector<std::string> expected_result;
	std::vector<std::unique_ptr<trompeloeil::expectation>> expectations;

	DOCTEST_SUBCASE("no roles")
	{
		input_roles = {};
		expected_result = {};
	}

	DOCTEST_SUBCASE("one role")
	{
		input_roles = {"no_child_roles"};
		expected_result = {"no_child_roles"};
		EXPECT_aclRole("no_child_roles", {});
	}

	DOCTEST_SUBCASE("multiple roles with no subroles")
	{
		// Three separate roles, the result must be in the original order.
		DOCTEST_SUBCASE("order 1")
		{
			input_roles = {"A", "B", "C"};
			expected_result = {"A", "B", "C"};
			EXPECT_aclRole("A", {});
			EXPECT_aclRole("B", {});
			EXPECT_aclRole("C", {});
		}

		DOCTEST_SUBCASE("order 2")
		{
			input_roles = {"B", "A", "C"};
			expected_result = {"B", "A", "C"};
			EXPECT_aclRole("B", {});
			EXPECT_aclRole("A", {});
			EXPECT_aclRole("C", {});
		}
	}

	DOCTEST_SUBCASE("one role with one subrole")
	{
		// A
		// |
		// B
		input_roles = {"A"};
		expected_result = {"A", "B"};
		EXPECT_aclRole("A", {"B"});
		EXPECT_aclRole("B", {});
	}

	DOCTEST_SUBCASE("more roles with multiple subroles")
	{
		// A   M   V
		// |   |   |
		// B   N   W
		// |      /|\        .
		// C     X Y Z
		input_roles = {"A", "M", "V"};
		expected_result = {"A", "M", "V", "B", "N", "W", "C", "X", "Y", "Z"};
		EXPECT_aclRole("A", {"B"});
		EXPECT_aclRole("M", {"N"});
		EXPECT_aclRole("V", {"W"});
		EXPECT_aclRole("B", {"C"});
		EXPECT_aclRole("N", {});
		EXPECT_aclRole("W", {"X", "Y", "Z"});
		EXPECT_aclRole("C", {});
		EXPECT_aclRole("X", {});
		EXPECT_aclRole("Y", {});
		EXPECT_aclRole("Z", {});
	}

	DOCTEST_SUBCASE("example from the spec")
	{
		// https://github.com/silicon-heaven/libshv/wiki/shv-access-control2
		//   A          B
		//  / \        / \     .
		// C  D       D  G
		//  /  \    /  \       .
		// E    F  E    F

		input_roles = {"A", "B"};
		expected_result = {
			"A", "B",
			"C", "D", "G",
			"E", "F"
		};
		EXPECT_aclRole("A", {"C", "D"});
		EXPECT_aclRole("B", {"D", "G"});
		EXPECT_aclRole("C", {});
		EXPECT_aclRole("D", {"E", "F"});
		EXPECT_aclRole("G", {});
		EXPECT_aclRole("E", {});
		EXPECT_aclRole("F", {});
	}

	DOCTEST_SUBCASE("cyclic role definition")
	{
		// A
		// |
		// B
		// |
		// C
		// |
		// A
		input_roles = {"A"};
		expected_result = {"A", "B", "C"};
		EXPECT_aclRole("A", {"B"});
		EXPECT_aclRole("B", {"C"});
		EXPECT_aclRole("C", {"A"});
	}

	REQUIRE(acl->userFlattenRoles("", input_roles) == expected_result);

}

DOCTEST_TEST_CASE("accessGrantForShvPath")
{
	NecroLog::setTopicsLogThresholds("AclManager:D,AclResolve:D");
	auto acl = std::make_unique<MockAclManager>(nullptr);
	std::string user;
	std::string shv_path;
	std::string method;
	bool is_request_from_master_broker = false;

	bool expected_valid;
	auto expected_access_level = static_cast<shv::chainpack::AccessLevel>(-1); // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)

	std::vector<std::unique_ptr<trompeloeil::expectation>> expectations;

	DOCTEST_SUBCASE("no users")
	{
		user = "invalid";
		shv_path = "shv/test";
		method = "ls";
		expected_valid = false;

		EXPECT_aclUsers();
	}

	DOCTEST_SUBCASE("user exists")
	{
		user = "user_name";
		shv_path = "shv/test";
		method = "ls";
		EXPECT_aclUsers(user);

		DOCTEST_SUBCASE("no roles")
		{
			expected_valid = false;
			EXPECT_aclUser(user, {});
		}

		DOCTEST_SUBCASE("rule matching")
		{
			EXPECT_aclUser(user, {"my_role"});
			EXPECT_aclRole("my_role", {}); // No subroles.
			EXPECT_aclAccessRoles({"my_role"});

			DOCTEST_SUBCASE("no rules")
			{
				expected_valid = false;
				EXPECT_aclAccessRoleRules("my_role", {});
			}

			DOCTEST_SUBCASE("one matching rule")
			{
				expected_valid = true;
				expected_access_level = shv::chainpack::AccessLevel::Admin;
				EXPECT_aclAccessRoleRules("my_role", R"([
					{"method":"", "pathPattern":"**", "role":"su"}
				])"_cpon);
			}

			DOCTEST_SUBCASE("one non-matching rule")
			{
				expected_valid = false;
				EXPECT_aclAccessRoleRules("my_role", R"([
					{"method":"", "pathPattern":"shv/test/subpath", "role":"su"}
				])"_cpon);
			}

			DOCTEST_SUBCASE("two matching rules, the first takes precedence")
			{
				expected_valid = true;

				DOCTEST_SUBCASE("matches with the wildcard")
				{
					expected_access_level = shv::chainpack::AccessLevel::Admin;
					EXPECT_aclAccessRoleRules("my_role", R"([
						{"method":"", "pathPattern":"shv/**", "role":"su"},
						{"method":"", "pathPattern":"shv/test", "role":"rd"}
					])"_cpon);
				}

				DOCTEST_SUBCASE("matches with the path")
				{
					expected_access_level = shv::chainpack::AccessLevel::Read;
					EXPECT_aclAccessRoleRules("my_role", R"([
						{"method":"", "pathPattern":"shv/test", "role":"rd"}
						{"method":"", "pathPattern":"shv/**", "role":"su"},
					])"_cpon);
				}
			}

			DOCTEST_SUBCASE("two rules, only one matches")
			{
				expected_valid = true;

				DOCTEST_SUBCASE("the first one")
				{
					expected_access_level = shv::chainpack::AccessLevel::Admin;
					EXPECT_aclAccessRoleRules("my_role", R"([
						{"method":"", "pathPattern":"shv/**", "role":"su"},
						{"method":"", "pathPattern":"shv/test/subpath", "role":"rd"}
					])"_cpon);
				}
				DOCTEST_SUBCASE("the second one")
				{
					expected_access_level = shv::chainpack::AccessLevel::Admin;
					EXPECT_aclAccessRoleRules("my_role", R"([
						{"method":"", "pathPattern":"shv/test/subpath", "role":"rd"}
						{"method":"", "pathPattern":"shv/**", "role":"su"},
					])"_cpon);
				}
			}
		}
	}

	auto acg = acl->accessGrantForShvPath(user, shv_path, method, is_request_from_master_broker, {});
	REQUIRE((acg.accessLevel > shv::chainpack::AccessLevel::None) == expected_valid);
	if (expected_valid) {
		REQUIRE(acg.accessLevel == expected_access_level);
	}
}
