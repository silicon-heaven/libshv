#include "appclioptions.h"

AppCliOptions::AppCliOptions()
{
	addOption("path").setType(shv::chainpack::RpcValue::Type::String).setNames("--path").setComment("Shv call path");
	addOption("method").setType(shv::chainpack::RpcValue::Type::String).setNames("--method").setComment("Shv call method");
	addOption("params").setType(shv::chainpack::RpcValue::Type::String).setNames("--params").setComment("Shv call params");
	addOption("isCponOutput").setType(shv::chainpack::RpcValue::Type::Bool).setNames("--cpon").setComment("Parse cpon output");
	addOption("isChainPackOutput").setType(shv::chainpack::RpcValue::Type::Bool).setNames("-x", "--chainpack-output").setComment("ChainPack output");
	addOption("shouldSubscribe").setType(shv::chainpack::RpcValue::Type::Bool).setNames("--subscribe").setComment("Whether shvcall should subscribe to a path and print events").setDefaultValue(false);
	addOption("subscribeFormat").setType(shv::chainpack::RpcValue::Type::String).setNames("--subscribe-format").setComment("Set the format of events. Available placeholders: {TIME}, {PATH}, {METHOD}, {VALUE_CPON}. This option is only useful with --subscribe.").setDefaultValue(R"({"timestamp": "{TIME}", "path": "{PATH}", "method": "{METHOD}", "param": {VALUE}})");
}
