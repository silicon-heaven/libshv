#include "appclioptions.h"

AppCliOptions::AppCliOptions()
{
	addOption("path").setType(shv::chainpack::RpcValue::Type::String).setNames("--path").setComment("Shv call path");
	addOption("method").setType(shv::chainpack::RpcValue::Type::String).setNames("--method").setComment("Shv call method");
	addOption("params").setType(shv::chainpack::RpcValue::Type::String).setNames("--params").setComment("Shv call params");
	addOption("isChainPackOutput").setType(shv::chainpack::RpcValue::Type::Bool).setNames("-x", "--chainpack-output").setComment("ChainPack output");
}
