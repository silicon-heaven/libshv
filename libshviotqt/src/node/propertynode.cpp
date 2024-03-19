#include <shv/iotqt/node/propertynode.h>

namespace shv::iotqt::node {
const std::vector<shv::chainpack::MetaMethod> PROPERTY_NODE_METHODS = {
	{shv::chainpack::Rpc::METH_DIR, shv::chainpack::MetaMethod::Flag::None, "param", "ret", shv::chainpack::MetaMethod::AccessLevel::Browse},
	{shv::chainpack::Rpc::METH_LS, shv::chainpack::MetaMethod::Flag::None, "param", "ret", shv::chainpack::MetaMethod::AccessLevel::Browse},
	{shv::chainpack::Rpc::METH_GET, shv::chainpack::MetaMethod::Flag::IsGetter, "", "RpcValue", shv::chainpack::MetaMethod::AccessLevel::Read, {chainpack::MetaMethod::Signal{"chng"}}},
	{shv::chainpack::Rpc::METH_SET, shv::chainpack::MetaMethod::Flag::IsSetter, "RpcValue", "", shv::chainpack::MetaMethod::AccessLevel::Write},
};
const std::vector<shv::chainpack::MetaMethod> READONLY_PROPERTY_NODE_METHODS = {
	{shv::chainpack::Rpc::METH_DIR, shv::chainpack::MetaMethod::Flag::None, "param", "void", shv::chainpack::MetaMethod::AccessLevel::Browse},
	{shv::chainpack::Rpc::METH_LS, shv::chainpack::MetaMethod::Flag::None, "param", "void", shv::chainpack::MetaMethod::AccessLevel::Browse},
	{shv::chainpack::Rpc::METH_GET, shv::chainpack::MetaMethod::Flag::IsGetter, "", "RpcValue", shv::chainpack::MetaMethod::AccessLevel::Read},
};
}
