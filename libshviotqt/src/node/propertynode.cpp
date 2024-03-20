#include <shv/iotqt/node/propertynode.h>

namespace shv::iotqt::node {
const std::vector<shv::chainpack::MetaMethod> PROPERTY_NODE_METHODS = {
	shv::chainpack::methods::DIR,
	shv::chainpack::methods::LS,
	{shv::chainpack::Rpc::METH_GET, shv::chainpack::MetaMethod::Flag::IsGetter, "", "RpcValue", shv::chainpack::MetaMethod::AccessLevel::Read, {chainpack::MetaMethod::Signal{"chng"}}},
	{shv::chainpack::Rpc::METH_SET, shv::chainpack::MetaMethod::Flag::IsSetter, "RpcValue", "", shv::chainpack::MetaMethod::AccessLevel::Write},
};
const std::vector<shv::chainpack::MetaMethod> READONLY_PROPERTY_NODE_METHODS = {
	shv::chainpack::methods::DIR,
	shv::chainpack::methods::LS,
	{shv::chainpack::Rpc::METH_GET, shv::chainpack::MetaMethod::Flag::IsGetter, "", "RpcValue", shv::chainpack::MetaMethod::AccessLevel::Read},
};
}
