#include "application.h"
#include "appclioptions.h"

#include <shv/iotqt/rpc/rpccall.h>
#include <shv/coreqt/log.h>

#include <iostream>
#include <regex>

namespace cp = shv::chainpack;
namespace si = shv::iotqt;

Application::Application(int &argc, char **argv, AppCliOptions *cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
	, m_status(EXIT_SUCCESS)
{
	m_rpcConnection = new si::rpc::ClientConnection(this);

	m_rpcConnection->setCliOptions(cli_opts);

	connect(m_rpcConnection, &si::rpc::ClientConnection::stateChanged, this, &Application::onShvStateChanged);
	shvInfo() << "Connecting to SHV Broker";
	m_rpcConnection->open();
}

namespace placeholders {
#define PLACEHOLDER_REGEX(placeholder_str) const auto placeholder_str = std::regex("\\{" #placeholder_str "\\}")
PLACEHOLDER_REGEX(TIME);
PLACEHOLDER_REGEX(PATH);
PLACEHOLDER_REGEX(METHOD);
PLACEHOLDER_REGEX(VALUE);
}

void Application::onShvStateChanged()
{
	if (m_rpcConnection->state() == si::rpc::ClientConnection::State::BrokerConnected) {
		cp::RpcValue params;
		if (m_cliOptions->shouldSubscribe()) {
			auto sub_id = m_rpcConnection->callMethodSubscribe(m_cliOptions->path(), m_cliOptions->method());
			connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, this, [this, sub_id] (const shv::chainpack::RpcMessage& msg)  {
				if (msg.isResponse()) {
					auto log_error = [this] (const auto& error_msg) {
						shvError() << "Couldn't subscribe to:" << m_cliOptions->path() << m_cliOptions->method() << "msg:" << error_msg;
						exit(1);
					};
					auto response = shv::chainpack::RpcResponse(msg);
					if (response.requestId() != sub_id) {
						return;
					}

					if (response.isError()) {
						log_error(response.error().toString());
						return;
					}

					if (response.result().toBool() != true) {
						log_error("Unexpected error.");
						return;
					}

					shvDebug() << "Subscription successful.";
					return;
				}

				if (msg.isSignal()) {
					const auto signal = shv::chainpack::RpcSignal(msg);
					auto res = m_cliOptions->subscribeFormat();
					const auto do_replace = [&res] (const std::regex& regex, const auto& replacement_fn) { // clazy:exclude=lambda-in-connect - https://bugs.kde.org/show_bug.cgi?id=443342
						std::smatch result;
						if (!std::regex_search(res, result, regex)) {
							return;
						}

						const auto replacement = replacement_fn();
						for (const auto& match : result) {
							res.replace(match.first, match.second, replacement);
						}
					};
					do_replace(placeholders::TIME, [] { return shv::chainpack::RpcValue::DateTime::now().toIsoString(); });
					do_replace(placeholders::PATH, [&signal] { return signal.shvPath().asString(); });
					do_replace(placeholders::METHOD, [&signal] { return signal.method().asString(); });
					do_replace(placeholders::VALUE, [&signal] { return signal.params().toCpon(); });
					std::cout << res << '\n';
					return;
				}
			});
			return;
		}

		try {
			params = m_cliOptions->params().empty() ? cp::RpcValue() : cp::RpcValue::fromCpon(m_cliOptions->params());
		} catch (std::exception&) {
			shvError() << "Couldn't parse params from the command line:" << m_cliOptions->params();
			m_rpcConnection->close();
			return;
		}

		shvInfo() << "SHV Broker connected";
		si::rpc::RpcCall *call = si::rpc::RpcCall::create(m_rpcConnection)
								 ->setShvPath(QString::fromStdString(m_cliOptions->path()))
								 ->setMethod(QString::fromStdString(m_cliOptions->method()))
								 ->setParams(params);
		connect(call, &si::rpc::RpcCall::maybeResult, this, [this](const ::shv::chainpack::RpcValue &result, const shv::chainpack::RpcError &error) {
			if (!error.isValid()) {
				if(m_cliOptions->isChainPackOutput()) {
					std::cout << result.toChainPack();
				}
				else if (m_cliOptions->isCponOutput()) {
					std::cout << cp::RpcValue::fromCpon(result.toString()).toCpon("\t") << "\n";
				}
				else {
					std::cout << result.toCpon("\t") << "\n";
				}
				m_rpcConnection->close();
			}
			else {
				shvInfo() << error.toString();
				m_status = EXIT_FAILURE;
				m_rpcConnection->close();
			}
		});
		call->start();
	}
	else if (m_rpcConnection->state() == shv::iotqt::rpc::ClientConnection::State::NotConnected) {
		shvInfo() << "SHV Broker disconnected";
		exit(m_status);
	}
}
