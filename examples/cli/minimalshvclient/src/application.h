#pragma once

#include <QCoreApplication>

namespace shv::chainpack { class RpcMessage; class RpcValue; class RpcError; }
namespace shv::iotqt::rpc { class ClientConnection; class ClientAppCliOptions; }

class Application : public QCoreApplication
{
	Q_OBJECT
private:
	using Super = QCoreApplication;
public:
	Application(int &argc, char **argv, const shv::iotqt::rpc::ClientAppCliOptions &cli_opts);
private:
	void callShvMethod(const std::string &shv_path, const std::string &method, const shv::chainpack::RpcValue &params,
						const QObject *context = nullptr,
						std::function<void(const shv::chainpack::RpcValue &)> success_callback = nullptr,
						std::function<void (const shv::chainpack::RpcError &)> error_callback = nullptr);
	void subscribeChanges(const std::string &shv_path);
	void unsubscribeChanges(const std::string &shv_path);
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);
private:
	shv::iotqt::rpc::ClientConnection *m_rpcConnection = nullptr;
};

