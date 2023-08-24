#pragma once

#include <shv/chainpack/rpcmessage.h>

namespace shv {
namespace broker {
namespace rpc {

class CommonRpcClientHandle
{
public:
	struct Subscription
	{
		std::string localPath;
		std::string subscribedPath;
		std::string method;

		Subscription() = default;
		Subscription(const std::string &local_path, const std::string &subscribed_path, const std::string &m);

		bool cmpSubscribed(const CommonRpcClientHandle::Subscription &o) const;
		bool match(const std::string_view &shv_path, const std::string_view &shv_method) const;
		std::string toString() const;
	};
public:
	CommonRpcClientHandle();
	virtual ~CommonRpcClientHandle();

	virtual int connectionId() const = 0;
	virtual bool isConnectedAndLoggedIn() const = 0;

	virtual Subscription createSubscription(const std::string &shv_path, const std::string &method) = 0;
	unsigned addSubscription(const Subscription &subs);
	bool removeSubscription(const Subscription &subs);
	int isSubscribed(const std::string &shv_path, const std::string &method) const;
	virtual std::string toSubscribedPath(const Subscription &subs, const std::string &abs_path) const = 0;
	size_t subscriptionCount() const;
	const Subscription& subscriptionAt(size_t ix) const;
	bool rejectNotSubscribedSignal(const std::string &path, const std::string &method);

	virtual std::string loggedUserName() = 0;
	virtual bool isSlaveBrokerConnection() const = 0;
	virtual bool isMasterBrokerConnection() const = 0;

	virtual void sendRawData(const shv::chainpack::RpcValue::MetaData &meta_data, std::string &&data) = 0;
	virtual void sendMessage(const shv::chainpack::RpcMessage &rpc_msg) = 0;
protected:
	std::vector<Subscription> m_subscriptions;
};

}}}

