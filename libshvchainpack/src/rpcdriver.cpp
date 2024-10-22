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
	RpcDriver::sendRpcFrame(msg.toToRpcFrame(m_clientProtocolType));
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
			frame = msg.toToRpcFrame(m_clientProtocolType);
		}
		auto frame_data = frame.toFrameData();
		//logRpcData().nospace() << "FRAME DATA WRITE " << frame_data.size() << " bytes of data:\n" << shv::chainpack::utils::hexDump(frame_data);
		writeFrameData(frame_data);
	}
	catch (const std::exception &e) {
		nError() << "ERROR send frame:" << e.what();
	}
}

void RpcDriver::onFrameDataRead(const std::string &frame_data)
{
	logRpcData().nospace() << "FRAME DATA READ " << frame_data.size() << " bytes of data read:\n" << shv::chainpack::utils::hexDump(frame_data);
	try {
		auto frame = RpcFrame::fromFrameData(frame_data);

		// set client protocol type according to protocol type received from it
		// default protocol type is chainpack, this is needed just for legacy devices support
		m_clientProtocolType = frame.protocol;

		onRpcFrameReceived(std::move(frame));
	}
	catch (const ParseException &e) {
		logRpcDataW() << "ERROR - Rpc frame data corrupted:" << e.what();
		//logRpcDataW() << "The error occured in data:\n" << shv::chainpack::utils::hexDump(m_readData.data(), 1024);
		onParseDataException(e);
		return;
	}
	catch (const std::exception &e) {
		nError() << "ERROR - Rpc frame process error:" << e.what();
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

void RpcDriver::onRpcFrameReceived(RpcFrame &&frame)
{
	auto msg = frame.toRpcMessage();
	onRpcMessageReceived(msg);
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
