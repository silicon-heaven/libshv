#pragma once

#include "rpcdriver.h"

#include <string>

namespace shv {
namespace chainpack {

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
	void writeMessageBegin() override;
	void writeMessageEnd() override;
	int64_t writeBytes(const char *bytes, size_t length) override;

	virtual void idleTaskOnSelectTimeout();
private:
	bool isOpenImpl() const;
	bool flush();
private:
	int m_socket = -1;
	std::string m_writeBuffer;
	size_t m_maxWriteBufferLength = 1024;
};

}}
