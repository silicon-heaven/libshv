#include "dotbrokernode.h"

#include "brokerappnode.h"
#include "rpc/clientconnectiononbroker.h"

#include <shv/broker/brokerapp.h>

using namespace shv::chainpack;

namespace shv::broker {

class ClientsNode : public shv::iotqt::node::MethodsTableNode
{
	using Super = shv::iotqt::node::MethodsTableNode;
public:
	ClientsNode(shv::iotqt::node::ShvNode *parent = nullptr)
		: Super("clients", &m_metaMethods, parent)
		, m_metaMethods {
			shv::chainpack::methods::DIR,
			shv::chainpack::methods::LS,
		}
	{ }

	StringList childNames(const StringViewList &shv_path) override
	{
		auto sl = Super::childNames(shv_path);
		if(shv_path.empty()) {
			std::vector<int> ids;
			ids.reserve(sl.size());
			for(const auto &s : sl)
				ids.push_back(std::stoi(s));
			std::sort(ids.begin(), ids.end());
			for (size_t i = 0; i < ids.size(); ++i)
				sl[i] = std::to_string(ids[i]);
		}
		return sl;
	}
private:
	std::vector<MetaMethod> m_metaMethods;
};

namespace {
const auto METH_MOUNTS = "mounts";
const auto METH_MOUNTED_CLIENT_INFO = "mountedClientInfo";
}

DotBrokerNode::DotBrokerNode(ShvNode *parent)
	: Super("", &m_metaMethods, parent)
	, m_metaMethods {
		shv::chainpack::methods::DIR,
		shv::chainpack::methods::LS,
		{METH_MOUNTS, shv::chainpack::MetaMethod::Flag::None, "n", "[s]", shv::chainpack::AccessLevel::Admin, {}},
		{METH_MOUNTED_CLIENT_INFO, shv::chainpack::MetaMethod::Flag::None, "s", "{i:clientId,s|n:userName,s|n:mountPoint,{i|n}:subscriptions}|n", shv::chainpack::AccessLevel::Admin, {}},
	}
{
	new BrokerAppNode(this);
	new CurrentClientShvNode(this);
	new ClientsNode(this);

}

chainpack::RpcValue DotBrokerNode::callMethod(const StringViewList &shv_path, const std::string &method, const chainpack::RpcValue &params, const chainpack::RpcValue &user_id)
{
	if (shv_path.empty()) {
		if (method == METH_MOUNTS) {
			auto *app = BrokerApp::instance();
			StringList lst;
			for(int id : app->clientConnectionIds()) {
				auto *conn = app->clientConnectionById(id);
				const auto mp = conn->mountPoint();
				if(!mp.empty())
					lst.push_back(mp);
			}
			std::sort(lst.begin(), lst.end());
			return lst;
		}
		if (method == METH_MOUNTED_CLIENT_INFO) {
			auto mount_point = params.asString();
			auto *app = BrokerApp::instance();
			for (auto id : app->clientConnectionIds()) {
				auto *client = app->clientConnectionById(id);
				if (client->mountPoint() == mount_point) {
					chainpack::RpcValue::Map subscriptions;
					for (size_t ix = 0; ix < client->subscriptionCount(); ++ix) {
						const auto &subs = client->subscriptionAt(ix);
						subscriptions[subs.toString()];
					}
					return chainpack::RpcValue::Map{
						{"clientId", id},
						{"userName", client->userName()},
						{"mountPoint", mount_point},
						{"subscriptions", subscriptions},
					};
				}
			}
			return chainpack::RpcValue(nullptr);
		}
	}
	return Super::callMethod(shv_path, method, params, user_id);
}

}
