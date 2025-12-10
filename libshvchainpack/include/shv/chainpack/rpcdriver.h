#pragma once

#include <shv/chainpack/shvchainpack_export.h>
#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/rpc.h>

#include <string>

namespace shv::chainpack {

class ParseException;

class LIBSHVCHAINPACK_CPP_EXPORT RpcDriver
{
public:
	explicit RpcDriver();
	virtual ~RpcDriver();

	void sendRpcMessage(const RpcMessage &msg);
	virtual void sendRpcFrame(RpcFrame &&frame);

	int rpcTimeoutMsec() const;
	void setRpcTimeoutMsec(int msec);

	static std::string frameToPrettyCpon(const RpcFrame &frame);
protected:
	virtual bool isOpen() = 0;

	virtual void writeFrame(const RpcFrame &frame) = 0;

	void processRpcFrame(RpcFrame &&frame);
	virtual void onRpcFrameReceived(RpcFrame &&frame);
	virtual void onParseDataException(const shv::chainpack::ParseException &e) = 0;
	virtual void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg) = 0;

	void setClientProtocolType(Rpc::ProtocolType pt) { m_clientProtocolType = pt; }
protected:
	/// We must remember recent message protocol type to support legacy CPON clients
	Rpc::ProtocolType m_clientProtocolType = Rpc::ProtocolType::Invalid;
private:
	int m_rpcTimeoutMsec = 5000;
};

}
