#include "sampleshvclientapp.h"
#include "appclioptions.h"

#include <shv/iotqt/rpc/rpcresponsecallback.h>
#include <shv/iotqt/rpc/clientconnection.h>
#include <shv/iotqt/node/shvnodetree.h>
#include <shv/iotqt/node/localfsnode.h>
#include <shv/coreqt/log.h>
#include <shv/chainpack/tunnelctl.h>
#include <shv/chainpack/metamethod.h>
#include <shv/chainpack/cponreader.h>

#include <shv/core/stringview.h>

#include <QProcess>
#include <QSocketNotifier>
#include <QTimer>
#include <QtGlobal>
#include <QFileInfo>
#include <QRegularExpression>
#include <QCryptographicHash>

#include <fstream>

#ifdef Q_OS_UNIX
#include <unistd.h>
#include <errno.h>
#include <string.h>
#endif

#ifdef HANDLE_UNIX_SIGNALS
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

using namespace std;

namespace cp = shv::chainpack;
namespace si = shv::iotqt;

static std::vector<cp::MetaMethod> meta_methods {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::ROLE_BROWSE},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::ROLE_BROWSE},
	{cp::Rpc::METH_APP_NAME, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_BROWSE},
	{cp::Rpc::METH_DEVICE_ID, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_BROWSE},
	{cp::Rpc::METH_DEVICE_TYPE, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_BROWSE},
};

size_t AppRootNode::methodCount(const StringViewList &shv_path)
{
	if(shv_path.empty())
		return meta_methods.size();
	return 0;
}

const shv::chainpack::MetaMethod *AppRootNode::metaMethod(const StringViewList &shv_path, size_t ix)
{
	if(shv_path.empty()) {
		if(meta_methods.size() <= ix)
			SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(meta_methods.size()));
		return &(meta_methods[ix]);
	}
	return nullptr;
}

shv::chainpack::RpcValue AppRootNode::callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id)
{
	if(shv_path.empty()) {
		if(method == cp::Rpc::METH_APP_NAME) {
			return QCoreApplication::instance()->applicationName().toStdString();
		}
	}
	return Super::callMethod(shv_path, method, params, user_id);
}

shv::chainpack::RpcValue AppRootNode::processRpcRequest(const shv::chainpack::RpcRequest &rq)
{
	if(rq.shvPath().asString().empty()) {
		if(rq.method() == cp::Rpc::METH_DEVICE_ID) {
			SampleShvClientApp *app = SampleShvClientApp::instance();
			const cp::RpcValue opts = app->rpcConnection()->connectionOptions();
			const cp::RpcValue::Map &dev = opts.asMap().valref(cp::Rpc::KEY_DEVICE).asMap();
			return dev.value(cp::Rpc::KEY_DEVICE_ID).toString();
		}
		if(rq.method() == cp::Rpc::METH_DEVICE_TYPE) {
			return "SampleShvClient";
		}
	}
	return Super::processRpcRequest(rq);
}

#ifdef HANDLE_UNIX_SIGNALS
namespace {
int sig_term_socket_ends[2];
QSocketNotifier *sig_term_socket_notifier = nullptr;
//struct sigaction* old_handlers[sizeof(handled_signals) / sizeof(int)];

void signal_handler(int sig, siginfo_t *siginfo, void *context)
{
	Q_UNUSED(siginfo)
	Q_UNUSED(context)
	shvInfo() << "SIG:" << sig;
	char a = sig;
	::write(sig_term_socket_ends[0], &a, sizeof(a));
}

}

void ShvAgentApp::handleUnixSignal()
{
	sig_term_socket_notifier->setEnabled(false);
	char sig;
	::read(sig_term_socket_ends[1], &sig, sizeof(sig));

	shvInfo() << "SIG" << (int)sig << "catched.";
	emit aboutToTerminate((int)sig);

	sig_term_socket_notifier->setEnabled(true);
	shvInfo() << "Terminating application.";
	quit();
}

void ShvAgentApp::installUnixSignalHandlers()
{
	shvInfo() << "installing Unix signals handlers";
	{
		struct sigaction sig_act;
		memset (&sig_act, '\0', sizeof(sig_act));
		// Use the sa_sigaction field because the handles has two additional parameters
		sig_act.sa_sigaction = &signal_handler;
		// The SA_SIGINFO flag tells sigaction() to use the sa_sigaction field, not sa_handler.
		sig_act.sa_flags = SA_SIGINFO;
		const int handled_signals[] = {SIGTERM, SIGINT, SIGHUP, SIGUSR1, SIGUSR2};
		for(int s : handled_signals)
			if	(sigaction(s, &sig_act, 0) > 0)
				shvError() << "Couldn't register handler for signal:" << s;
	}
	if(::socketpair(AF_UNIX, SOCK_STREAM, 0, sig_term_socket_ends))
		qFatal("Couldn't create SIG_TERM socketpair");
	sig_term_socket_notifier = new QSocketNotifier(sig_term_socket_ends[1], QSocketNotifier::Read, this);
	connect(sig_term_socket_notifier, &QSocketNotifier::activated, this, &ShvAgentApp::handleUnixSignal);
	shvInfo() << "SIG_TERM handler installed OK";
}
#endif

SampleShvClientApp::SampleShvClientApp(int &argc, char **argv, AppCliOptions* cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
{
#ifdef HANDLE_UNIX_SIGNALS
	installUnixSignalHandlers();
#endif
//#ifdef Q_OS_UNIX
//	if(0 != ::setpgid(0, 0))
//		shvError() << "Cannot make shvagent process the group leader, error set process group ID:" << errno << ::strerror(errno);
//#endif

	m_rpcConnection = new si::rpc::ClientConnection(this);
	m_rpcConnection->setCliOptions(cli_opts);

	connect(m_rpcConnection, &si::rpc::ClientConnection::brokerConnectedChanged, this, &SampleShvClientApp::onBrokerConnectedChanged);
	connect(m_rpcConnection, &si::rpc::ClientConnection::rpcMessageReceived, this, &SampleShvClientApp::onRpcMessageReceived);

	AppRootNode *root = new AppRootNode();
	m_shvTree = new si::node::ShvNodeTree(root, this);
	connect(m_shvTree->root(), &si::node::ShvRootNode::sendRpcMessage, m_rpcConnection, &si::rpc::ClientConnection::sendMessage);
	//m_shvTree->mkdir("sys/rproc");

	QTimer::singleShot(0, m_rpcConnection, &si::rpc::ClientConnection::open);
}

SampleShvClientApp::~SampleShvClientApp()
{
	shvInfo() << "destroying shv agent application";
}

SampleShvClientApp *SampleShvClientApp::instance()
{
	return qobject_cast<SampleShvClientApp *>(QCoreApplication::instance());
}

void SampleShvClientApp::onBrokerConnectedChanged(bool is_connected)
{
	m_isBrokerConnected = is_connected;
	if(is_connected) {
		QTimer::singleShot(0, [this]() {
			subscribeChanges();
			testRpcCall();
		});
	}
}

void SampleShvClientApp::onRpcMessageReceived(const shv::chainpack::RpcMessage &msg)
{
	shvLogFuncFrame() << msg.toCpon();
	if(msg.isRequest()) {
		cp::RpcRequest rq(msg);
		shvInfo() << "RPC request received:" << rq.toPrettyString();
		if(m_shvTree->root()) {
			m_shvTree->root()->handleRpcRequest(rq);
		}
	}
	else if(msg.isResponse()) {
		cp::RpcResponse rp(msg);
		shvInfo() << "RPC response received:" << rp.toPrettyString();
	}
	else if(msg.isSignal()) {
		cp::RpcSignal nt(msg);
		shvInfo() << "RPC signal received:" << nt.toPrettyString();
	}
}

static constexpr int RPC_CALLBACK_TIMEOUT = 2000;

void SampleShvClientApp::testRpcCall()
{
	int rq_id = rpcConnection()->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(rpcConnection(), rq_id, this);
	cb->start(RPC_CALLBACK_TIMEOUT, this, [rq_id](const cp::RpcResponse &resp) {
		if(resp.isSuccess())
			shvInfo() << "rqid:" << rq_id << "Got RPC response, result:" << resp.result().toCpon();
		else
			shvError() << "rqid:" << rq_id << "RPC call error:" << resp.errorString();
	});
	cp::RpcRequest rq;
	rq.setRequestId(rq_id);
	rq.setShvPath("test");
	rq.setMethod("ls");
	shvInfo() << "rqid:" << rq_id << "Sending RPC request:" << rq.toCpon();
	rpcConnection()->sendMessage(rq);
}

void SampleShvClientApp::subscribeChanges()
{
	string shv_path = "test";
	string method = shv::chainpack::Rpc::METH_SUBSCRIBE;
	string signal_name = shv::chainpack::Rpc::SIG_VAL_CHANGED;
	cp::RpcValue::Map params = {
		{"method", signal_name},
		{"path", shv_path},
	};

	int rq_id = rpcConnection()->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(rpcConnection(), rq_id, this);
	cb->start(RPC_CALLBACK_TIMEOUT, this, [this, rq_id, shv_path, signal_name](const cp::RpcResponse &resp) {
		if(resp.isSuccess()) {
			shvInfo() << "rqid:" << rq_id << "Signal:" << signal_name << "on SHV path:" << shv_path << "subscribed successfully";
			// generate data change without ret value check
			rpcConnection()->callShvMethod(shv_path + "/someInt", "set", 123);
			QTimer::singleShot(500, [this, shv_path]() {
				rpcConnection()->callShvMethod(shv_path + "/someInt", "set", 321);
			});
		}
		else {
			shvError() << "rqid:" << rq_id << "Signal:" << signal_name << "on SHV path:" << shv_path << "subscribe error:" << resp.errorString();
		}
	});
	shvInfo() << "rqid:" << rq_id << "Subscribing data change on path:" << shv_path;
	rpcConnection()->callMethodSubscribe(rq_id, shv_path, cp::Rpc::SIG_VAL_CHANGED);
}

