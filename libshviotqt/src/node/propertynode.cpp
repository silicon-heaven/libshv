#include <shv/iotqt/node/propertynode.h>

namespace shv::iotqt::node {
const std::vector<shv::chainpack::MetaMethod> PROPERTY_NODE_METHODS = {
	shv::chainpack::methods::DIR,
	shv::chainpack::methods::LS,
	{shv::chainpack::Rpc::METH_GET, shv::chainpack::MetaMethod::Flag::IsGetter, {}, "RpcValue", shv::chainpack::AccessLevel::Read, {chainpack::MetaMethod::Signal{chainpack::Rpc::SIG_VAL_CHANGED}}},
	{shv::chainpack::Rpc::METH_SET, shv::chainpack::MetaMethod::Flag::IsSetter, "RpcValue", {}, shv::chainpack::AccessLevel::Write},
};
const std::vector<shv::chainpack::MetaMethod> READONLY_PROPERTY_NODE_METHODS = {
	shv::chainpack::methods::DIR,
	shv::chainpack::methods::LS,
	{shv::chainpack::Rpc::METH_GET, shv::chainpack::MetaMethod::Flag::IsGetter, {}, "RpcValue", shv::chainpack::AccessLevel::Read, {chainpack::MetaMethod::Signal{chainpack::Rpc::SIG_VAL_CHANGED}}},
};
}
