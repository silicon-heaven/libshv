#include <shv/chainpack/socketrpcdriver.h>

#include <shv/chainpack/chainpackreader.h>
#include <shv/chainpack/utils.h>

#include <necrolog.h>

#include <cassert>
#include <cstring>

#ifdef FREE_RTOS
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#elif defined __GNUC__
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#endif

namespace cp = shv::chainpack;

namespace shv::chainpack {

SocketRpcDriver::SocketRpcDriver() = default;

SocketRpcDriver::~SocketRpcDriver()
{
	closeConnection();
}

void SocketRpcDriver::closeConnection()
{
	if(isOpenImpl())
		::close(m_socket);
	m_socket = -1;
}

bool SocketRpcDriver::isOpenImpl() const
{
	return m_socket >= 0;
}

bool SocketRpcDriver::isOpen()
{
	return isOpenImpl();
}

void SocketRpcDriver::idleTaskOnSelectTimeout()
{
}

void SocketRpcDriver::writeFrameData(const std::string &frame_data)
{
	if(!isOpen()) {
		nInfo() << "Write to closed socket";
		return;
	}
	if (m_writeBuffer.size() + frame_data.size() < m_maxWriteBufferLength) {
		m_writeBuffer += frame_data;
		flush();
	}
}

bool SocketRpcDriver::flush()
{
	if(m_writeBuffer.empty()) {
		nDebug() << "write buffer is empty";
		return false;
	}
	nDebug() << "Flushing write buffer, buffer len:" << m_writeBuffer.size() << "...";
	nDebug() << "writing to socket:" << Utils::toHex(m_writeBuffer);
	int64_t n = ::write(m_socket, m_writeBuffer.data(), m_writeBuffer.length());
	nDebug() << "\t" << n << "bytes written";
	if(n > 0)
		m_writeBuffer = m_writeBuffer.substr(static_cast<size_t>(n));
	return (n > 0);
}

bool SocketRpcDriver::connectToHost(const std::string &host, int port)
{
	closeConnection();
	m_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_socket < 0) {
		 nError() << "ERROR opening socket";
		 return false;
	}
	{
		struct sockaddr_in serv_addr;
		struct hostent *server;
		server = gethostbyname(host.c_str());

		if (server == nullptr) {
			nError() << "ERROR, no such host" << host;
			return false;
		}

		memset(reinterpret_cast<char *> (&serv_addr), 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		memcpy(reinterpret_cast<char *>(&serv_addr.sin_addr.s_addr), server->h_addr, static_cast<size_t>(server->h_length));
		serv_addr.sin_port = htons(static_cast<uint16_t>(port));

		/* Now connect to the server */
		nInfo().nospace() << "connecting to " << host << ":" << port;
		if (::connect(m_socket, reinterpret_cast<struct sockaddr*>(&serv_addr), sizeof(serv_addr)) < 0) {
			nError() << "ERROR, connecting host" << host;
			return false;
		}
	}


	{
		//set_socket_nonblock
		int flags;
		flags = fcntl(m_socket, F_GETFL, 0);
		assert(flags != -1);
		fcntl(m_socket, F_SETFL, flags | O_NONBLOCK);
	}

	nInfo() << "... connected";

	return true;
}

void SocketRpcDriver::exec()
{
	fd_set read_flags,write_flags; // the flag sets to be used
	struct timeval waitd;

	static constexpr size_t BUFF_LEN = 1024;
	std::array<char, BUFF_LEN> in;
	std::array<char, BUFF_LEN> out;
	memset(in.data(), 0, BUFF_LEN);
	memset(out.data(), 0, BUFF_LEN);

	while (true) {
		waitd.tv_sec = 5;
		waitd.tv_usec = 0;

		FD_ZERO(&read_flags);
		FD_ZERO(&write_flags);
		FD_SET(m_socket, &read_flags);
		if(!m_writeBuffer.empty())
			FD_SET(m_socket, &write_flags);

		int sel = select(FD_SETSIZE, &read_flags, &write_flags, static_cast<fd_set*>(nullptr), &waitd);

		//if an error with select
		if(sel < 0) {
			nError() << "select failed errno:" << errno;
			return;
		}
		if(sel == 0) {
			nInfo() << "\t timeout";
			idleTaskOnSelectTimeout();
			continue;
		}

		//socket ready for reading
		if(FD_ISSET(m_socket, &read_flags)) {
			nInfo() << "\t read fd is set";
			//clear set
			FD_CLR(m_socket, &read_flags);

			memset(&in, 0, BUFF_LEN);

			auto n = read(m_socket, in.data(), BUFF_LEN);
			nInfo() << "\t " << n << "bytes read";
			if(n <= 0) {
				nError() << "Closing socket";
				closeConnection();
				return;
			}
			{
				m_readBuffer += std::string(in.data(), n);
				while (true) {
					if (m_readFrameSize == 0) {
						// read length
						std::istringstream in2(m_readBuffer);
						int err_code;
						uint64_t frame_size = ChainPackReader::readUIntData(in2, &err_code);
						if(err_code == CCPCP_RC_BUFFER_UNDERFLOW) {
							// not enough data
							break;
						}
						if(err_code == CCPCP_RC_OK) {
							auto consumed_len = in2.tellg();
							m_readBuffer = std::string(m_readBuffer, consumed_len);
							m_readFrameSize = frame_size;
						}
						else {
							nWarning() << "Read RPC message length error.";
							m_readBuffer.clear();
							m_readFrameSize = 0;
							break;
						}
					}
					if (m_readFrameSize > 0 && m_readFrameSize <= m_readBuffer.size()) {
						auto frame = std::string(m_readBuffer, 0, m_readFrameSize);
						m_readBuffer = std::string(m_readBuffer, m_readFrameSize);
						onFrameDataRead(std::move(frame));
					}
					if (m_readFrameSize == 0  || m_readFrameSize > m_readBuffer.size()) {
						break;
					}
				}
			}
		}

		//socket ready for writing
		if(FD_ISSET(m_socket, &write_flags)) {
			nInfo() << "\t write fd is set";
			FD_CLR(m_socket, &write_flags);
			flush(); // trigger write next frame in driver
		}
	}
}

void SocketRpcDriver::sendResponse(int request_id, const cp::RpcValue &result)
{
	cp::RpcResponse resp;
	resp.setRequestId(request_id);
	resp.setResult(result);
	nInfo() << "sending response:" << resp.toCpon();
	sendRpcMessage(resp.value());
}

void SocketRpcDriver::sendNotify(std::string &&method, const cp::RpcValue &result)
{
	nInfo() << "sending notify:" << method;
	cp::RpcSignal ntf;
	ntf.setMethod(std::move(method));
	ntf.setParams(result);
	sendRpcMessage(ntf.value());
}

}
