#pragma once
#include <shv/broker/groupmapping.h>

#include <string>
#include <vector>

struct AzureConfig {
	std::vector<GroupMapping> groupMapping;
};
