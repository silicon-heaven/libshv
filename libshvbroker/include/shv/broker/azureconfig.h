#pragma once
#include <shv/broker/groupmapping.h>

#include <optional>
#include <string>
#include <vector>

struct AzureConfig {
	std::string clientId;
	std::string authorizeUrl;
	std::string tokenUrl;
	std::vector<std::string> scopes;
	std::vector<GroupMapping> groupMapping;
};
