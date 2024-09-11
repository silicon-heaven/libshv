#pragma once

#include <shv/chainpack/shvchainpackglobal.h>
#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/rpc.h>

#include <string>

namespace shv::chainpack {

class ParseException;

class SHVCHAINPACK_DECL_EXPORT RpcDriver
{
public:
	static constexpr auto SND_LOG_ARROW = "<==S";
	static constexpr auto RCV_LOG_ARROW = "R==>";
public:
	explicit RpcDriver();
	virtual ~RpcDriver();

	void sendRpcMessage(const RpcMessage &msg);
	virtual void sendRpcFrame(RpcFrame &&frame);

	static int defaultRpcTimeoutMsec();
	static void setDefaultRpcTimeoutMsec(int msec);

	static std::string frameToPrettyCpon(const RpcFrame &frame);
protected:
	virtual bool isOpen() = 0;

	virtual void writeFrameData(const std::string &frame_data) = 0;
	virtual void onFrameDataRead(const std::string &frame_data);

	virtual void onRpcFrameReceived(RpcFrame &&frame);
	virtual void onParseDataException(const shv::chainpack::ParseException &e) = 0;
	virtual void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg) = 0;
private:
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
	static int s_defaultRpcTimeoutMsec;
	Rpc::ProtocolType m_clientProtocolType = Rpc::ProtocolType::ChainPack;
};

}
