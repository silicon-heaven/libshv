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
#include <optional>

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
inline NecroLog &operator<<(NecroLog log, const shv::chainpack::RpcDecimal &d) { return log.operator <<(d.toDouble()); }
inline NecroLog &operator<<(NecroLog log, const shv::chainpack::RpcValue::DateTime &d) { return log.operator <<(d.toIsoString()); }
inline NecroLog &operator<<(NecroLog log, const shv::chainpack::List &d) { return log.operator <<("some_list:" + std::to_string(d.size())); }
inline NecroLog &operator<<(NecroLog log, const shv::chainpack::RpcMap &d) { return log.operator <<("some_map:" + std::to_string(d.size())); }
inline NecroLog &operator<<(NecroLog log, const shv::chainpack::RpcIMap &d) { return log.operator <<("some_imap:" + std::to_string(d.size())); }
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

std::optional<int64_t> safeMul(int64_t a, int64_t b) {
#if defined(__GNUC__) || defined(__clang__)
	int64_t result;
	if (__builtin_mul_overflow(a, b, &result)) {
		return std::nullopt;
	}
	return result;

#elif defined(_MSC_VER)
	__int64 high;
	__int64 low = _mul128(a, b, &high);
	if (high != 0 && high != -1) {
		return std::nullopt;
	}
	return low;

#else
#error "compiler intrinsic for safeMul missing"
#endif
}

std::optional<int64_t> pow10(unsigned n) {
	switch (n) {
	case 0: return 1;
	case 1: return 10;
	case 2: return 100;
	case 3: return 1000;
	case 4: return 10000;
	case 5: return 100000;
	case 6: return 1000000;
	case 7: return 10000000;
	case 8: return 100000000;
	default: return std::nullopt;
	}
}
}

namespace {
const CowPtr<RpcValue::String>& static_empty_string() { static const CowPtr<RpcValue::String> s{std::make_shared<RpcValue::String>()}; return s; }
const CowPtr<RpcValue::Blob>& static_empty_blob() { static const CowPtr<RpcValue::Blob> s{std::make_shared<RpcValue::Blob>()}; return s; }
const CowPtr<RpcList>& static_empty_list() { static const CowPtr<RpcList> s{std::make_shared<RpcList>()}; return s; }
const CowPtr<RpcMap>& static_empty_map() { static const CowPtr<RpcMap> s{std::make_shared<RpcMap>()}; return s; }
const CowPtr<RpcIMap>& static_empty_imap() { static const CowPtr<RpcIMap> s{std::make_shared<RpcIMap>()}; return s; }
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
	case Type::List: return RpcValue{RpcList()};
	case Type::Map: return RpcValue{Map()};
	case Type::IMap: return RpcValue{IMap()};
	case Type::Decimal: return RpcValue{Decimal()};
	}
	return RpcValue();
}
RpcValue::RpcValue(std::nullptr_t) noexcept : m_value(Null{}) {}
RpcValue::RpcValue(double value) : m_value(value) {}
RpcValue::RpcValue(const RpcDecimal& value) : m_value(value) {}
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

RpcValue::RpcValue(const RpcList &values) : m_value(CowPtr{std::make_shared<RpcList>(values)}) {}
RpcValue::RpcValue(RpcList &&values) : m_value(CowPtr{std::make_shared<RpcList>(std::move(values))}) {}

RpcValue::RpcValue(const RpcMap &values) : m_value(CowPtr{std::make_shared<RpcMap>(values)}) {}
RpcValue::RpcValue(RpcMap &&values) : m_value(CowPtr{std::make_shared<RpcMap>(std::move(values))}) {}

RpcValue::RpcValue(const RpcIMap &values) : m_value(CowPtr{std::make_shared<RpcIMap>(values)}) {}
RpcValue::RpcValue(RpcIMap &&values) : m_value(CowPtr{std::make_shared<RpcIMap>(std::move(values))}) {}

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
const RpcMetaData &RpcValue::metaData() const
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

void RpcValue::setMetaData(RpcMetaData &&meta_data)
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
	case RpcValue::Type::Decimal: return (toDecimal().mantissa() == 0);
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
				   std::is_same<TypeX, CowPtr<RpcMap>>() ||
				   std::is_same<TypeX, CowPtr<RpcIMap>>() ||
				   std::is_same<TypeX, CowPtr<RpcList>>()) {
			return double{0};
		} else {
			static_assert(not_implemented_for_type<TypeX>, "toDouble not implemented for this type");
		}
	}, m_value);
}

namespace {
template <typename DefaultType>
decltype(auto) try_convert_or_default(const RpcValue::VariantType& value, const DefaultType& default_value)
{
	using DesiredType = std::decay_t<decltype(default_value)>;
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
							 std::is_same<TypeX, uint64_t>()) {
			return static_cast<ResultType>(x);
		} else if constexpr (std::is_same<TypeX, double>()) {
			return static_cast<ResultType>(convert_to_int(x));
		} else if constexpr (std::is_same<TypeX, RpcDecimal>()) {
			return static_cast<ResultType>(convert_to_int(x.toDouble()));
		} else if constexpr (std::is_same<TypeX, RpcValue::DateTime>()) {
			return static_cast<ResultType>(x.msecsSinceEpoch());
		} else if constexpr (std::is_same<TypeX, RpcValue::Invalid>() ||
				   std::is_same<TypeX, RpcValue::Null>() ||
				   std::is_same<TypeX, CowPtr<RpcValue::String>>() ||
				   std::is_same<TypeX, CowPtr<RpcValue::Blob>>() ||
				   std::is_same<TypeX, CowPtr<RpcMap>>() ||
				   std::is_same<TypeX, CowPtr<RpcIMap>>() ||
				   std::is_same<TypeX, CowPtr<RpcList>>() ||
				   std::is_same<TypeX, bool>()) { // Casting a bool to an int is prohibited.
			return ResultType{0};
		} else {
			static_assert(not_implemented_for_type<TypeX>, "impl_to_int not implemented for this type");
		}
	}, value);
}
}

RpcDecimal RpcValue::toDecimal() const
{
	return try_convert_or_default(m_value, Decimal{});
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
			return x.mantissa() != 0;
		} else if constexpr (std::is_arithmetic_v<TypeX>) {
			return x != 0;
		} else if constexpr (std::is_same_v<TypeX, bool>) {
			return x;
		} else if constexpr (std::is_same<TypeX, RpcValue::Invalid>() ||
							 std::is_same<TypeX, RpcValue::Null>() ||
							 std::is_same<TypeX, CowPtr<RpcValue::Blob>>() ||
							 std::is_same<TypeX, CowPtr<RpcMap>>() ||
							 std::is_same<TypeX, CowPtr<RpcIMap>>() ||
							 std::is_same<TypeX, CowPtr<RpcList>>() ||
							 std::is_same<TypeX, CowPtr<RpcValue::String>>()) {
			return false;
		} else {
			static_assert(not_implemented_for_type<TypeX>, "toBool not implemented for this type");
		}
	}, m_value);
}

RpcValue::DateTime RpcValue::toDateTime() const
{
	return try_convert_or_default(m_value, RpcValue::DateTime{});
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
				   std::is_same<TypeX, RpcDecimal>() ||
				   std::is_same<TypeX, CowPtr<RpcMap>>() ||
				   std::is_same<TypeX, CowPtr<RpcIMap>>() ||
				   std::is_same<TypeX, CowPtr<RpcList>>()) {
			return std::string();
		} else {
			static_assert(not_implemented_for_type<TypeX>, "toString not implemented for this type");
		}
	}, m_value);
}

const RpcValue::String& RpcValue::asString() const
{
	return *try_convert_or_default(m_value, static_empty_string());
}

const RpcValue::Blob& RpcValue::asBlob() const
{
	return *try_convert_or_default(m_value, static_empty_blob());
}

const RpcList& RpcValue::asList() const
{
	return *try_convert_or_default(m_value, static_empty_list());
}

const RpcMap& RpcValue::asMap() const
{
	return *try_convert_or_default(m_value, static_empty_map());
}

const RpcIMap& RpcValue::asIMap() const
{
	return *try_convert_or_default(m_value, static_empty_imap());
}

size_t RpcValue::count() const
{
	return std::visit([] (const auto& x) {
		using TypeX = std::remove_cvref_t<decltype(x)>;
		if constexpr (std::is_same<TypeX, CowPtr<RpcList>>() ||
					  std::is_same<TypeX, CowPtr<IMap>>() ||
					  std::is_same<TypeX, CowPtr<Map>>()) {
			return x->size();
		} else if constexpr (std::is_same<TypeX, RpcValue::Invalid>() ||
				   std::is_same<TypeX, RpcValue::Null>() ||
				   std::is_arithmetic<TypeX>() ||
				   std::is_same<TypeX, CowPtr<String>>() ||
				   std::is_same<TypeX, CowPtr<Blob>>() ||
				   std::is_same<TypeX, RpcValue::DateTime>() ||
				   std::is_same<TypeX, RpcDecimal>()) {
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
		if constexpr (std::is_same<TypeX, CowPtr<RpcList>>()) {
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
				   std::is_same<TypeX, RpcDecimal>()) {
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
				   std::is_same<TypeX, CowPtr<RpcList>>() ||
				   std::is_same<TypeX, RpcValue::DateTime>() ||
				   std::is_same<TypeX, RpcDecimal>()) {
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
		if constexpr (std::is_same<TypeX, CowPtr<RpcMap>>() ||
					  std::is_same<TypeX, CowPtr<RpcIMap>>()) {
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
				   std::is_same<TypeX, CowPtr<RpcList>>() ||
				   std::is_same<TypeX, RpcValue::DateTime>() ||
				   std::is_same<TypeX, RpcDecimal>()) {
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
							 std::is_same<TypeX, CowPtr<RpcList>>()) {
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
		if constexpr ((std::is_same<TypeX, CowPtr<RpcMap>>() && std::is_same<std::remove_cvref_t<KeyType>, std::string>()) ||
					  (std::is_same<TypeX, CowPtr<RpcIMap>>() && std::is_same<std::remove_cvref_t<KeyType>, RpcValue::Int>())) {
			if (value.isValid()) {
				x->insert_or_assign(key, value);
			} else {
				x->erase(key);
			}
		} else if constexpr (std::is_same<TypeX, RpcValue::Invalid>() ||
				   std::is_same<TypeX, RpcValue::Null>() ||
				   std::is_arithmetic<TypeX>() ||
				   std::is_same<TypeX, CowPtr<RpcValue::Blob>>() ||
				   std::is_same<TypeX, CowPtr<RpcIMap>>() ||
				   std::is_same<TypeX, CowPtr<RpcList>>() ||
				   std::is_same<TypeX, CowPtr<RpcMap>>() ||
				   std::is_same<TypeX, CowPtr<RpcValue::String>>() ||
				   std::is_same<TypeX, RpcValue::DateTime>() ||
				   std::is_same<TypeX, RpcDecimal>()) {
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
		if constexpr (std::is_same<std::remove_cvref_t<decltype(x)>, CowPtr<RpcList>>()) {
			x->emplace_back(val);
		} else if constexpr (std::is_same<TypeX, RpcValue::Invalid>() ||
				   std::is_same<TypeX, RpcValue::Null>() ||
				   std::is_arithmetic<TypeX>() ||
				   std::is_same<TypeX, CowPtr<RpcIMap>>() ||
				   std::is_same<TypeX, CowPtr<RpcMap>>() ||
				   std::is_same<TypeX, CowPtr<RpcValue::String>>() ||
				   std::is_same<TypeX, CowPtr<RpcValue::Blob>>() ||
				   std::is_same<TypeX, RpcValue::DateTime>() ||
				   std::is_same<TypeX, RpcDecimal>()) {
			nError() << " Cannot set value to a non-list RpcValue!";
		} else {
			static_assert(not_implemented_for_type<TypeX>, "append() not implemented for this type");
		}
	}, m_value);
}

RpcMetaData RpcValue::takeMeta()
{
	if (m_meta) {
		return *std::exchange(m_meta, nullptr);
	}
	return RpcMetaData();
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

RpcDecimal::Num::Num()
	: exponent(-1)
{
}

RpcDecimal::Num::Num(int64_t m, int e)
	: mantissa(m)
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

RpcValue RpcValue::fromChainPack(std::istream &in, std::string *err, const std::function<void(std::streamoff)>& progress_callback)
{
	RpcValue ret;
	ChainPackReader rd(in, progress_callback);
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

RpcValue RpcValue::fromChainPack(const std::string &str, std::string *err, const std::function<void(std::streamoff)>& progress_callback)
{
	auto in = std::istringstream(str);
	return fromChainPack(in, err, progress_callback);
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

RpcDateTime::RpcDateTime()
	: m_dtm{.tz = 0, .msec = 0}
{
}

int64_t RpcDateTime::msecsSinceEpoch() const
{
	return m_dtm.msec;
}

int RpcDateTime::utcOffsetMin() const
{
	return m_dtm.tz * 15;
}

RpcDateTime RpcDateTime::now()
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
	RpcDateTime ret;
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

RpcDateTime RpcDateTime::fromIsoString(const std::string &utc_date_time_str, size_t *plen)
{
	if(utc_date_time_str.empty()) {
		if(plen)
			*plen = 0;
		return RpcDateTime();
	}
	std::tm tm;
	int msec;
	int64_t epoch_msec;
	int utc_offset;
	RpcDateTime ret;
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

RpcDateTime RpcDateTime::fromMSecsSinceEpoch(int64_t msecs, int utc_offset_min)
{
	RpcDateTime ret;
	ret.setMsecsSinceEpoch(msecs);
	ret.setUtcOffsetMin(utc_offset_min);
	return ret;
}

void RpcDateTime::setMsecsSinceEpoch(int64_t msecs)
{
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif
	m_dtm.msec = msecs;
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
}

void RpcDateTime::setUtcOffsetMin(int utc_offset_min)
{
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif
	m_dtm.tz = (utc_offset_min / 15) & 0x7F;
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
}

std::string RpcDateTime::toLocalString() const
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

std::string RpcDateTime::toIsoString() const
{
	return toIsoString(MsecPolicy::Auto, IncludeTimeZone);
}

std::string RpcDateTime::toIsoString(RpcDateTime::MsecPolicy msec_policy, bool include_tz) const
{
	ccpcp_pack_context ctx;
	std::array<char, 32> buff;
	ccpcp_pack_context_init(&ctx, buff.data(), buff.size(), nullptr);
	ccpon_pack_date_time_str(&ctx, msecsSinceEpoch(), utcOffsetMin(), static_cast<ccpon_msec_policy>(msec_policy), include_tz);
	return std::string(buff.data(), ctx.current);
}

RpcDateTime::Parts::Parts() = default;

RpcDateTime::Parts::Parts(int y, int m, int d, int h, int mn, int s, int ms)
	: year(y), month(m), day(d), hour(h), min(mn), sec(s), msec(ms)
{
}

bool RpcDateTime::Parts::isValid() const
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

bool RpcDateTime::Parts::operator==(const Parts &o) const
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

RpcDateTime::Parts RpcDateTime::toParts() const
{
	struct tm tm;
	ccpon_gmtime(msecsSinceEpoch() / 1000 + utcOffsetMin() * 60, &tm);
	return Parts {tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, static_cast<int>(msecsSinceEpoch() % 1000)};
}

RpcDateTime RpcDateTime::fromParts(const Parts &parts)
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
		return RpcDateTime::fromMSecsSinceEpoch(msec);
	}
	return {};
}

bool RpcDateTime::operator==(const RpcDateTime &o) const
{
	return (m_dtm.msec == o.m_dtm.msec);
}

bool RpcDateTime::operator<(const RpcDateTime &o) const
{
	return m_dtm.msec < o.m_dtm.msec;
}

bool RpcDateTime::operator>=(const RpcDateTime &o) const
{
	return !(*this < o);
}

bool RpcDateTime::operator>(const RpcDateTime &o) const
{
	return m_dtm.msec > o.m_dtm.msec;
}

bool RpcDateTime::operator<=(const RpcDateTime &o) const
{
	return !(*this > o);
}


#ifdef DEBUG_RPCVAL
static int cnt = 0;
#endif
#ifdef DEBUG_RPCVAL
RpcMetaData::MetaData()
{
	logDebugRpcVal() << ++cnt << "+++MM default" << this;
}
#else
RpcMetaData::RpcMetaData() = default;
#endif

RpcMetaData::RpcMetaData(const RpcMetaData &o)
{
#ifdef DEBUG_RPCVAL
	logDebugRpcVal() << ++cnt << "+++MM copy" << this << "<------" << &o;
#endif
	m_imap = o.m_imap;
	m_smap = o.m_smap;
}

RpcMetaData::RpcMetaData(RpcMetaData &&o) noexcept
	: RpcMetaData()
{
#ifdef DEBUG_RPCVAL
	logDebugRpcVal() << ++cnt << "+++MM move" << this << "<------" << &o;
#endif
	swap(o);
}

RpcMetaData::RpcMetaData(RpcIMap &&imap)
{
#ifdef DEBUG_RPCVAL
	logDebugRpcVal() << ++cnt << "+++MM move imap" << this;
#endif
	m_imap = std::move(imap);
}

RpcMetaData::RpcMetaData(RpcMap &&smap)
{
#ifdef DEBUG_RPCVAL
	logDebugRpcVal() << ++cnt << "+++MM move smap" << this;
#endif
	m_smap = std::move(smap);
}

RpcMetaData::RpcMetaData(RpcIMap &&imap, RpcMap &&smap)
{
#ifdef DEBUG_RPCVAL
	logDebugRpcVal() << ++cnt << "+++MM move imap smap" << this;
#endif
	m_imap = std::move(imap);
	m_smap = std::move(smap);
}

RpcMetaData &RpcMetaData::operator=(RpcMetaData &&o) noexcept
{
#ifdef DEBUG_RPCVAL
	logDebugRpcVal() << "===MM op= move ref" << this;
#endif
	swap(o);
	return *this;
}

RpcMetaData &RpcMetaData::operator=(const RpcMetaData &o)
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

int RpcMetaData::metaTypeId() const
{
	return value(meta::Tag::MetaTypeId).toInt();
}

void RpcMetaData::setMetaTypeId(RpcValue::Int id)
{
	setValue(meta::Tag::MetaTypeId, id);
}

int RpcMetaData::metaTypeNameSpaceId() const
{
	return value(meta::Tag::MetaTypeNameSpaceId).toInt();
}

void RpcMetaData::setMetaTypeNameSpaceId(RpcValue::Int id)
{
	setValue(meta::Tag::MetaTypeNameSpaceId, id);
}


std::vector<RpcValue::Int> RpcMetaData::iKeys() const
{
	std::vector<RpcValue::Int> ret;
	for(const auto &it : iValues())
		ret.push_back(it.first);
	return ret;
}

std::vector<RpcValue::String> RpcMetaData::sKeys() const
{
	std::vector<RpcValue::String> ret;
	for(const auto &it : sValues())
		ret.push_back(it.first);
	return ret;
}

bool RpcMetaData::hasKey(RpcValue::Int key) const
{
	const RpcIMap &m = iValues();
	auto it = m.find(key);
	return (it != m.end());
}

bool RpcMetaData::hasKey(const RpcValue::String &key) const
{
	const RpcMap &m = sValues();
	auto it = m.find(key);
	return (it != m.end());
}

RpcValue RpcMetaData::value(RpcValue::Int key, const RpcValue &def_val) const
{
	const RpcIMap &m = iValues();
	auto it = m.find(key);
	if(it != m.end())
		return it->second;
	return def_val;
}

RpcValue RpcMetaData::value(const RpcValue::String &key, const RpcValue &def_val) const
{
	const RpcMap &m = sValues();
	auto it = m.find(key);
	if(it != m.end())
		return it->second;
	return def_val;
}

const RpcValue &RpcMetaData::valref(RpcValue::Int key) const
{
	const RpcIMap &m = iValues();
	auto it = m.find(key);
	if(it != m.end())
		return it->second;
	static const RpcValue def_val;
	return def_val;
}

const RpcValue &RpcMetaData::valref(const RpcValue::String &key) const
{
	const RpcMap &m = sValues();
	auto it = m.find(key);
	if(it != m.end())
		return it->second;
	static const RpcValue def_val;
	return def_val;
}

void RpcMetaData::setValue(RpcValue::Int key, const RpcValue &val)
{
	if(val.isValid()) {
		m_imap[key] = val;
	}
	else {
		m_imap.erase(key);
	}
}

void RpcMetaData::setValue(const RpcValue::String &key, const RpcValue &val)
{
	if(val.isValid()) {
		m_smap[key] = val;
	}
	else {
		m_smap.erase(key);
	}
}

size_t RpcMetaData::size() const
{
	return m_imap.size() + m_smap.size();
}

bool RpcMetaData::isEmpty() const
{
	return size() == 0;
}

bool RpcMetaData::operator==(const RpcMetaData &o) const
{
	return iValues() == o.iValues() && sValues() == o.sValues();
}

const RpcIMap &RpcMetaData::iValues() const
{
	return m_imap;
}

const RpcMap &RpcMetaData::sValues() const
{
	return m_smap;
}

std::string RpcMetaData::toPrettyString() const
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

std::string RpcMetaData::toString(const std::string &indent) const
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

//RpcMetaData *RpcMetaData::clone() const
//{
//	auto *md = new MetaData(*this);
//	return md;
//}

void RpcMetaData::swap(RpcMetaData &o) noexcept
{
	std::swap(m_imap, o.m_imap);
	std::swap(m_smap, o.m_smap);
}

RpcDecimal::RpcDecimal() = default;

RpcDecimal::RpcDecimal(int64_t mantissa, int exponent)
	: m_num{mantissa, exponent}
{
}

RpcDecimal::RpcDecimal(int dec_places)
	: RpcDecimal(0, -dec_places)
{
}

int64_t RpcDecimal::mantissa() const
{
	return m_num.mantissa;
}

int RpcDecimal::exponent() const
{
	return m_num.exponent;
}

RpcDecimal RpcDecimal::fromDouble(double d, int round_to_dec_places)
{
	int exponent = -round_to_dec_places;
	if(round_to_dec_places > 0) {
		for(; round_to_dec_places > 0; round_to_dec_places--) d *= Base;
	}
	else if(round_to_dec_places < 0) {
		for(; round_to_dec_places < 0; round_to_dec_places++) d /= Base;
	}
	return RpcDecimal(std::lround(d), exponent);
}

void RpcDecimal::setDouble(double d)
{
	RpcDecimal dc = fromDouble(d, -m_num.exponent);
	m_num.mantissa = dc.mantissa();
}

double RpcDecimal::toDouble() const
{
	auto ret = static_cast<double>(mantissa());
	int exp = exponent();
	if(exp > 0)
		for(; exp > 0; exp--) ret *= Base;
	else
		for(; exp < 0; exp++) ret /= Base;
	return ret;
}

std::string RpcDecimal::toString() const
{
	std::string ret = RpcValue(*this).toCpon();
	return ret;
}

RpcDecimal RpcDecimal::normalize(const RpcDecimal &d)
{
	if (d.mantissa() == 0) {
		return RpcDecimal(0, 0);	// canonical zero
	}

	int64_t m = d.mantissa();
	int e = d.exponent();

	while (m != 0 && (m % 10 == 0)) {
		m /= 10;
		++e;
	}
	return RpcDecimal(m, e);
}

std::strong_ordering RpcDecimal::operator<=>(const RpcDecimal& other) const
{
	RpcDecimal a = normalize(*this);
	RpcDecimal b = normalize(other);

	if (a.exponent() == b.exponent()) {
		return a.mantissa() <=> b.mantissa();
	}

	// We will scale the mantissa of the Decimal with the higher exponent.
	auto& to_scale = a.exponent() > b.exponent() ? a : b;

	auto exponent_diff = std::abs(a.exponent() - b.exponent());
	if (auto multiply_by = pow10(exponent_diff)) {
		if (auto scaled = safeMul(to_scale.mantissa(), multiply_by.value())) {
			to_scale.m_num.mantissa = scaled.value();
			return a.mantissa() <=> b.mantissa();
		}
	}

	auto da = a.toDouble();
	auto db = b.toDouble();

	if (da < db) {
		return std::strong_ordering::less;
	}
	if (da > db) {
		return std::strong_ordering::greater;
	}

	// We can't actually be sure if the RpcDecimals are equal here, because double does not have strong ordering.
	return std::strong_ordering::equal;
}

bool RpcDecimal::operator==(const RpcDecimal& other) const
{
	return operator<=>(other) == std::strong_ordering::equal;
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

RpcValue RpcList::value(size_t ix) const
{
	if(ix >= size())
		return RpcValue();
	return operator [](ix);
}

const RpcValue& RpcList::valref(size_t ix) const
{
	if(ix >= size()) {
		static RpcValue s;
		return s;
	}
	return operator [](ix);
}

RpcList RpcList::fromStringList(const std::vector<std::string> &sl)
{
	RpcList ret;
	for(const std::string &s : sl)
		ret.push_back(s);
	return ret;
}

RpcValue RpcMap::take(const RpcValue::String &key, const RpcValue &default_val)
{
	auto it = find(key);
	if(it == end())
		return default_val;
	auto ret = it->second;
	erase(it);
	return ret;
}

RpcValue RpcMap::value(const RpcValue::String &key, const RpcValue &default_val) const
{
	auto it = find(key);
	if(it == end())
		return default_val;
	return it->second;
}

const RpcValue& RpcMap::valref(const RpcValue::String &key) const
{
	auto it = find(key);
	if(it == end()) {
		static const auto s = RpcValue();
		return s;
	}
	return it->second;
}

void RpcMap::setValue(const RpcValue::String &key, const RpcValue &val)
{
	if(val.isValid())
		(*this)[key] = val;
	else
		this->erase(key);
}

bool RpcMap::hasKey(const RpcValue::String &key) const
{
	auto it = find(key);
	return !(it == end());
}

std::vector<RpcValue::String> RpcMap::keys() const
{
	std::vector<RpcValue::String> ret;
	for(const auto &kv : *this)
		ret.push_back(kv.first);
	return ret;
}

RpcValue RpcIMap::value(RpcValue::Int key, const RpcValue &default_val) const
{
	auto it = find(key);
	if(it == end())
		return default_val;
	return it->second;
}

const RpcValue& RpcIMap::valref(RpcValue::Int key) const
{
	auto it = find(key);
	if(it == end()) {
		static const auto s = RpcValue();
		return s;
	}
	return it->second;
}

void RpcIMap::setValue(RpcValue::Int key, const RpcValue &val)
{
	if(val.isValid())
		(*this)[key] = val;
	else
		this->erase(key);
}

bool RpcIMap::hasKey(RpcValue::Int key) const
{
	auto it = find(key);
	return !(it == end());
}

std::vector<RpcValue::Int> RpcIMap::keys() const
{
	std::vector<RpcValue::Int> ret;
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

RpcList RpcValueGenList::toList() const
{
	if(m_val.isList())
		return m_val.asList();
	return m_val.isValid()? RpcList{m_val}: RpcList{};
}
}


