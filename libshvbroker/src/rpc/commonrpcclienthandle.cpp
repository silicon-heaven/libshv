#include "commonrpcclienthandle.h"

#include <shv/iotqt/node/shvnode.h>
#include <shv/core/utils/shvurl.h>
#include <shv/core/utils/shvpath.h>

#include <shv/coreqt/log.h>

#include <shv/core/stringview.h>
#include <shv/core/exception.h>

#define logSubscriptionsD() nCDebug("Subscr").color(NecroLog::Color::Yellow)
#define logSigResolveD() nCDebug("SigRes").color(NecroLog::Color::Yellow)

namespace shv::broker::rpc {

//=====================================================================
// CommonRpcClientHandle::Subscription
//=====================================================================
CommonRpcClientHandle::Subscription::Subscription(const std::string &local_path, const std::string &subscribed_path, const std::string &m, const std::string& s)
	: subscribedPath(subscribed_path)
	, method(m)
	, source(s)
{
	// remove leading and trailing slash from path
	size_t ix1 = 0;
	while(ix1 < local_path.size()) {
		if(local_path[ix1] == '/')
			ix1++;
		else
			break;
	}
	size_t len = local_path.size();
	while(len > 0) {
		if(local_path[len - 1] == '/')
			len--;
		else
			break;
	}
	localPath = local_path.substr(ix1, len);
}

bool CommonRpcClientHandle::Subscription::cmpSubscribed(const CommonRpcClientHandle::Subscription &o) const
{
	// 2 subscribed paths can have same localPath with service providers
	int i = subscribedPath.compare(o.subscribedPath);
	if(i == 0)
		return method == o.method && source == o.source;
	return false;
}

bool CommonRpcClientHandle::Subscription::match(const shv::core::StringView &shv_path, const shv::core::StringView &shv_method, const std::string_view& shv_source) const
{
	bool path_match = shv::core::utils::ShvPath::startsWithPath(shv_path, localPath);
	if(path_match)
		return (method.empty() || shv_method == method) && (source.empty() || shv_source == source);
	return false;
}

std::string CommonRpcClientHandle::Subscription::toString() const
{
	return localPath + ':' + method;
}
//=====================================================================
// CommonRpcClientHandle
//=====================================================================
CommonRpcClientHandle::CommonRpcClientHandle() = default;

CommonRpcClientHandle::~CommonRpcClientHandle() = default;

unsigned CommonRpcClientHandle::addSubscription(const CommonRpcClientHandle::Subscription &subs)
{
	logSubscriptionsD() << "adding subscription for connection id:" << connectionId()
						<< "local path:" << subs.localPath
						<< "subscribed path:" << subs.subscribedPath << "method:" << subs.method;
	auto it = std::find_if(m_subscriptions.begin(), m_subscriptions.end(),
					 [&subs](const Subscription &s) { return subs.cmpSubscribed(s); });

	if(it == m_subscriptions.end()) {
		logSubscriptionsD() << "new subscription";
		m_subscriptions.push_back(subs);
		return static_cast<unsigned>(m_subscriptions.size() - 1);
	}

	logSubscriptionsD() << "subscription exists:" << "subscribed path:" << it->subscribedPath << "method:" << it->method;
	*it = subs;
	return static_cast<unsigned>(it - m_subscriptions.begin());
}

bool CommonRpcClientHandle::removeSubscription(const CommonRpcClientHandle::Subscription &subs)
{
	logSubscriptionsD() << "request to remove subscription for connection id:" << connectionId()
						<< "local path:" << subs.localPath
						<< "subscribed path:" << subs.subscribedPath << "method:" << subs.method;
	auto it = std::find_if(m_subscriptions.begin(), m_subscriptions.end(),
					 [&subs](const Subscription &s) { return subs.cmpSubscribed(s); });
	if(it == m_subscriptions.end()) {
		logSubscriptionsD() << "subscription not found";
		return false;
	}

	logSubscriptionsD() << "removed subscription local path:" << it->localPath
		<< "subscribed path:" << it->subscribedPath << "method:" << it->method;
	m_subscriptions.erase(it);
	return true;

}

int CommonRpcClientHandle::isSubscribed(const std::string &shv_path, const std::string &method, const std::string& source) const
{
	logSigResolveD() << "connection id:" << connectionId() << "checking if signal:" << shv_path << "method:" << method;
	for (size_t i = 0; i < subscriptionCount(); ++i) {
		const Subscription &subs = subscriptionAt(i);
		logSigResolveD() << "\tchecking local path:" << subs.localPath << "subscribed as:" << subs.subscribedPath << "method:" << subs.method;
		if(subs.match(shv_path, method, source)) {
			logSigResolveD() << "\t\tHIT";
			return static_cast<int>(i);
		}
	}
	return -1;
}

size_t CommonRpcClientHandle::subscriptionCount() const
{
	return m_subscriptions.size();
}

const CommonRpcClientHandle::Subscription& CommonRpcClientHandle::subscriptionAt(size_t ix) const
{
	return m_subscriptions.at(ix);
}

bool CommonRpcClientHandle::rejectNotSubscribedSignal(const std::string &path, const std::string &method, const std::string& source)
{
	logSubscriptionsD() << "unsubscribing rejected signal, shv_path:" << path << "method:" << method << "source:" << source;
	int most_explicit_subs_ix = -1;
	size_t max_path_len = 0;
	shv::core::StringView shv_path(path);
	for (size_t i = 0; i < subscriptionCount(); ++i) {
		const Subscription &subs = subscriptionAt(i);
		if(subs.match(shv_path, method, source)) {
			if(subs.method.empty()) {
				most_explicit_subs_ix = static_cast<int>(i);
				break;
			}
			if(subs.localPath.size() > max_path_len) {
				max_path_len = subs.localPath.size();
				most_explicit_subs_ix = static_cast<int>(i);
			}
		}
	}
	if(most_explicit_subs_ix >= 0) {
		logSubscriptionsD() << "\t found subscription:" << m_subscriptions.at(static_cast<size_t>(most_explicit_subs_ix)).toString();
		m_subscriptions.erase(m_subscriptions.begin() + most_explicit_subs_ix);
		return true;
	}
	logSubscriptionsD() << "\t not found";
	return false;
}

}
