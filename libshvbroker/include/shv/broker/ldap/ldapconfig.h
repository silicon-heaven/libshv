#pragma once
#include <shv/broker/groupmapping.h>
#include <optional>
#include <string>
#include <vector>

struct LdapConfig {
	std::optional<std::string> brokerUsername;
	std::optional<std::string> brokerPassword;
	std::string hostName;
	std::string searchBaseDN;
	std::vector<std::string> searchAttrs;
	std::vector<GroupMapping> groupMapping;
};
