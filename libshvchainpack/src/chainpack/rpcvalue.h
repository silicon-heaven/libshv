#pragma once

#include "../shvchainpackglobal.h"
#include "metatypes.h"

#include <stdexcept>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <initializer_list>

#ifndef CHAINPACK_UINT
	#define CHAINPACK_UINT unsigned
#endif

namespace shv::chainpack { class RpcValue; }

namespace shv {
namespace chainpack {

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
	typedef std::shared_ptr<T> RefPtr;

private:
	RefPtr m_sp;

	void detach()
	{
		T* tmp = m_sp.get();
		if( !( tmp == nullptr || m_sp.use_count() == 1 ) ) {
			m_sp = RefPtr( tmp->copy() );
		}
	}

public:
	CowPtr(T* t)
		:   m_sp(t)
	{}
	CowPtr(const RefPtr& refptr)
		:   m_sp(refptr)
	{}
	bool isNull() const
	{
		return m_sp.get() == nullptr;
	}
	long refCnt() const
	{
		return m_sp.use_count();
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
};

class SHVCHAINPACK_DECL_EXPORT RpcValue
{
public:
	class AbstractValueData;

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
	class SHVCHAINPACK_DECL_EXPORT Decimal
	{
		static constexpr int Base = 10;
		struct Num {
			int64_t mantisa = 0;
			int exponent = 0;

			Num();
			Num(int64_t m, int e);
		};
		Num m_num;
	public:
		Decimal();
		Decimal(int64_t mantisa, int exponent);
		Decimal(int dec_places);

		int64_t mantisa() const;
		int exponent() const;

		static Decimal fromDouble(double d, int round_to_dec_places);
		void setDouble(double d);
		double toDouble() const;
		std::string toString() const;
	};
	class SHVCHAINPACK_DECL_EXPORT DateTime
	{
	public:
		enum class MsecPolicy {Auto = 0, Always, Never};
		static constexpr bool IncludeTimeZone = true;
	public:
		DateTime();
		int64_t msecsSinceEpoch() const;
		int utcOffsetMin() const;
		/// @deprecated
		int minutesFromUtc() const;
		bool isZero() const;

		static DateTime now();
		static DateTime fromLocalString(const std::string &local_date_time_str);
		static DateTime fromUtcString(const std::string &utc_date_time_str, size_t *plen = nullptr);
		static DateTime fromMSecsSinceEpoch(int64_t msecs, int utc_offset_min = 0);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
		void setMsecsSinceEpoch(int64_t msecs);
		void setUtcOffsetMin(int utc_offset_min);
#pragma GCC diagnostic pop
		/// @deprecated
		void setTimeZone(int utc_offset_min);

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
		static DateTime fromParts(const Parts &parts);

		bool operator ==(const DateTime &o) const;
		bool operator <(const DateTime &o) const;
		bool operator >=(const DateTime &o) const;
		bool operator >(const DateTime &o) const;
		bool operator <=(const DateTime &o) const;
	private:
		struct MsTz {
			int64_t tz: 7, msec: 57;
		};
		MsTz m_dtm = {0, 0};
	};

	using String = std::string;
	using Blob = std::vector<uint8_t>;

	static String blobToString(const Blob &s, bool *check_utf8 = nullptr);
	static Blob stringToBlob(const String &s);

	class SHVCHAINPACK_DECL_EXPORT List : public std::vector<RpcValue>
	{
		using Super = std::vector<RpcValue>;
		using Super::Super; // expose base class constructors
	public:
		RpcValue value(size_t ix) const;
		const RpcValue& valref(size_t ix) const;
		static List fromStringList(const std::vector<std::string> &sl);
	};
	class SHVCHAINPACK_DECL_EXPORT Map : public std::map<String, RpcValue>
	{
		using Super = std::map<String, RpcValue>;
		using Super::Super; // expose base class constructors
	public:
		RpcValue take(const String &key, const RpcValue &default_val = RpcValue());
		RpcValue value(const String &key, const RpcValue &default_val = RpcValue()) const;
		const RpcValue& valref(const String &key) const;
		void setValue(const String &key, const RpcValue &val);
		bool hasKey(const String &key) const;
		std::vector<String> keys() const;
	};
	class SHVCHAINPACK_DECL_EXPORT IMap : public std::map<Int, RpcValue>
	{
		using Super = std::map<Int, RpcValue>;
		using Super::Super; // expose base class constructors
	public:
		RpcValue value(Int key, const RpcValue &default_val = RpcValue()) const;
		const RpcValue& valref(Int key) const;
		void setValue(Int key, const RpcValue &val);
		bool hasKey(Int key) const;
		std::vector<Int> keys() const;
	};

	class SHVCHAINPACK_DECL_EXPORT MetaData
	{
	public:
		MetaData();
		MetaData(const MetaData &o);
		MetaData(MetaData &&o) noexcept;
		MetaData(RpcValue::IMap &&imap);
		MetaData(RpcValue::Map &&smap);
		MetaData(RpcValue::IMap &&imap, RpcValue::Map &&smap);
		~MetaData();

		MetaData& operator =(MetaData &&o) noexcept;

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
		const RpcValue& valref(const String &key) const;
		void setValue(RpcValue::Int key, const RpcValue &val);
		void setValue(const RpcValue::String &key, const RpcValue &val);
		size_t size() const;
		bool isEmpty() const;
		bool operator==(const MetaData &o) const;
		const RpcValue::IMap& iValues() const;
		const RpcValue::Map& sValues() const;
		std::string toPrettyString() const;
		std::string toString(const std::string &indent = std::string()) const;

		MetaData* clone() const;
	private:
		MetaData& operator=(const MetaData &o);
		void swap(MetaData &o);
	private:
		RpcValue::IMap *m_imap = nullptr;
		RpcValue::Map *m_smap = nullptr;
	};

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
	RpcValue(const Decimal& value);     // Decimal
	RpcValue(const DateTime &value);

	RpcValue(const uint8_t *value, size_t size);
	RpcValue(const RpcValue::Blob &value); // String
	RpcValue(RpcValue::Blob &&value);      // String

	RpcValue(const std::string &value); // String
	RpcValue(std::string &&value);      // String
	RpcValue(const char *value);       // String
	RpcValue(const List &values);      // List
	RpcValue(List &&values);           // List
	RpcValue(const Map &values);     // Map
	RpcValue(Map &&values);          // Map
	RpcValue(const IMap &values);     // IMap
	RpcValue(IMap &&values);          // IMap

	// Implicit constructor: map-like objects (std::map, std::unordered_map, etc)
	template <class M, typename std::enable_if<
				  std::is_constructible<RpcValue::String, typename M::key_type>::value
				  && std::is_constructible<RpcValue, typename M::mapped_type>::value,
				  int>::type = 0>
	RpcValue(const M & m) : RpcValue(Map(m.begin(), m.end())) {}

	// Implicit constructor: vector-like objects (std::list, std::vector, std::set, etc)
	template <class V, typename std::enable_if<
				  std::is_constructible<RpcValue, typename V::value_type>::value,
				  int>::type = 0>
	RpcValue(const V & v) : RpcValue(List(v.begin(), v.end())) {}

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
	Decimal toDecimal() const;
	Int toInt() const;
	UInt toUInt() const;
	int64_t toInt64() const;
	uint64_t toUInt64() const;
	bool toBool() const;
	DateTime toDateTime() const;
	RpcValue::String toString() const;

	const RpcValue::String &asString() const;
	const RpcValue::Blob &asBlob() const;
	std::pair<const uint8_t*, size_t> asBytes() const;
	std::pair<const char*, size_t> asData() const;

	const List &asList() const;
	const Map &asMap() const;
	const IMap &asIMap() const;

	/// deprecated, new applications should us asString, asInt, ...
	[[deprecated("Use asList instead")]] const List &toList() const;
	[[deprecated("Use asMap instead")]] const Map &toMap() const;
	[[deprecated("Use asIMap instead")]] const IMap &toIMap() const;

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
		else if constexpr (std::is_same<T, DateTime>())
			return toDateTime();
		else if constexpr (std::is_same<T, Decimal>())
			return toDecimal();
		else if constexpr (std::is_same<T, RpcValue::List>())
			return asList();
	}

	size_t count() const;
	bool has(Int i) const;
	bool has(const RpcValue::String &key) const;
	RpcValue at(Int i) const;
	RpcValue at(Int i, const RpcValue &def_val) const;
	RpcValue at(const RpcValue::String &key) const;
	RpcValue at(const RpcValue::String &key, const RpcValue &def_val) const;
	void set(Int ix, const RpcValue &val);
	void set(const RpcValue::String &key, const RpcValue &val);
	void append(const RpcValue &val);

	RpcValue metaStripped() const;

	std::string toPrettyString(const std::string &indent = std::string()) const;
	std::string toStdString() const;
	std::string toCpon(const std::string &indent = std::string()) const;
	static RpcValue fromCpon(const std::string & str, std::string *err = nullptr);

	std::string toChainPack() const;
	static RpcValue fromChainPack(const std::string & str, std::string *err = nullptr);

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

	long refCnt() const;
private:
	CowPtr<AbstractValueData> m_ptr;
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

template<typename T> inline RpcValue RpcValue::fromValue(const T &t) { return RpcValue{t}; }

class SHVCHAINPACK_DECL_EXPORT RpcValueGenList
{
public:
	RpcValueGenList(const RpcValue &v);

	RpcValue value(size_t ix) const;
	bool size() const;
	bool empty() const;
	RpcValue::List toList() const;
private:
	RpcValue m_val;
};

}}
template<typename T> [[deprecated("Use RpcValue::to<>")]] T rpcvalue_cast(const shv::chainpack::RpcValue &v)
{
	return v.to<T>();
}
