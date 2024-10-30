#include "appnode.h"

#include <shv/broker/brokerapp.h>

namespace shv::broker {
AppNode::AppNode(shv::iotqt::node::ShvNode *parent)
	: Super("", &m_metaMethods, parent)
	, m_metaMethods {
		shv::chainpack::methods::DIR,
		shv::chainpack::methods::LS,
		{shv::chainpack::Rpc::METH_PING, shv::chainpack::MetaMethod::Flag::None, "Null", "Null", shv::chainpack::AccessLevel::Browse, {}, "https://silicon-heaven.github.io/shv-doc/rpcmethods/app.html#appping"},
	}
{
}

shv::chainpack::RpcValue AppNode::callMethod(const StringViewList& shv_path, const std::string& method, const shv::chainpack::RpcValue& params, const chainpack::RpcValue& user_id)
{
	if (shv_path.empty()) {
		if (method == shv::chainpack::Rpc::METH_PING) {
			return chainpack::RpcValue(nullptr);
		}
	}
	return Super::callMethod(shv_path, method, params, user_id);
}
}
