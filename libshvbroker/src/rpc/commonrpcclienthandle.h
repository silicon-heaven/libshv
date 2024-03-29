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
		std::string path;
		std::string method;
		std::string source;

		Subscription() = default;
		Subscription(const std::string &path_, const std::string &method_, const std::string& source_);

		bool cmpSubscribed(const CommonRpcClientHandle::Subscription &o) const;
		bool match(const std::string_view &shv_path, const std::string_view &shv_method, const std::string_view& source) const;
		std::string toString() const;
	};
public:
	CommonRpcClientHandle();
	virtual ~CommonRpcClientHandle();

	virtual int connectionId() const = 0;
	virtual bool isConnectedAndLoggedIn() const = 0;

	virtual Subscription createSubscription(const std::string &shv_path, const std::string &method, const std::string& source) = 0;
	unsigned addSubscription(const Subscription &subs);
	bool removeSubscription(const Subscription &subs);
	int isSubscribed(const std::string &shv_path, const std::string &method, const std::string& source) const;
	virtual std::string toSubscribedPath(const std::string &abs_path) const = 0;
	size_t subscriptionCount() const;
	const Subscription& subscriptionAt(size_t ix) const;
	bool rejectNotSubscribedSignal(const std::string &path, const std::string &method, const std::string& source);

	virtual std::string loggedUserName() = 0;
	virtual bool isSlaveBrokerConnection() const = 0;
	virtual bool isMasterBrokerConnection() const = 0;

	virtual void sendRpcFrame(chainpack::RpcFrame &&frame) = 0;
	virtual void sendRpcMessage(const shv::chainpack::RpcMessage &rpc_msg) = 0;
protected:
	std::vector<Subscription> m_subscriptions;
};

}}}

