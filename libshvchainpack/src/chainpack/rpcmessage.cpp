#include "rpcmessage.h"
#include "metatypes.h"
#include "tunnelctl.h"
#include "datachange.h"
#include "accessgrant.h"
#include "abstractstreamwriter.h"

#include <cassert>

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
		{static_cast<int>(Tag::ProtocolType), {static_cast<int>(Tag::ProtocolType), "protocol"}},
		{static_cast<int>(Tag::RevCallerIds), {static_cast<int>(Tag::RevCallerIds), "rcid"}},
		{static_cast<int>(Tag::AccessGrant), {static_cast<int>(Tag::AccessGrant), "grant"}},
		{static_cast<int>(Tag::TunnelCtl), {static_cast<int>(Tag::TunnelCtl), "tctl"}},
		{static_cast<int>(Tag::UserId), {static_cast<int>(Tag::UserId), "userId"}},
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

RpcMessage::RpcMessage(const RpcMessage &val) = default;

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

RpcValue RpcMessage::accessGrant(const RpcValue::MetaData &meta)
{
	return meta.value(RpcMessage::MetaType::Tag::AccessGrant);
}

void RpcMessage::setAccessGrant(RpcValue::MetaData &meta, const RpcValue &ag)
{
	meta.setValue(RpcMessage::MetaType::Tag::AccessGrant, ag);
}

RpcValue RpcMessage::accessGrant() const
{
	return metaValue(RpcMessage::MetaType::Tag::AccessGrant);
}

void RpcMessage::setAccessGrant(const RpcValue &ag)
{
	setMetaValue(RpcMessage::MetaType::Tag::AccessGrant, ag);
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
		RpcValue::List array = curr_caller_id.asList();
		array.push_back(RpcValue(caller_id));
		setCallerIds(meta, array);
	}
	else if(curr_caller_id.isInt() || curr_caller_id.isUInt()) {
		RpcValue::List array;
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
		shv::chainpack::RpcValue::List array = caller_ids.asList();
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
		const shv::chainpack::RpcValue::List &array = caller_ids.asList();
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
		RpcValue::List array = curr_caller_id.asList();
		array.push_back(RpcValue(caller_id));
		setRevCallerIds(meta, array);
	}
	else if(curr_caller_id.isInt() || curr_caller_id.isUInt()) {
		RpcValue::List array;
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

Rpc::ProtocolType RpcMessage::protocolType(const RpcValue::MetaData &meta)
{
	return static_cast<Rpc::ProtocolType>(meta.value(RpcMessage::MetaType::Tag::ProtocolType).toUInt());
}

void RpcMessage::setProtocolType(RpcValue::MetaData &meta, Rpc::ProtocolType ver)
{
	meta.setValue(RpcMessage::MetaType::Tag::ProtocolType, ver == Rpc::ProtocolType::Invalid? RpcValue(): RpcValue(static_cast<unsigned>(ver)));
}

Rpc::ProtocolType RpcMessage::protocolType() const
{
	return static_cast<Rpc::ProtocolType>(metaValue(RpcMessage::MetaType::Tag::ProtocolType).toUInt());
}

void RpcMessage::setProtocolType(Rpc::ProtocolType ver)
{
	setMetaValue(RpcMessage::MetaType::Tag::ProtocolType, ver == Rpc::ProtocolType::Invalid? RpcValue(): RpcValue(static_cast<unsigned>(ver)));
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
	AccessGrant::MetaType::registerMetaType();
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

//==================================================================
// RpcRequest
//==================================================================
RpcRequest::~RpcRequest() = default;

RpcRequest::RpcRequest()
	: Super()
{
}

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
RpcSignal::RpcSignal()
	: Super()
{
}

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
static constexpr auto JSONRPC_ERROR_CODE = "code";
static constexpr auto JSONRPC_ERROR_MESSAGE = "message";
static constexpr auto JSONRPC_ERROR_DATA = "data";
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
	};
	return Utils::toString(code);
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
	return !m_value.asIMap().empty();
}

//==================================================================
// RpcResponse
//==================================================================
RpcResponse::~RpcResponse() = default;

RpcResponse::RpcResponse()
	: Super()
{
}

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

bool RpcResponse::hasRetVal() const
{
	return error().isValid() || result().isValid();
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
