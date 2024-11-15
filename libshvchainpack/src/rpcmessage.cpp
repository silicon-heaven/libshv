#include <shv/chainpack/rpcmessage.h>

#include <shv/chainpack/metatypes.h>
#include <shv/chainpack/tunnelctl.h>
#include <shv/chainpack/datachange.h>
#include <shv/chainpack/accessgrant.h>
#include <shv/chainpack/abstractstreamwriter.h>
#include <shv/chainpack/chainpackwriter.h>
#include <shv/chainpack/cponwriter.h>
#include <shv/chainpack/cponreader.h>
#include <shv/chainpack/chainpackreader.h>

#include <cassert>
#include <sstream>

namespace shv::chainpack {

RpcMessage::MetaType::MetaType()
	: Super("RpcMessage")
{
	m_keys = {
		{static_cast<int>(Key::Params), {static_cast<int>(Key::Params), Rpc::PAR_PARAMS}},
		{static_cast<int>(Key::Result), {static_cast<int>(Key::Result), Rpc::JSONRPC_RESULT}},
		{static_cast<int>(Key::Error), {static_cast<int>(Key::Error), Rpc::JSONRPC_ERROR}},
	};
	m_tags = {
		{static_cast<int>(Tag::RequestId), {static_cast<int>(Tag::RequestId), Rpc::JSONRPC_REQUEST_ID}},
		{static_cast<int>(Tag::ShvPath), {static_cast<int>(Tag::ShvPath), Rpc::PAR_PATH}},
		{static_cast<int>(Tag::Method), {static_cast<int>(Tag::Method), Rpc::PAR_METHOD}},
		{static_cast<int>(Tag::CallerIds), {static_cast<int>(Tag::CallerIds), "cid"}},
		{static_cast<int>(Tag::RevCallerIds), {static_cast<int>(Tag::RevCallerIds), "rcid"}},
		{static_cast<int>(Tag::Access), {static_cast<int>(Tag::Access), "access"}},
		{static_cast<int>(Tag::AccessLevel), {static_cast<int>(Tag::AccessLevel), "accessLevel"}},
		{static_cast<int>(Tag::TunnelCtl), {static_cast<int>(Tag::TunnelCtl), "tctl"}},
		{static_cast<int>(Tag::UserId), {static_cast<int>(Tag::UserId), "userId"}},
		{static_cast<int>(Tag::Source), {static_cast<int>(Tag::Source), "source"}},
	};
}

void RpcMessage::MetaType::registerMetaType()
{
	static bool is_init = false;
	if(!is_init) {
		is_init = true;
		static MetaType s;
		meta::registerType(meta::GlobalNS::ID, MetaType::ID, &s);
	}
}

//==================================================================
// RpcFrame
//==================================================================
RpcMessage RpcFrame::toRpcMessage(std::string *errmsg) const
{
	auto make_rpcmsg = [](const RpcValue &val, std::string *err_msg) {
		try {
			return RpcMessage(val);
		}
		catch (const std::exception &e) {
			if (err_msg) {
				*err_msg = e.what();
			}
		}
		return RpcMessage();
	};
	switch (protocol) {
	case Rpc::ProtocolType::ChainPack: {
		auto val = RpcValue::fromChainPack(data, errmsg);
		if (!errmsg || (errmsg && errmsg->empty())) {
			auto m = meta;
			val.setMetaData(std::move(m));
			return make_rpcmsg(val, errmsg);
		}
		break;
	}
	case Rpc::ProtocolType::Cpon: {
		auto val = RpcValue::fromCpon(data, errmsg);
		if (!errmsg || (errmsg && errmsg->empty())) {
			auto m = meta;
			val.setMetaData(std::move(m));
			return make_rpcmsg(val, errmsg);
		}
		break;
	}
	default: {
		constexpr auto msg = "Invalid protocol type";
		if (errmsg) {
			*errmsg = msg;
		}
		else {
			throw std::runtime_error(msg);
		}
	}
	}

	return {};
}

std::string RpcFrame::toFrameHead() const
{
	switch (protocol) {
	case Rpc::ProtocolType::ChainPack: {
		std::ostringstream out;
		{
			ChainPackWriter wr(out);
			wr << meta;
		}
		auto ret = out.str();
		ret = static_cast<char>(protocol) + ret;
		ret += data;
		return ret;
	}
	case Rpc::ProtocolType::Cpon: {
		std::ostringstream out;
		{
			CponWriter wr(out);
			wr << meta;
		}
		auto ret = out.str();
		ret = static_cast<char>(protocol) + ret;
		return ret;
	}
	default: {
		throw std::runtime_error("Invalid protocol type");
	}
	}
}

std::string RpcFrame::toFrameData() const
{
	auto ret = toFrameHead();
	ret += data;
	return ret;
}

RpcFrame RpcFrame::fromFrameData(const std::string &frame_data)
{
	std::istringstream in(frame_data);
	auto protocol = static_cast<Rpc::ProtocolType>(in.get());
	switch (protocol) {
	case Rpc::ProtocolType::ChainPack: {
		ChainPackReader rd(in);
		RpcValue::MetaData meta;
		rd.read(meta);
		if(meta.isEmpty())
			throw ParseException(CCPCP_RC_MALFORMED_INPUT, "Metadata missing", -1, {});
		auto pos = in.tellg();
		if(pos < 0)
			throw ParseException(CCPCP_RC_MALFORMED_INPUT, "Metadata missing", -1, {});
		auto data = std::string(frame_data, static_cast<size_t>(pos));
		RpcFrame frame(protocol, std::move(meta), std::move(data));
		return frame;
	}
	case Rpc::ProtocolType::Cpon: {
		CponReader rd(in);
		RpcValue::MetaData meta;
		rd.read(meta);
		if(meta.isEmpty())
			throw ParseException(CCPCP_RC_MALFORMED_INPUT, "Metadata missing", -1, {});
		auto pos = in.tellg();
		if(pos < 0)
			throw ParseException(CCPCP_RC_MALFORMED_INPUT, "Metadata missing", -1, {});
		auto data = std::string(frame_data, static_cast<size_t>(pos));
		RpcFrame frame(protocol, std::move(meta), std::move(data));
		return frame;
	}
	default:
		throw std::runtime_error("Invalid protocol type");
	}
	return {};
}

//==================================================================
// RpcMessage
//==================================================================
const bool RpcMessage::m_isMetaTypeExplicit = true;

RpcMessage::RpcMessage()
{
	MetaType::registerMetaType();
}

RpcMessage::RpcMessage(const RpcValue &val)
	: RpcMessage()
{
	if(!val.isIMap())
		SHVCHP_EXCEPTION("Value is not IMap");
	m_value = val;
}

RpcMessage::~RpcMessage() = default;

const RpcValue& RpcMessage::value() const
{
	return m_value;
}

bool RpcMessage::hasKey(RpcValue::Int key) const
{
	return m_value.asIMap().count(key);
}

RpcValue RpcMessage::value(RpcValue::Int key) const
{
	return m_value.at(key);
}

void RpcMessage::setValue(RpcValue::Int key, const RpcValue &val)
{
	assert(key >= RpcMessage::MetaType::Key::Params && key < RpcMessage::MetaType::Key::MAX);
	checkMetaValues();
	m_value.set(key, val);
}

const RpcValue::MetaData& RpcMessage::metaData() const
{
	return m_value.metaData();
}

RpcValue RpcMessage::metaValue(RpcValue::Int key) const
{
	return m_value.metaValue(key);
}

void RpcMessage::setMetaValue(RpcValue::Int key, const RpcValue &val)
{
	checkMetaValues();
	m_value.setMetaValue(key, val);
}

RpcValue RpcMessage::metaValue(const RpcValue::String &key) const
{
	return m_value.metaValue(key);
}

void RpcMessage::setMetaValue(const RpcValue::String &key, const RpcValue &val)
{
	checkMetaValues();
	m_value.setMetaValue(key, val);
}

RpcValue RpcMessage::requestId() const
{
	if(isValid())
		return requestId(m_value.metaData());
	return RpcValue();
}

void RpcMessage::setRequestId(const RpcValue &id)
{
	checkMetaValues();
	setMetaValue(RpcMessage::MetaType::Tag::RequestId, id);
}

bool RpcMessage::hasMethod(const RpcValue::MetaData &meta)
{
	return meta.hasKey(RpcMessage::MetaType::Tag::Method);
}

RpcValue RpcMessage::method(const RpcValue::MetaData &meta)
{
	return meta.value(RpcMessage::MetaType::Tag::Method);
}

void RpcMessage::setMethod(RpcValue::MetaData &meta, const RpcValue::String &method)
{
	meta.setValue(RpcMessage::MetaType::Tag::Method, method);
}

bool RpcMessage::hasMethod() const
{
	return m_value.metaData().hasKey(RpcMessage::MetaType::Tag::Method);
}

RpcValue RpcMessage::method() const
{
	return metaValue(RpcMessage::MetaType::Tag::Method);
}

void RpcMessage::setMethod(const RpcValue::String &method)
{
	checkMetaValues();
	setMetaValue(RpcMessage::MetaType::Tag::Method, method);
}

bool RpcMessage::isValid() const
{
	return m_value.isValid();
}

bool RpcMessage::isRequest() const
{
	return hasRequestId() && hasMethod();
}

bool RpcMessage::isSignal() const
{
	return !hasRequestId() && hasMethod();
}

bool RpcMessage::isResponse() const
{
	return hasRequestId() && !hasMethod();
}

bool RpcMessage::isRequest(const RpcValue::MetaData &meta)
{
	return hasRequestId(meta) && hasMethod(meta);
}

bool RpcMessage::isResponse(const RpcValue::MetaData &meta)
{
	return hasRequestId(meta) && !hasMethod(meta);
}

bool RpcMessage::isSignal(const RpcValue::MetaData &meta)
{
	return !hasRequestId(meta) && hasMethod(meta);
}

bool RpcMessage::hasRequestId(const RpcValue::MetaData &meta)
{
	return meta.hasKey(RpcMessage::MetaType::Tag::RequestId);
}

RpcValue RpcMessage::requestId(const RpcValue::MetaData &meta)
{
	return meta.value(RpcMessage::MetaType::Tag::RequestId);
}

void RpcMessage::setRequestId(RpcValue::MetaData &meta, const RpcValue &id)
{
	meta.setValue(RpcMessage::MetaType::Tag::RequestId, id);
}

bool RpcMessage::hasRequestId() const
{
	return m_value.metaData().hasKey(RpcMessage::MetaType::Tag::RequestId);
}

RpcValue RpcMessage::shvPath(const RpcValue::MetaData &meta)
{
	return meta.value(RpcMessage::MetaType::Tag::ShvPath);
}

void RpcMessage::setShvPath(RpcValue::MetaData &meta, const RpcValue::String &path)
{
	meta.setValue(RpcMessage::MetaType::Tag::ShvPath, path);
}

RpcValue RpcMessage::shvPath() const
{
	return metaValue(RpcMessage::MetaType::Tag::ShvPath);
}

void RpcMessage::setShvPath(const RpcValue::String &path)
{
	setMetaValue(RpcMessage::MetaType::Tag::ShvPath, path);
}

AccessGrant RpcMessage::accessGrant(const RpcValue::MetaData &meta)
{
	return AccessGrant::fromShv2Access(access(meta), accessLevel(meta));
}

void RpcMessage::setAccessGrant(RpcValue::MetaData &meta, const AccessGrant &ag)
{
	setAccessLevel(meta, ag.accessLevel);
	setAccess(meta, ag.toShv2Access());
}

AccessGrant RpcMessage::accessGrant() const
{
	return accessGrant(metaData());
}

void RpcMessage::setAccessGrant(const AccessGrant &ag)
{
	setAccessLevel(ag.accessLevel);
	setAccess(ag.toShv2Access());
}

std::string RpcMessage::access(const RpcValue::MetaData &meta)
{
	return meta.value(RpcMessage::MetaType::Tag::Access).asString();
}

void RpcMessage::setAccess(RpcValue::MetaData &meta, const RpcValue::String &access)
{
	meta.setValue(RpcMessage::MetaType::Tag::Access, access.empty()? RpcValue(): RpcValue(access));
}

void RpcMessage::setAccess(const RpcValue::String &access)
{
	setMetaValue(RpcMessage::MetaType::Tag::Access, access.empty()? RpcValue(): RpcValue(access));
}

int RpcMessage::accessLevel(const RpcValue::MetaData &meta)
{
	return meta.value(RpcMessage::MetaType::Tag::AccessLevel).toInt();
}

void RpcMessage::setAccessLevel(RpcValue::MetaData &meta, AccessLevel level)
{
	meta.setValue(RpcMessage::MetaType::Tag::AccessLevel, level == AccessLevel::None? RpcValue(): RpcValue(static_cast<int>(level)));
}

void RpcMessage::setAccessLevel(AccessLevel level)
{
	setMetaValue(RpcMessage::MetaType::Tag::AccessLevel, level == AccessLevel::None? RpcValue(): RpcValue(static_cast<int>(level)));
}

TunnelCtl RpcMessage::tunnelCtl(const RpcValue::MetaData &meta)
{
	return meta.value(RpcMessage::MetaType::Tag::TunnelCtl);
}

void RpcMessage::setTunnelCtl(RpcValue::MetaData &meta, const TunnelCtl &tc)
{
	meta.setValue(RpcMessage::MetaType::Tag::TunnelCtl, tc);
}

TunnelCtl RpcMessage::tunnelCtl() const
{
	return metaValue(RpcMessage::MetaType::Tag::TunnelCtl);
}

void RpcMessage::setTunnelCtl(const TunnelCtl &tc)
{
	setMetaValue(RpcMessage::MetaType::Tag::TunnelCtl, tc);
}

RpcValue RpcMessage::callerIds(const RpcValue::MetaData &meta)
{
	return meta.value(RpcMessage::MetaType::Tag::CallerIds);
}

void RpcMessage::setCallerIds(RpcValue::MetaData &meta, const RpcValue &caller_id)
{
	meta.setValue(RpcMessage::MetaType::Tag::CallerIds, caller_id);
}

void RpcMessage::pushCallerId(RpcValue::MetaData &meta, RpcValue::Int caller_id)
{
	RpcValue curr_caller_id = RpcMessage::callerIds(meta);
	if(curr_caller_id.isList()) {
		RpcList array = curr_caller_id.asList();
		array.push_back(RpcValue(caller_id));
		setCallerIds(meta, array);
	}
	else if(curr_caller_id.isInt() || curr_caller_id.isUInt()) {
		RpcList array;
		array.push_back(curr_caller_id.toInt());
		array.push_back(RpcValue(caller_id));
		setCallerIds(meta, array);
	}
	else {
		setCallerIds(meta, caller_id);
	}
}

RpcValue RpcMessage::popCallerId(const RpcValue &caller_ids, RpcValue::Int &id)
{
	if(caller_ids.isList()) {
		shv::chainpack::RpcList array = caller_ids.asList();
		if(array.empty()) {
			id = 0;
		}
		else {
			id = array.back().toInt();
			array.pop_back();
		}
		return RpcValue(array);
	}
	id = caller_ids.toInt();

	return RpcValue();
}

RpcValue::Int RpcMessage::popCallerId(RpcValue::MetaData &meta)
{
	RpcValue::Int ret = 0;
	setCallerIds(meta, popCallerId(callerIds(meta), ret));
	return ret;
}

RpcValue::Int RpcMessage::popCallerId()
{
	RpcValue::Int ret = 0;
	setCallerIds(popCallerId(callerIds(), ret));
	return ret;
}

RpcValue::Int RpcMessage::peekCallerId(const RpcValue::MetaData &meta)
{
	RpcValue caller_ids = callerIds(meta);
	if(caller_ids.isList()) {
		const shv::chainpack::RpcList &array = caller_ids.asList();
		if(array.empty()) {
			return 0;
		}

		return array.back().toInt();
	}

	return  caller_ids.toInt();
}

RpcValue::Int RpcMessage::peekCallerId() const
{
	return peekCallerId(m_value.metaData());
}

RpcValue RpcMessage::callerIds() const
{
	return metaValue(RpcMessage::MetaType::Tag::CallerIds);
}

void RpcMessage::setCallerIds(const RpcValue &callerId)
{
	setMetaValue(RpcMessage::MetaType::Tag::CallerIds, callerId);
}

RpcValue RpcMessage::revCallerIds(const RpcValue::MetaData &meta)
{
	return meta.value(RpcMessage::MetaType::Tag::RevCallerIds);
}

void RpcMessage::setRevCallerIds(RpcValue::MetaData &meta, const RpcValue &caller_ids)
{
	meta.setValue(RpcMessage::MetaType::Tag::RevCallerIds, caller_ids);
}

void RpcMessage::pushRevCallerId(RpcValue::MetaData &meta, RpcValue::Int caller_id)
{
	RpcValue curr_caller_id = RpcMessage::revCallerIds(meta);
	if(curr_caller_id.isList()) {
		RpcList array = curr_caller_id.asList();
		array.push_back(RpcValue(caller_id));
		setRevCallerIds(meta, array);
	}
	else if(curr_caller_id.isInt() || curr_caller_id.isUInt()) {
		RpcList array;
		array.push_back(curr_caller_id.toInt());
		array.push_back(RpcValue(caller_id));
		setRevCallerIds(meta, array);
	}
	else {
		setRevCallerIds(meta, caller_id);
	}
}

RpcValue RpcMessage::revCallerIds() const
{
	return metaValue(RpcMessage::MetaType::Tag::RevCallerIds);
}

void RpcMessage::setRegisterRevCallerIds()
{
	setMetaValue(RpcMessage::MetaType::Tag::RevCallerIds, nullptr);
}

bool RpcMessage::isRegisterRevCallerIds(const RpcValue::MetaData &meta)
{
	return revCallerIds(meta).isValid();
}

RpcValue RpcMessage::userId() const
{
	return metaValue(RpcMessage::MetaType::Tag::UserId);
}

void RpcMessage::setUserId(const RpcValue &user_id)
{
	setMetaValue(RpcMessage::MetaType::Tag::UserId, user_id);
}

RpcValue RpcMessage::userId(RpcValue::MetaData &meta)
{
	return meta.value(RpcMessage::MetaType::Tag::UserId);
}

void RpcMessage::setUserId(RpcValue::MetaData &meta, const RpcValue &user_id)
{
	meta.setValue(RpcMessage::MetaType::Tag::UserId, user_id);
}

RpcValue::String RpcMessage::source() const
{
	return metaValue(RpcMessage::MetaType::Tag::Source).asString();
}

void RpcMessage::setSource(const RpcValue::String& source)
{
	setMetaValue(RpcMessage::MetaType::Tag::Source, source);
}

RpcValue::String RpcMessage::source(const RpcValue::MetaData& meta)
{
	return meta.value(RpcMessage::MetaType::Tag::Source).asString();
}

void RpcMessage::setSource(RpcValue::MetaData &meta, const RpcValue::String& source)
{
	meta.setValue(RpcMessage::MetaType::Tag::Source, source);
}

void RpcMessage::write(AbstractStreamWriter &wr) const
{
	assert(m_value.isValid());
	wr.write(m_value);
}

void RpcMessage::registerMetaTypes()
{
	RpcMessage::MetaType::registerMetaType();
	TunnelCtl::MetaType::registerMetaType();
	DataChange::MetaType::registerMetaType();
}

void RpcMessage::checkMetaValues()
{
	if(!m_value.isValid()) {
		m_value = RpcValue::IMap();
		/// not needed, RpcMessage is only type used so far
		if(m_isMetaTypeExplicit)
			setMetaValue(meta::Tag::MetaTypeId, RpcMessage::MetaType::ID);
	}
}

std::string RpcMessage::toPrettyString() const
{
	return m_value.toPrettyString();
}

std::string RpcMessage::toCpon() const
{
	return m_value.toCpon();
}

std::string RpcMessage::toChainPack() const
{
	return m_value.toChainPack();
}

RpcFrame RpcMessage::toRpcFrame(Rpc::ProtocolType protocol) const
{
	auto val = m_value;
	auto meta = val.takeMeta();
	std::string data;
	switch (protocol) {
	case Rpc::ProtocolType::ChainPack:
		data = val.toChainPack();
		break;
	case Rpc::ProtocolType::Cpon:
		data = val.toCpon();
		break;
	default:
		throw std::runtime_error("Invalid protocol type");
	}
	RpcFrame frame{protocol, std::move(meta), std::move(data)};
	return frame;
}

//==================================================================
// RpcRequest
//==================================================================
RpcRequest::~RpcRequest() = default;

RpcRequest::RpcRequest() = default;

RpcRequest::RpcRequest(const RpcMessage &msg)
	: Super(msg)
{
}

RpcRequest &RpcRequest::setMethod(const RpcValue::String &met)
{
	return setMethod(RpcValue::String(met));
}

RpcRequest &RpcRequest::setMethod(RpcValue::String &&met)
{
	setMetaValue(RpcMessage::MetaType::Tag::Method, RpcValue{std::move(met)});
	return *this;
}

RpcValue RpcRequest::params() const
{
	return value(RpcMessage::MetaType::Key::Params);
}

RpcResponse RpcRequest::makeResponse() const
{
	return RpcResponse::forRequest(*this);
}

RpcRequest& RpcRequest::setParams(const RpcValue& p)
{
	setValue(RpcMessage::MetaType::Key::Params, p);
	return *this;
}

RpcRequest& RpcRequest::setRequestId(const RpcValue::Int id)
{
	Super::setRequestId(id); return *this;
}

//==================================================================
// RpcNotify
//==================================================================
RpcSignal::RpcSignal() = default;

RpcSignal::RpcSignal(const RpcMessage &msg)
	: Super(msg)
{
}

RpcSignal::~RpcSignal() = default;

void RpcSignal::write(AbstractStreamWriter &wr, const std::string &method, std::function<void (AbstractStreamWriter &)> write_params_callback)
{
	RpcValue::MetaData md;
	md.setMetaTypeId(RpcMessage::MetaType::ID);
	md.setValue(RpcMessage::MetaType::Tag::Method, method);
	wr.write(md);
	wr.writeContainerBegin(RpcValue::Type::IMap);
	wr.writeIMapKey(RpcMessage::MetaType::Key::Params);
	write_params_callback(wr);
	wr.writeContainerEnd();
}

//==================================================================
// RpcException
//==================================================================
RpcException::RpcException(int err_code, const std::string &_msg, const std::string &_where)
	: Super(_msg, _where)
	, m_errorCode(err_code)
{}

RpcException::~RpcException() = default;

int RpcException::errorCode() const
{
	return m_errorCode;
}

//==================================================================
// RpcError
//==================================================================
namespace {
constexpr auto JSONRPC_ERROR_CODE = "code";
constexpr auto JSONRPC_ERROR_MESSAGE = "message";
constexpr auto JSONRPC_ERROR_DATA = "data";
}

RpcError::RpcError() = default;

RpcError::RpcError(const std::string &msg, int code, const RpcValue &data)
{
	setMessage(msg);
	setCode(code);
	setData(data);
}

RpcError& RpcError::setMessage(const RpcValue::String &mess)
{
	return setValue(KeyMessage, mess);
}

RpcError& RpcError::setCode(int c)
{
	return setValue(KeyCode, c);
}

int RpcError::code() const
{
	return value(KeyCode).toInt();
}

RpcValue::String RpcError::message() const
{
	return value(KeyMessage).asString();
}

RpcValue RpcError::data() const
{
	return value(KeyData);
}

RpcError &RpcError::setData(const RpcValue &mv)
{
	return setValue(KeyData, mv);
}

std:: string RpcError::errorCodeToString(int code)
{
	switch(code) {
	case ErrorCode::NoError: return "NoError";
	case ErrorCode::InvalidRequest: return "InvalidRequest";
	case ErrorCode::MethodNotFound: return "MethodNotFound";
	case ErrorCode::InvalidParams: return "InvalidParams";
	case ErrorCode::InternalError: return "InternalError";
	case ErrorCode::ParseError: return "ParseError";
	case ErrorCode::MethodCallTimeout: return "SyncMethodCallTimeout";
	case ErrorCode::MethodCallCancelled: return "SyncMethodCallCancelled";
	case ErrorCode::MethodCallException: return "MethodCallException";
	case ErrorCode::Unknown:  return "Unknown";
	default: return std::to_string(code);
	};
}

RpcValue::String RpcError::toString() const
{
	auto ret = errorCodeToString(code()) + ": " + message();
	if(auto d = data(); d.isValid()) {
		ret += " data: " + d.toCpon();
	}
	return ret;
}

RpcValue::Map RpcError::toJson() const
{
	return RpcValue::Map {
		{JSONRPC_ERROR_CODE, code()},
		{JSONRPC_ERROR_MESSAGE, message()},
		{JSONRPC_ERROR_DATA, data()},
	};
}

RpcError RpcError::fromJson(const RpcValue::Map &json)
{
	return fromRpcValue(RpcValue::IMap {
		{KeyCode, json.value(JSONRPC_ERROR_CODE).toInt()},
		{KeyMessage, json.value(JSONRPC_ERROR_MESSAGE).asString()},
		{KeyData, json.value(JSONRPC_ERROR_DATA)},
	});
}

RpcValue RpcError::toRpcValue() const
{
	return m_value;
}

RpcError RpcError::fromRpcValue(const RpcValue &rv)
{
	RpcError ret;
	if(rv.isIMap())
		ret.m_value = rv;
	return ret;
}

RpcError RpcError::create(int c, const RpcValue::String &msg, const RpcValue &data)
{
	RpcError ret;
	ret.setCode(c).setMessage(msg).setData(data);
	return ret;
}

RpcError RpcError::createMethodCallExceptionError(const RpcValue::String &msg)
{
	return create(MethodCallException, (msg.empty())? "Method call exception": msg);
}

RpcError RpcError::createInternalError(const RpcValue::String &msg)
{
	return create(InternalError, (msg.empty())? "Internal error": msg);
}

RpcError RpcError::createSyncMethodCallTimeout(const RpcValue::String &msg)
{
	return create(MethodCallTimeout, (msg.empty())? "Sync method call timeout": msg);
}

bool RpcError::isValid() const
{
	return code() != NoError;
}

//==================================================================
// RpcResponse
//==================================================================
RpcResponse::~RpcResponse() = default;

RpcResponse::RpcResponse() = default;

RpcResponse::RpcResponse(const RpcMessage &msg)
	: Super(msg)
{
}

RpcResponse::RpcResponse(const RpcResponse &msg) = default;

RpcResponse RpcResponse::forRequest(const RpcValue::MetaData &meta)
{
	RpcResponse ret;
	RpcValue id = requestId(meta);
	if(id.isValid())
		ret.setRequestId(id);
	auto caller_id = callerIds(meta);
	if(caller_id.isValid())
		ret.setCallerIds(caller_id);
	return ret;
}

RpcResponse RpcResponse::forRequest(const RpcRequest &rq)
{
	return forRequest(rq.metaData());
}

bool RpcResponse::hasResult() const
{
	return error().isValid() || result().isValid();
}

bool RpcResponse::isSuccess() const
{
	return result().isValid() && !isError();
}

bool RpcResponse::isError() const
{
	return error().isValid();
}


std::string RpcResponse::errorString() const
{
	if(isError())
		return error().toString();
	if(isSuccess())
		return std::string();
	if(isValid())
		return "Empty RPC message";
	return "Invalid RPC message";
}

RpcValue RpcError::value(int key) const
{
	return m_value.asIMap().value(key);
}

RpcError &RpcError::setValue(int key, const RpcValue &v)
{
	if(!m_value.isIMap()) {
		m_value = RpcValue(RpcValue::IMap());
	}
	m_value.set(key, v);
	return *this;
}

RpcError RpcResponse::error() const
{
	return RpcError::fromRpcValue(value(RpcMessage::MetaType::Key::Error));
}

RpcResponse& RpcResponse::setError(const RpcError &err)
{
	setValue(RpcMessage::MetaType::Key::Error, err.toRpcValue());
	return *this;
}

RpcValue RpcResponse::result() const
{
	return value(RpcMessage::MetaType::Key::Result);
}

RpcResponse& RpcResponse::setResult(const RpcValue& res)
{
	setValue(RpcMessage::MetaType::Key::Result, res);
	return *this;
}

RpcResponse& RpcResponse::setRequestId(const RpcValue &id)
{
	Super::setRequestId(id);
	return *this;
}

} // namespace shv
