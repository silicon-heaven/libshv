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

#ifdef DEBUG_RPCVAL
#define logDebugRpcVal nWarning
#else
#define logDebugRpcVal if(0) nWarning
#endif
namespace shv::chainpack {

class RpcValue::AbstractValueData
{
public:
	virtual ~AbstractValueData() = default;

	virtual RpcValue::Type type() const {return RpcValue::Type::Invalid;}

	virtual const RpcValue::MetaData &metaData() const = 0;
	virtual void setMetaData(RpcValue::MetaData &&meta_data) = 0;
	virtual void setMetaValue(RpcValue::Int key, const RpcValue &val) = 0;
	virtual void setMetaValue(RpcValue::String key, const RpcValue &val) = 0;

	virtual bool equals(const AbstractValueData * other) const = 0;

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

	virtual bool hasI(RpcValue::Int i) const { (void)i; return false; }
	virtual bool hasS(const RpcValue::String &key) const { (void)key; return false; }
	virtual RpcValue atI(RpcValue::Int i) const { (void)i; return RpcValue(); }
	virtual RpcValue atS(const RpcValue::String &key) const { (void)key; return RpcValue(); }
	virtual void setI(RpcValue::Int ix, const RpcValue &val);
	virtual void setS(const RpcValue::String &key, const RpcValue &val);
	virtual void append(const RpcValue &);

	virtual std::string toStdString() const = 0;
	virtual RpcValue::MetaData stripMeta() = 0;

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
public:
	~ValueData() override
	{
#ifdef DEBUG_RPCVAL
		logDebugRpcVal() << "---" << value_data_cnt-- << RpcValue::typeToName(tag) << this << m_value;
#endif
		delete m_metaData;
	}

	ValueData(const ValueData &o) = delete;
	ValueData& operator=(const ValueData &o) = delete;

protected:
	explicit ValueData(const T &value)
		: m_value(value)
	{
#ifdef DEBUG_RPCVAL
		logDebugRpcVal() << "+++" << ++value_data_cnt << RpcValue::typeToName(tag) << this << value;
#endif
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

	RpcValue::MetaData stripMeta() override
	{
		if (m_metaData) {
			auto ret = *m_metaData;
			delete m_metaData;
			m_metaData = nullptr;
			return ret;
		}
		return {};
	}

protected:
	T m_value;
	RpcValue::MetaData *m_metaData = nullptr;
};

namespace {
auto convert_to_int(const double d)
{
	if (std::numeric_limits<RpcValue::Int>::max() <= d) {
		return std::numeric_limits<RpcValue::Int>::max();
	}
	if (std::numeric_limits<RpcValue::Int>::min() >= d) {
		return std::numeric_limits<RpcValue::Int>::min();
	}

	return static_cast<RpcValue::Int>(d);
}
}

class ChainPackDouble final : public ValueData<RpcValue::Type::Double, double>
{
	ChainPackDouble* create() override { return new ChainPackDouble(0); }
	std::string toStdString() const override { return std::to_string(m_value); }
	double toDouble() const override { return m_value; }
	bool toBool() const override { return !(m_value == 0.); }
	RpcValue::Int toInt() const override { return convert_to_int(m_value); }
	RpcValue::UInt toUInt() const override { return static_cast<RpcValue::UInt>(m_value); }
	int64_t toInt64() const override { return static_cast<int64_t>(m_value); }
	uint64_t toUInt64() const override { return static_cast<uint64_t>(m_value); }
	bool equals(const RpcValue::AbstractValueData * other) const override {
		return m_value == other->toDouble();
	}
public:
	explicit ChainPackDouble(double value) : ValueData(value) {}
};

class ChainPackDecimal final : public ValueData<RpcValue::Type::Decimal, RpcValue::Decimal>
{
	ChainPackDecimal* create() override { return new ChainPackDecimal(RpcValue::Decimal()); }
	std::string toStdString() const override { return m_value.toString(); }

	double toDouble() const override { return m_value.toDouble(); }
	bool toBool() const override { return !(m_value.mantisa() == 0); }
	RpcValue::Int toInt() const override { return convert_to_int(m_value.toDouble()); }
	RpcValue::UInt toUInt() const override { return static_cast<RpcValue::UInt>(m_value.toDouble()); }
	int64_t toInt64() const override { return static_cast<int64_t>(m_value.toDouble()); }
	uint64_t toUInt64() const override { return static_cast<uint64_t>(m_value.toDouble()); }
	RpcValue::Decimal toDecimal() const override { return m_value; }
	bool equals(const RpcValue::AbstractValueData * other) const override { return toDouble() == other->toDouble(); }
public:
	explicit ChainPackDecimal(const RpcValue::Decimal& value) : ValueData(value) {}
};

class ChainPackInt final : public ValueData<RpcValue::Type::Int, int64_t>
{
	ChainPackInt* create() override { return new ChainPackInt(0); }
	std::string toStdString() const override { return Utils::toString(m_value); }

	double toDouble() const override { return static_cast<double>(m_value); }
	bool toBool() const override { return !(m_value == 0); }
	RpcValue::Int toInt() const override { return static_cast<RpcValue::Int>(m_value); }
	RpcValue::UInt toUInt() const override { return static_cast<RpcValue::UInt>(m_value); }
	int64_t toInt64() const override { return m_value; }
	uint64_t toUInt64() const override { return static_cast<uint64_t>(m_value); }
	bool equals(const RpcValue::AbstractValueData * other) const override { return m_value == other->toInt64(); }
public:
	explicit ChainPackInt(int64_t value) : ValueData(value) {}
};

class ChainPackUInt final : public ValueData<RpcValue::Type::UInt, uint64_t>
{
	ChainPackUInt* create() override { return new ChainPackUInt(0); }
	std::string toStdString() const override { return Utils::toString(m_value); }

	double toDouble() const override { return static_cast<double>(m_value); }
	bool toBool() const override { return !(m_value == 0); }
	RpcValue::Int toInt() const override { return static_cast<RpcValue::Int>(m_value); }
	RpcValue::UInt toUInt() const override { return static_cast<RpcValue::UInt>(m_value); }
	int64_t toInt64() const override { return static_cast<int64_t>(m_value); }
	uint64_t toUInt64() const override { return m_value; }
	bool equals(const RpcValue::AbstractValueData * other) const override { return m_value == other->toUInt64(); }
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
	uint64_t toUInt64() const override { return static_cast<uint64_t>(m_value.msecsSinceEpoch()); }
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
	explicit ChainPackString(RpcValue::String &&value) : ValueData(value) {}
};

class ChainPackBlob final : public ValueData<RpcValue::Type::Blob, RpcValue::Blob>
{
	ChainPackBlob* create() override { return new ChainPackBlob(RpcValue::Blob()); }
	std::string toStdString() const override { return std::string(m_value.begin(), m_value.end()); }
	const RpcValue::Blob &asBlob() const override { return m_value; }
	bool equals(const RpcValue::AbstractValueData * other) const override { return m_value == other->asBlob(); }
public:
	explicit ChainPackBlob(const RpcValue::Blob &value) : ValueData(value) {}
	explicit ChainPackBlob(RpcValue::Blob &&value) : ValueData(value) {}
	explicit ChainPackBlob(const uint8_t *bytes, size_t size) : ValueData(RpcValue::Blob(bytes, bytes + size)) {}
};

class ChainPackList final : public ValueData<RpcValue::Type::List, RpcValue::List>
{
	ChainPackList* create() override { return new ChainPackList(RpcValue::List()); }
	std::string toStdString() const override { return std::string(); }

	size_t count() const override {return m_value.size();}
	RpcValue atI(RpcValue::Int i) const override;
	void setI(RpcValue::Int i, const RpcValue &val) override;
	void append(const RpcValue &v) override;
	bool equals(const RpcValue::AbstractValueData * other) const override { return m_value == other->asList(); }
public:
	explicit ChainPackList(const RpcValue::List &value) : ValueData(value) {}
	explicit ChainPackList(RpcValue::List &&value) : ValueData(value) {}

	const RpcValue::List &asList() const override { return m_value; }
};

RpcValue ChainPackList::atI(RpcValue::Int ix) const
{
	if(ix < 0)
		ix = static_cast<RpcValue::Int>(m_value.size()) + ix;
	if (ix < 0 || ix >= static_cast<int>(m_value.size()))
		return RpcValue();

	return m_value[static_cast<size_t>(ix)];
}

void ChainPackList::setI(RpcValue::Int ix, const RpcValue &val)
{
	if(ix < 0)
		ix = static_cast<RpcValue::Int>(m_value.size()) + ix;
	if(ix > 0) {
		if (ix == static_cast<int>(m_value.size()))
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
	bool hasS(const RpcValue::String &key) const override;
	RpcValue atS(const RpcValue::String &key) const override;
	void setS(const RpcValue::String &key, const RpcValue &val) override;
	bool equals(const RpcValue::AbstractValueData * other) const override { return m_value == other->asMap(); }
public:
	explicit ChainPackMap(const RpcValue::Map &value) : ValueData(value) {}
	explicit ChainPackMap(RpcValue::Map &&value) : ValueData(value) {}

	const RpcValue::Map &asMap() const override { return m_value; }
};

class ChainPackIMap final : public ValueData<RpcValue::Type::IMap, RpcValue::IMap>
{
	ChainPackIMap* create() override { return new ChainPackIMap(RpcValue::IMap()); }
	std::string toStdString() const override { return std::string(); }
	size_t count() const override {return m_value.size();}
	bool hasI(RpcValue::Int key) const override;
	RpcValue atI(RpcValue::Int key) const override;
	void setI(RpcValue::Int key, const RpcValue &val) override;
	bool equals(const RpcValue::AbstractValueData * other) const override { return m_value == other->asIMap(); }
public:
	explicit ChainPackIMap(const RpcValue::IMap &value) : ValueData(value) {}
	explicit ChainPackIMap(RpcValue::IMap &&value) : ValueData(value) {}

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

static const CowPtr<RpcValue::AbstractValueData> static_null(nullptr);

RpcValue::RpcValue() noexcept : m_ptr(static_null) {}

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
RpcValue::RpcValue(std::nullptr_t) noexcept : m_ptr(std::make_shared<ChainPackNull>()) {}
RpcValue::RpcValue(double value) : m_ptr(std::make_shared<ChainPackDouble>(value)) {}
RpcValue::RpcValue(const RpcValue::Decimal& value) : m_ptr(std::make_shared<ChainPackDecimal>(value)) {}
RpcValue::RpcValue(short value) : m_ptr(std::make_shared<ChainPackInt>(value)) {}
RpcValue::RpcValue(int value) : m_ptr(std::make_shared<ChainPackInt>(value)) {}
RpcValue::RpcValue(long value) : m_ptr(std::make_shared<ChainPackInt>(value)) {}
RpcValue::RpcValue(long long value) : m_ptr(std::make_shared<ChainPackInt>(value)) {}
RpcValue::RpcValue(unsigned short value) : m_ptr(std::make_shared<ChainPackUInt>(value)) {}
RpcValue::RpcValue(unsigned int value) : m_ptr(std::make_shared<ChainPackUInt>(value)) {}
RpcValue::RpcValue(unsigned long value) : m_ptr(std::make_shared<ChainPackUInt>(value)) {}
RpcValue::RpcValue(unsigned long long value) : m_ptr(std::make_shared<ChainPackUInt>(value)) {}

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
	std::swap(m_ptr, other.m_ptr);
}
#endif

/* * * * * * * * * * * * * * * * * * * *
 * Accessors
 */

RpcValue::Type RpcValue::type() const
{
	return !m_ptr.isNull()? m_ptr->type(): Type::Invalid;
}
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
	return !m_ptr.isNull();
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

const RpcValue::List & RpcValue::asList() const { return !m_ptr.isNull()? m_ptr->asList(): static_empty_list(); }
const RpcValue::Map & RpcValue::asMap() const { return !m_ptr.isNull()? m_ptr->asMap(): static_empty_map(); }
const RpcValue::IMap &RpcValue::asIMap() const { return !m_ptr.isNull()? m_ptr->asIMap(): static_empty_imap(); }

[[deprecated("Use asList instead")]] const RpcValue::List &RpcValue::toList() const
{
	return asList();
}

[[deprecated("Use asMap instead")]] const RpcValue::Map &RpcValue::toMap() const
{
	return asMap();
}

[[deprecated("Use asIMap instead")]] const RpcValue::IMap &RpcValue::toIMap() const
{
	return asIMap();
}

size_t RpcValue::count() const { return !m_ptr.isNull()? m_ptr->count(): 0; }
RpcValue RpcValue::at(RpcValue::Int i) const { return !m_ptr.isNull()? m_ptr->atI(i): RpcValue(); }
RpcValue RpcValue::at(Int i, const RpcValue &def_val) const  { return has(i)? at(i): def_val; }
RpcValue RpcValue::at(const RpcValue::String &key) const { return !m_ptr.isNull()? m_ptr->atS(key): RpcValue(); }
RpcValue RpcValue::at(const RpcValue::String &key, const RpcValue &def_val) const { return has(key)? at(key): def_val; }
bool RpcValue::has(RpcValue::Int i) const { return !m_ptr.isNull()? m_ptr->hasI(i): false; }
bool RpcValue::has(const RpcValue::String &key) const { return !m_ptr.isNull()? m_ptr->hasS(key): false; }

std::string RpcValue::toStdString() const { return !m_ptr.isNull()? m_ptr->toStdString(): std::string(); }

void RpcValue::set(RpcValue::Int ix, const RpcValue &val)
{
	if(m_ptr.isNull())
		nError() << " Cannot set value to invalid ChainPack value! Index: " << ix;
	else
		m_ptr->setI(ix, val);
}

void RpcValue::set(const RpcValue::String &key, const RpcValue &val)
{
	if(m_ptr.isNull())
		nError() << " Cannot set value to invalid ChainPack value! Key: " << key;
	else
		m_ptr->setS(key, val);
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

RpcValue::MetaData RpcValue::stripMeta()
{
	return m_ptr->stripMeta();
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

void RpcValue::AbstractValueData::setI(RpcValue::Int ix, const RpcValue &)
{
	nError() << "RpcValue::AbstractValueData::set: trivial implementation called! Key: " << ix;
}

void RpcValue::AbstractValueData::setS(const RpcValue::String &key, const RpcValue &)
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

long RpcValue::refCnt() const
{
	return m_ptr.refCnt();
}

RpcValue string_literals::operator""_cpon(const char* data, size_t size)
{
	return RpcValue::fromCpon(std::string{data, size});
}

RpcValue::Decimal::Num::Num()
	: mantisa(0), exponent(-1)
{
}

RpcValue::Decimal::Num::Num(int64_t m, int e)
	: mantisa(m), exponent(e)
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

bool ChainPackMap::hasS(const RpcValue::String &key) const
{
	auto iter = m_value.find(key);
	return (iter != m_value.end());
}

RpcValue ChainPackMap::atS(const RpcValue::String &key) const
{
	auto iter = m_value.find(key);
	return (iter == m_value.end()) ? RpcValue() : iter->second;
}

void ChainPackMap::setS(const RpcValue::String &key, const RpcValue &val)
{
	if(val.isValid())
		m_value[key] = val;
	else
		m_value.erase(key);
}

bool ChainPackIMap::hasI(RpcValue::Int key) const
{
	auto iter = m_value.find(key);
	return (iter != m_value.end());
}

RpcValue ChainPackIMap::atI(RpcValue::Int key) const
{
	auto iter = m_value.find(key);
	return (iter == m_value.end()) ? RpcValue() : iter->second;
}

void ChainPackIMap::setI(RpcValue::Int key, const RpcValue &val)
{
	if(val.isValid())
		m_value[key] = val;
	else
		m_value.erase(key);
}

static long long parse_ISO_DateTime(const std::string &s, std::tm &tm, int &msec, int64_t &msec_since_epoch, int &minutes_from_utc)
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

int RpcValue::DateTime::minutesFromUtc() const
{
	return utcOffsetMin();
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
		nError() << "Invalid date time string:" << utc_date_time_str;
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
void RpcValue::DateTime::setTimeZone(int utc_offset_min)
{
	setUtcOffsetMin(utc_offset_min);
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
		ret += '.' + Utils::toString(msecs % 1000);
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
	ccpon_pack_date_time_str(&ctx, msecsSinceEpoch(), minutesFromUtc(), static_cast<ccpon_msec_policy>(msec_policy), include_tz);
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
	if(o.m_imap && !o.m_imap->empty())
		m_imap = new RpcValue::IMap(*o.m_imap);
	if(o.m_smap && !o.m_smap->empty())
		m_smap = new RpcValue::Map(*o.m_smap);
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
	delete m_imap;
	delete m_smap;
}

RpcValue::MetaData &RpcValue::MetaData::operator =(RpcValue::MetaData &&o) noexcept
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

	if(o.m_imap && !o.m_imap->empty())
		m_imap = new RpcValue::IMap(*o.m_imap);
	if(o.m_smap && !o.m_smap->empty())
		m_smap = new RpcValue::Map(*o.m_smap);
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
	auto *md = new MetaData(*this);
	return md;
}

void RpcValue::MetaData::swap(RpcValue::MetaData &o)
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


