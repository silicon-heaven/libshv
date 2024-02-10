#pragma once

#include <shv/iotqt/node/shvnode.h>

class CurrentClientShvNode : public shv::iotqt::node::MethodsTableNode
{
	Q_OBJECT
	using Super = shv::iotqt::node::MethodsTableNode;
public:
	static constexpr auto NodeId = "currentClient";

	CurrentClientShvNode(shv::iotqt::node::ShvNode *parent = nullptr);

	shv::chainpack::RpcValue callMethodRq(const shv::chainpack::RpcRequest &rq) override;
private:
	std::vector<shv::chainpack::MetaMethod> m_metaMethods;
};
