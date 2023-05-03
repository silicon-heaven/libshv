#pragma once

#include "../shvchainpackglobal.h"
#include "exception.h"
#include "metatypes.h"

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <initializer_list>

#ifndef CHAINPACK_UINT
	#define CHAINPACK_UINT unsigned
#endif
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
		//Array,
		Map,
		IMap,
		Decimal,
		//MetaMap,
	};
	static const char* typeToName(Type t);
	const char* typeName() const { return typeToName(type()); }
	static Type typeForName(const std::string &type_name, int len = -1);

	using Int = int; //int64_t;
	using UInt = unsigned; //uint64_t;
	class SHVCHAINPACK_DECL_EXPORT Decimal
	{
		static constexpr int Base = 10;
		struct Num {
			int64_t mantisa = 0;
			int exponent = 0;

			Num() : mantisa(0), exponent(-1) {}
			Num(int64_t m, int e) : mantisa(m), exponent(e) {}
		};
		Num m_num;
	public:
		Decimal() {}
		Decimal(int64_t mantisa, int exponent) : m_num{mantisa, exponent} {}
		Decimal(int dec_places) : Decimal(0, -dec_places) {}

		int64_t mantisa() const {return m_num.mantisa;}
		int exponent() const {return m_num.exponent;}

		static Decimal fromDouble(double d, int round_to_dec_places)
		{
			int exponent = -round_to_dec_places;
			if(round_to_dec_places > 0) {
				for(; round_to_dec_places > 0; round_to_dec_places--) d *= Base;
			}
			else if(round_to_dec_places < 0) {
				for(; round_to_dec_places < 0; round_to_dec_places++) d /= Base;
			}
			return Decimal(static_cast<int64_t>(d + 0.5), exponent);
		}
		void setDouble(double d)
		{
			Decimal dc = fromDouble(d, -m_num.exponent);
			m_num.mantisa = dc.mantisa();
		}
		double toDouble() const
		{
			double ret = mantisa();
			int exp = exponent();
			if(exp > 0)
				for(; exp > 0; exp--) ret *= Base;
			else
				for(; exp < 0; exp++) ret /= Base;
			return ret;
		}
		//bool isValid() const {return !(mantisa() == 0 && exponent() != 0);}
		std::string toString() const;
	};
	class SHVCHAINPACK_DECL_EXPORT DateTime
	{
	public:
		enum class MsecPolicy {Auto = 0, Always, Never};
		static constexpr bool IncludeTimeZone = true;
	public:
		DateTime() : m_dtm{0, 0} {}
		int64_t msecsSinceEpoch() const { return m_dtm.msec; }
		int utcOffsetMin() const { return m_dtm.tz * 15; }
		/// @deprecated
		int minutesFromUtc() const { return utcOffsetMin(); }
		bool isZero() const {return msecsSinceEpoch() == 0;}

		static DateTime now();
		static DateTime fromLocalString(const std::string &local_date_time_str);
		static DateTime fromUtcString(const std::string &utc_date_time_str, size_t *plen = nullptr);
		static DateTime fromMSecsSinceEpoch(int64_t msecs, int utc_offset_min = 0);

		void setMsecsSinceEpoch(int64_t msecs) { m_dtm.msec = msecs; }
		void setUtcOffsetMin(int utc_offset_min) {m_dtm.tz = utc_offset_min / 15;}
		/// @deprecated
		void setTimeZone(int utc_offset_min) {setUtcOffsetMin(utc_offset_min);}

		std::string toLocalString() const;
		std::string toIsoString() const {return toIsoString(MsecPolicy::Auto, IncludeTimeZone);}
		std::string toIsoString(MsecPolicy msec_policy, bool include_tz) const;

		bool operator ==(const DateTime &o) const { return (m_dtm.msec == o.m_dtm.msec); }
		bool operator <(const DateTime &o) const { return m_dtm.msec < o.m_dtm.msec; }
		bool operator >=(const DateTime &o) const { return !(*this < o); }
		bool operator >(const DateTime &o) const { return m_dtm.msec > o.m_dtm.msec; }
		bool operator <=(const DateTime &o) const { return !(*this > o); }
	private:
		struct MsTz {
			int64_t tz: 7, msec: 57;
		};
		MsTz m_dtm = {0, 0};
	};

	using String = std::string;
	using Blob = std::vector<uint8_t>;

	// maybe in future I'll find a way how to do this without allocation
	//static String blobToString(Blob &&s, bool *check_utf8 = nullptr);
	static String blobToString(const Blob &s, bool *check_utf8 = nullptr);
	static Blob stringToBlob(const String &s);

	class List : public std::vector<RpcValue>
	{
		using Super = std::vector<RpcValue>;
		using Super::Super; // expose base class constructors
	public:
		RpcValue value(size_t ix) const
		{
			if(ix >= size())
				return RpcValue();
			return operator [](ix);
		}
		const RpcValue& valref(size_t ix) const
		{
			if(ix >= size()) {
				static RpcValue s;
				return s;
			}
			return operator [](ix);
		}
		static List fromStringList(const std::vector<std::string> &sl)
		{
			List ret;
			for(const std::string &s : sl)
				ret.push_back(s);
			return ret;
		}
	};
	class Map : public std::map<String, RpcValue>
	{
		using Super = std::map<String, RpcValue>;
		using Super::Super; // expose base class constructors
	public:
		RpcValue value(const String &key, const RpcValue &default_val = RpcValue()) const
		{
			auto it = find(key);
			if(it == end())
				return default_val;
			return it->second;
		}
		const RpcValue& valref(const String &key) const
		{
			auto it = find(key);
			if(it == end()) {
				static const auto s = RpcValue();
				return s;
			}
			return it->second;
		}
		void setValue(const String &key, const RpcValue &val)
		{
			if(val.isValid())
				(*this)[key] = val;
			else
				this->erase(key);
		}
		bool hasKey(const String &key) const
		{
			auto it = find(key);
			return !(it == end());
		}
		std::vector<String> keys() const
		{
			std::vector<String> ret;
			for(const auto &kv : *this)
				ret.push_back(kv.first);
			return ret;
		}
	};
	class IMap : public std::map<Int, RpcValue>
	{
		using Super = std::map<Int, RpcValue>;
		using Super::Super; // expose base class constructors
	public:
		RpcValue value(Int key, const RpcValue &default_val = RpcValue()) const
		{
			auto it = find(key);
			if(it == end())
				return default_val;
			return it->second;
		}
		const RpcValue& valref(Int key) const
		{
			auto it = find(key);
			if(it == end()) {
				static const auto s = RpcValue();
				return s;
			}
			return it->second;
		}
		void setValue(Int key, const RpcValue &val)
		{
			if(val.isValid())
				(*this)[key] = val;
			else
				this->erase(key);
		}
		bool hasKey(Int key) const
		{
			auto it = find(key);
			return !(it == end());
		}
		std::vector<Int> keys() const
		{
			std::vector<Int> ret;
			for(const auto &kv : *this)
				ret.push_back(kv.first);
			return ret;
		}
	};

	class SHVCHAINPACK_DECL_EXPORT MetaData
	{
	public:
		MetaData();
		MetaData(const MetaData &o);
		MetaData(MetaData &&o);
		MetaData(RpcValue::IMap &&imap);
		MetaData(RpcValue::Map &&smap);
		MetaData(RpcValue::IMap &&imap, RpcValue::Map &&smap);
		~MetaData();

		MetaData& operator =(MetaData &&o);

		int metaTypeId() const {return value(meta::Tag::MetaTypeId).toInt();}
		void setMetaTypeId(RpcValue::Int id) {setValue(meta::Tag::MetaTypeId, id);}
		int metaTypeNameSpaceId() const {return value(meta::Tag::MetaTypeNameSpaceId).toInt();}
		void setMetaTypeNameSpaceId(RpcValue::Int id) {setValue(meta::Tag::MetaTypeNameSpaceId, id);}
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

	RpcValue(int32_t value);                // Int
	RpcValue(uint32_t value);                // UInt
	RpcValue(int64_t value);                // Int
	RpcValue(uint64_t value);                // UInt
	RpcValue(double value);             // Double
	RpcValue(Decimal value);             // Decimal
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

	// Implicit constructor: anything with a toRpcValue() function.
	// dangerous
	//template <class T, class = decltype(&T::toRpcValue)>
	//RpcValue(const T & t) : RpcValue(t.toRpcValue()) {}

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
	int metaTypeId() const {return metaValue(meta::Tag::MetaTypeId).toInt();}
	int metaTypeNameSpaceId() const {return metaValue(meta::Tag::MetaTypeNameSpaceId).toInt();}
	void setMetaTypeId(int id) {setMetaValue(meta::Tag::MetaTypeId, id);}
	void setMetaTypeId(int ns, int id) {setMetaValue(meta::Tag::MetaTypeNameSpaceId, ns); setMetaValue(meta::Tag::MetaTypeId, id);}

	bool isDefaultValue() const;
	void setDefaultValue();

	bool isValid() const;
	bool isNull() const { return type() == Type::Null; }
	bool isInt() const { return type() == Type::Int; }
	bool isUInt() const { return type() == Type::UInt; }
	bool isDouble() const { return type() == Type::Double; }
	bool isBool() const { return type() == Type::Bool; }
	bool isString() const { return type() == Type::String; }
	bool isBlob() const { return type() == Type::Blob; }
	bool isDecimal() const { return type() == Type::Decimal; }
	bool isDateTime() const { return type() == Type::DateTime; }
	bool isList() const { return type() == Type::List; }
	bool isMap() const { return type() == Type::Map; }
	bool isIMap() const { return type() == Type::IMap; }
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
	const List &toList() const { return asList(); }
	const Map &toMap() const { return asMap(); }
	const IMap &toIMap() const { return asIMap(); }

	size_t count() const;
	bool has(Int i) const;
	bool has(const RpcValue::String &key) const;
	RpcValue at(Int i) const;
	RpcValue at(Int i, const RpcValue &def_val) const  { return has(i)? at(i): def_val; }
	RpcValue at(const RpcValue::String &key) const;
	RpcValue at(const RpcValue::String &key, const RpcValue &def_val) const  { return has(key)? at(key): def_val; }
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
	//static constexpr bool CloneMetaData = true;
	//RpcValue clone(bool clone_meta_data = CloneMetaData) const;

	bool operator== (const RpcValue &rhs) const;
	bool operator!= (const RpcValue &rhs) const {return !operator==(rhs);}
#ifdef RPCVALUE_COPY_AND_SWAP
	RpcValue& operator= (RpcValue rhs) noexcept
	{
		swap(rhs);
		return *this;
	}
	void swap(RpcValue& other) noexcept;
#endif
	/*
	bool operator<  (const ChainPack &rhs) const;
	bool operator!= (const ChainPack &rhs) const { return !(*this == rhs); }
	bool operator<= (const ChainPack &rhs) const { return !(rhs < *this); }
	bool operator>  (const ChainPack &rhs) const { return  (rhs < *this); }
	bool operator>= (const ChainPack &rhs) const { return !(*this < rhs); }
	*/
	template<typename T> static inline Type guessType();
	template<typename T> static inline RpcValue fromValue(const T &t);

	long refCnt() const { return m_ptr.refCnt();}
private:
	CowPtr<AbstractValueData> m_ptr;
};

template<typename T> RpcValue::Type RpcValue::guessType() { throw std::runtime_error("guessing of this type is not implemented"); }
template<> inline RpcValue::Type RpcValue::guessType<RpcValue::Int>() { return Type::Int; }
template<> inline RpcValue::Type RpcValue::guessType<RpcValue::UInt>() { return Type::UInt; }
template<> inline RpcValue::Type RpcValue::guessType<uint16_t>() { return Type::UInt; }
template<> inline RpcValue::Type RpcValue::guessType<bool>() { return Type::Bool; }
template<> inline RpcValue::Type RpcValue::guessType<RpcValue::DateTime>() { return Type::DateTime; }
template<> inline RpcValue::Type RpcValue::guessType<RpcValue::Decimal>() { return Type::Decimal; }

template<typename T> inline RpcValue RpcValue::fromValue(const T &t) { return RpcValue{t}; }

class RpcValueGenList
{
public:
	RpcValueGenList(const RpcValue &v) : m_val(v) {}

	RpcValue value(size_t ix) const
	{
		if(m_val.isList())
			return m_val.toList().value(ix);
		else if(ix == 0)
			return m_val;
		return RpcValue();
	}
	bool size() const
	{
		if(m_val.isList())
			return m_val.toList().size();
		return m_val.isValid()? 1: 0;
	}
	bool empty() const {return size() == 0;}
	RpcValue::List toList() const
	{
		if(m_val.isList())
			return m_val.toList();
		return m_val.isValid()? RpcValue::List{m_val}: RpcValue::List{};
	}
private:
	RpcValue m_val;
};

}}

template<typename T> inline T rpcvalue_cast(const shv::chainpack::RpcValue &v)
{
	//static_assert(false, "Cannot cast RpcValue type.");
	return T{v.toString()};
}

template<> inline shv::chainpack::RpcValue rpcvalue_cast<shv::chainpack::RpcValue>(const shv::chainpack::RpcValue &v) { return v; }
template<> inline bool rpcvalue_cast<bool>(const shv::chainpack::RpcValue &v) { return v.toBool(); }
template<> inline shv::chainpack::RpcValue::Int rpcvalue_cast<shv::chainpack::RpcValue::Int>(const shv::chainpack::RpcValue &v) { return v.toInt(); }
template<> inline shv::chainpack::RpcValue::UInt rpcvalue_cast<shv::chainpack::RpcValue::UInt>(const shv::chainpack::RpcValue &v) { return v.toUInt(); }
template<> inline shv::chainpack::RpcValue::String rpcvalue_cast<shv::chainpack::RpcValue::String>(const shv::chainpack::RpcValue &v) { return v.asString(); }
template<> inline shv::chainpack::RpcValue::DateTime rpcvalue_cast<shv::chainpack::RpcValue::DateTime>(const shv::chainpack::RpcValue &v) { return v.toDateTime(); }
template<> inline shv::chainpack::RpcValue::Decimal rpcvalue_cast<shv::chainpack::RpcValue::Decimal>(const shv::chainpack::RpcValue &v) { return v.toDecimal(); }
