#pragma once

#include <shv/chainpack/rpcdriver.h>

#include <string>

namespace shv::chainpack {

class SHVCHAINPACK_DECL_EXPORT SocketRpcDriver : public RpcDriver
{
	using Super = RpcDriver;
public:
	SocketRpcDriver();
	~SocketRpcDriver() override;
	virtual bool connectToHost(const std::string & host, int port);
	void closeConnection();
	void exec();

	void sendResponse(int request_id, const RpcValue &result);
	void sendNotify(std::string &&method, const RpcValue &result);
protected:
	bool isOpen() override;
	void writeFrameData(const std::string &frame_data) override;

	virtual void onFrameDataRead(const std::string &frame_data);
	virtual void idleTaskOnSelectTimeout();
private:
	bool isOpenImpl() const;
	bool flush();
private:
	int m_socket = -1;
	std::string m_writeBuffer;
	size_t m_maxWriteBufferLength = 1024;
	std::string m_readBuffer;
	size_t m_readFrameSize = 0;
};
}
