#pragma once

#include <shv/chainpack/shvchainpackglobal.h>

#include <shv/chainpack/rpcvalue.h>
#include <shv/chainpack/utils.h>

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT TunnelCtl : public shv::chainpack::RpcValue
{
	using Super = shv::chainpack::RpcValue;
public:
	class MetaType : public chainpack::meta::MetaType
	{
		using Super = chainpack::meta::MetaType;
	public:
		enum {ID = chainpack::meta::GlobalNS::MetaTypeId::TunnelCtl};
		struct Key { enum Enum {State = 1, Host, Port, Secret, RequestId, CallerIds, MAX};};

		MetaType();

		static void registerMetaType();
	};
public:
	struct State {enum Enum {
			Invalid = 0,
			FindTunnelRequest,
			FindTunnelResponse,
			CreateTunnelRequest,
			CreateTunnelResponse,
			CloseTunnel,
		};};

	SHV_IMAP_FIELD_IMPL2(int, MetaType::Key::State, s, setS, tate, State::Invalid)
public:
	TunnelCtl() = default;
	TunnelCtl(State::Enum st);
	TunnelCtl(const RpcValue &o);
};

class SHVCHAINPACK_DECL_EXPORT FindTunnelReqCtl : public TunnelCtl
{
	using Super = TunnelCtl;

	SHV_IMAP_FIELD_IMPL(std::string, MetaType::Key::Host, h, setH, ost)
	SHV_IMAP_FIELD_IMPL(int, MetaType::Key::Port, p, setP, ort)
	SHV_IMAP_FIELD_IMPL(std::string, MetaType::Key::Secret, s, setS, ecret)
	SHV_IMAP_FIELD_IMPL(shv::chainpack::RpcValue, MetaType::Key::CallerIds, c, setC, allerIds)

public:
	FindTunnelReqCtl();
	FindTunnelReqCtl(const TunnelCtl &o);
};

class SHVCHAINPACK_DECL_EXPORT FindTunnelRespCtl : public TunnelCtl
{
	using Super = TunnelCtl;

	SHV_IMAP_FIELD_IMPL(std::string, MetaType::Key::Host, h, setH, ost)
	SHV_IMAP_FIELD_IMPL(int, MetaType::Key::Port, p, setP, ort)
	SHV_IMAP_FIELD_IMPL(std::string, MetaType::Key::Secret, s, setS, ecret)
	SHV_IMAP_FIELD_IMPL(int, MetaType::Key::RequestId, r, setR, equestId)
	SHV_IMAP_FIELD_IMPL(shv::chainpack::RpcValue, MetaType::Key::CallerIds, c, setC, allerIds)

public:
	FindTunnelRespCtl();
	FindTunnelRespCtl(const TunnelCtl &o);

	static FindTunnelRespCtl fromFindTunnelRequest(const FindTunnelReqCtl &rq);

};

class SHVCHAINPACK_DECL_EXPORT CreateTunnelReqCtl : public TunnelCtl
{
	using Super = TunnelCtl;

	SHV_IMAP_FIELD_IMPL(std::string, MetaType::Key::Host, h, setH, ost)
	SHV_IMAP_FIELD_IMPL(int, MetaType::Key::Port, p, setP, ort)
	SHV_IMAP_FIELD_IMPL(std::string, MetaType::Key::Secret, s, setS, ecret)
	SHV_IMAP_FIELD_IMPL(int, MetaType::Key::RequestId, r, setR, equestId)
	SHV_IMAP_FIELD_IMPL(shv::chainpack::RpcValue, MetaType::Key::CallerIds, c, setC, allerIds)

public:
	CreateTunnelReqCtl();
	CreateTunnelReqCtl(const TunnelCtl &o);

	static CreateTunnelReqCtl fromFindTunnelResponse(const FindTunnelRespCtl &resp);
};

class SHVCHAINPACK_DECL_EXPORT CreateTunnelRespCtl : public TunnelCtl
{
	using Super = TunnelCtl;

public:
	CreateTunnelRespCtl();
	CreateTunnelRespCtl(const TunnelCtl &o);
};

class SHVCHAINPACK_DECL_EXPORT CloseTunnelCtl : public TunnelCtl
{
	using Super = TunnelCtl;

public:
	CloseTunnelCtl();
	CloseTunnelCtl(const TunnelCtl &o);
};

}}
