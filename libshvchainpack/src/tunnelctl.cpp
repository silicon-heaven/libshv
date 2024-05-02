#include <shv/chainpack/tunnelctl.h>

namespace shv::chainpack {

//================================================================
// TunnelCtl
//================================================================
TunnelCtl::MetaType::MetaType()
	: Super("TunnelCtl")
{
	m_keys = {
		RPC_META_KEY_DEF(State),
		RPC_META_KEY_DEF(Host),
		RPC_META_KEY_DEF(Port),
		RPC_META_KEY_DEF(Secret),
		RPC_META_KEY_DEF(RequestId),
		RPC_META_KEY_DEF(CallerIds),
	};
}

void TunnelCtl::MetaType::registerMetaType()
{
	static bool is_init = false;
	if(!is_init) {
		is_init = true;
		static MetaType s;
		shv::chainpack::meta::registerType(shv::chainpack::meta::GlobalNS::ID, MetaType::ID, &s);
	}
}

TunnelCtl::TunnelCtl(State::Enum st)
 : Super(RpcValue::IMap())
{
	MetaType::registerMetaType();
	setMetaValue(chainpack::meta::Tag::MetaTypeId, MetaType::ID);
	setState(st);
}
TunnelCtl::TunnelCtl(const RpcValue &o)
	: Super(o)
{
}

FindTunnelReqCtl::FindTunnelReqCtl()
	: Super(State::FindTunnelRequest)
{
}

FindTunnelReqCtl::FindTunnelReqCtl(const TunnelCtl &o)
	: Super(o)
{
}

//================================================================
// FindTunnelResponse
//================================================================
FindTunnelRespCtl::FindTunnelRespCtl()
	: Super(State::FindTunnelResponse)
{
}

FindTunnelRespCtl::FindTunnelRespCtl(const TunnelCtl &o)
	: Super(o)
{
}

FindTunnelRespCtl FindTunnelRespCtl::fromFindTunnelRequest(const FindTunnelReqCtl &rq)
{
	FindTunnelRespCtl ret;
	ret.setHost(rq.host());
	ret.setPort(rq.port());
	ret.setCallerIds(rq.callerIds());
	ret.setSecret(rq.secret());
	return ret;
}

//================================================================
// CreateTunnelRequest
//================================================================
CreateTunnelReqCtl::CreateTunnelReqCtl()
	: Super(State::CreateTunnelRequest)
{
}

CreateTunnelReqCtl::CreateTunnelReqCtl(const TunnelCtl &o)
	: Super(o)
{
}

CreateTunnelReqCtl CreateTunnelReqCtl::fromFindTunnelResponse(const FindTunnelRespCtl &resp)
{
	CreateTunnelReqCtl ret;
	ret.setHost(resp.host());
	ret.setPort(resp.port());
	ret.setSecret(resp.secret());
	ret.setCallerIds(resp.callerIds());
	return ret;
}

CreateTunnelRespCtl::CreateTunnelRespCtl()
	: Super(State::CreateTunnelResponse)
{
}

CreateTunnelRespCtl::CreateTunnelRespCtl(const TunnelCtl &o)
	: Super(o)
{
}

CloseTunnelCtl::CloseTunnelCtl()
	: Super(State::CreateTunnelResponse)
{
}

CloseTunnelCtl::CloseTunnelCtl(const TunnelCtl &o)
	: Super(o)
{
}

} // namespace shv
