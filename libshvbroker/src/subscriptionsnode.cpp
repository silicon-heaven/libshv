#include "subscriptionsnode.h"

#include "rpc/clientconnectiononbroker.h"

#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/rpc.h>
#include <shv/chainpack/metamethod.h>
#include <shv/core/exception.h>
#include <shv/core/log.h>
#include <shv/core/stringview.h>
#include <shv/core/utils/shvpath.h>

namespace cp = shv::chainpack;

namespace shv::broker {

namespace {
const auto ND_BY_ID = "byId";
const auto ND_BY_PATH = "byPath";

const auto METH_PATH = "path";
const auto METH_METHOD = "method";

const std::vector<cp::MetaMethod> meta_methods1 {
	shv::chainpack::methods::DIR,
	shv::chainpack::methods::LS,
};
const std::vector<cp::MetaMethod> meta_methods2 {
	shv::chainpack::methods::DIR,
	shv::chainpack::methods::LS,
	{METH_PATH, cp::MetaMethod::Flag::IsGetter, "void", "ret"},
	{METH_METHOD, cp::MetaMethod::Flag::IsGetter, "void", "ret"},
};
}

SubscriptionsNode::SubscriptionsNode(rpc::CommonRpcClientHandle *conn, Super *parent)
	: Super("subscriptions", parent)
	, m_client(conn)
{
}

size_t SubscriptionsNode::methodCount(const StringViewList &shv_path)
{
	return (shv_path.size() < 2) ?  meta_methods1.size() : meta_methods2.size();
}

const shv::chainpack::MetaMethod *SubscriptionsNode::metaMethod(const StringViewList &shv_path, size_t ix)
{
	const std::vector<cp::MetaMethod> &mms = (shv_path.size() < 2) ?  meta_methods1 : meta_methods2;
	if(mms.size() <= ix)
		SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(meta_methods1.size()));
	return &(mms[ix]);
}

shv::iotqt::node::ShvNode::StringList SubscriptionsNode::childNames(const StringViewList &shv_path)
{
	shvLogFuncFrame() << shv_path.join('/');
	if(shv_path.empty())
		return shv::iotqt::node::ShvNode::StringList{ND_BY_ID, ND_BY_PATH};
	if(shv_path.size() == 1) {
		if(shv_path[0] == ND_BY_ID) {
			shv::iotqt::node::ShvNode::StringList ret;
			for (size_t i = 0; i < m_client->subscriptionCount(); ++i) {
				ret.push_back(std::to_string(i));
			}
			return ret;
		}
		if(shv_path[0] == ND_BY_PATH) {
			shv::iotqt::node::ShvNode::StringList ret;
			for (size_t i = 0; i < m_client->subscriptionCount(); ++i) {
				const rpc::ClientConnectionOnBroker::Subscription &subs = m_client->subscriptionAt(i);
				ret.push_back(shv::core::utils::ShvPath::SHV_PATH_QUOTE + subs.path + shv::core::utils::ShvPath::SHV_PATH_METHOD_DELIM + subs.method + shv::core::utils::ShvPath::SHV_PATH_QUOTE);
			}
			std::sort(ret.begin(), ret.end());
			return ret;
		}
	}
	return shv::iotqt::node::ShvNode::StringList{};
}

shv::chainpack::RpcValue SubscriptionsNode::callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const chainpack::RpcValue &user_id)
{
	if(shv_path.size() == 2) {
		if(method == METH_PATH || method == METH_METHOD) {
			const rpc::ClientConnectionOnBroker::Subscription *subs = nullptr;
			if(shv_path.at(0) == ND_BY_ID) {
				subs = &m_client->subscriptionAt(std::stoul(std::string{shv_path.at(1)}));
			}
			else if(shv_path.at(0) == ND_BY_PATH) {
				shv::core::StringView path = shv_path.at(1);
				for (size_t i = 0; i < m_client->subscriptionCount(); ++i) {
					const rpc::ClientConnectionOnBroker::Subscription &subs1 = m_client->subscriptionAt(i);
					std::string p = shv::core::utils::ShvPath::SHV_PATH_QUOTE + subs1.path + shv::core::utils::ShvPath::SHV_PATH_METHOD_DELIM + subs1.method + shv::core::utils::ShvPath::SHV_PATH_QUOTE;
					if(path == p) {
						subs = &subs1;
						break;
					}
				}
			}
			if(subs == nullptr)
				SHV_EXCEPTION("Method " + method + " called on invalid path " + shv_path.join('/'));
			if(method == METH_PATH)
				return subs->path;
			if(method == METH_METHOD)
				return subs->method;
		}
	}
	return Super::callMethod(shv_path, method, params, user_id);
}
}
