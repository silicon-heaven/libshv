#include "rpcdriver.h"
#include "exception.h"
#include "cponwriter.h"
#include "cponreader.h"
#include "chainpackwriter.h"
#include "chainpackreader.h"
#include "utils.h"

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

namespace shv::chainpack {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
int RpcDriver::s_defaultRpcTimeoutMsec = 5000;

RpcDriver::RpcDriver() = default;

RpcDriver::~RpcDriver() = default;

Rpc::ProtocolType RpcDriver::protocolType() const
{
	return m_protocolType;
}

void RpcDriver::setProtocolType(Rpc::ProtocolType v)
{
	m_protocolType = v;
}

void RpcDriver::sendRpcValue(const RpcValue &msg)
{
	using namespace std;
	logRpcRawMsg() << SND_LOG_ARROW << msg.toPrettyString();
	std::string packed_data = codeRpcValue(protocolType(), msg);
	logRpcData() << "SEND protocol:" << Rpc::protocolTypeToString(protocolType())
				 << "packed data:"
				 << ((protocolType() == Rpc::ProtocolType::ChainPack)? Utils::toHex(packed_data, 0, 250): packed_data.substr(0, 250));
	enqueueDataToSend(MessageData{std::move(packed_data)});
}

void RpcDriver::sendRawData(std::string &&data)
{
	logRpcRawMsg() << SND_LOG_ARROW << "send raw data: " << (data.size() > 250? "<... long data ...>" : Utils::toHex(data));
	enqueueDataToSend(MessageData{std::move(data)});
}

void RpcDriver::sendRawData(const RpcValue::MetaData &meta_data, std::string &&data)
{
	logRpcRawMsg() << SND_LOG_ARROW << "protocol:" << Rpc::protocolTypeToString(protocolType()) << "send raw meta + data: " << meta_data.toPrettyString()
				<< Utils::toHex(data, 0, 250);
	using namespace std;
	std::ostringstream os_packed_meta_data;
	switch (protocolType()) {
	case Rpc::ProtocolType::Cpon: {
		CponWriter wr(os_packed_meta_data);
		wr << meta_data;
		break;
	}
	case Rpc::ProtocolType::ChainPack: {
		ChainPackWriter wr(os_packed_meta_data);
		wr << meta_data;
		break;
	}
	case Rpc::ProtocolType::JsonRpc: {
		break;
	}
	default:
		SHVCHP_EXCEPTION("Cannot serialize data without protocol version specified.");
	}
	Rpc::ProtocolType packed_data_ver = RpcMessage::protocolType(meta_data);
	if(protocolType() == Rpc::ProtocolType::JsonRpc) {
		// JSON RPC must be handled separately
		if(packed_data_ver == Rpc::ProtocolType::Invalid)
			SHVCHP_EXCEPTION("Cannot serialize to JSON-RPC data without protocol version specified.");
		// recode data;
		RpcValue val = decodeData(packed_data_ver, data, 0);
		val.setMetaData(RpcValue::MetaData(meta_data));
		enqueueDataToSend(MessageData(codeRpcValue(Rpc::ProtocolType::JsonRpc, val)));
	}
	else {
		if(packed_data_ver == Rpc::ProtocolType::Invalid || packed_data_ver == protocolType()) {
			enqueueDataToSend(MessageData(os_packed_meta_data.str(), std::move(data)));
		}
		else {
			// recode data;
			RpcValue val = decodeData(packed_data_ver, data, 0);
			enqueueDataToSend(MessageData(os_packed_meta_data.str(), codeRpcValue(protocolType(), val)));
		}
	}
}

RpcMessage RpcDriver::composeRpcMessage(RpcValue::MetaData &&meta_data, const std::string &data, std::string *errmsg)
{
	Rpc::ProtocolType protocol_type = RpcMessage::protocolType(meta_data);
	RpcValue val = decodeData(protocol_type, data, 0);
	if(!val.isValid()) {
		const char * msg = "Compose RPC message error.";
		if(!errmsg) {
			SHVCHP_EXCEPTION(msg);
		}

		*errmsg = msg;
		return RpcMessage();
	}
	val.setMetaData(std::move(meta_data));
	return RpcMessage(val);
}

void RpcDriver::enqueueDataToSend(RpcDriver::MessageData &&chunk_to_enqueue)
{
	/// LOCK_FOR_SEND lock mutex here in the multithreaded environment
	lockSendQueueGuard();
	if(!chunk_to_enqueue.empty()) {
		m_sendQueue.push_back(std::move(chunk_to_enqueue));
		logWriteQueue() << "===========> write chunk added, new queue len:" << m_sendQueue.size();
	}
	writeQueue();
	/// UNLOCK_FOR_SEND unlock mutex here in the multithreaded environment
	unlockSendQueueGuard();
}

void RpcDriver::writeQueue()
{
	if(m_sendQueue.empty())
		return;
	logWriteQueue() << "writeQueue(), queue len:" << m_sendQueue.size();
	if(!isOpen()) {
		nError() << "write data error, socket is not open!";
		return;
	}
	const MessageData &chunk = m_sendQueue[0];
	logWriteQueue() << "chunk size:" << chunk.size() << "meta:" << chunk.metaData.size() << "data:" << chunk.data.size();
	if(!m_topMessageDataHeaderWritten) {
		writeMessageBegin();
		std::string protocol_type_data;
		{
			std::ostringstream os;
			{ ChainPackWriter wr(os); wr.writeUIntData(static_cast<unsigned>(protocolType()));}
			protocol_type_data = os.str();
		}
		{
			std::ostringstream os;
			{ ChainPackWriter wr(os); wr.writeUIntData(chunk.size() + protocol_type_data.length()); }
			std::string packet_len_data = os.str();
			auto len = writeBytes(packet_len_data.data(), packet_len_data.length());
			if(len < 0)
				SHVCHP_EXCEPTION("Write socket error!");
			if(len < static_cast<int>(packet_len_data.length()))
				SHVCHP_EXCEPTION("Design error! Chunk length shall be always written at once to the socket");
		}
		{
			auto len = writeBytes(protocol_type_data.data(), protocol_type_data.length());
			logWriteQueue() << "\twrite header len:" << len;
			if(len < 0)
				SHVCHP_EXCEPTION("Write socket error!");
			if(len != 1)
				SHVCHP_EXCEPTION("Design error! Protocol version shall be always written at once to the socket");
		}
		m_topMessageDataHeaderWritten = true;
	}
	if(m_topMessageDataBytesWrittenSoFar < chunk.metaData.size()) {
		auto len = writeBytes_helper(chunk.metaData, m_topMessageDataBytesWrittenSoFar, chunk.metaData.size() - m_topMessageDataBytesWrittenSoFar);
		logWriteQueue() << "\twrite metadata len:" << len;
		m_topMessageDataBytesWrittenSoFar += static_cast<size_t>(len);
	}
	if(m_topMessageDataBytesWrittenSoFar >= chunk.metaData.size()) {
		auto len = writeBytes_helper(chunk.data
									 , m_topMessageDataBytesWrittenSoFar - chunk.metaData.size()
									 , chunk.data.size() - (m_topMessageDataBytesWrittenSoFar - chunk.metaData.size()));
		logWriteQueue() << "\twrite data len:" << len;
		m_topMessageDataBytesWrittenSoFar += static_cast<size_t>(len);
	}
	logWriteQueue() << "----- bytes written so far:" << m_topMessageDataBytesWrittenSoFar
					<< "remaining:" << (chunk.size() - m_topMessageDataBytesWrittenSoFar)
					<< "queue len:" << m_sendQueue.size();
	if(m_topMessageDataBytesWrittenSoFar == chunk.size()) {
		m_topMessageDataHeaderWritten = false;
		m_topMessageDataBytesWrittenSoFar = 0;
		m_sendQueue.pop_front();
		writeMessageEnd();
		logWriteQueue() << "<=========== write chunk finished, new queue len:" << m_sendQueue.size();
	}
}

int64_t RpcDriver::writeBytes_helper(const std::string &str, size_t from, size_t length)
{
	if(length == 0)
		SHVCHP_EXCEPTION("Design error! Atempt to write empty buffer to the socket");
	auto len = writeBytes(str.data() + from, length);
	if(len < 0)
		SHVCHP_EXCEPTION("Write socket error!");
	if(len == 0)
		SHVCHP_EXCEPTION("Design error! At least 1 byte of data shall be always written to the socket");
	return len;
}

void RpcDriver::onBytesRead(std::string &&bytes)
{
	logRpcData().nospace() << __FUNCTION__ << " " << bytes.length() << " bytes of data read:\n" << shv::chainpack::Utils::hexDump(bytes);
	m_readData += bytes;
	while(true) {
		auto old_len = m_readData.size();
		processReadData();
		if(old_len == m_readData.size())
			break;
	}
}

void RpcDriver::clearSendBuffers()
{
	m_sendQueue.clear();
	m_topMessageDataHeaderWritten = false;
	m_topMessageDataBytesWrittenSoFar = 0;
	m_readData.clear();
}

int RpcDriver::defaultRpcTimeoutMsec()
{
	return s_defaultRpcTimeoutMsec;
}

void RpcDriver::setDefaultRpcTimeoutMsec(int msec)
{
	s_defaultRpcTimeoutMsec = msec;
}

void RpcDriver::processReadData()
{
	logRpcData() << __PRETTY_FUNCTION__ << "+++++++++++++++++++++++++++++++++";
	using namespace shv::chainpack;

	size_t message_len;
	Rpc::ProtocolType protocol_type;
	RpcValue::MetaData meta_data;
	size_t meta_data_end_pos = 0;
	while (!m_readData.empty()) {
		logRpcData().nospace() << "READ DATA " << m_readData.length() << " bytes of data read:\n" << shv::chainpack::Utils::hexDump(m_readData);
		const std::string &read_data = m_readData;
		try {
			// read header and metadata
			std::istringstream in(read_data);
			int err_code;
			uint64_t chunk_len = ChainPackReader::readUIntData(in, &err_code);
			if(err_code == CCPCP_RC_BUFFER_UNDERFLOW) {
				// not enough data
				logRpcData() << "msg len not complete";
				return;
			}
			if(err_code != CCPCP_RC_OK) {
				throw ParseException(err_code, "Read RPC message length error.", -1);
			}

			message_len = static_cast<size_t>(in.tellg()) + chunk_len;
			protocol_type = static_cast<Rpc::ProtocolType>(ChainPackReader::readUIntData(in, &err_code));
			if(err_code == CCPCP_RC_BUFFER_UNDERFLOW) {
				// not enough data
				logRpcData() << "protocol_type not complete";
				return;
			}
			if(err_code != CCPCP_RC_OK) {
				throw ParseException(err_code, "Read RPC message protocol type error.", -1);
			}

			if(in.tellg() < 0) {
				// end of stream
				return;
			}

			logRpcData() << "\t protocol_type:" << static_cast<int>(protocol_type) << Rpc::protocolTypeToString(protocol_type);
			logRpcData() << "\t expected message data length:" << message_len << "length available:" << read_data.size();
			// wait for complete message
			if(message_len > read_data.length())
				return;

			meta_data_end_pos = decodeMetaData(meta_data, protocol_type, read_data, static_cast<size_t>(in.tellg()));
			logRpcData() << "\t meta_data_end_pos:" << meta_data_end_pos << meta_data.toPrettyString();
			if(meta_data_end_pos > message_len)
				throw ParseException(CCPCP_RC_MALFORMED_INPUT, "Metadata length mismatch", -1);
		}
		catch (const ParseException &e) {
			if(e.errCode() == CCPCP_RC_BUFFER_UNDERFLOW) {
				// not enough data
				return;
			}
			logRpcDataW() << "ERROR - RpcMessage header corrupted:" << e.msg();
			logRpcDataW() << "The error occured in data:\n" << shv::chainpack::utils::hexDump(m_readData.data(), 1024);
			onParseDataException(e);
			return;
		}

		if(m_protocolType == Rpc::ProtocolType::Invalid && protocol_type != Rpc::ProtocolType::Invalid) {
			// if protocol version is not explicitly specified,
			// it is set from first received message
			m_protocolType = protocol_type;
		}

		assert(meta_data_end_pos > 0);
		std::string msg_data;
		try {
			msg_data = read_data.substr(meta_data_end_pos, message_len - meta_data_end_pos);
			logRpcData() << message_len << "bytes of" << m_readData.size() << "processed";
			m_readData = m_readData.substr(message_len);
		}
		catch (const std::exception &e) {
			nError() << "internal exception:" << e.what();
		}
		try {
			onRpcDataReceived(protocol_type, std::move(meta_data), std::move(msg_data));
		}
		catch (const std::exception &e) {
			nError() << "onRpcDataReceived exception:" << e.what();
		}
		return;
	}
}

size_t RpcDriver::decodeMetaData(RpcValue::MetaData &meta_data, Rpc::ProtocolType protocol_type, const std::string &data, size_t start_pos)
{
	size_t meta_data_end_pos = start_pos;
	std::istringstream in(data);
	in.seekg(static_cast<long>(start_pos));

	switch (protocol_type) {
	case Rpc::ProtocolType::JsonRpc: {
		CponReader rd(in);
		RpcValue msg;
		rd.read(msg);
		if(!msg.isMap()) {
			throw ParseException(CCPCP_RC_MALFORMED_INPUT, "JSON message cannot be translated to ChainPack", -1);
		}

		const RpcValue::Map &map = msg.asMap();
		int id = map.value(Rpc::JSONRPC_REQUEST_ID).toInt();
		int caller_id = map.value(Rpc::JSONRPC_CALLER_ID).toInt();
		RpcValue::String method = map.value(Rpc::JSONRPC_METHOD).toString();
		std::string shv_path = map.value(Rpc::JSONRPC_SHV_PATH).toString();
		if(id > 0)
			RpcMessage::setRequestId(meta_data, id);
		if(!method.empty())
			RpcMessage::setMethod(meta_data, method);
		if(!shv_path.empty())
			RpcMessage::setShvPath(meta_data, shv_path);
		if(caller_id > 0)
			RpcMessage::setCallerIds(meta_data, caller_id);
		break;
	}
	case Rpc::ProtocolType::Cpon: {
		CponReader rd(in);
		rd.read(meta_data);
		meta_data_end_pos = (in.tellg() < 0)? data.size(): static_cast<size_t>(in.tellg());
		if(meta_data_end_pos == start_pos)
			throw ParseException(CCPCP_RC_MALFORMED_INPUT, "Metadata missing", -1);
		break;
	}
	case Rpc::ProtocolType::ChainPack: {
		ChainPackReader rd(in);
		rd.read(meta_data);
		meta_data_end_pos = (in.tellg() < 0)? data.size(): static_cast<size_t>(in.tellg());
		if(meta_data_end_pos == start_pos)
			throw ParseException(CCPCP_RC_MALFORMED_INPUT, "Metadata missing", -1);
		break;
	}
	default:
		throw ParseException(CCPCP_RC_MALFORMED_INPUT, "Unknown protocol type: " + Utils::toString(static_cast<int>(protocol_type)), -1);
	}

	return meta_data_end_pos;
}

RpcValue RpcDriver::decodeData(Rpc::ProtocolType protocol_type, const std::string &data, size_t start_pos)
{
	RpcValue ret;
	std::istringstream in(data);
	in.seekg(static_cast<long>(start_pos));
	try {
		switch (protocol_type) {
		case Rpc::ProtocolType::JsonRpc: {
			CponReader rd(in);
			rd.read(ret);
			RpcValue::Map map = ret.asMap();
			RpcValue::IMap imap;
			RpcValue params = map.value(Rpc::JSONRPC_PARAMS);
			if(params.isValid()) {
				imap[RpcMessage::MetaType::Key::Params] = params;
			}
			else {
				RpcValue result = map.value(Rpc::JSONRPC_RESULT);
				if(result.isValid()) {
					imap[RpcMessage::MetaType::Key::Result] = result;
				}
				else {
					RpcValue error = map.value(Rpc::JSONRPC_ERROR);
					if(error.isValid())
						imap[RpcMessage::MetaType::Key::Error] = RpcResponse::Error::fromJson(error.asMap()).toRpcValue();
				}
			}
			ret = imap;
			break;
		}
		case Rpc::ProtocolType::Cpon: {
			CponReader rd(in);
			rd.read(ret);
			break;
		}
		case Rpc::ProtocolType::ChainPack: {
			ChainPackReader rd(in);
			rd.read(ret);
			break;
		}
		default:
			logRpcDataW() << "Don't know how to decode message with unknown protocol version:" << static_cast<unsigned>(protocol_type);
			break;
		}
	}
	catch(ParseException &e) {
		logRpcDataW() << Rpc::protocolTypeToString(protocol_type) << "Decode data error:" << e.msg();
		auto err_pos = std::min(static_cast<size_t>(e.pos()), data.size());
		auto hexdump_start_pos = err_pos - std::min(err_pos, static_cast<size_t>(10*16));
		logRpcDataW().nospace() << "Start offset: " << start_pos << " Data: from pos:"
								<< hexdump_start_pos << "\n" << shv::chainpack::utils::hexDump(data.data() + hexdump_start_pos, 10*16);
	}
	return ret;
}

std::string RpcDriver::codeRpcValue(Rpc::ProtocolType protocol_type, const RpcValue &val)
{
	std::ostringstream os_packed_data;
	switch (protocol_type) {
	case Rpc::ProtocolType::JsonRpc: {
		RpcValue::Map json_msg;
		RpcMessage rpc_msg(val);

		const RpcValue rq_id = rpc_msg.requestId();
		if(rq_id.isValid())
			json_msg[Rpc::JSONRPC_REQUEST_ID] = rq_id;

		const RpcValue shv_path = rpc_msg.shvPath();
		if(shv_path.isString())
			json_msg[Rpc::JSONRPC_SHV_PATH] = shv_path.toString();

		const RpcValue caller_id = rpc_msg.callerIds();
		if(caller_id.isValid())
			json_msg[Rpc::JSONRPC_CALLER_ID] = caller_id;
					;
		if(rpc_msg.isResponse()) {
			// response
			RpcResponse resp(rpc_msg);
			if(resp.isError())
				json_msg[Rpc::JSONRPC_ERROR] = resp.error().toJson();
			else
				json_msg[Rpc::JSONRPC_RESULT] = resp.result();
		}
		else {
			// request
			RpcRequest rq(rpc_msg);
			json_msg[Rpc::JSONRPC_METHOD] = rq.method();
			if(rq.params().isValid())
				json_msg[Rpc::JSONRPC_PARAMS] = rq.params();
		}
		CponWriterOptions opts;
		opts.setJsonFormat(true);
		CponWriter wr(os_packed_data, opts);
		wr.write(json_msg);
		break;
	}
	case Rpc::ProtocolType::Cpon: {
		CponWriter wr(os_packed_data);
		wr << val;
		break;
	}
	case Rpc::ProtocolType::ChainPack: {
		ChainPackWriter wr(os_packed_data);
		wr << val;
		break;
	}
	default:
		SHVCHP_EXCEPTION("Cannot serialize data without protocol version specified.");
	}
	return os_packed_data.str();
}

void RpcDriver::onRpcDataReceived(Rpc::ProtocolType protocol_type, RpcValue::MetaData &&md, std::string &&data)
{
	RpcValue msg = decodeData(protocol_type, data, 0);
	if(msg.isValid()) {
		msg.setMetaData(std::move(md));
		logRpcRawMsg() << RCV_LOG_ARROW << msg.toPrettyString();
		onRpcValueReceived(msg);
	}
	else {
		nError() << "Throwing away invalid or empty message";
	}
}

void RpcDriver::onRpcValueReceived(const RpcValue &rpc_val)
{
	logRpcData() << "\t message received:" << rpc_val.toCpon();
	RpcMessage msg(rpc_val);
	onRpcMessageReceived(msg);

}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static int lock_cnt = 0;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static int logged_lock_cnt = 0;
void RpcDriver::lockSendQueueGuard()
{
	lock_cnt++;
	if(lock_cnt != 1) {
		if(logged_lock_cnt != lock_cnt) {
			logged_lock_cnt = lock_cnt;
			logWriteQueueW() << "Invalid write queue lock count:" << lock_cnt;
		}
	}
}

void RpcDriver::unlockSendQueueGuard()
{
	lock_cnt--;
}

std::string RpcDriver::dataToPrettyCpon(Rpc::ProtocolType protocol_type, const RpcValue::MetaData &md, const std::string &data, size_t start_pos, size_t data_len)
{
	shv::chainpack::RpcValue rpc_val;
	if(data_len == 0)
		data_len = data.size() - start_pos;
	if(data_len < 256) {
		rpc_val = shv::chainpack::RpcDriver::decodeData(protocol_type, data, start_pos);
	}
	else {
		rpc_val = " ... " + std::to_string(data_len) + " bytes of data ... ";
	}
	rpc_val.setMetaData(shv::chainpack::RpcValue::MetaData(md));
	return rpc_val.toPrettyString();
}

RpcDriver::MessageData::MessageData() = default;

RpcDriver::MessageData::MessageData(std::string &&meta_data_, std::string &&data_)
	: metaData(std::move(meta_data_)), data(std::move(data_))
{
}

RpcDriver::MessageData::MessageData(std::string &&data_)
	: data(std::move(data_))
{
}

RpcDriver::MessageData::MessageData(MessageData &&) noexcept = default;

bool RpcDriver::MessageData::empty() const
{
	return metaData.empty() && data.empty();
}

size_t RpcDriver::MessageData::size() const
{
	return metaData.size() + data.size();
}


} // namespace shv
