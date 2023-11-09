#pragma once

#include <shv/iotqt/node/shvnode.h>

namespace shv {
namespace broker {

class ClientConnectionNode : public shv::iotqt::node::MethodsTableNode
{
	Q_OBJECT
	using Super = shv::iotqt::node::MethodsTableNode;
public:
	ClientConnectionNode(int client_id, shv::iotqt::node::ShvNode *parent = nullptr);

	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id) override;
private:
	int m_clientId = 0;
};

}}
