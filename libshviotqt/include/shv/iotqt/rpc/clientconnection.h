#pragma once

#include <shv/iotqt/rpc/socketrpcconnection.h>

#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/rpcdriver.h>
#include <shv/chainpack/irpcconnection.h>

#include <shv/core/utils.h>
#include <shv/coreqt/utils.h>

#include <QElapsedTimer>
#include <QObject>
#include <QUrl>
#ifdef WITH_SHV_OAUTH2_AZURE
#include <QFuture>
#endif

class QTimer;

namespace shv::iotqt::rpc {

class ClientAppCliOptions;

class LIBSHVIOTQT_EXPORT ClientConnection : public SocketRpcConnection
{
	Q_OBJECT
	using Super = SocketRpcConnection;

public:
	enum class State {NotConnected = 0, Connecting, SocketConnected, BrokerConnected, ConnectionError};

	SHV_FIELD_IMPL(std::string, u, U, ser)
	SHV_FIELD_BOOL_IMPL2(p, P, eerVerify, true)
	SHV_FIELD_IMPL(std::string, p, P, assword)
	SHV_FIELD_IMPL(shv::chainpack::IRpcConnection::LoginType, l, L, oginType)
	SHV_FIELD_IMPL(std::function<void(const std::function<void(const std::string&)>&)>, t, T, okenPasswordCallback)
	SHV_FIELD_IMPL2(shv::chainpack::RpcValue, c, C, onnectionOptions, shv::chainpack::RpcValue::Map())
	SHV_FIELD_IMPL2(int, h, H, eartBeatInterval, 60)
	SHV_FIELD_IMPL2(bool, o, O, auth2Azure, false)


public:
	explicit ClientConnection(const std::string& user_agent, QObject *parent = nullptr);
	// User agent can't be constructed from nullptr.
	explicit ClientConnection(std::nullptr_t) = delete;
	~ClientConnection() Q_DECL_OVERRIDE;

	QUrl connectionUrl() const;
	void setConnectionUrl(const QUrl &url);
	void setConnectionString(const QString &connection_string);
	void setHost(const std::string &host);

	static const char* stateToString(State state);

	virtual void open();
	void close() override;
	void abort() override;

	void setCliOptions(const ClientAppCliOptions *cli_opts);

	void setTunnelOptions(const shv::chainpack::RpcValue &opts);

	// for CPON clients
	void setProtocolType(shv::chainpack::Rpc::ProtocolType protocol_type);

	void setCheckBrokerConnectedInterval(int ms);
	int checkBrokerConnectedInterval() const;

	void onRpcFrameReceived(chainpack::RpcFrame&&) override;
	Q_SIGNAL void rpcFrameReceived();
	Q_SIGNAL void rpcMessageReceived(const shv::chainpack::RpcMessage &msg);

	bool isBrokerConnected() const;
	Q_SIGNAL void authorizeWithBrowser(const QUrl& url);
	Q_SIGNAL void brokerConnectedChanged(bool is_connected);
	Q_SIGNAL void brokerLoginError(const shv::chainpack::RpcError &err);

	State state() const;
	Q_SIGNAL void stateChanged(State state);

	const shv::chainpack::RpcValue::Map &loginResult() const;

	int brokerClientId() const;
	void muteShvPathInLog(const std::string &shv_path, const std::string &method);
	void setRawRpcMessageLog(bool b);
protected:
	bool isShvPathMutedInLog(const std::string &shv_path, const std::string &method) const;
public:
	/// IRpcConnection interface implementation
	void sendRpcMessage(const shv::chainpack::RpcMessage &rpc_msg) override;
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg) override;
	static QUrl connectionUrlFromString(const QString &url_str);
protected:
	void setState(State state);
	void closeOrAbort(bool is_abort);

	void sendHello();
	void sendLogin(const shv::chainpack::RpcValue &server_hello);

	void checkBrokerConnected();
	void whenBrokerConnectedChanged(bool b);
	void checkShvApiVersion();

	void onSocketConnectedChanged(bool is_connected);

	bool isLoginPhase() const;
	void processLoginPhase(const chainpack::RpcMessage &msg);
	void createLoginParams(const shv::chainpack::RpcValue &server_hello, const std::function<void(chainpack::RpcValue)>& params_callback) const;

	struct ConnectionState
	{
		State state = State::NotConnected;
		int helloRequestId = 0;
		int loginRequestId = 0;
		int workflowsRequestId = 0;
		int pingRqId = 0;
		shv::chainpack::RpcValue loginResult;
		bool oAuth2WaitingForUser = false;
		std::optional<std::string> token;
	};
	ConnectionState m_connectionState;
private:
	bool isAutoConnect() const;
	void restartIfAutoConnect();
	const std::string& pingShvPath() const;

	static void tst_connectionUrlFromString();
#ifdef WITH_SHV_OAUTH2_AZURE
	QFuture<std::variant<QFuture<QString>, QFuture<QString>>> doAzureAuth(const QString& client_id, const QString& authorize_url, const QString& token_url, const QSet<QString>& scopes);
#endif
private:
	QUrl m_connectionUrl;
	QTimer *m_checkBrokerConnectedTimer = nullptr;
	int m_checkBrokerConnectedInterval = 0;
	QTimer *m_heartBeatTimer = nullptr;
	struct MutedPath {
		std::string pathPattern;
		std::string methodPattern;
	};
	std::vector<MutedPath> m_mutedShvPathsInLog;
	std::vector<std::tuple<int64_t, QElapsedTimer>> m_responseIdsMutedInLog;
	bool m_rawRpcMessageLog = false;
	std::string m_userAgent;
};

} // namespace shv

