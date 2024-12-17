#include <shv/chainpack/rpcdriver.h>
#include <shv/chainpack/exception.h>
#include <shv/chainpack/cponwriter.h>
#include <shv/chainpack/cponreader.h>
#include <shv/chainpack/chainpackwriter.h>
#include <shv/chainpack/chainpackreader.h>
#include <shv/chainpack/utils.h>

#include <necrolog.h>

#define logRpcRawMsg() nCMessage("RpcRawMsg")
#define logRpcData() nCMessage("RpcData")
#define logRpcDataW() nCWarning("RpcData")
#define logWriteQueue() nCMessage("WriteQueue")
#define logWriteQueueW() nCWarning("WriteQueue")

using namespace std;

namespace shv::chainpack {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
int RpcDriver::s_defaultRpcTimeoutMsec = 5000;

RpcDriver::RpcDriver() = default;

RpcDriver::~RpcDriver() = default;

void RpcDriver::sendRpcMessage(const RpcMessage &msg)
{
	RpcDriver::sendRpcFrame(msg.toRpcFrame(m_clientProtocolType));
}

void RpcDriver::sendRpcFrame(RpcFrame &&frame)
{
	logRpcRawMsg() << Rpc::SND_LOG_ARROW
				   << "protocol:"  << Rpc::protocolTypeToString(frame.protocol)
				   << "send raw meta + data: " << frame.meta.toPrettyString()
				   << Utils::toHex(frame.data, 0, 250);
	try {
		if (frame.protocol != m_clientProtocolType) {
			// convert chainpack to cpon if client needs it
			// clients communicating with Cpon are deprecated
			// and support will be ended in 2024
			std::string errmsg;
			auto msg = frame.toRpcMessage(&errmsg);
			if (!errmsg.empty()) {
				throw std::runtime_error("Cannot convert RPC frame to message: " + errmsg);
			}
			auto frame2 = msg.toRpcFrame(m_clientProtocolType);
			writeFrame(frame2);
		}
		else {
			writeFrame(frame);
		}
	}
	catch (const std::exception &e) {
		nError() << "ERROR send frame:" << e.what();
	}
}

int RpcDriver::defaultRpcTimeoutMsec()
{
	return s_defaultRpcTimeoutMsec;
}

void RpcDriver::setDefaultRpcTimeoutMsec(int msec)
{
	s_defaultRpcTimeoutMsec = msec;
}

void RpcDriver::processRpcFrame(RpcFrame &&frame)
{
	// set client protocol type according to protocol type received from it
	// default protocol type is chainpack, this is needed just for legacy devices support
	if (m_clientProtocolType == Rpc::ProtocolType::Invalid) {
		m_clientProtocolType = frame.protocol;
	}

	onRpcFrameReceived(std::move(frame));
}

void RpcDriver::onRpcFrameReceived(RpcFrame &&frame)
{
	std::string errmsg;
	auto msg = frame.toRpcMessage(&errmsg);
	if (errmsg.empty()) {
		onRpcMessageReceived(msg);
	}
	else {
		logRpcDataW() << "ERROR - Rpc frame data corrupted:" << errmsg;
	}
}

std::string RpcDriver::frameToPrettyCpon(const RpcFrame &frame)
{
	shv::chainpack::RpcValue rpc_val;
	if(frame.data.length() < 256) {
		string errmsg;
		auto msg = frame.toRpcMessage(&errmsg);
		return msg.toCpon();
	}
	auto s = frame.meta.toPrettyString();
	s += " ... " + std::to_string(frame.data.size()) + " bytes of data ... ";
	return s;
}

} // namespace shv
