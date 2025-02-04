#include "serverconnection.h"
#include <shv/iotqt/rpc/socketrpcconnection.h>
#include <shv/iotqt/rpc/socket.h>

#include <shv/coreqt/log.h>

#include <shv/core/exception.h>
#include <shv/core/utils.h>

#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/utils.h>

#include <QTcpSocket>
#include <QTimer>
#include <QCryptographicHash>
#include <QHostAddress>

namespace cp = shv::chainpack;

namespace shv::iotqt::rpc {

static const int s_initPhaseTimeout = 10000;

ServerConnection::ServerConnection(Socket *socket, const std::optional<std::string>& azureClientId, QObject *parent)
	: Super(parent)
	, m_azureClientId(azureClientId)
{
	setSocket(socket);
	connect(this, &ServerConnection::socketConnectedChanged, [this](bool is_connected) {
		if(is_connected) {
			m_helloReceived = m_loginReceived = false;
			setConnectionName(peerAddress() + ':' + std::to_string(peerPort()));
		}
	});
	QTimer::singleShot(s_initPhaseTimeout, this, [this]() {
		if(isLoginPhase()) {
			shvWarning() << "Client should login in" << (s_initPhaseTimeout/1000) << "seconds, dropping out connection.";
			abort();
		}
	});
}

ServerConnection::~ServerConnection()
{
	shvInfo() << "Destroying Connection ID:" << connectionId() << "name:" << connectionName();
	abortSocket();
}

const std::string& ServerConnection::connectionName()
{
	return m_connectionName;
}

void ServerConnection::setConnectionName(const std::string &n)
{
	m_connectionName = n;
	setObjectName(QString::fromStdString(n));
}

void ServerConnection::close()
{
	closeSocket();
}

void ServerConnection::abort()
{
	abortSocket();
}

void ServerConnection::unregisterAndDeleteLater()
{
	emit aboutToBeDeleted(connectionId());
	abort();
	deleteLater();
}

const shv::chainpack::RpcValue::Map& ServerConnection::connectionOptions() const
{
	return m_connectionOptions.asMap();
}

const std::string& ServerConnection::userName() const
{
	return m_userLogin.user;
}

bool ServerConnection::isConnectedAndLoggedIn() const
{
	return isSocketConnected() && m_loginOk;
}

bool ServerConnection::isSlaveBrokerConnection() const
{
	return m_connectionOptions.asMap().hasKey(cp::Rpc::KEY_BROKER);
}

void ServerConnection::onRpcFrameReceived(chainpack::RpcFrame &&frame)
{
	if(isLoginPhase()) {
		auto msg = frame.toRpcMessage();
		processLoginPhase(msg);
		return;
	}
	Super::onRpcFrameReceived(std::move(frame));
}

void ServerConnection::onRpcMessageReceived(const chainpack::RpcMessage &msg)
{
	emit rpcMessageReceived(msg);
}

bool ServerConnection::isLoginPhase() const
{
	return !m_loginOk;
}

void ServerConnection::processLoginPhase(const chainpack::RpcMessage &msg)
{
	cp::RpcRequest rq(msg);
	try {
		if(!msg.isRequest())
			SHV_EXCEPTION("Initial message is not RPC request! Dropping client connection. " + connectionName() + " " + msg.toCpon());

		if(!m_helloReceived && !m_loginReceived && rq.method() == shv::chainpack::Rpc::METH_HELLO) {
			shvInfo().nospace() << "Client hello received from: " << socket()->peerAddress().toString().toStdString() << ':' << socket()->peerPort();
			m_helloReceived = true;
			shvInfo() << "sending hello response:" << connectionName();
			m_userLoginContext.serverNounce = std::to_string(std::rand());
			cp::RpcValue::Map params {
				{"nonce", m_userLoginContext.serverNounce},
			};
			if (m_azureClientId.has_value()) {
				params.emplace("azureClientId", m_azureClientId.value());
			}
			sendResponse(rq.requestId(), params);
			return;
		}
		if(m_helloReceived && !m_loginReceived && rq.method() == shv::chainpack::Rpc::METH_LOGIN) {
			shvInfo() << "Client login received";// << profile;// << "device id::" << m.value("deviceId").toStdString();
			m_loginReceived = true;
			m_userLoginContext.loginRequest = msg;
			m_userLoginContext.connectionId = connectionId();
			cp::RpcValue::Map params = rq.params().asMap();
			m_connectionOptions = params.value(cp::Rpc::KEY_OPTIONS);
			processLoginPhase();
			return;
		}
	}
	catch(shv::core::Exception &e) {
		sendError(rq.requestId(), cp::RpcResponse::Error::create(cp::RpcResponse::Error::MethodCallException, e.message()));
	}
	shvError() << "Initial handshake error! Dropping client connection." << connectionName() << msg.toCpon();
	QTimer::singleShot(100, this, &ServerConnection::abort); // need some time to send error to client
}

void ServerConnection::processLoginPhase()
{
	m_userLogin = m_userLoginContext.userLogin();
	shvInfo() << "login - user:" << userName();// << "password:" << password_hash;
}

void ServerConnection::setLoginResult(const chainpack::UserLoginResult &result)
{
	m_loginOk = result.passwordOk;
	auto resp = cp::RpcResponse::forRequest(m_userLoginContext.loginRequest);
	if(result.passwordOk) {
		shvInfo().nospace() << "Client logged in user: " << m_userLogin.user << " from: " << peerAddress() << ':' << peerPort();
		resp.setResult(result.toRpcValue());
	}
	else {
		shvWarning().nospace() << "Invalid authentication for user: " << m_userLogin.user
							<< " reason: " + result.loginError
							<< " at: " + connectionName();
		resp.setError(cp::RpcResponse::Error::createMethodCallExceptionError("Invalid authentication for user: " + m_userLogin.user
																			 + " reason: " + result.loginError
																			 + " at: " + connectionName()));
	}
	sendRpcMessage(resp);
}

}


