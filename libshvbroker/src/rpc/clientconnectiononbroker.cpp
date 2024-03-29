#include "clientconnectiononbroker.h"

#include <shv/broker/brokerapp.h>

#include <shv/chainpack/cponwriter.h>
#include <shv/chainpack/accessgrant.h>
#include <shv/chainpack/rpc.h>
#include <shv/coreqt/log.h>
#include <shv/core/stringview.h>
#include <shv/core/utils/shvpath.h>
#include <shv/iotqt/rpc/socket.h>

#include <QCryptographicHash>
#include <QTcpSocket>
#include <QTimer>

#define logSubscriptionsD() nCDebug("Subscr").color(NecroLog::Color::Yellow)
#define logSubsResolveD() nCDebug("SubsRes").color(NecroLog::Color::LightGreen)

#define ACCESS_EXCEPTION(msg) SHV_EXCEPTION_V(msg, "Access")

namespace cp = shv::chainpack;

using namespace std;

namespace shv::broker::rpc {

ClientConnectionOnBroker::ClientConnectionOnBroker(shv::iotqt::rpc::Socket *socket, QObject *parent)
	: Super(socket, parent)
{
	shvDebug() << __FUNCTION__;
	connect(this, &ClientConnectionOnBroker::socketConnectedChanged, this, &ClientConnectionOnBroker::onSocketConnectedChanged);
}

ClientConnectionOnBroker::~ClientConnectionOnBroker()
{
	shvDebug() << __FUNCTION__;
	// disconnect ClientConnectionOnBroker::onSocketConnectedChanged()
	// this should not be called from destructor and can cause app crash
	disconnect(this, nullptr, this, nullptr);
}

void ClientConnectionOnBroker::onSocketConnectedChanged(bool is_connected)
{
	if(!is_connected) {
		shvInfo() << "Socket disconnected, deleting connection:" << connectionId();
		unregisterAndDeleteLater();
	}
}

shv::chainpack::RpcValue ClientConnectionOnBroker::tunnelOptions() const
{
	return connectionOptions().value(cp::Rpc::KEY_TUNNEL);
}

shv::chainpack::RpcValue ClientConnectionOnBroker::deviceOptions() const
{
	return connectionOptions().value(cp::Rpc::KEY_DEVICE);
}

shv::chainpack::RpcValue ClientConnectionOnBroker::deviceId() const
{
	return deviceOptions().asMap().value(cp::Rpc::KEY_DEVICE_ID);
}

void ClientConnectionOnBroker::setMountPoint(const std::string &mp)
{
	m_mountPoint = mp;
}

const std::string& ClientConnectionOnBroker::mountPoint() const
{
	return m_mountPoint;
}

int ClientConnectionOnBroker::idleTime() const
{
	if(!m_idleWatchDogTimer || !m_idleWatchDogTimer->isActive())
		return -1;
	int t = m_idleWatchDogTimer->interval() - m_idleWatchDogTimer->remainingTime();
	if(t < 0)
		t = 0;
	return t;
}

int ClientConnectionOnBroker::idleTimeMax() const
{
	if(!m_idleWatchDogTimer || !m_idleWatchDogTimer->isActive())
		return -1;
	return  m_idleWatchDogTimer->interval();
}

void ClientConnectionOnBroker::setIdleWatchDogTimeOut(int sec)
{
	if(sec == 0) {
		static constexpr int MAX_IDLE_TIME_SEC = 10 * 60 * 60;
		shvInfo() << "connection ID:" << connectionId() << "Cannot switch idle watch dog timeout OFF entirely, the inactive connections can last forever then, setting to max time:" << MAX_IDLE_TIME_SEC/60 << "min";
		sec = MAX_IDLE_TIME_SEC;
	}
	if(!m_idleWatchDogTimer) {
		m_idleWatchDogTimer = new QTimer(this);
		connect(m_idleWatchDogTimer, &QTimer::timeout, this, [this]() {
			std::string mount_point = mountPoint();
			shvError() << "Connection id:" << connectionId() << "device id:" << deviceId().toCpon() << "mount point:" << mount_point
					   << "was idle for more than" << m_idleWatchDogTimer->interval()/1000 << "sec. It will be aborted.";
			unregisterAndDeleteLater();
		});
	}
	shvInfo() << "connection ID:" << connectionId() << "setting idle watch dog timeout to" << sec << "seconds";
	m_idleWatchDogTimer->start(sec * 1000);
}

void ClientConnectionOnBroker::sendRpcMessage(const shv::chainpack::RpcMessage &rpc_msg)
{
	logRpcMsg() << SND_LOG_ARROW
				<< "client id:" << connectionId()
				<< rpc_msg.toPrettyString();
	chainpack::RpcDriver::sendRpcMessage(rpc_msg);
}

void ClientConnectionOnBroker::sendRpcFrame(chainpack::RpcFrame &&frame)
{
	logRpcMsg() << SND_LOG_ARROW
				<< "client id:" << connectionId()
				<< RpcDriver::frameToPrettyCpon(frame);
	chainpack::RpcDriver::sendRpcFrame(std::move(frame));
}

ClientConnectionOnBroker::Subscription ClientConnectionOnBroker::createSubscription(const std::string &shv_path, const std::string &method, const std::string& source)
{
	logSubscriptionsD() << "Create client subscription for path:" << shv_path << "method:" << method << "source:" << source;
	auto acg = BrokerApp::instance()->aclManager()->accessGrantForShvPath(loggedUserName(), shv_path, method, isMasterBrokerConnection(), {});
	if(acg.accessLevel < cp::AccessLevel::Read)
		ACCESS_EXCEPTION("Acces to shv signal '" + shv_path + '/' + method + "()' not granted for user '" + loggedUserName() + "'");
	return Subscription(shv_path, method, source);
}

string ClientConnectionOnBroker::toSubscribedPath(const string &signal_path) const
{
	return signal_path;
}

void ClientConnectionOnBroker::onRpcFrameReceived(chainpack::RpcFrame &&frame)
{
	logRpcMsg() << RCV_LOG_ARROW
				<< "client id:" << connectionId()
				<< RpcDriver::frameToPrettyCpon(frame);
	try {
		if(isLoginPhase()) {
			Super::onRpcFrameReceived(std::move(frame));
			return;
		}
		if(m_idleWatchDogTimer)
			m_idleWatchDogTimer->start();
		BrokerApp::instance()->onRpcFrameReceived(connectionId(), std::move(frame));
	}
	catch (std::exception &e) {
		shvError() << e.what();
	}
}

void ClientConnectionOnBroker::processLoginPhase()
{
	const shv::chainpack::RpcValue::Map &opts = connectionOptions();
	auto t = opts.value(cp::Rpc::OPT_IDLE_WD_TIMEOUT, 3 * 60).toInt();
	setIdleWatchDogTimeOut(t);
	if(tunnelOptions().isMap()) {
		const std::string secret = tunnelOptions().asMap().valref(cp::Rpc::KEY_SECRET).asString();
		cp::UserLoginResult result;
		result.passwordOk = checkTunnelSecret(secret);
		setLoginResult(result);
		return;
	}
	Super::processLoginPhase();
	BrokerApp::instance()->checkLogin(m_userLoginContext, this, [this] (auto result) {
		if (result.userNameOverride) {
			m_userLogin.user = result.userNameOverride.value();
		}
		setLoginResult(result);
	});
}

void ClientConnectionOnBroker::setLoginResult(const chainpack::UserLoginResult &result)
{
	auto login_result = result;
	login_result.clientId = connectionId();
	Super::setLoginResult(login_result);
	if(login_result.passwordOk) {
		BrokerApp::instance()->onClientLogin(connectionId());
	}
	else {
		// take some time to send error message and close connection
		QTimer::singleShot(1000, this, &ClientConnectionOnBroker::close);
	}
}

bool ClientConnectionOnBroker::checkTunnelSecret(const std::string &s)
{
	return BrokerApp::instance()->checkTunnelSecret(s);
}

QVector<int> ClientConnectionOnBroker::callerIdsToList(const shv::chainpack::RpcValue &caller_ids)
{
	QVector<int> res;
	if(caller_ids.isList()) {
		for (const cp::RpcValue &list_item : caller_ids.asList()) {
			res << list_item.toInt();
		}
	}
	else if(caller_ids.isInt() || caller_ids.isUInt()) {
		res << caller_ids.toInt();
	}
	std::sort(res.begin(), res.end());
	return res;
}

void ClientConnectionOnBroker::propagateSubscriptionToSlaveBroker(const CommonRpcClientHandle::Subscription &subs)
{
	if(!isSlaveBrokerConnection())
		return;
	const std::string &mount_point = mountPoint();
	if(shv::core::utils::ShvPath(subs.path).startsWithPath(mount_point)) {
		std::string slave_path = subs.path.substr(mount_point.size());
		if(!slave_path.empty()) {
			if(slave_path[0] != '/')
				return;
			slave_path = slave_path.substr(1);
		}
		logSubscriptionsD() << "Propagating client subscription for local path:" << subs.path << "method:" << subs.method << "to slave broker as:" << slave_path;
		callMethodSubscribe(slave_path, subs.method);
		return;
	}
	if(shv::core::utils::ShvPath(mount_point).startsWithPath(subs.path)
			&& (mount_point.size() == subs.path.size() || mount_point[subs.path.size()] == '/')
	) {
		logSubscriptionsD() << "Propagating client subscription for local path:" << subs.path << "method:" << subs.method << "to slave broker as: ''";
		callMethodSubscribe(std::string(), subs.method);
		return;
	}
}

int ClientConnectionOnBroker::connectionId() const
{
	return Super::connectionId();
}

bool ClientConnectionOnBroker::isConnectedAndLoggedIn() const
{
	return Super::isConnectedAndLoggedIn();
}

bool ClientConnectionOnBroker::isSlaveBrokerConnection() const
{
	return Super::isSlaveBrokerConnection();
}

bool ClientConnectionOnBroker::isMasterBrokerConnection() const
{
	return false;
}

std::string ClientConnectionOnBroker::loggedUserName()
{
	return Super::userName();
}
}
