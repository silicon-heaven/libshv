#pragma once

#include <shv/iotqt/node/shvnode.h>
#include <shv/chainpack/rpcvalue.h>

namespace shv::broker {

class AppNode : public shv::iotqt::node::MethodsTableNode
{
	Q_OBJECT

	using Super = shv::iotqt::node::MethodsTableNode;
public:
	AppNode(shv::iotqt::node::ShvNode *parent = nullptr);

	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id) override;
private:
	std::vector<shv::chainpack::MetaMethod> m_metaMethods;
};
}
