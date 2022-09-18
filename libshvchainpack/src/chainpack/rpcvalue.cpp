#include "rpcvalue.h"

#include "cponwriter.h"
#include "cponreader.h"
#include "chainpackwriter.h"
#include "chainpackreader.h"
#include "exception.h"
#include "utils.h"

#include "../../c/ccpon.h"

#include <necrolog.h>

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <sstream>
#include <iostream>
#include <chrono>

#ifdef DEBUG_RPCVAL
#define logDebugRpcVal nWarning
#else
#define logDebugRpcVal if(0) nWarning
#endif
/*
namespace {
#if defined _WIN32 || defined LIBC_NEWLIB
// see http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap04.html#tag_04_15
// see https://stackoverflow.com/questions/16647819/timegm-cross-platform
int is_leap(unsigned y)
{
	y += 1900;
	return (y % 4) == 0 && ((y % 100) != 0 || (y % 400) == 0);
}

time_t timegm(struct tm *tm)
{
	static const unsigned ndays[2][12] = {
		{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
		{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
	};
	time_t res = 0;
	int i;

	for (i = 70; i < tm->tm_year; ++i) {
		res += is_leap(i) ? 366 : 365;
	}

	for (i = 0; i < tm->tm_mon; ++i) {
		res += ndays[is_leap(tm->tm_year)][i];
	}
	res += tm->tm_mday - 1;
	res *= 24;
	res += tm->tm_hour;
	res *= 60;
	res += tm->tm_min;
	res *= 60;
	res += tm->tm_sec;
	return res;
}
#endif
} // namespace
*/
namespace shv {
namespace chainpack {

class RpcValue::AbstractValueData
{
public:
	virtual ~AbstractValueData() {}

	virtual RpcValue::Type type() const {return RpcValue::Type::Invalid;}

	virtual const RpcValue::MetaData &metaData() const = 0;
	virtual void setMetaData(RpcValue::MetaData &&meta_data) = 0;
	virtual void setMetaValue(RpcValue::Int key, const RpcValue &val) = 0;
	virtual void setMetaValue(RpcValue::String key, const RpcValue &val) = 0;

	virtual bool equals(const AbstractValueData * other) const = 0;
	//virtual bool less(const Data * other) const = 0;

	virtual bool isNull() const {return false;}
	virtual double toDouble() const {return 0;}
	virtual RpcValue::Decimal toDecimal() const { return RpcValue::Decimal{}; }
	virtual RpcValue::Int toInt() const {return 0;}
	virtual RpcValue::UInt toUInt() const {return 0;}
	virtual int64_t toInt64() const {return 0;}
	virtual uint64_t toUInt64() const {return 0;}
	virtual bool toBool() const {return false;}
	virtual RpcValue::DateTime toDateTime() const { return RpcValue::DateTime{}; }
	virtual const RpcValue::String &asString() const;
	virtual const RpcValue::Blob &asBlob() const;
	virtual const RpcValue::List &asList() const;
	virtual const RpcValue::Map &asMap() const;
	virtual const RpcValue::IMap &asIMap() const;
	virtual size_t count() const {return 0;}

	virtual bool has(RpcValue::Int i) const { (void)i; return false; }
	virtual bool has(const RpcValue::String &key) const { (void)key; return false; }
	virtual RpcValue at(RpcValue::Int i) const { (void)i; return RpcValue(); }
	virtual RpcValue at(const RpcValue::String &key) const { (void)key; return RpcValue(); }
	virtual void set(RpcValue::Int ix, const RpcValue &val);
	virtual void set(const RpcValue::String &key, const RpcValue &val);
	virtual void append(const RpcValue &);

	virtual std::string toStdString() const = 0;
	virtual void stripMeta() = 0;

	virtual AbstractValueData* copy() = 0;
};

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

template <RpcValue::Type tag, typename T>
class ValueData : public RpcValue::AbstractValueData
{
protected:
	explicit ValueData(const T &value)
		: m_value(value)
	{
#ifdef DEBUG_RPCVAL
		logDebugRpcVal() << "+++" << ++value_data_cnt << RpcValue::typeToName(tag) << this << value;
#endif
	}
	//explicit ValueData(T &&value) : m_value(std::move(value)) {}
	// disable copy (because of m_metaData)
	ValueData(const ValueData &o) = delete;
	ValueData& operator=(const ValueData &o) = delete;
	//ValueData(ValueData &&o) = delete;
	//ValueData& operator=(ValueData &&o) = delete;
	virtual ~ValueData() override
	{
#ifdef DEBUG_RPCVAL
		logDebugRpcVal() << "---" << value_data_cnt-- << RpcValue::typeToName(tag) << this << m_value;
#endif
		if(m_metaData)
			delete m_metaData;
	}

	RpcValue::Type type() const override { return tag; }

	const RpcValue::MetaData &metaData() const override
	{
		static RpcValue::MetaData md;
		if(!m_metaData)
			return md;
		return *m_metaData;
	}

	void setMetaData(RpcValue::MetaData &&d) override
	{
		if(m_metaData)
			(*m_metaData) = std::move(d);
		else
			m_metaData = new RpcValue::MetaData(std::move(d));
	}

	void setMetaValue(RpcValue::Int key, const RpcValue &val) override
	{
		if(!m_metaData)
			m_metaData = new RpcValue::MetaData();
		m_metaData->setValue(key, val);
	}
	void setMetaValue(RpcValue::String key, const RpcValue &val) override
	{
		if(!m_metaData)
			m_metaData = new RpcValue::MetaData();
		m_metaData->setValue(key, val);
	}

	virtual ValueData<tag, T>* create() = 0;
	AbstractValueData* copy() override
	{
		return copy_helper(this);
	}
	AbstractValueData* copy_helper(ValueData<tag, T> *orig)
	{
		ValueData<tag, T> *tmp = create();
		if(m_metaData) {
			tmp->m_metaData = orig->metaData().clone();
		}
		tmp->m_value = orig->m_value;
		return tmp;
	}

	void stripMeta() override
	{
		if(m_metaData)
			delete m_metaData;
		m_metaData = nullptr;
	}

protected:
	T m_value;
	RpcValue::MetaData *m_metaData = nullptr;
};

class ChainPackDouble final : public ValueData<RpcValue::Type::Double, double>
{
	ChainPackDouble* create() override { return new ChainPackDouble(0); }
	std::string toStdString() const override { return shv::chainpack::Utils::toString(m_value); }
	double toDouble() const override { return m_value; }
	bool toBool() const override { return !(m_value == 0.); }
	RpcValue::Int toInt() const override { return static_cast<RpcValue::Int>(m_value); }
	RpcValue::UInt toUInt() const override { return static_cast<RpcValue::UInt>(m_value); }
	int64_t toInt64() const override { return static_cast<int64_t>(m_value); }
	uint64_t toUInt64() const override { return static_cast<uint64_t>(m_value); }
	bool equals(const RpcValue::AbstractValueData * other) const override {
		return m_value == other->toDouble();
	}
	//bool less(const Data * other) const override { return m_value < other->toDouble(); }
public:
	explicit ChainPackDouble(double value) : ValueData(value) {}
};

class ChainPackDecimal final : public ValueData<RpcValue::Type::Decimal, RpcValue::Decimal>
{
	ChainPackDecimal* create() override { return new ChainPackDecimal(RpcValue::Decimal()); }
	std::string toStdString() const override { return m_value.toString(); }

	double toDouble() const override { return m_value.toDouble(); }
	bool toBool() const override { return !(m_value.mantisa() == 0); }
	RpcValue::Int toInt() const override { return static_cast<RpcValue::Int>(m_value.toDouble()); }
	RpcValue::UInt toUInt() const override { return static_cast<RpcValue::UInt>(m_value.toDouble()); }
	int64_t toInt64() const override { return static_cast<int64_t>(m_value.toDouble()); }
	uint64_t toUInt64() const override { return static_cast<uint64_t>(m_value.toDouble()); }
	RpcValue::Decimal toDecimal() const override { return m_value; }
	bool equals(const RpcValue::AbstractValueData * other) const override { return toDouble() == other->toDouble(); }
	//bool less(const Data * other) const override { return m_value < other->toDouble(); }
public:
	explicit ChainPackDecimal(RpcValue::Decimal &&value) : ValueData(std::move(value)) {}
};

class ChainPackInt final : public ValueData<RpcValue::Type::Int, int64_t>
{
	ChainPackInt* create() override { return new ChainPackInt(0); }
	std::string toStdString() const override { return Utils::toString(m_value); }

	double toDouble() const override { return m_value; }
	bool toBool() const override { return !(m_value == 0); }
	RpcValue::Int toInt() const override { return static_cast<RpcValue::Int>(m_value); }
	RpcValue::UInt toUInt() const override { return static_cast<RpcValue::UInt>(m_value); }
	int64_t toInt64() const override { return m_value; }
	uint64_t toUInt64() const override { return static_cast<uint64_t>(m_value); }
	bool equals(const RpcValue::AbstractValueData * other) const override { return m_value == other->toInt64(); }
	//bool less(const Data * other) const override { return m_value < other->toDouble(); }
public:
	explicit ChainPackInt(int64_t value) : ValueData(value) {}
};

class ChainPackUInt final : public ValueData<RpcValue::Type::UInt, uint64_t>
{
	ChainPackUInt* create() override { return new ChainPackUInt(0); }
	std::string toStdString() const override { return Utils::toString(m_value); }

	double toDouble() const override { return m_value; }
	bool toBool() const override { return !(m_value == 0); }
	RpcValue::Int toInt() const override { return static_cast<RpcValue::Int>(m_value); }
	RpcValue::UInt toUInt() const override { return static_cast<RpcValue::UInt>(m_value); }
	int64_t toInt64() const override { return static_cast<int64_t>(m_value); }
	uint64_t toUInt64() const override { return m_value; }
	bool equals(const RpcValue::AbstractValueData * other) const override { return m_value == other->toUInt64(); }
	//bool less(const Data * other) const override { return m_value < other->toDouble(); }
public:
	explicit ChainPackUInt(uint64_t value) : ValueData(value) {}
};

class ChainPackBoolean final : public ValueData<RpcValue::Type::Bool, bool>
{
	ChainPackBoolean* create() override { return new ChainPackBoolean(false); }
	std::string toStdString() const override { return m_value? "true": "false"; }

	bool toBool() const override { return m_value; }
	RpcValue::Int toInt() const override { return m_value; }
	RpcValue::UInt toUInt() const override { return m_value; }
	int64_t toInt64() const override { return m_value; }
	uint64_t toUInt64() const override { return m_value; }
	bool equals(const RpcValue::AbstractValueData * other) const override { return m_value == other->toBool(); }
public:
	explicit ChainPackBoolean(bool value) : ValueData(value) {}
};

class ChainPackDateTime final : public ValueData<RpcValue::Type::DateTime, RpcValue::DateTime>
{
	ChainPackDateTime* create() override { return new ChainPackDateTime(RpcValue::DateTime()); }
	std::string toStdString() const override { return m_value.toIsoString(); }

	bool toBool() const override { return m_value.msecsSinceEpoch() != 0; }
	int64_t toInt64() const override { return m_value.msecsSinceEpoch(); }
	uint64_t toUInt64() const override { return m_value.msecsSinceEpoch(); }
	RpcValue::DateTime toDateTime() const override { return m_value; }
	bool equals(const RpcValue::AbstractValueData * other) const override { return m_value.msecsSinceEpoch() == other->toDateTime().msecsSinceEpoch(); }
public:
	explicit ChainPackDateTime(RpcValue::DateTime value) : ValueData(value) {}
};

class ChainPackString : public ValueData<RpcValue::Type::String, RpcValue::String>
{
	ChainPackString* create() override { return new ChainPackString(RpcValue::String()); }
	std::string toStdString() const override { return asString(); }

	const std::string &asString() const override { return m_value; }
	bool equals(const RpcValue::AbstractValueData * other) const override { return m_value == other->asString(); }
public:
	explicit ChainPackString(const RpcValue::String &value) : ValueData(value) {}
	explicit ChainPackString(RpcValue::String &&value) : ValueData(std::move(value)) {}
};

class ChainPackBlob final : public ValueData<RpcValue::Type::Blob, RpcValue::Blob>
{
	ChainPackBlob* create() override { return new ChainPackBlob(RpcValue::Blob()); }
	std::string toStdString() const override { return std::string(m_value.begin(), m_value.end()); }
	//const std::string &toString() const override { return Utils::toHex(m_value); }
	const RpcValue::Blob &asBlob() const override { return m_value; }
	bool equals(const RpcValue::AbstractValueData * other) const override { return m_value == other->asBlob(); }
public:
	explicit ChainPackBlob(const RpcValue::Blob &value) : ValueData(value) {}
	explicit ChainPackBlob(RpcValue::Blob &&value) : ValueData(std::move(value)) {}
	explicit ChainPackBlob(const uint8_t *bytes, size_t size) : ValueData(RpcValue::Blob(bytes, bytes + size)) {}
};

class ChainPackList final : public ValueData<RpcValue::Type::List, RpcValue::List>
{
	ChainPackList* create() override { return new ChainPackList(RpcValue::List()); }
	std::string toStdString() const override { return std::string(); }

	size_t count() const override {return m_value.size();}
	RpcValue at(RpcValue::Int i) const override;
	void set(RpcValue::Int i, const RpcValue &val) override;
	void append(const RpcValue &v) override;
	bool equals(const RpcValue::AbstractValueData * other) const override { return m_value == other->asList(); }
public:
	explicit ChainPackList(const RpcValue::List &value) : ValueData(value) {}
	explicit ChainPackList(RpcValue::List &&value) : ValueData(move(value)) {}

	const RpcValue::List &asList() const override { return m_value; }
};

RpcValue ChainPackList::at(RpcValue::Int ix) const
{
	if(ix < 0)
		ix = static_cast<RpcValue::Int>(m_value.size()) + ix;
	if (ix < 0 || ix >= (int)m_value.size())
		return RpcValue();
	else
		return m_value[static_cast<size_t>(ix)];
}

void ChainPackList::set(RpcValue::Int ix, const RpcValue &val)
{
	if(ix < 0)
		ix = static_cast<RpcValue::Int>(m_value.size()) + ix;
	if(ix > 0) {
		if (ix == (int)m_value.size())
			m_value.resize(static_cast<size_t>(ix) + 1);
		m_value[static_cast<size_t>(ix)] = val;
	}
}

void ChainPackList::append(const RpcValue &v)
{
	m_value.push_back(v);
}

class ChainPackMap final : public ValueData<RpcValue::Type::Map, RpcValue::Map>
{
	ChainPackMap* create() override { return new ChainPackMap(RpcValue::Map()); }
	std::string toStdString() const override { return std::string(); }

	size_t count() const override {return m_value.size();}
	bool has(const RpcValue::String &key) const override;
	RpcValue at(const RpcValue::String &key) const override;
	void set(const RpcValue::String &key, const RpcValue &val) override;
	bool equals(const RpcValue::AbstractValueData * other) const override { return m_value == other->asMap(); }
public:
	explicit ChainPackMap(const RpcValue::Map &value) : ValueData(value) {}
	explicit ChainPackMap(RpcValue::Map &&value) : ValueData(move(value)) {}

	const RpcValue::Map &asMap() const override { return m_value; }
};

class ChainPackIMap final : public ValueData<RpcValue::Type::IMap, RpcValue::IMap>
{
	ChainPackIMap* create() override { return new ChainPackIMap(RpcValue::IMap()); }
	std::string toStdString() const override { return std::string(); }
	//const ChainPack::Map &toMap() const override { return m_value; }
	size_t count() const override {return m_value.size();}
	bool has(RpcValue::Int key) const override;
	RpcValue at(RpcValue::Int key) const override;
	void set(RpcValue::Int key, const RpcValue &val) override;
	bool equals(const RpcValue::AbstractValueData * other) const override { return m_value == other->asIMap(); }
public:
	explicit ChainPackIMap(const RpcValue::IMap &value) : ValueData(value) {}
	explicit ChainPackIMap(RpcValue::IMap &&value) : ValueData(std::move(value)) {}

	const RpcValue::IMap &asIMap() const override { return m_value; }
};

class ChainPackNull final : public ValueData<RpcValue::Type::Null, std::nullptr_t>
{
	ChainPackNull* create() override { return new ChainPackNull(); }
	std::string toStdString() const override { return "null"; }

	bool isNull() const override {return true;}
	bool equals(const RpcValue::AbstractValueData * other) const override { return other->isNull(); }
public:
	ChainPackNull() : ValueData({}) {}
};

static const RpcValue::String & static_empty_string() { static const RpcValue::String s{}; return s; }
static const RpcValue::Blob & static_empty_blob() { static const RpcValue::Blob s{}; return s; }
static const RpcValue::List & static_empty_list() { static const RpcValue::List s{}; return s; }
static const RpcValue::Map & static_empty_map() { static const RpcValue::Map s{}; return s; }
static const RpcValue::IMap & static_empty_imap() { static const RpcValue::IMap s{}; return s; }

/* * * * * * * * * * * * * * * * * * * *
 * Constructors
 */

RpcValue::RpcValue() noexcept : m_ptr(nullptr) {}

RpcValue RpcValue::fromType(RpcValue::Type t) noexcept
{
	switch(t) {
	case Type::Invalid: return RpcValue{};
	case Type::Null: return RpcValue{nullptr};
	case Type::UInt: return RpcValue{static_cast<UInt>(0)};
	case Type::Int: return RpcValue{static_cast<Int>(0)};
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
RpcValue::RpcValue(std::nullptr_t) noexcept : m_ptr(std::make_shared<ChainPackNull>()) {}
RpcValue::RpcValue(double value) : m_ptr(std::make_shared<ChainPackDouble>(value)) {}
RpcValue::RpcValue(RpcValue::Decimal value) : m_ptr(std::make_shared<ChainPackDecimal>(std::move(value))) {}
RpcValue::RpcValue(int32_t value) : m_ptr(std::make_shared<ChainPackInt>(value)) {}
RpcValue::RpcValue(uint32_t value) : m_ptr(std::make_shared<ChainPackUInt>(value)) {}
RpcValue::RpcValue(int64_t value) : m_ptr(std::make_shared<ChainPackInt>(value)) {}
RpcValue::RpcValue(uint64_t value) : m_ptr(std::make_shared<ChainPackUInt>(value)) {}
RpcValue::RpcValue(bool value) : m_ptr(std::make_shared<ChainPackBoolean>(value)) {}
RpcValue::RpcValue(const DateTime &value) : m_ptr(std::make_shared<ChainPackDateTime>(value)) {}

RpcValue::RpcValue(const RpcValue::Blob &value) : m_ptr(std::make_shared<ChainPackBlob>(value)) {}
RpcValue::RpcValue(RpcValue::Blob &&value) : m_ptr(std::make_shared<ChainPackBlob>(std::move(value))) {}
RpcValue::RpcValue(const uint8_t * value, size_t size) : m_ptr(std::make_shared<ChainPackBlob>(value, size)) {}

RpcValue::RpcValue(const std::string &value) : m_ptr(std::make_shared<ChainPackString>(value)) {}
RpcValue::RpcValue(std::string &&value) : m_ptr(std::make_shared<ChainPackString>(std::move(value))) {}
RpcValue::RpcValue(const char * value) : m_ptr(std::make_shared<ChainPackString>(value)) {}

RpcValue::RpcValue(const RpcValue::List &values) : m_ptr(std::make_shared<ChainPackList>(values)) {}
RpcValue::RpcValue(RpcValue::List &&values) : m_ptr(std::make_shared<ChainPackList>(std::move(values))) {}

RpcValue::RpcValue(const RpcValue::Map &values) : m_ptr(std::make_shared<ChainPackMap>(values)) {}
RpcValue::RpcValue(RpcValue::Map &&values) : m_ptr(std::make_shared<ChainPackMap>(std::move(values))) {}

RpcValue::RpcValue(const RpcValue::IMap &values) : m_ptr(std::make_shared<ChainPackIMap>(values)) {}
RpcValue::RpcValue(RpcValue::IMap &&values) : m_ptr(std::make_shared<ChainPackIMap>(std::move(values))) {}

#ifdef RPCVALUE_COPY_AND_SWAP
void RpcValue::swap(RpcValue& other) noexcept
{
	/*
	std::cerr << __FUNCTION__ << " xxxxxxxxxx "
			  << m_ptr.get() << " ref cnt: " << m_ptr.use_count() << " val: " << toStdString()
			  << " X "
			  << other.m_ptr.get() << " ref cnt: " << other.m_ptr.use_count() << " val: " << other.toStdString()
			  << std::endl;
	*/
	std::swap(m_ptr, other.m_ptr);
}
#endif
//Value::Value(const Value::MetaTypeId &value) : m_ptr(std::make_shared<ChainPackMetaTypeId>(value)) {}
//Value::Value(const Value::MetaTypeNameSpaceId &value) : m_ptr(std::make_shared<ChainPackMetaTypeNameSpaceId>(value)) {}
//Value::Value(const Value::MetaTypeName &value) : m_ptr(std::make_shared<ChainPackMetaTypeName>(value)) {}
//Value::Value(const Value::MetaTypeNameSpaceName &value) : m_ptr(std::make_shared<ChainPackMetaTypeNameSpaceName>(value)) {}

/* * * * * * * * * * * * * * * * * * * *
 * Accessors
 */

RpcValue::Type RpcValue::type() const
{
	return !m_ptr.isNull()? m_ptr->type(): Type::Invalid;
}
/*
RpcValue::Type RpcValue::arrayType() const
{
	return !m_ptr.isNull()? m_ptr->arrayType(): Type::Invalid;
}
*/
const RpcValue::MetaData &RpcValue::metaData() const
{
	static MetaData md;
	if(!m_ptr.isNull())
		return m_ptr->metaData();
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
	if(m_ptr.isNull() && !meta_data.isEmpty())
		SHVCHP_EXCEPTION("Cannot set valid meta data to invalid ChainPack value!");
	if(!m_ptr.isNull())
		m_ptr->setMetaData(std::move(meta_data));
}

void RpcValue::setMetaValue(RpcValue::Int key, const RpcValue &val)
{
	if(m_ptr.isNull() && val.isValid())
		SHVCHP_EXCEPTION("Cannot set valid meta value to invalid ChainPack value!");
	if(!m_ptr.isNull())
		m_ptr->setMetaValue(key, val);
}

void RpcValue::setMetaValue(const RpcValue::String &key, const RpcValue &val)
{
	if(m_ptr.isNull() && val.isValid())
		SHVCHP_EXCEPTION("Cannot set valid meta value to invalid ChainPack value!");
	if(!m_ptr.isNull())
		m_ptr->setMetaValue(key, val);
}

bool RpcValue::isDefaultValue() const
{
	switch (type()) {
	case RpcValue::Type::Invalid: return true;
	case RpcValue::Type::Null: return true;
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
	return !m_ptr.isNull();
}

bool RpcValue::isValueNotAvailable() const
{
	return !isValid() || isNull();
}

double RpcValue::toDouble() const { return !m_ptr.isNull()? m_ptr->toDouble(): 0; }
RpcValue::Decimal RpcValue::toDecimal() const { return !m_ptr.isNull()? m_ptr->toDecimal(): Decimal(); }
RpcValue::Int RpcValue::toInt() const { return !m_ptr.isNull()? m_ptr->toInt(): 0; }
RpcValue::UInt RpcValue::toUInt() const { return !m_ptr.isNull()? m_ptr->toUInt(): 0; }
int64_t RpcValue::toInt64() const { return !m_ptr.isNull()? m_ptr->toInt64(): 0; }
uint64_t RpcValue::toUInt64() const { return !m_ptr.isNull()? m_ptr->toUInt64(): 0; }
bool RpcValue::toBool() const { return !m_ptr.isNull()? m_ptr->toBool(): false; }
RpcValue::DateTime RpcValue::toDateTime() const { return !m_ptr.isNull()? m_ptr->toDateTime(): RpcValue::DateTime{}; }

RpcValue::String RpcValue::toString() const
{
	if(type() == Type::Blob)
		return blobToString(asBlob());
	return asString();
}

const RpcValue::String & RpcValue::asString() const { return !m_ptr.isNull()? m_ptr->asString(): static_empty_string(); }
const RpcValue::Blob & RpcValue::asBlob() const { return !m_ptr.isNull()? m_ptr->asBlob(): static_empty_blob(); }

std::pair<const uint8_t *, size_t> RpcValue::asBytes() const
{
	using Ret = std::pair<const uint8_t *, size_t>;
	if(type() == Type::Blob) {
		const Blob &blob = asBlob();
		return Ret(blob.data(), blob.size());
	}
	const String &s = asString();
	return Ret((const uint8_t*)s.data(), s.size());
}

std::pair<const char *, size_t> RpcValue::asData() const
{
	using Ret = std::pair<const char *, size_t>;
	if(type() == Type::Blob) {
		const Blob &blob = asBlob();
		return Ret((const char*)blob.data(), blob.size());
	}
	const String &s = asString();
	return Ret(s.data(), s.size());
}

const RpcValue::List & RpcValue::asList() const { return !m_ptr.isNull()? m_ptr->asList(): static_empty_list(); }
const RpcValue::Map & RpcValue::asMap() const { return !m_ptr.isNull()? m_ptr->asMap(): static_empty_map(); }
const RpcValue::IMap &RpcValue::asIMap() const { return !m_ptr.isNull()? m_ptr->asIMap(): static_empty_imap(); }

size_t RpcValue::count() const { return !m_ptr.isNull()? m_ptr->count(): 0; }
RpcValue RpcValue::at (RpcValue::Int i) const { return !m_ptr.isNull()? m_ptr->at(i): RpcValue(); }
RpcValue RpcValue::at (const RpcValue::String &key) const { return !m_ptr.isNull()? m_ptr->at(key): RpcValue(); }
bool RpcValue::has (RpcValue::Int i) const { return !m_ptr.isNull()? m_ptr->has(i): false; }
bool RpcValue::has (const RpcValue::String &key) const { return !m_ptr.isNull()? m_ptr->has(key): false; }

std::string RpcValue::toStdString() const { return !m_ptr.isNull()? m_ptr->toStdString(): std::string(); }

void RpcValue::set(RpcValue::Int ix, const RpcValue &val)
{
	if(m_ptr.isNull())
		nError() << " Cannot set value to invalid ChainPack value! Index: " << ix;
	else
		m_ptr->set(ix, val);
}

void RpcValue::set(const RpcValue::String &key, const RpcValue &val)
{
	if(m_ptr.isNull())
		nError() << " Cannot set value to invalid ChainPack value! Key: " << key;
	else
		m_ptr->set(key, val);
}

void RpcValue::append(const RpcValue &val)
{
	if(m_ptr.isNull())
		nError() << "Cannot append to invalid ChainPack value!";
	else
		m_ptr->append(val);
}

RpcValue RpcValue::metaStripped() const
{
	RpcValue ret = *this;
	ret.m_ptr->stripMeta();
	return ret;
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

const std::string & RpcValue::AbstractValueData::asString() const { return static_empty_string(); }
const RpcValue::Blob & RpcValue::AbstractValueData::asBlob() const { return static_empty_blob(); }
const RpcValue::List & RpcValue::AbstractValueData::asList() const { return static_empty_list(); }
const RpcValue::Map & RpcValue::AbstractValueData::asMap() const { return static_empty_map(); }
const RpcValue::IMap & RpcValue::AbstractValueData::asIMap() const { return static_empty_imap(); }

void RpcValue::AbstractValueData::set(RpcValue::Int ix, const RpcValue &)
{
	nError() << "RpcValue::AbstractValueData::set: trivial implementation called! Key: " << ix;
}

void RpcValue::AbstractValueData::set(const RpcValue::String &key, const RpcValue &)
{
	nError() << "RpcValue::AbstractValueData::set: trivial implementation called! Key: " << key;
}

void RpcValue::AbstractValueData::append(const RpcValue &)
{
	nError() << "RpcValue::AbstractValueData::append: trivial implementation called!";
}

/* * * * * * * * * * * * * * * * * * * *
 * Comparison
 */
bool RpcValue::operator== (const RpcValue &other) const
{
	if(isValid() && other.isValid()) {
		if (
			(m_ptr->type() == other.m_ptr->type())
			|| (m_ptr->type() == RpcValue::Type::UInt && other.m_ptr->type() == RpcValue::Type::Int)
			|| (m_ptr->type() == RpcValue::Type::Int && other.m_ptr->type() == RpcValue::Type::UInt)
			|| (m_ptr->type() == RpcValue::Type::Double && other.m_ptr->type() == RpcValue::Type::Decimal)
			|| (m_ptr->type() == RpcValue::Type::Decimal && other.m_ptr->type() == RpcValue::Type::Double)
		) {
			return m_ptr->equals(other.m_ptr.operator->());
		}
		return false;
	}
	return (!isValid() && !other.isValid());
}
/*
bool ChainPack::operator< (const ChainPack &other) const
{
	if(isValid() && other.isValid()) {
		if (m_ptr->type() != other.m_ptr->type())
			return m_ptr->type() < other.m_ptr->type();
		return m_ptr->less(other.m_ptr.get());
	}
	return (!isValid() && other.isValid());
}
*/

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

RpcValue::Type RpcValue::typeForName(const std::string &type_name, int len)
{
	static const char str_Null[] = "Null";
	static const char str_UInt[] = "UInt";
	static const char str_Int[] = "Int";
	static const char str_Double[] = "Double";
	static const char str_Bool[] = "Bool";
	static const char str_String[] = "String";
	static const char str_List[] = "List";
	static const char str_Map[] = "Map";
	static const char str_IMap[] = "IMap";
	static const char str_DateTime[] = "DateTime";
	static const char str_Decimal[] = "Decimal";

	if(type_name.compare(0, (len < 0)? sizeof(str_Null)-1: (unsigned)len, str_Null) == 0) return Type::Null;
	if(type_name.compare(0, (len < 0)? sizeof(str_UInt)-1: (unsigned)len, str_UInt) == 0) return Type::UInt;
	if(type_name.compare(0, (len < 0)? sizeof(str_Int)-1: (unsigned)len, str_Int) == 0) return Type::Int;
	if(type_name.compare(0, (len < 0)? sizeof(str_Double)-1: (unsigned)len, str_Double) == 0) return Type::Double;
	if(type_name.compare(0, (len < 0)? sizeof(str_Bool)-1: (unsigned)len, str_Bool) == 0) return Type::Bool;
	if(type_name.compare(0, (len < 0)? sizeof(str_String)-1: (unsigned)len, str_String) == 0) return Type::String;
	if(type_name.compare(0, (len < 0)? sizeof(str_List)-1: (unsigned)len, str_List) == 0) return Type::List;
	if(type_name.compare(0, (len < 0)? sizeof(str_Map)-1: (unsigned)len, str_Map) == 0) return Type::Map;
	if(type_name.compare(0, (len < 0)? sizeof(str_IMap)-1: (unsigned)len, str_IMap) == 0) return Type::IMap;
	if(type_name.compare(0, (len < 0)? sizeof(str_DateTime)-1: (unsigned)len, str_DateTime) == 0) return Type::DateTime;
	if(type_name.compare(0, (len < 0)? sizeof(str_Decimal)-1: (unsigned)len, str_Decimal) == 0) return Type::Decimal;

	return Type::Invalid;
}

bool ChainPackMap::has(const RpcValue::String &key) const
{
	auto iter = m_value.find(key);
	return (iter != m_value.end());
}

RpcValue ChainPackMap::at(const RpcValue::String &key) const
{
	auto iter = m_value.find(key);
	return (iter == m_value.end()) ? RpcValue() : iter->second;
}

void ChainPackMap::set(const RpcValue::String &key, const RpcValue &val)
{
	if(val.isValid())
		m_value[key] = val;
	else
		m_value.erase(key);
}

bool ChainPackIMap::has(RpcValue::Int key) const
{
	auto iter = m_value.find(key);
	return (iter != m_value.end());
}

RpcValue ChainPackIMap::at(RpcValue::Int key) const
{
	auto iter = m_value.find(key);
	return (iter == m_value.end()) ? RpcValue() : iter->second;
}

void ChainPackIMap::set(RpcValue::Int key, const RpcValue &val)
{
	if(val.isValid())
		m_value[key] = val;
	else
		m_value.erase(key);
}

static long parse_ISO_DateTime(const std::string &s, std::tm &tm, int &msec, int64_t &msec_since_epoch, int &minutes_from_utc)
{
	ccpcp_unpack_context ctx;
	ccpcp_unpack_context_init(&ctx, s.data(), s.size(), nullptr, nullptr);
	ccpon_unpack_date_time(&ctx, &tm, &msec, &minutes_from_utc);
	if(ctx.err_no == CCPCP_RC_OK) {
		msec_since_epoch = ctx.item.as.DateTime.msecs_since_epoch;
		//minutes_from_utc = ctx.item.as.DateTime.minutes_from_utc;
		return ctx.current - ctx.start;
	}
	return 0;
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
	utc_offset = (tim - utc_tim) / 60;
	epoch_msec = static_cast<int64_t>(utc_tim) * 60 * 1000 + msec;
	ret.m_dtm.msec = epoch_msec;
	ret.m_dtm.tz = utc_offset;

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
	long len = parse_ISO_DateTime(utc_date_time_str, tm, msec, epoch_msec, utc_offset);
	if(len == 0) {
		nError() << "Invalid date time string:" << utc_date_time_str;
		if(plen)
			*plen = 0;
		return ret;
	}
	ret.m_dtm.msec = epoch_msec;
	ret.m_dtm.tz = utc_offset;

	if(plen)
		*plen = static_cast<size_t>(len);

	return ret;
}

RpcValue::DateTime RpcValue::DateTime::fromMSecsSinceEpoch(int64_t msecs, int utc_offset_min)
{
	DateTime ret;
	ret.m_dtm.msec = msecs;
	ret.m_dtm.tz = utc_offset_min / 15;
	return ret;
}
/*
RpcValue::DateTime RpcValue::DateTime::fromMSecs20180202(int64_t msecs, int utc_offset_min)
{
	DateTime ret;
	ret.m_dtm.msec = msecs + SHV_EPOCH_MSEC;
	ret.m_dtm.tz = utc_offset_min / 15;
	return ret;
}
*/
std::string RpcValue::DateTime::toLocalString() const
{
	std::time_t tim = m_dtm.msec / 1000 + m_dtm.tz * 15 * 60;
	std::tm *tm = std::localtime(&tim);
	if(tm == nullptr) {
		nError() << "Invalid date time: " << m_dtm.msec;
		return std::string();
	}
	char buffer[80];
	std::strftime(buffer, sizeof(buffer),"%Y-%m-%dT%H:%M:%S",tm);
	std::string ret(buffer);
	int msecs = m_dtm.msec % 1000;
	if(msecs > 0)
		ret += '.' + Utils::toString(msecs % 1000);
	return ret;
}

std::string RpcValue::DateTime::toIsoString(RpcValue::DateTime::MsecPolicy msec_policy, bool include_tz) const
{
	ccpcp_pack_context ctx;
	char buff[32];
	ccpcp_pack_context_init(&ctx, buff, sizeof(buff), nullptr);
	ccpon_pack_date_time_str(&ctx, msecsSinceEpoch(), minutesFromUtc(), (ccpon_msec_policy)msec_policy, include_tz);
	return std::string(buff, ctx.current);
#if 0
	std::time_t tim = m_dtm.msec / 1000 + m_dtm.tz * 15 * 60;
	std::tm *tm = std::gmtime(&tim);
	if(tm == nullptr) {
		nError() << "Invalid date time: " << m_dtm.msec;
		return std::string();
	}
	char buffer[80];
	std::strftime(buffer, sizeof(buffer),"%Y-%m-%dT%H:%M:%S",tm);
	std::string ret(buffer);
	int msecs = m_dtm.msec % 1000;
	if((msec_policy == MsecPolicy::Always) || (msecs > 0 && msec_policy != MsecPolicy::Auto))
		ret += '.' + Utils::toString(msecs % 1000, 3);
	if(include_tz) {
		if(m_dtm.tz == 0) {
			ret += 'Z';
		}
		else {
			int min = minutesFromUtc();
			if(min < 0)
				min = -min;
			int hour = min / 60;
			min = min % 60;
			std::string tz = Utils::toString(hour, 2);
			if(min != 0)
				tz += Utils::toString(min, 2);
			ret += ((m_dtm.tz < 0)? '-': '+') + tz;
		}
	}
	return ret;
#endif
}
#ifdef DEBUG_RPCVAL
static int cnt = 0;
#endif
RpcValue::MetaData::MetaData()
{
#ifdef DEBUG_RPCVAL
	logDebugRpcVal() << ++cnt << "+++MM default" << this;
#endif
}

RpcValue::MetaData::MetaData(const RpcValue::MetaData &o)
{
#ifdef DEBUG_RPCVAL
	logDebugRpcVal() << ++cnt << "+++MM copy" << this << "<------" << &o;
#endif
	if(o.m_imap && !o.m_imap->empty())
		m_imap = new RpcValue::IMap(*o.m_imap);
	if(o.m_smap && !o.m_smap->empty())
		m_smap = new RpcValue::Map(*o.m_smap);
}

RpcValue::MetaData::MetaData(RpcValue::MetaData &&o)
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
	if(!imap.empty())
		m_imap = new RpcValue::IMap(std::move(imap));
}

RpcValue::MetaData::MetaData(RpcValue::Map &&smap)
{
#ifdef DEBUG_RPCVAL
	logDebugRpcVal() << ++cnt << "+++MM move smap" << this;
#endif
	if(!smap.empty())
		m_smap = new RpcValue::Map(std::move(smap));
}

RpcValue::MetaData::MetaData(RpcValue::IMap &&imap, RpcValue::Map &&smap)
{
#ifdef DEBUG_RPCVAL
	logDebugRpcVal() << ++cnt << "+++MM move imap smap" << this;
#endif
	if(!imap.empty())
		m_imap = new RpcValue::IMap(std::move(imap));
	if(!smap.empty())
		m_smap = new RpcValue::Map(std::move(smap));
}

RpcValue::MetaData::~MetaData()
{
#ifdef DEBUG_RPCVAL
	logDebugRpcVal() << cnt-- << "---MM cnt:" << size() << this;
#endif
	if(m_imap)
		delete m_imap;
	if(m_smap)
		delete m_smap;
}

RpcValue::MetaData &RpcValue::MetaData::operator =(RpcValue::MetaData &&o)
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
	if(o.m_imap && !o.m_imap->empty())
		m_imap = new RpcValue::IMap(*o.m_imap);
	if(o.m_smap && !o.m_smap->empty())
		m_smap = new RpcValue::Map(*o.m_smap);
	return *this;
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

void RpcValue::MetaData::setValue(RpcValue::Int key, const RpcValue &val)
{
	if(val.isValid()) {
		if(!m_imap)
			m_imap = new RpcValue::IMap();
		(*m_imap)[key] = val;
	}
	else {
		if(m_imap)
			m_imap->erase(key);
	}
}

void RpcValue::MetaData::setValue(const RpcValue::String &key, const RpcValue &val)
{
	if(val.isValid()) {
		if(!m_smap)
			m_smap = new RpcValue::Map();
		(*m_smap)[key] = val;
	}
	else {
		if(m_smap)
			m_smap->erase(key);
	}
}

size_t RpcValue::MetaData::size() const
{
	return (m_imap? m_imap->size(): 0) + (m_smap? m_smap->size(): 0);
}

bool RpcValue::MetaData::isEmpty() const
{
	return size() == 0;
}

bool RpcValue::MetaData::operator==(const RpcValue::MetaData &o) const
{
	/*
	std::cerr << "this" << std::endl;
	for(const auto &it : m_imap)
		std::cerr << '\t' << it.first << ": " << it.second.dumpText() << std::endl;
	std::cerr << "other" << std::endl;
	for(const auto &it : o.m_imap)
		std::cerr << '\t' << it.first << ": " << it.second.dumpText() << std::endl;
	*/
	return iValues() == o.iValues() && sValues() == o.sValues();
}

const RpcValue::IMap &RpcValue::MetaData::iValues() const
{
	static RpcValue::IMap m;
	return m_imap? *m_imap: m;
}

const RpcValue::Map &RpcValue::MetaData::sValues() const
{
	static RpcValue::Map m;
	return m_smap? *m_smap: m;
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

RpcValue::MetaData *RpcValue::MetaData::clone() const
{
	MetaData *md = new MetaData(*this);
	return md;
}

void RpcValue::MetaData::swap(RpcValue::MetaData &o)
{
	std::swap(m_imap, o.m_imap);
	std::swap(m_smap, o.m_smap);
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

}}


