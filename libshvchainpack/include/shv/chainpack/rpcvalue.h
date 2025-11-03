#pragma once

#include <shv/chainpack/shvchainpackglobal.h>
#include <shv/chainpack/metatypes.h>

#include <functional>
#include <ios>
#include <stdexcept>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <variant>

#ifndef CHAINPACK_UINT
	#define CHAINPACK_UINT unsigned
#endif

namespace shv::chainpack { class RpcValue; }
template <class ...> static constexpr std::false_type not_implemented_for_type [[maybe_unused]] {};

namespace shv::chainpack {

/// This implementation of copy-on-write is generic, but apart from the inconvenience of having to refer to the inner object through smart pointer dereferencing,
/// it suffers from at least one drawback: classes that return references to their internal state, like
///
/// char & String::operator[](int)
///
/// can lead to unexpected behaviour.
template <class T>
class CowPtr
{
public:
private:
	std::shared_ptr<T> m_sp;

	void detach()
	{
		T* tmp = m_sp.get();
		if( !( tmp == nullptr || m_sp.use_count() == 1 ) ) {
			m_sp = std::make_shared<T>(*tmp);
		}
	}

public:
	CowPtr(T* t)
		:   m_sp(t)
	{}
	CowPtr(const std::shared_ptr<T>& refptr)
		:   m_sp(refptr)
	{}
	bool isNull() const
	{
		return m_sp.get() == nullptr;
	}
	const T& operator*() const
	{
		return *m_sp;
	}
	T& operator*()
	{
		detach();
		return *m_sp;
	}
	const T* operator->() const
	{
		return m_sp.operator->();
	}
	T* operator->()
	{
		detach();
		return m_sp.operator->();
	}
	explicit operator bool() const noexcept
	{
		return m_sp.operator bool();
	}
};

class RpcList;

class SHVCHAINPACK_DECL_EXPORT RpcDateTime
{
public:
	enum class MsecPolicy {Auto = 0, Always, Never};
	static constexpr bool IncludeTimeZone = true;
public:
	RpcDateTime();
	int64_t msecsSinceEpoch() const;
	int utcOffsetMin() const;

	static RpcDateTime now();
	static RpcDateTime fromLocalString(const std::string &local_date_time_str);
	static RpcDateTime fromUtcString(const std::string &utc_date_time_str, size_t *plen = nullptr);
	static RpcDateTime fromMSecsSinceEpoch(int64_t msecs, int utc_offset_min = 0);

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif
	void setMsecsSinceEpoch(int64_t msecs);
	void setUtcOffsetMin(int utc_offset_min);
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

	std::string toLocalString() const;
	std::string toIsoString() const;
	std::string toIsoString(MsecPolicy msec_policy, bool include_tz) const;

	struct SHVCHAINPACK_DECL_EXPORT Parts
	{
		int year = 0;
		int month = 0; // 1-12
		int day = 0; // 1-31
		int hour = 0; // 0-23
		int min = 0; // 0-59
		int sec = 0; // 0-59
		int msec = 0; // 0-999

		Parts();
		Parts(int y, int m, int d, int h = 0, int mn = 0, int s = 0, int ms = 0);

		bool isValid() const;
		bool operator==(const Parts &o) const;
	};
	Parts toParts() const;
	static RpcDateTime fromParts(const Parts &parts);

	bool operator ==(const RpcDateTime &o) const;
	bool operator <(const RpcDateTime &o) const;
	bool operator >=(const RpcDateTime &o) const;
	bool operator >(const RpcDateTime &o) const;
	bool operator <=(const RpcDateTime &o) const;
private:
	struct MsTz {
		int64_t tz: 7, msec: 57;
	};
	MsTz m_dtm = {.tz = 0, .msec = 0};
};

class SHVCHAINPACK_DECL_EXPORT RpcDecimal
{
	static constexpr int Base = 10;
	struct Num {
		int64_t mantissa = 0;
		int exponent = 0;

		Num();
		Num(int64_t m, int e);
		bool operator==(const Num&) const = default;
	};
	Num m_num;
public:
	RpcDecimal();
	RpcDecimal(int64_t mantissa, int exponent);
	RpcDecimal(int dec_places);
	explicit RpcDecimal(double d);

	[[deprecated]] int64_t mantisa() const { return mantissa(); }
	int64_t mantissa() const;
	int exponent() const;

	static RpcDecimal fromDouble(double d, int round_to_dec_places);
	void setDouble(double d);
	double toDouble() const;
	std::string toString() const;

	static RpcDecimal normalize(const RpcDecimal &d);

	bool operator==(const RpcDecimal &other) const;
	bool operator!=(const RpcDecimal &other) const;
	bool operator<(const RpcDecimal &other) const;
	bool operator>(const RpcDecimal &other) const;
	bool operator<=(const RpcDecimal &other) const;
	bool operator>=(const RpcDecimal &other) const;
};

class RpcMap;
class RpcIMap;
class RpcMetaData;

class SHVCHAINPACK_DECL_EXPORT RpcValue
{
public:

	enum class Type {
		Invalid,
		Null,
		UInt,
		Int,
		Double,
		Bool,
		Blob, //binary string
		String, // UTF8 string
		DateTime,
		List,
		Map,
		IMap,
		Decimal,
	};
	static const char* typeToName(Type t);
	const char* typeName() const;
	static Type typeForName(const std::string &type_name, int len = -1);

	using Int = int; //int64_t;
	using UInt = unsigned; //uint64_t;
	using Double = double;
	using Bool = bool;
	using List = RpcList;
	using DateTime = RpcDateTime;
	using Decimal = RpcDecimal;

	using String = std::string;
	using Blob = std::vector<uint8_t>;

	static String blobToString(const Blob &s, bool *check_utf8 = nullptr);
	static Blob stringToBlob(const String &s);

	using Map = RpcMap;
	using IMap = RpcIMap;
	using MetaData = RpcMetaData;

	// Constructors for the various types of JSON value.
	RpcValue() noexcept;                // Invalid
#ifdef RPCVALUE_COPY_AND_SWAP
	RpcValue(const RpcValue &other) noexcept : m_ptr(other.m_ptr) {}
	RpcValue(RpcValue &&other) noexcept : RpcValue() { swap(other); }
#endif
	RpcValue(std::nullptr_t) noexcept;  // Null
	RpcValue(bool value);               // Bool

	RpcValue(short value);              // Int
	RpcValue(int value);                // Int
	RpcValue(long value);               // Int
	RpcValue(long long value);          // Int
	RpcValue(unsigned short value);     // UInt
	RpcValue(unsigned int value);       // UInt
	RpcValue(unsigned long value);      // UInt
	RpcValue(unsigned long long value); // UInt
	RpcValue(double value);             // Double
	RpcValue(const RpcDecimal& value);     // Decimal
	RpcValue(const RpcDateTime &value);

	RpcValue(const uint8_t *value, size_t size);
	RpcValue(const RpcValue::Blob &value); // String
	RpcValue(RpcValue::Blob &&value);      // String

	RpcValue(const std::string &value); // String
	RpcValue(std::string &&value);      // String
	RpcValue(const char *value);       // String
	RpcValue(const RpcList &values);      // List
	RpcValue(RpcList &&values);           // List
	RpcValue(const Map &values);     // Map
	RpcValue(Map &&values);          // Map
	RpcValue(const IMap &values);     // IMap
	RpcValue(IMap &&values);          // IMap

	// Implicit constructor: map-like objects (std::map, std::unordered_map, etc)
	template <class M, std::enable_if_t<
				  std::is_constructible_v<RpcValue::String, typename M::key_type>
				  && std::is_constructible_v<RpcValue, typename M::mapped_type>,
				  int> = 0>
	RpcValue(const M & m) : RpcValue(Map(m.begin(), m.end())) {}

	// Implicit constructor: vector-like objects (std::list, std::vector, std::set, etc)
	template <class V, std::enable_if_t<
				  std::is_constructible_v<RpcValue, typename V::value_type>,
				  int> = 0>
	RpcValue(const V & v) : RpcValue(RpcList(v.begin(), v.end())) {}

	// This prevents RpcValue(some_pointer) from accidentally producing a bool. Use
	// RpcValue(bool(some_pointer)) if that behavior is desired.
	RpcValue(void *) = delete;

	Type type() const;
	static RpcValue fromType(RpcValue::Type t) noexcept;

	const MetaData &metaData() const;
	RpcValue metaValue(RpcValue::Int key, const RpcValue &default_value = RpcValue()) const;
	RpcValue metaValue(const RpcValue::String &key, const RpcValue &default_value = RpcValue()) const;
	void setMetaData(MetaData &&meta_data);
	void setMetaValue(Int key, const RpcValue &val);
	void setMetaValue(const String &key, const RpcValue &val);
	int metaTypeId() const;
	int metaTypeNameSpaceId() const;
	void setMetaTypeId(int id);
	void setMetaTypeId(int ns, int id);

	bool isDefaultValue() const;
	void setDefaultValue();

	bool isValid() const;
	bool isNull() const;
	bool isInt() const;
	bool isUInt() const;
	bool isDouble() const;
	bool isBool() const;
	bool isString() const;
	bool isBlob() const;
	bool isDecimal() const;
	bool isDateTime() const;
	bool isList() const;
	bool isMap() const;
	bool isIMap() const;
	bool isValueNotAvailable() const;

	double toDouble() const;
	RpcDecimal toDecimal() const;
	Int toInt() const;
	UInt toUInt() const;
	int64_t toInt64() const;
	uint64_t toUInt64() const;
	bool toBool() const;
	RpcDateTime toDateTime() const;
	RpcValue::String toString() const;

	const RpcValue::String &asString() const;
	const RpcValue::Blob &asBlob() const;

	const RpcList &asList() const;
	const Map &asMap() const;
	const IMap &asIMap() const;

	template<typename T> T to() const
	{
		if constexpr (std::is_same<T, bool>())
			return toBool();
		else if constexpr (std::is_same<T, RpcValue>())
			return *this;
		else if constexpr (std::is_same<T, Int>())
			return toInt();
		else if constexpr (std::is_same<T, UInt>())
			return toUInt();
		else if constexpr (std::is_same<T, String>())
			return asString();
		else if constexpr (std::is_same<T, RpcDateTime>())
			return toDateTime();
		else if constexpr (std::is_same<T, RpcDecimal>())
			return toDecimal();
		else if constexpr (std::is_same<T, RpcList>())
			return asList();
		else
			static_assert(not_implemented_for_type<T>, "RpcValue::to<T> is not implemented for this type (maybe you're missing an include?)");
	}

	template<typename T> bool is() const
	{
		if constexpr (std::is_same<T, bool>())
			return isBool();
		else if constexpr (std::is_same<T, Int>())
			return isInt();
		else if constexpr (std::is_same<T, UInt>())
			return isUInt();
		else if constexpr (std::is_same<T, String>())
			return isString();
		else if constexpr (std::is_same<T, RpcDateTime>())
			return isDateTime();
		else if constexpr (std::is_same<T, RpcDecimal>())
			return isDecimal();
		else if constexpr (std::is_same<T, RpcList>())
			return isList();
		else
			static_assert(not_implemented_for_type<T>, "RpcValue::is<T> is not implemented for this type");
	}

	size_t count() const;
	bool has(Int i) const;
	bool has(const RpcValue::String &key) const;
	RpcValue at(Int i, const RpcValue &def_val = RpcValue{}) const;
	RpcValue at(const RpcValue::String &key, const RpcValue &def_val = RpcValue{}) const;
	void set(Int ix, const RpcValue &val);
	void set(const RpcValue::String &key, const RpcValue &val);
	void append(const RpcValue &val);

	MetaData takeMeta();

	std::string toPrettyString(const std::string &indent = std::string()) const;
	std::string toStdString() const;
	std::string toCpon(const std::string &indent = std::string()) const;
	static RpcValue fromCpon(const std::string & str, std::string *err = nullptr);

	std::string toChainPack() const;
	static RpcValue fromChainPack(const std::string & str, std::string *err = nullptr, const std::function<void(std::streamoff)>& progress_callback = nullptr);
	static RpcValue fromChainPack(std::istream& in, std::string* err = nullptr, const std::function<void(std::streamoff)>& progress_callback = nullptr);

	bool operator== (const RpcValue &rhs) const;
#ifdef RPCVALUE_COPY_AND_SWAP
	RpcValue& operator= (RpcValue rhs) noexcept
	{
		swap(rhs);
		return *this;
	}
	void swap(RpcValue& other) noexcept;
#endif
	template<typename T> static inline Type guessType();
	template<typename T> static inline RpcValue fromValue(const T &t);

	struct Invalid {
		bool operator==(const Invalid&) const = default;
	};
	struct Null {
		bool operator==(const Null&) const = default;
	};

	using VariantType = std::variant<RpcValue::Invalid, RpcValue::Null, uint64_t, int64_t, RpcValue::Double, RpcValue::Bool, CowPtr<RpcValue::Blob>, CowPtr<RpcValue::String>, RpcValue::DateTime, CowPtr<RpcList>, CowPtr<RpcValue::Map>, CowPtr<RpcValue::IMap>, RpcValue::Decimal>;
private:
	CowPtr<MetaData> m_meta = nullptr;
	VariantType m_value;
};

class SHVCHAINPACK_DECL_EXPORT RpcList : public std::vector<RpcValue>
{
	using Super = std::vector<RpcValue>;
	using Super::Super; // expose base class constructors
public:
	RpcValue value(size_t ix) const;
	const RpcValue& valref(size_t ix) const;
	static RpcList fromStringList(const std::vector<std::string> &sl);
};

class SHVCHAINPACK_DECL_EXPORT RpcMap : public std::map<RpcValue::String, RpcValue>
{
	using Super = std::map<std::string, RpcValue>;
	using Super::Super; // expose base class constructors
public:
	RpcValue take(const RpcValue::String &key, const RpcValue &default_val = RpcValue());
	RpcValue value(const RpcValue::String &key, const RpcValue &default_val = RpcValue()) const;
	const RpcValue& valref(const RpcValue::String &key) const;
	void setValue(const RpcValue::String &key, const RpcValue &val);
	bool hasKey(const RpcValue::String &key) const;
	std::vector<RpcValue::String> keys() const;
};

class SHVCHAINPACK_DECL_EXPORT RpcIMap : public std::map<RpcValue::Int, RpcValue>
{
	using Super = std::map<RpcValue::Int, RpcValue>;
	using Super::Super; // expose base class constructors
public:
	RpcValue value(RpcValue::Int key, const RpcValue &default_val = RpcValue()) const;
	const RpcValue& valref(RpcValue::Int key) const;
	void setValue(RpcValue::Int key, const RpcValue &val);
	bool hasKey(RpcValue::Int key) const;
	std::vector<RpcValue::Int> keys() const;
};

class SHVCHAINPACK_DECL_EXPORT RpcMetaData
{
public:
	RpcMetaData();
	RpcMetaData(const RpcMetaData &o);
	RpcMetaData(RpcMetaData &&o) noexcept;
	RpcMetaData(RpcValue::IMap &&imap);
	RpcMetaData(RpcValue::Map &&smap);
	RpcMetaData(RpcValue::IMap &&imap, RpcValue::Map &&smap);
	~RpcMetaData() = default;

	RpcMetaData& operator=(const RpcMetaData &o);
	RpcMetaData& operator =(RpcMetaData &&o) noexcept;

	int metaTypeId() const;
	void setMetaTypeId(RpcValue::Int id);
	int metaTypeNameSpaceId() const;
	void setMetaTypeNameSpaceId(RpcValue::Int id);
	std::vector<RpcValue::Int> iKeys() const;
	std::vector<RpcValue::String> sKeys() const;
	bool hasKey(RpcValue::Int key) const;
	bool hasKey(const RpcValue::String &key) const;
	RpcValue value(RpcValue::Int key, const RpcValue &def_val = RpcValue()) const;
	RpcValue value(const RpcValue::String &key, const RpcValue &def_val = RpcValue()) const;
	const RpcValue& valref(RpcValue::Int key) const;
	const RpcValue& valref(const RpcValue::String &key) const;
	void setValue(RpcValue::Int key, const RpcValue &val);
	void setValue(const RpcValue::String &key, const RpcValue &val);
	size_t size() const;
	bool isEmpty() const;
	bool operator==(const RpcMetaData &o) const;
	const RpcValue::IMap& iValues() const;
	const RpcValue::Map& sValues() const;
	std::string toPrettyString() const;
	std::string toString(const std::string &indent = std::string()) const;

	//MetaData* clone() const;
private:
	void swap(RpcMetaData &o) noexcept;
private:
	RpcValue::IMap m_imap;
	RpcValue::Map m_smap;
};

namespace string_literals {
SHVCHAINPACK_DECL_EXPORT RpcValue operator""_cpon(const char* data, size_t size);
}

template<typename T> RpcValue::Type RpcValue::guessType() { throw std::runtime_error("guessing of this type is not implemented"); }
template<> inline RpcValue::Type RpcValue::guessType<RpcValue::Int>() { return Type::Int; }
template<> inline RpcValue::Type RpcValue::guessType<RpcValue::UInt>() { return Type::UInt; }
template<> inline RpcValue::Type RpcValue::guessType<uint16_t>() { return Type::UInt; }
template<> inline RpcValue::Type RpcValue::guessType<bool>() { return Type::Bool; }
template<> inline RpcValue::Type RpcValue::guessType<RpcValue::DateTime>() { return Type::DateTime; }
template<> inline RpcValue::Type RpcValue::guessType<RpcValue::Decimal>() { return Type::Decimal; }
template<> inline RpcValue::Type RpcValue::guessType<RpcValue::String>() { return Type::String; }

template<typename T> inline RpcValue RpcValue::fromValue(const T &t) { return RpcValue{t}; }

class SHVCHAINPACK_DECL_EXPORT RpcValueGenList
{
public:
	RpcValueGenList(const RpcValue &v);

	RpcValue value(size_t ix) const;
	size_t size() const;
	bool empty() const;
	RpcList toList() const;
private:
	RpcValue m_val;
};

}
