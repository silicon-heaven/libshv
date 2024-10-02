#include <shv/chainpack/rpcvalue.h>

#include <shv/chainpack/cponwriter.h>
#include <shv/chainpack/cponreader.h>
#include <shv/chainpack/chainpackwriter.h>
#include <shv/chainpack/chainpackreader.h>
#include <shv/chainpack/exception.h>
#include <shv/chainpack/utils.h>

#include <shv/chainpack/ccpon.h>

#include <necrolog.h>

#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <chrono>
#include <utility>

#ifdef DEBUG_RPCVAL
#define logDebugRpcVal nWarning
#else
#define logDebugRpcVal if(0) nWarning
#endif
namespace shv::chainpack {

//==============================================
// value wrappers
//==============================================
#ifdef DEBUG_RPCVAL
inline NecroLog &operator<<(NecroLog log, const shv::chainpack::RpcValue::Decimal &d) { return log.operator <<(d.toDouble()); }
inline NecroLog &operator<<(NecroLog log, const shv::chainpack::RpcValue::DateTime &d) { return log.operator <<(d.toIsoString()); }
inline NecroLog &operator<<(NecroLog log, const shv::chainpack::RpcValue::List &d) { return log.operator <<("some_list:" + std::to_string(d.size())); }
inline NecroLog &operator<<(NecroLog log, const shv::chainpack::RpcValue::Map &d) { return log.operator <<("some_map:" + std::to_string(d.size())); }
inline NecroLog &operator<<(NecroLog log, const shv::chainpack::RpcValue::IMap &d) { return log.operator <<("some_imap:" + std::to_string(d.size())); }
inline NecroLog &operator<<(NecroLog log, std::nullptr_t) { return log.operator <<("NULL"); }

static int value_data_cnt = 0;
#endif

namespace {
auto convert_to_int(const double d)
{
	if (std::isnan(d)) {
		throw std::runtime_error("Can't convert NaN to int");
	}

	if (std::numeric_limits<RpcValue::Int>::max() <= d) {
		return std::numeric_limits<RpcValue::Int>::max();
	}
	if (std::numeric_limits<RpcValue::Int>::min() >= d) {
		return std::numeric_limits<RpcValue::Int>::min();
	}

	return static_cast<RpcValue::Int>(d);
}
}

namespace {
const CowPtr<RpcValue::String>& static_empty_string() { static const CowPtr<RpcValue::String> s{std::make_shared<RpcValue::String>()}; return s; }
const CowPtr<RpcValue::Blob>& static_empty_blob() { static const CowPtr<RpcValue::Blob> s{std::make_shared<RpcValue::Blob>()}; return s; }
const CowPtr<RpcValue::List>& static_empty_list() { static const CowPtr<RpcValue::List> s{std::make_shared<RpcValue::List>()}; return s; }
const CowPtr<RpcValue::Map>& static_empty_map() { static const CowPtr<RpcValue::Map> s{std::make_shared<RpcValue::Map>()}; return s; }
const CowPtr<RpcValue::IMap>& static_empty_imap() { static const CowPtr<RpcValue::IMap> s{std::make_shared<RpcValue::IMap>()}; return s; }
}

/* * * * * * * * * * * * * * * * * * * *
 * Constructors
 */

RpcValue::RpcValue() noexcept = default;

RpcValue RpcValue::fromType(RpcValue::Type t) noexcept
{
	switch(t) {
	case Type::Invalid: return RpcValue{};
	case Type::Null: return RpcValue{nullptr};
	case Type::UInt: return RpcValue{static_cast<UInt>(0)};
	case Type::Int: return RpcValue{0};
	case Type::Double: return RpcValue{static_cast<double>(0)};
	case Type::Bool: return RpcValue{false};
	case Type::String: return RpcValue{String()};
	case Type::Blob: return RpcValue{Blob()};
	case Type::DateTime: return RpcValue{DateTime()};
	case Type::List: return RpcValue{List()};
	case Type::Map: return RpcValue{Map()};
	case Type::IMap: return RpcValue{IMap()};
	case Type::Decimal: return RpcValue{Decimal()};
	}
	return RpcValue();
}
RpcValue::RpcValue(std::nullptr_t) noexcept : m_value(Null{}) {}
RpcValue::RpcValue(double value) : m_value(value) {}
RpcValue::RpcValue(const RpcValue::Decimal& value) : m_value(value) {}
RpcValue::RpcValue(short value) : m_value(int64_t{value}) {}
RpcValue::RpcValue(int value) : m_value(int64_t{value}) {}
RpcValue::RpcValue(long value) : m_value(int64_t{value}) {}
RpcValue::RpcValue(long long value) : m_value(int64_t{value}) {}
RpcValue::RpcValue(unsigned short value) : m_value(uint64_t{value}) {}
RpcValue::RpcValue(unsigned int value) : m_value(uint64_t{value}) {}
RpcValue::RpcValue(unsigned long value) : m_value(uint64_t{value}) {}
RpcValue::RpcValue(unsigned long long value) : m_value(uint64_t{value}) {}

RpcValue::RpcValue(bool value) : m_value(value) {}
RpcValue::RpcValue(const DateTime &value) : m_value(value) {}

RpcValue::RpcValue(const RpcValue::Blob &value) : m_value(std::make_shared<RpcValue::Blob>(value)) {}
RpcValue::RpcValue(RpcValue::Blob &&value) : m_value(std::make_shared<RpcValue::Blob>(std::move(value))) {}
RpcValue::RpcValue(const uint8_t * value, size_t size) : m_value(std::make_shared<Blob>(value, value + size)) {}

RpcValue::RpcValue(const std::string &value) : m_value(std::make_shared<std::string>(value)) {}
RpcValue::RpcValue(std::string &&value) : m_value(std::make_shared<std::string>(std::move(value))) {}
RpcValue::RpcValue(const char * value) : m_value(std::make_shared<std::string>(value)) {}

RpcValue::RpcValue(const RpcValue::List &values) : m_value(CowPtr{std::make_shared<RpcValue::List>(values)}) {}
RpcValue::RpcValue(RpcValue::List &&values) : m_value(CowPtr{std::make_shared<RpcValue::List>(std::move(values))}) {}

RpcValue::RpcValue(const RpcValue::Map &values) : m_value(CowPtr{std::make_shared<RpcValue::Map>(values)}) {}
RpcValue::RpcValue(RpcValue::Map &&values) : m_value(CowPtr{std::make_shared<RpcValue::Map>(std::move(values))}) {}

RpcValue::RpcValue(const RpcValue::IMap &values) : m_value(CowPtr{std::make_shared<RpcValue::IMap>(values)}) {}
RpcValue::RpcValue(RpcValue::IMap &&values) : m_value(CowPtr{std::make_shared<RpcValue::IMap>(std::move(values))}) {}

#ifdef RPCVALUE_COPY_AND_SWAP
void RpcValue::swap(RpcValue& other) noexcept
{
	std::swap(m_ptr, other.m_ptr);
}
#endif

/* * * * * * * * * * * * * * * * * * * *
 * Accessors
 */

RpcValue::Type RpcValue::type() const
{
	return static_cast<Type>(m_value.index());
}
const RpcValue::MetaData &RpcValue::metaData() const
{
	static MetaData md;
	if (m_meta) {
		return *m_meta;
	}
	return md;
}

RpcValue RpcValue::metaValue(RpcValue::Int key, const RpcValue &default_value) const
{
	const MetaData &md = metaData();
	RpcValue ret = md.value(key, default_value);
	return ret;
}

RpcValue RpcValue::metaValue(const RpcValue::String &key, const RpcValue &default_value) const
{
	const MetaData &md = metaData();
	RpcValue ret = md.value(key, default_value);
	return ret;
}

void RpcValue::setMetaData(RpcValue::MetaData &&meta_data)
{
	if (type() == Type::Invalid)
		SHVCHP_EXCEPTION("Cannot set valid meta data to invalid ChainPack value!");

	m_meta = std::make_shared<MetaData>(std::move(meta_data));
}

void RpcValue::setMetaValue(RpcValue::Int key, const RpcValue &val)
{
	if (type() == Type::Invalid)
		SHVCHP_EXCEPTION("Cannot set valid meta value to invalid ChainPack value!");

	if (!m_meta) {
		m_meta = std::make_shared<MetaData>();
	}
	m_meta->setValue(key, val);
}

void RpcValue::setMetaValue(const RpcValue::String &key, const RpcValue &val)
{
	if (type() == Type::Invalid)
		SHVCHP_EXCEPTION("Cannot set valid meta value to invalid ChainPack value!");

	if (!m_meta) {
		m_meta = std::make_shared<MetaData>();
	}
	m_meta->setValue(key, val);
}

int RpcValue::metaTypeId() const
{
	return metaValue(meta::Tag::MetaTypeId).toInt();
}

int RpcValue::metaTypeNameSpaceId() const
{
	return metaValue(meta::Tag::MetaTypeNameSpaceId).toInt();
}

void RpcValue::setMetaTypeId(int id)
{
	setMetaValue(meta::Tag::MetaTypeId, id);
}

void RpcValue::setMetaTypeId(int ns, int id)
{
	setMetaValue(meta::Tag::MetaTypeNameSpaceId, ns); setMetaValue(meta::Tag::MetaTypeId, id);
}


bool RpcValue::isDefaultValue() const
{
	switch (type()) {
	case RpcValue::Type::Invalid: return true;
	case RpcValue::Type::Null:
	case RpcValue::Type::Bool: return (toBool() == false);
	case RpcValue::Type::Int: return (toInt() == 0);
	case RpcValue::Type::UInt: return (toUInt() == 0);
	case RpcValue::Type::DateTime: return (toDateTime().msecsSinceEpoch() == 0);
	case RpcValue::Type::Decimal: return (toDecimal().mantisa() == 0);
	case RpcValue::Type::Double: return (toDouble() == 0);
	case RpcValue::Type::String: return (asString().empty());
	case RpcValue::Type::Blob: return (asBlob().empty());
	case RpcValue::Type::List: return (asList().empty());
	case RpcValue::Type::Map: return (asMap().empty());
	case RpcValue::Type::IMap: return (asIMap().empty());
	}
	return false;
}

void RpcValue::setDefaultValue()
{
	auto val = RpcValue::fromType(type());
	*this = val;
}

bool RpcValue::isValid() const
{
	return type() != Type::Invalid;
}

bool RpcValue::isNull() const
{
	return type() == Type::Null;
}

bool RpcValue::isInt() const
{
	return type() == Type::Int;
}

bool RpcValue::isUInt() const
{
	return type() == Type::UInt;
}

bool RpcValue::isDouble() const
{
	return type() == Type::Double;
}

bool RpcValue::isBool() const
{
	return type() == Type::Bool;
}

bool RpcValue::isString() const
{
	return type() == Type::String;
}

bool RpcValue::isBlob() const
{
	return type() == Type::Blob;
}

bool RpcValue::isDecimal() const
{
	return type() == Type::Decimal;
}

bool RpcValue::isDateTime() const
{
	return type() == Type::DateTime;
}

bool RpcValue::isList() const
{
	return type() == Type::List;
}

bool RpcValue::isMap() const
{
	return type() == Type::Map;
}

bool RpcValue::isIMap() const
{
	return type() == Type::IMap;
}

bool RpcValue::isValueNotAvailable() const
{
	return !isValid() || isNull();
}

double RpcValue::toDouble() const
{
	return std::visit([] (const auto& x) {
		using TypeX = std::remove_cvref_t<decltype(x)>;
		if constexpr (std::is_same<TypeX, Decimal>()) {
			return x.toDouble();
		} else if constexpr (std::is_same<TypeX, Double>() ||
							 std::is_same<TypeX, int64_t>() ||
							 std::is_same<TypeX, uint64_t>()) {
			return static_cast<double>(x);
		} else if constexpr (std::is_same<TypeX, RpcValue::Invalid>() ||
				   std::is_same<TypeX, RpcValue::Null>() ||
				   std::is_same<TypeX, RpcValue::DateTime>() ||
				   std::is_same<TypeX, RpcValue::Bool>() ||
				   std::is_same<TypeX, CowPtr<RpcValue::String>>() ||
				   std::is_same<TypeX, CowPtr<RpcValue::Blob>>() ||
				   std::is_same<TypeX, CowPtr<RpcValue::Map>>() ||
				   std::is_same<TypeX, CowPtr<RpcValue::IMap>>() ||
				   std::is_same<TypeX, CowPtr<RpcValue::List>>()) {
			return double{0};
		} else {
			static_assert(not_implemented_for_type<TypeX>, "toDouble not implemented for this type");
		}
	}, m_value);
}

namespace {
template <typename DesiredType>
const DesiredType& try_convert_or_default(const RpcValue::VariantType& value, const DesiredType& default_value)
{
	if (std::holds_alternative<DesiredType>(value)) {
		return std::get<DesiredType>(value);
	}

	return default_value;
}

#ifdef __ANDROID__
template <typename ResultType>
#else
template <std::integral ResultType>
#endif
ResultType impl_to_int(const RpcValue::VariantType& value)
{
	return std::visit([] (const auto& x) {
		using TypeX = std::remove_cvref_t<decltype(x)>;
		if constexpr (std::is_same<TypeX, int64_t>() ||
					  std::is_same<TypeX, uint64_t>() ||
					  std::is_same<TypeX, bool>()) {
			return static_cast<ResultType>(x);
		} else if constexpr (std::is_same<TypeX, double>()) {
			return static_cast<ResultType>(convert_to_int(x));
		} else if constexpr (std::is_same<TypeX, RpcValue::Decimal>()) {
			return static_cast<ResultType>(convert_to_int(x.toDouble()));
		} else if constexpr (std::is_same<TypeX, RpcValue::DateTime>()) {
			return static_cast<ResultType>(x.msecsSinceEpoch());
		} else if constexpr (std::is_same<TypeX, RpcValue::Invalid>() ||
				   std::is_same<TypeX, RpcValue::Null>() ||
				   std::is_same<TypeX, CowPtr<RpcValue::String>>() ||
				   std::is_same<TypeX, CowPtr<RpcValue::Blob>>() ||
				   std::is_same<TypeX, CowPtr<RpcValue::Map>>() ||
				   std::is_same<TypeX, CowPtr<RpcValue::IMap>>() ||
				   std::is_same<TypeX, CowPtr<RpcValue::List>>()) {
			return ResultType{0};
		} else {
			static_assert(not_implemented_for_type<TypeX>, "impl_to_int not implemented for this type");
		}
	}, value);
}
}

RpcValue::Decimal RpcValue::toDecimal() const
{
	return try_convert_or_default<RpcValue::Decimal>(m_value, Decimal{});
}

RpcValue::Int RpcValue::toInt() const
{
	return impl_to_int<RpcValue::Int>(m_value);
}

RpcValue::UInt RpcValue::toUInt() const
{
	return impl_to_int<RpcValue::UInt>(m_value);
}

int64_t RpcValue::toInt64() const
{
	return impl_to_int<int64_t>(m_value);
}

uint64_t RpcValue::toUInt64() const
{
	return impl_to_int<uint64_t>(m_value);
}

template<class T, template<class> class U>
inline constexpr bool is_instance_of_v = std::false_type{};

template<template<class> class U, class V>
inline constexpr bool is_instance_of_v<U<V>,U> = std::true_type{};

bool RpcValue::toBool() const
{
	return std::visit([] (const auto& x) {
		using TypeX = std::remove_cvref_t<decltype(x)>;
		if constexpr (std::is_same_v<TypeX, DateTime>) {
			return x.msecsSinceEpoch() != 0;
		} else if constexpr (std::is_same_v<TypeX, Decimal>) {
			return x.mantisa() != 0;
		} else if constexpr (std::is_arithmetic_v<TypeX>) {
			return x != 0;
		} else if constexpr (std::is_same_v<TypeX, bool>) {
			return x;
		} else if constexpr (std::is_same<TypeX, RpcValue::Invalid>() ||
							 std::is_same<TypeX, RpcValue::Null>() ||
							 std::is_same<TypeX, CowPtr<RpcValue::Blob>>() ||
							 std::is_same<TypeX, CowPtr<RpcValue::Map>>() ||
							 std::is_same<TypeX, CowPtr<RpcValue::IMap>>() ||
							 std::is_same<TypeX, CowPtr<RpcValue::List>>() ||
							 std::is_same<TypeX, CowPtr<RpcValue::String>>()) {
			return false;
		} else {
			static_assert(not_implemented_for_type<TypeX>, "toBool not implemented for this type");
		}
	}, m_value);
}

RpcValue::DateTime RpcValue::toDateTime() const
{
	return try_convert_or_default<RpcValue::DateTime>(m_value, {});
}

RpcValue::String RpcValue::toString() const
{
	return std::visit([this] (const auto& x) {
		using TypeX = std::remove_cvref_t<decltype(x)>;
		if constexpr (std::is_same<std::remove_cvref_t<decltype(x)>, CowPtr<Blob>>()) {
			return blobToString(*x);
		} else if constexpr (std::is_same<TypeX, CowPtr<RpcValue::String>>()) {
			return asString();
		} else if constexpr (std::is_same<TypeX, RpcValue::Invalid>() ||
				   std::is_same<TypeX, RpcValue::Null>() ||
				   std::is_arithmetic<TypeX>() ||
				   std::is_same<TypeX, RpcValue::DateTime>() ||
				   std::is_same<TypeX, RpcValue::Decimal>() ||
				   std::is_same<TypeX, CowPtr<RpcValue::Map>>() ||
				   std::is_same<TypeX, CowPtr<RpcValue::IMap>>() ||
				   std::is_same<TypeX, CowPtr<RpcValue::List>>()) {
			return std::string();
		} else {
			static_assert(not_implemented_for_type<TypeX>, "toString not implemented for this type");
		}
	}, m_value);
}

const RpcValue::String& RpcValue::asString() const
{
	return *try_convert_or_default<CowPtr<RpcValue::String>>(m_value, static_empty_string());
}

const RpcValue::Blob& RpcValue::asBlob() const
{
	return *try_convert_or_default<CowPtr<RpcValue::Blob>>(m_value, static_empty_blob());
}

const RpcValue::List& RpcValue::asList() const
{
	return *try_convert_or_default<CowPtr<RpcValue::List>>(m_value, static_empty_list());
}

const RpcValue::Map& RpcValue::asMap() const
{
	return *try_convert_or_default<CowPtr<RpcValue::Map>>(m_value, static_empty_map());
}

const RpcValue::IMap& RpcValue::asIMap() const
{
	return *try_convert_or_default<CowPtr<RpcValue::IMap>>(m_value, static_empty_imap());
}

std::pair<const uint8_t *, size_t> RpcValue::asBytes() const
{
	using Ret = std::pair<const uint8_t *, size_t>;
	if(type() == Type::Blob) {
		const Blob &blob = asBlob();
		return Ret(blob.data(), blob.size());
	}
	const String &s = asString();
	return Ret(reinterpret_cast<const uint8_t*>(s.data()), s.size());
}

std::pair<const char *, size_t> RpcValue::asData() const
{
	using Ret = std::pair<const char *, size_t>;
	if(type() == Type::Blob) {
		const Blob &blob = asBlob();
		return Ret(reinterpret_cast<const char*>(blob.data()), blob.size());
	}
	const String &s = asString();
	return Ret(s.data(), s.size());
}

size_t RpcValue::count() const
{
	return std::visit([] (const auto& x) {
		using TypeX = std::remove_cvref_t<decltype(x)>;
		if constexpr (std::is_same<TypeX, CowPtr<List>>() ||
					  std::is_same<TypeX, CowPtr<IMap>>() ||
					  std::is_same<TypeX, CowPtr<Map>>()) {
			return x->size();
		} else if constexpr (std::is_same<TypeX, RpcValue::Invalid>() ||
				   std::is_same<TypeX, RpcValue::Null>() ||
				   std::is_arithmetic<TypeX>() ||
				   std::is_same<TypeX, CowPtr<String>>() ||
				   std::is_same<TypeX, CowPtr<Blob>>() ||
				   std::is_same<TypeX, RpcValue::DateTime>() ||
				   std::is_same<TypeX, RpcValue::Decimal>()) {
			return size_t{0};
		} else {
			static_assert(not_implemented_for_type<TypeX>, "count() not implemented for this type");
		}
	}, m_value);
}

RpcValue RpcValue::at(RpcValue::Int ix, const RpcValue& default_value) const
{
	return std::visit([ix, &default_value] (const auto& x) mutable {
		using TypeX = std::remove_cvref_t<decltype(x)>;
		if constexpr (std::is_same<TypeX, CowPtr<List>>()) {
			if (ix < 0) {
				ix = static_cast<RpcValue::Int>(x->size()) + ix;
			}

			if (ix < 0 || ix >= static_cast<int>(x->size())) {
				return default_value;
			}

			return x->at(ix);
		} else if constexpr (std::is_same<TypeX, CowPtr<IMap>>()) {
			auto iter = x->find(ix);
			return (iter == x->end()) ? default_value : iter->second;
		} else if constexpr (std::is_same<TypeX, RpcValue::Invalid>() ||
				   std::is_same<TypeX, RpcValue::Null>() ||
				   std::is_arithmetic<TypeX>() ||
				   std::is_same<TypeX, CowPtr<String>>() ||
				   std::is_same<TypeX, CowPtr<Blob>>() ||
				   std::is_same<TypeX, CowPtr<Map>>() ||
				   std::is_same<TypeX, RpcValue::DateTime>() ||
				   std::is_same<TypeX, RpcValue::Decimal>()) {
			return default_value;
		} else {
			static_assert(not_implemented_for_type<TypeX>, "at(int) not implemented for this type");
		}
	}, m_value);
}

RpcValue RpcValue::at(const std::string& key, const RpcValue& default_value) const
{
	return std::visit([key, &default_value] (const auto& x) mutable {
		using TypeX = std::remove_cvref_t<decltype(x)>;
		if constexpr (std::is_same<TypeX, CowPtr<Map>>()) {
			auto iter = x->find(key);
			return (iter == x->end()) ? default_value : iter->second;
		} else if constexpr (std::is_same<TypeX, RpcValue::Invalid>() ||
				   std::is_same<TypeX, RpcValue::Null>() ||
				   std::is_arithmetic<TypeX>() ||
				   std::is_same<TypeX, CowPtr<String>>() ||
				   std::is_same<TypeX, CowPtr<Blob>>() ||
				   std::is_same<TypeX, CowPtr<IMap>>() ||
				   std::is_same<TypeX, CowPtr<List>>() ||
				   std::is_same<TypeX, RpcValue::DateTime>() ||
				   std::is_same<TypeX, RpcValue::Decimal>()) {
			return default_value;
		} else {
			static_assert(not_implemented_for_type<TypeX>, "at(const string&) not implemented for this type");
		}
	}, m_value);
}

namespace {
template <typename MapType, typename KeyType>
bool impl_has(const RpcValue::VariantType& value, const KeyType& key)
{
	return std::visit([key] (const auto& x) mutable {
		using TypeX = std::remove_cvref_t<decltype(x)>;
		if constexpr (std::is_same<TypeX, CowPtr<RpcValue::Map>>() ||
					  std::is_same<TypeX, CowPtr<RpcValue::IMap>>()) {
			if constexpr (std::is_same<TypeX, MapType>()) {
				auto iter = x->find(key);
				return iter != x->end();
			} else {
				return false;
			}
		} else if constexpr (std::is_same<TypeX, RpcValue::Invalid>() ||
				   std::is_same<TypeX, RpcValue::Null>() ||
				   std::is_arithmetic<TypeX>() ||
				   std::is_same<TypeX, CowPtr<RpcValue::String>>() ||
				   std::is_same<TypeX, CowPtr<RpcValue::Blob>>() ||
				   std::is_same<TypeX, CowPtr<RpcValue::List>>() ||
				   std::is_same<TypeX, RpcValue::DateTime>() ||
				   std::is_same<TypeX, RpcValue::Decimal>()) {
			return false;
		} else {
			static_assert(not_implemented_for_type<TypeX>, "impl_has not implemented for this type");
		}
	}, value);
}
}

bool RpcValue::has(RpcValue::Int ix) const
{
	return impl_has<CowPtr<IMap>>(m_value, ix);
}

bool RpcValue::has(const std::string& key) const
{
	return impl_has<CowPtr<Map>>(m_value, key);
}

std::string RpcValue::toStdString() const
{
	return std::visit([] (const auto& x) {
		using namespace std::string_literals;
		using TypeX = std::remove_cvref_t<decltype(x)>;
		if constexpr (std::is_same<TypeX, Double>() ||
					  std::is_same<TypeX, uint64_t>() ||
					  std::is_same<TypeX, int64_t>()) {
			return std::to_string(x);
		} else if constexpr (std::is_same<TypeX, DateTime>()) {
			return x.toIsoString();
		} else if constexpr (std::is_same<TypeX, CowPtr<Blob>>()) {
			return blobToString(*x);
		} else if constexpr (std::is_same<TypeX, Decimal>()) {
			return x.toString();
		} else if constexpr (std::is_same<TypeX, Bool>()) {
			return x ? "true"s : "false"s;
		} else if constexpr (std::is_same<TypeX, Null>()) {
			return "null"s;
		} else if constexpr (std::is_same<TypeX, CowPtr<String>>()) {
			return *x;
		} else if constexpr (std::is_same<TypeX, RpcValue::Invalid>() ||
							 std::is_same<TypeX, CowPtr<Map>>() ||
							 std::is_same<TypeX, CowPtr<IMap>>() ||
							 std::is_same<TypeX, CowPtr<List>>()) {
			return std::string();
		} else {
			static_assert(not_implemented_for_type<TypeX>, "toStdString not implemented for this type");
		}
	}, m_value);
}

namespace {
template <typename KeyType>
void impl_set(RpcValue::VariantType& map, const KeyType& key, const RpcValue& value)
{
	return std::visit([&key, &value] (auto& x) {
		using TypeX = std::remove_cvref_t<decltype(x)>;
		if constexpr ((std::is_same<TypeX, CowPtr<RpcValue::Map>>() && std::is_same<std::remove_cvref_t<KeyType>, std::string>()) ||
					  (std::is_same<TypeX, CowPtr<RpcValue::IMap>>() && std::is_same<std::remove_cvref_t<KeyType>, RpcValue::Int>())) {
			if (value.isValid()) {
				x->insert_or_assign(key, value);
			} else {
				x->erase(key);
			}
		} else if constexpr (std::is_same<TypeX, RpcValue::Invalid>() ||
				   std::is_same<TypeX, RpcValue::Null>() ||
				   std::is_arithmetic<TypeX>() ||
				   std::is_same<TypeX, CowPtr<RpcValue::Blob>>() ||
				   std::is_same<TypeX, CowPtr<RpcValue::IMap>>() ||
				   std::is_same<TypeX, CowPtr<RpcValue::List>>() ||
				   std::is_same<TypeX, CowPtr<RpcValue::Map>>() ||
				   std::is_same<TypeX, CowPtr<RpcValue::String>>() ||
				   std::is_same<TypeX, RpcValue::DateTime>() ||
				   std::is_same<TypeX, RpcValue::Decimal>()) {
			nError() << " Cannot set value to a non-map RpcValue! Key: " << key;
		} else {
			static_assert(not_implemented_for_type<TypeX>, "impl_set() not implemented for this type");
		}
	}, map);
}
}

void RpcValue::set(RpcValue::Int ix, const RpcValue &val)
{
	impl_set(m_value, ix, val);
}

void RpcValue::set(const RpcValue::String &key, const RpcValue &val)
{
	impl_set(m_value, key, val);
}

void RpcValue::append(const RpcValue &val)
{
	std::visit([&val] (auto& x) {
		using TypeX = std::remove_cvref_t<decltype(x)>;
		if constexpr (std::is_same<std::remove_cvref_t<decltype(x)>, CowPtr<RpcValue::List>>()) {
			x->emplace_back(val);
		} else if constexpr (std::is_same<TypeX, RpcValue::Invalid>() ||
				   std::is_same<TypeX, RpcValue::Null>() ||
				   std::is_arithmetic<TypeX>() ||
				   std::is_same<TypeX, CowPtr<RpcValue::IMap>>() ||
				   std::is_same<TypeX, CowPtr<RpcValue::Map>>() ||
				   std::is_same<TypeX, CowPtr<RpcValue::String>>() ||
				   std::is_same<TypeX, CowPtr<RpcValue::Blob>>() ||
				   std::is_same<TypeX, RpcValue::DateTime>() ||
				   std::is_same<TypeX, RpcValue::Decimal>()) {
			nError() << " Cannot set value to a non-list RpcValue!";
		} else {
			static_assert(not_implemented_for_type<TypeX>, "append() not implemented for this type");
		}
	}, m_value);
}

RpcValue::MetaData RpcValue::takeMeta()
{
	return *std::exchange(m_meta, nullptr);
}

std::string RpcValue::toPrettyString(const std::string &indent) const
{
	if(isValid()) {
		std::ostringstream out;
		{
			CponWriterOptions opts;
			opts.setTranslateIds(true).setIndent(indent);
			CponWriter wr(out, opts);
			wr << *this;
		}
		return out.str();
	}
	return "<invalid>";
}

std::string RpcValue::toCpon(const std::string &indent) const
{
	std::ostringstream out;
	{
		CponWriterOptions opts;
		opts.setTranslateIds(false).setIndent(indent);
		CponWriter wr(out, opts);
		wr << *this;
	}
	return out.str();
}

/* * * * * * * * * * * * * * * * * * * *
 * Comparison
 */
bool RpcValue::operator== (const RpcValue &other) const
{
	return std::visit([] (const auto& x, const auto& y) {
		using TypeX = std::remove_cvref_t<decltype(x)>;
		using TypeY = std::remove_cvref_t<decltype(y)>;
		if constexpr (std::is_same_v<TypeX, TypeY>) {
			if constexpr(is_instance_of_v<TypeX, CowPtr>) {
				return *x == *y;
			} else {
				return x == y;
			}
		} else if constexpr (std::is_arithmetic<TypeX>() && std::is_arithmetic<TypeY>()) {
			if constexpr (std::is_floating_point<TypeX>() || std::is_floating_point<TypeY>()) { // Double promotion.
				return static_cast<double>(x) == static_cast<double>(y);
			} else if constexpr (std::is_same_v<TypeX, bool> || std::is_same_v<TypeY, bool>) { // Bool conversion.
				return static_cast<bool>(x) == static_cast<bool>(y);
			} else { // Comparing int64_t and uint64_t
				// If the uint64_t is larger than int64_t_max, the comparison atumatically fails.
				if constexpr (std::is_same_v<TypeX, uint64_t>) {
					if (x > std::numeric_limits<int64_t>::max()) {
						return false;
					}
				} else {
					if (y > std::numeric_limits<int64_t>::max()) {
						return false;
					}
				}

				// If it's not larger, then we can safely convert and compare as int64_t.
				return static_cast<int64_t>(x) == static_cast<int64_t>(y);
			}
		} else if constexpr ((std::is_same_v<TypeX, Double> && std::is_same_v<TypeY, Decimal>) ||
							 (std::is_same_v<TypeX, Decimal> && std::is_same_v<TypeY, Double>)) {
			if constexpr (std::is_same_v<TypeX, Decimal>) {
				return x.toDouble() == y;
			} else {
				return y.toDouble() == x;
			}
		} else {
			return false;
		}

	}, m_value, other.m_value);
}

RpcValue RpcValue::fromCpon(const std::string &str, std::string *err)
{
	RpcValue ret;
	std::istringstream in(str);
	CponReader rd(in);
	if(err) {
		err->clear();
		try {
			rd >> ret;
			if(err)
				*err = std::string();
		}
		catch(ParseException &e) {
			if(err)
				*err = e.what();
		}
	}
	else {
		rd >> ret;
	}
	return ret;
}

RpcValue string_literals::operator""_cpon(const char* data, size_t size)
{
	return RpcValue::fromCpon(std::string{data, size});
}

RpcValue::Decimal::Num::Num()
	: exponent(-1)
{
}

RpcValue::Decimal::Num::Num(int64_t m, int e)
	: mantisa(m)
	, exponent(e)
{
}

std::string RpcValue::toChainPack() const
{
	std::ostringstream out;
	{
		ChainPackWriter wr(out);
		wr << *this;
	}
	return out.str();
}

RpcValue RpcValue::fromChainPack(const std::string &str, std::string *err)
{
	RpcValue ret;
	std::istringstream in(str);
	ChainPackReader rd(in);
	if(err) {
		err->clear();
		try {
			rd >> ret;
			if(err)
				*err = std::string();
		}
		catch(ParseException &e) {
			if(err)
				*err = e.what();
		}
	}
	else {
		rd >> ret;
	}
	return ret;
}

const char *RpcValue::typeToName(RpcValue::Type t)
{
	switch (t) {
	case Type::Invalid: return "INVALID";
	case Type::Null: return "Null";
	case Type::UInt: return "UInt";
	case Type::Int: return "Int";
	case Type::Double: return "Double";
	case Type::Bool: return "Bool";
	case Type::String: return "String";
	case Type::Blob: return "Blob";
	case Type::List: return "List";
	case Type::Map: return "Map";
	case Type::IMap: return "IMap";
	case Type::DateTime: return "DateTime";
	case Type::Decimal: return "Decimal";
	}
	return "UNKNOWN"; // just to remove mingw warning
}

const char* RpcValue::typeName() const
{
	return typeToName(type());
}

RpcValue::Type RpcValue::typeForName(const std::string &type_name, int len)
{
	using namespace std::string_view_literals;
	static const std::initializer_list<std::pair<std::string_view, RpcValue::Type>> mapping = {
		{"Null", Type::Null},
		{"UInt", Type::UInt},
		{"Int", Type::Int},
		{"Double", Type::Double},
		{"Bool", Type::Bool},
		{"String", Type::String},
		{"List", Type::List},
		{"Map", Type::Map},
		{"IMap", Type::IMap},
		{"DateTime", Type::DateTime},
		{"Decimal", Type::Decimal}
	};

	for (const auto& [strType, type] : mapping) {
		if (type_name.compare(0, (len < 0) ? strType.size() : static_cast<unsigned>(len), strType) == 0) {
			return type;
		}
	}

	return Type::Invalid;
}

namespace {
long long parse_ISO_DateTime(const std::string &s, std::tm &tm, int &msec, int64_t &msec_since_epoch, int &minutes_from_utc)
{
	ccpcp_unpack_context ctx;
	ccpcp_unpack_context_init(&ctx, s.data(), s.size(), nullptr, nullptr);
	ccpon_unpack_date_time(&ctx, &tm, &msec, &minutes_from_utc);
	if(ctx.err_no == CCPCP_RC_OK) {
		msec_since_epoch = ctx.item.as.DateTime.msecs_since_epoch;
		return ctx.current - ctx.start;
	}
	return 0;
}
}

RpcValue::DateTime::DateTime()
	: m_dtm{0, 0}
{
}

int64_t RpcValue::DateTime::msecsSinceEpoch() const
{
	return m_dtm.msec;
}

int RpcValue::DateTime::utcOffsetMin() const
{
	return m_dtm.tz * 15;
}

bool RpcValue::DateTime::isZero() const
{
	return msecsSinceEpoch() == 0;
}

RpcValue::DateTime RpcValue::DateTime::now()
{
	std::chrono::time_point<std::chrono::system_clock> p1 = std::chrono::system_clock::now();
	int64_t msecs = std::chrono::duration_cast<std::chrono:: milliseconds>(p1.time_since_epoch()).count();
	return fromMSecsSinceEpoch(msecs);
}

RpcValue::DateTime RpcValue::DateTime::fromLocalString(const std::string &local_date_time_str)
{
	std::tm tm;
	int msec;
	int64_t epoch_msec;
	int utc_offset;
	DateTime ret;
	if(!parse_ISO_DateTime(local_date_time_str, tm, msec, epoch_msec, utc_offset)) {
		nError() << "Invalid date time string:" << local_date_time_str;
		return ret;
	}

	std::time_t tim = std::mktime(&tm);
	std::time_t utc_tim = ccpon_timegm(&tm);
	if(tim < 0 || utc_tim < 0) {
		nError() << "Invalid date time string:" << local_date_time_str;
		return ret;
	}
	utc_offset = static_cast<int>((tim - utc_tim) / 60);
	epoch_msec = utc_tim * 60 * 1000 + msec;
	ret.setMsecsSinceEpoch(epoch_msec);
	ret.setUtcOffsetMin(utc_offset);

	return ret;
}

RpcValue::DateTime RpcValue::DateTime::fromUtcString(const std::string &utc_date_time_str, size_t *plen)
{
	if(utc_date_time_str.empty()) {
		if(plen)
			*plen = 0;
		return DateTime();
	}
	std::tm tm;
	int msec;
	int64_t epoch_msec;
	int utc_offset;
	DateTime ret;
	auto len = parse_ISO_DateTime(utc_date_time_str, tm, msec, epoch_msec, utc_offset);
	if(len == 0) {
		nInfo() << "Invalid date time string:" << utc_date_time_str;
		if(plen)
			*plen = 0;
		return ret;
	}
	ret.setMsecsSinceEpoch(epoch_msec);
	ret.setUtcOffsetMin(utc_offset);

	if(plen)
		*plen = static_cast<size_t>(len);

	return ret;
}

RpcValue::DateTime RpcValue::DateTime::fromMSecsSinceEpoch(int64_t msecs, int utc_offset_min)
{
	DateTime ret;
	ret.setMsecsSinceEpoch(msecs);
	ret.setUtcOffsetMin(utc_offset_min);
	return ret;
}

void RpcValue::DateTime::setMsecsSinceEpoch(int64_t msecs)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
	m_dtm.msec = msecs;
#pragma GCC diagnostic pop
}

void RpcValue::DateTime::setUtcOffsetMin(int utc_offset_min)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
	m_dtm.tz = (utc_offset_min / 15) & 0x7F;
#pragma GCC diagnostic pop
}

std::string RpcValue::DateTime::toLocalString() const
{
	std::time_t tim = m_dtm.msec / 1000 + m_dtm.tz * 15 * 60;
	std::tm *tm = std::localtime(&tim);
	if(tm == nullptr) {
		nError() << "Invalid date time: " << m_dtm.msec;
		return std::string();
	}
	std::array<char, 80> buffer;
	std::strftime(buffer.data(), buffer.size(),"%Y-%m-%dT%H:%M:%S",tm);
	std::string ret(buffer.data());
	int msecs = static_cast<int>(m_dtm.msec % 1000);
	if(msecs > 0)
		ret += '.' + std::to_string(msecs % 1000);
	return ret;
}

std::string RpcValue::DateTime::toIsoString() const
{
	return toIsoString(MsecPolicy::Auto, IncludeTimeZone);
}

std::string RpcValue::DateTime::toIsoString(RpcValue::DateTime::MsecPolicy msec_policy, bool include_tz) const
{
	ccpcp_pack_context ctx;
	std::array<char, 32> buff;
	ccpcp_pack_context_init(&ctx, buff.data(), buff.size(), nullptr);
	ccpon_pack_date_time_str(&ctx, msecsSinceEpoch(), utcOffsetMin(), static_cast<ccpon_msec_policy>(msec_policy), include_tz);
	return std::string(buff.data(), ctx.current);
}

RpcValue::DateTime::Parts::Parts() = default;

RpcValue::DateTime::Parts::Parts(int y, int m, int d, int h, int mn, int s, int ms)
	: year(y), month(m), day(d), hour(h), min(mn), sec(s), msec(ms)
{
}

bool RpcValue::DateTime::Parts::isValid() const
{
	return
	year >= 1970
	&& month >= 1 && month <= 12
	&& day >= 1 && day <= 31
	&& hour >= 0 && hour <= 59
	&& min >= 0 && min <= 59
	&& sec >= 0 && sec <= 59
	&& msec >= 0 && msec <= 999;
}

bool RpcValue::DateTime::Parts::operator==(const Parts &o) const
{
	return
	year == o.year
	&& month == o.month
	&& day == o.day
	&& hour == o.hour
	&& min == o.min
	&& sec == o.sec
	&& msec == o.msec;
}

RpcValue::DateTime::Parts RpcValue::DateTime::toParts() const
{
	struct tm tm;
	ccpon_gmtime(msecsSinceEpoch() / 1000 + utcOffsetMin() * 60, &tm);
	return Parts {tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, static_cast<int>(msecsSinceEpoch() % 1000)};
}

RpcValue::DateTime RpcValue::DateTime::fromParts(const Parts &parts)
{
	if (parts.isValid()) {
		struct tm tm;
		tm.tm_year = parts.year - 1900;
		tm.tm_mon = parts.month - 1;
		tm.tm_mday = parts.day;
		tm.tm_hour = parts.hour;
		tm.tm_min = parts.min;
		tm.tm_sec = parts.sec;
		auto msec = ccpon_timegm(&tm) * 1000 + parts.msec;
		return DateTime::fromMSecsSinceEpoch(msec);
	}
	return {};
}

bool RpcValue::DateTime::operator==(const DateTime &o) const
{
	return (m_dtm.msec == o.m_dtm.msec);
}

bool RpcValue::DateTime::operator<(const DateTime &o) const
{
	return m_dtm.msec < o.m_dtm.msec;
}

bool RpcValue::DateTime::operator>=(const DateTime &o) const
{
	return !(*this < o);
}

bool RpcValue::DateTime::operator>(const DateTime &o) const
{
	return m_dtm.msec > o.m_dtm.msec;
}

bool RpcValue::DateTime::operator<=(const DateTime &o) const
{
	return !(*this > o);
}


#ifdef DEBUG_RPCVAL
static int cnt = 0;
#endif
#ifdef DEBUG_RPCVAL
RpcValue::MetaData::MetaData()
{
	logDebugRpcVal() << ++cnt << "+++MM default" << this;
}
#else
RpcValue::MetaData::MetaData() = default;
#endif

RpcValue::MetaData::MetaData(const RpcValue::MetaData &o)
{
#ifdef DEBUG_RPCVAL
	logDebugRpcVal() << ++cnt << "+++MM copy" << this << "<------" << &o;
#endif
	m_imap = o.m_imap;
	m_smap = o.m_smap;
}

RpcValue::MetaData::MetaData(RpcValue::MetaData &&o) noexcept
	: MetaData()
{
#ifdef DEBUG_RPCVAL
	logDebugRpcVal() << ++cnt << "+++MM move" << this << "<------" << &o;
#endif
	swap(o);
}

RpcValue::MetaData::MetaData(RpcValue::IMap &&imap)
{
#ifdef DEBUG_RPCVAL
	logDebugRpcVal() << ++cnt << "+++MM move imap" << this;
#endif
	m_imap = std::move(imap);
}

RpcValue::MetaData::MetaData(RpcValue::Map &&smap)
{
#ifdef DEBUG_RPCVAL
	logDebugRpcVal() << ++cnt << "+++MM move smap" << this;
#endif
	m_smap = std::move(smap);
}

RpcValue::MetaData::MetaData(RpcValue::IMap &&imap, RpcValue::Map &&smap)
{
#ifdef DEBUG_RPCVAL
	logDebugRpcVal() << ++cnt << "+++MM move imap smap" << this;
#endif
	m_imap = std::move(imap);
	m_smap = std::move(smap);
}

RpcValue::MetaData &RpcValue::MetaData::operator=(RpcValue::MetaData &&o) noexcept
{
#ifdef DEBUG_RPCVAL
	logDebugRpcVal() << "===MM op= move ref" << this;
#endif
	swap(o);
	return *this;
}

RpcValue::MetaData &RpcValue::MetaData::operator=(const RpcValue::MetaData &o)
{
#ifdef DEBUG_RPCVAL
	logDebugRpcVal() << "===MM op= const ref" << this;
#endif
	if (this == &o) {
		return *this;
	}
	m_imap = o.m_imap;
	m_smap = o.m_smap;
	return *this;
}

int RpcValue::MetaData::metaTypeId() const
{
	return value(meta::Tag::MetaTypeId).toInt();
}

void RpcValue::MetaData::setMetaTypeId(RpcValue::Int id)
{
	setValue(meta::Tag::MetaTypeId, id);
}

int RpcValue::MetaData::metaTypeNameSpaceId() const
{
	return value(meta::Tag::MetaTypeNameSpaceId).toInt();
}

void RpcValue::MetaData::setMetaTypeNameSpaceId(RpcValue::Int id)
{
	setValue(meta::Tag::MetaTypeNameSpaceId, id);
}


std::vector<RpcValue::Int> RpcValue::MetaData::iKeys() const
{
	std::vector<RpcValue::Int> ret;
	for(const auto &it : iValues())
		ret.push_back(it.first);
	return ret;
}

std::vector<RpcValue::String> RpcValue::MetaData::sKeys() const
{
	std::vector<RpcValue::String> ret;
	for(const auto &it : sValues())
		ret.push_back(it.first);
	return ret;
}

bool RpcValue::MetaData::hasKey(RpcValue::Int key) const
{
	const IMap &m = iValues();
	auto it = m.find(key);
	return (it != m.end());
}

bool RpcValue::MetaData::hasKey(const RpcValue::String &key) const
{
	const Map &m = sValues();
	auto it = m.find(key);
	return (it != m.end());
}

RpcValue RpcValue::MetaData::value(RpcValue::Int key, const RpcValue &def_val) const
{
	const IMap &m = iValues();
	auto it = m.find(key);
	if(it != m.end())
		return it->second;
	return def_val;
}

RpcValue RpcValue::MetaData::value(const String &key, const RpcValue &def_val) const
{
	const Map &m = sValues();
	auto it = m.find(key);
	if(it != m.end())
		return it->second;
	return def_val;
}

const RpcValue &RpcValue::MetaData::valref(Int key) const
{
	const IMap &m = iValues();
	auto it = m.find(key);
	if(it != m.end())
		return it->second;
	static const RpcValue def_val;
	return def_val;
}

const RpcValue &RpcValue::MetaData::valref(const String &key) const
{
	const Map &m = sValues();
	auto it = m.find(key);
	if(it != m.end())
		return it->second;
	static const RpcValue def_val;
	return def_val;
}

void RpcValue::MetaData::setValue(RpcValue::Int key, const RpcValue &val)
{
	if(val.isValid()) {
		m_imap[key] = val;
	}
	else {
		m_imap.erase(key);
	}
}

void RpcValue::MetaData::setValue(const RpcValue::String &key, const RpcValue &val)
{
	if(val.isValid()) {
		m_smap[key] = val;
	}
	else {
		m_smap.erase(key);
	}
}

size_t RpcValue::MetaData::size() const
{
	return m_imap.size() + m_smap.size();
}

bool RpcValue::MetaData::isEmpty() const
{
	return size() == 0;
}

bool RpcValue::MetaData::operator==(const RpcValue::MetaData &o) const
{
	return iValues() == o.iValues() && sValues() == o.sValues();
}

const RpcValue::IMap &RpcValue::MetaData::iValues() const
{
	return m_imap;
}

const RpcValue::Map &RpcValue::MetaData::sValues() const
{
	return m_smap;
}

std::string RpcValue::MetaData::toPrettyString() const
{
	std::ostringstream out;
	{
		CponWriterOptions opts;
		opts.setTranslateIds(true);
		CponWriter wr(out, opts);
		wr << *this;
	}
	return out.str();
}

std::string RpcValue::MetaData::toString(const std::string &indent) const
{
	std::ostringstream out;
	{
		CponWriterOptions opts;
		opts.setTranslateIds(false);
		opts.setIndent(indent);
		CponWriter wr(out, opts);
		wr << *this;
	}
	return out.str();
}

//RpcValue::MetaData *RpcValue::MetaData::clone() const
//{
//	auto *md = new MetaData(*this);
//	return md;
//}

void RpcValue::MetaData::swap(RpcValue::MetaData &o) noexcept
{
	std::swap(m_imap, o.m_imap);
	std::swap(m_smap, o.m_smap);
}

RpcValue::Decimal::Decimal() = default;

RpcValue::Decimal::Decimal(int64_t mantisa, int exponent)
	: m_num{mantisa, exponent}
{
}

RpcValue::Decimal::Decimal(int dec_places)
	: Decimal(0, -dec_places)
{
}

int64_t RpcValue::Decimal::mantisa() const
{
	return m_num.mantisa;
}

int RpcValue::Decimal::exponent() const
{
	return m_num.exponent;
}

RpcValue::Decimal RpcValue::Decimal::fromDouble(double d, int round_to_dec_places)
{
	int exponent = -round_to_dec_places;
	if(round_to_dec_places > 0) {
		for(; round_to_dec_places > 0; round_to_dec_places--) d *= Base;
	}
	else if(round_to_dec_places < 0) {
		for(; round_to_dec_places < 0; round_to_dec_places++) d /= Base;
	}
	return Decimal(std::lround(d), exponent);
}

void RpcValue::Decimal::setDouble(double d)
{
	Decimal dc = fromDouble(d, -m_num.exponent);
	m_num.mantisa = dc.mantisa();
}

double RpcValue::Decimal::toDouble() const
{
	auto ret = static_cast<double>(mantisa());
	int exp = exponent();
	if(exp > 0)
		for(; exp > 0; exp--) ret *= Base;
	else
		for(; exp < 0; exp++) ret /= Base;
	return ret;
}

std::string RpcValue::Decimal::toString() const
{
	std::string ret = RpcValue(*this).toCpon();
	return ret;
}

RpcValue::String RpcValue::blobToString(const RpcValue::Blob &s, bool *check_utf8)
{
	(void)check_utf8;
	return String(s.begin(), s.end());
}

RpcValue::Blob RpcValue::stringToBlob(const RpcValue::String &s)
{
	return Blob(s.begin(), s.end());
}

RpcValue RpcValue::List::value(size_t ix) const
{
	if(ix >= size())
		return RpcValue();
	return operator [](ix);
}

const RpcValue& RpcValue::List::valref(size_t ix) const
{
	if(ix >= size()) {
		static RpcValue s;
		return s;
	}
	return operator [](ix);
}

RpcValue::List RpcValue::List::fromStringList(const std::vector<std::string> &sl)
{
	List ret;
	for(const std::string &s : sl)
		ret.push_back(s);
	return ret;
}

RpcValue RpcValue::Map::take(const String &key, const RpcValue &default_val)
{
	auto it = find(key);
	if(it == end())
		return default_val;
	auto ret = it->second;
	erase(it);
	return ret;
}

RpcValue RpcValue::Map::value(const String &key, const RpcValue &default_val) const
{
	auto it = find(key);
	if(it == end())
		return default_val;
	return it->second;
}

const RpcValue& RpcValue::Map::valref(const String &key) const
{
	auto it = find(key);
	if(it == end()) {
		static const auto s = RpcValue();
		return s;
	}
	return it->second;
}

void RpcValue::Map::setValue(const String &key, const RpcValue &val)
{
	if(val.isValid())
		(*this)[key] = val;
	else
		this->erase(key);
}

bool RpcValue::Map::hasKey(const String &key) const
{
	auto it = find(key);
	return !(it == end());
}

std::vector<RpcValue::String> RpcValue::Map::keys() const
{
	std::vector<String> ret;
	for(const auto &kv : *this)
		ret.push_back(kv.first);
	return ret;
}

RpcValue RpcValue::IMap::value(Int key, const RpcValue &default_val) const
{
	auto it = find(key);
	if(it == end())
		return default_val;
	return it->second;
}

const RpcValue& RpcValue::IMap::valref(Int key) const
{
	auto it = find(key);
	if(it == end()) {
		static const auto s = RpcValue();
		return s;
	}
	return it->second;
}

void RpcValue::IMap::setValue(Int key, const RpcValue &val)
{
	if(val.isValid())
		(*this)[key] = val;
	else
		this->erase(key);
}

bool RpcValue::IMap::hasKey(Int key) const
{
	auto it = find(key);
	return !(it == end());
}

std::vector<RpcValue::Int> RpcValue::IMap::keys() const
{
	std::vector<Int> ret;
	for(const auto &kv : *this)
		ret.push_back(kv.first);
	return ret;
}

RpcValueGenList::RpcValueGenList(const RpcValue &v)
	: m_val(v)
{
}

RpcValue RpcValueGenList::value(size_t ix) const
{
	if(m_val.isList())
		return m_val.asList().value(ix);
	if(ix == 0)
		return m_val;
	return RpcValue();
}
size_t RpcValueGenList::size() const
{
	if(m_val.isList())
		return m_val.asList().size();
	return m_val.isValid()? 1: 0;
}

bool RpcValueGenList::empty() const
{
	return size() == 0;
}

RpcValue::List RpcValueGenList::toList() const
{
	if(m_val.isList())
		return m_val.asList();
	return m_val.isValid()? RpcValue::List{m_val}: RpcValue::List{};
}
}


