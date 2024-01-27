#include <shv/chainpack/rpcdriver.h>
#include <shv/chainpack/exception.h>
#include <shv/chainpack/cponwriter.h>
#include <shv/chainpack/cponreader.h>
#include <shv/chainpack/chainpackwriter.h>
#include <shv/chainpack/chainpackreader.h>
#include <shv/chainpack/utils.h>

#include <necrolog.h>

#include <sstream>
#include <iostream>
#include <cassert>
#include <algorithm>

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
	using namespace std;
	logRpcRawMsg() << SND_LOG_ARROW << msg.toPrettyString();
	auto frame_data = msg.toChainPack();
	logRpcData() << "SEND data:" << Utils::toHex(frame_data, 0, 250);
	writeFrameData(std::move(frame_data));
}

void RpcDriver::sendRpcFrame(RpcFrame &&frame)
{
	logRpcRawMsg() << SND_LOG_ARROW
				   << "protocol:"  << Rpc::protocolTypeToString(frame.protocol)
				   << "send raw meta + data: " << frame.meta.toPrettyString()
				   << Utils::toHex(frame.data, 0, 250);
	auto frame_data = frame.toChainPack();
	logRpcData() << "SEND data:" << Utils::toHex(frame_data, 0, 250);
	writeFrameData(std::move(frame_data));
}

void RpcDriver::onFrameDataRead(std::string &&frame_data)
{
	logRpcData().nospace() << __FUNCTION__ << " " << frame_data.length() << " bytes of data read:\n" << shv::chainpack::Utils::hexDump(frame_data);
	processReadFrameData(std::move(frame_data));
}

int RpcDriver::defaultRpcTimeoutMsec()
{
	return s_defaultRpcTimeoutMsec;
}

void RpcDriver::setDefaultRpcTimeoutMsec(int msec)
{
	s_defaultRpcTimeoutMsec = msec;
}

void RpcDriver::processReadFrameData(std::string &&frame_data)
{
	logRpcData() << __PRETTY_FUNCTION__ << "+++++++++++++++++++++++++++++++++";
	using namespace shv::chainpack;

	logRpcData().nospace() << "READ DATA " << frame_data.size() << " bytes of data read:\n" << shv::chainpack::Utils::hexDump(frame_data);
	try {
		auto frame = RpcFrame::fromChainPack(std::move(frame_data));
		onRpcFrameReceived(std::move(frame));
	}
	catch (const ParseException &e) {
		logRpcDataW() << "ERROR - RpcMessage header corrupted:" << e.msg();
		//logRpcDataW() << "The error occured in data:\n" << shv::chainpack::utils::hexDump(m_readData.data(), 1024);
		onParseDataException(e);
		return;
	}
	catch (const std::exception &e) {
		nError() << "processReadFrameData exception:" << e.what();
	}
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
	else {
		auto s = frame.meta.toPrettyString();
		s += " ... " + std::to_string(frame.data.size()) + " bytes of data ... ";
		return s;
	}
}

} // namespace shv
