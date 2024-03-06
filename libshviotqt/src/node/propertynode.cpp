#include <shv/iotqt/node/propertynode.h>

namespace shv::iotqt::node {
const std::vector<shv::chainpack::MetaMethod> PROPERTY_NODE_METHODS = {
	{shv::chainpack::Rpc::METH_DIR, shv::chainpack::MetaMethod::Signature::RetParam, shv::chainpack::MetaMethod::Flag::None, shv::chainpack::MetaMethod::AccessLevel::Browse},
	{shv::chainpack::Rpc::METH_LS, shv::chainpack::MetaMethod::Signature::RetParam, shv::chainpack::MetaMethod::Flag::None, shv::chainpack::MetaMethod::AccessLevel::Browse},
	{shv::chainpack::Rpc::METH_GET, shv::chainpack::MetaMethod::Signature::RetVoid, shv::chainpack::MetaMethod::Flag::IsGetter, shv::chainpack::MetaMethod::AccessLevel::Read},
	{shv::chainpack::Rpc::METH_SET, shv::chainpack::MetaMethod::Signature::RetVoid, shv::chainpack::MetaMethod::Flag::IsSetter, shv::chainpack::MetaMethod::AccessLevel::Write},
	{shv::chainpack::Rpc::SIG_VAL_CHANGED, shv::chainpack::MetaMethod::Signature::RetVoid, shv::chainpack::MetaMethod::Flag::IsSignal, shv::chainpack::MetaMethod::AccessLevel::Read},
};
const std::vector<shv::chainpack::MetaMethod> READONLY_PROPERTY_NODE_METHODS = {
	{shv::chainpack::Rpc::METH_DIR, shv::chainpack::MetaMethod::Signature::RetParam, shv::chainpack::MetaMethod::Flag::None, shv::chainpack::MetaMethod::AccessLevel::Browse},
	{shv::chainpack::Rpc::METH_LS, shv::chainpack::MetaMethod::Signature::RetParam, shv::chainpack::MetaMethod::Flag::None, shv::chainpack::MetaMethod::AccessLevel::Browse},
	{shv::chainpack::Rpc::METH_GET, shv::chainpack::MetaMethod::Signature::RetVoid, shv::chainpack::MetaMethod::Flag::IsGetter, shv::chainpack::MetaMethod::AccessLevel::Read},
	{shv::chainpack::Rpc::SIG_VAL_CHANGED, shv::chainpack::MetaMethod::Signature::RetVoid, shv::chainpack::MetaMethod::Flag::IsSignal, shv::chainpack::MetaMethod::AccessLevel::Read},
};
}
