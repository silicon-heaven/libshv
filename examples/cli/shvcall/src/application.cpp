#include "application.h"
#include "appclioptions.h"

#include <shv/iotqt/rpc/rpccall.h>
#include <shv/coreqt/log.h>

#include <iostream>

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

void Application::onShvStateChanged()
{
	if (m_rpcConnection->state() == si::rpc::ClientConnection::State::BrokerConnected) {
		cp::RpcValue params;
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
