#pragma once
#include <shv/broker/groupmapping.h>

#include <string>
#include <vector>

struct AzureConfig {
	std::string clientId;
	std::vector<GroupMapping> groupMapping;
};
