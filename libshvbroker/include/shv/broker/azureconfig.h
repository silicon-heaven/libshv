#pragma once
#include <shv/broker/groupmapping.h>

#include <optional>
#include <string>
#include <vector>

struct AzureConfig {
	std::optional<std::string> clientId;
	std::vector<GroupMapping> groupMapping;
};
