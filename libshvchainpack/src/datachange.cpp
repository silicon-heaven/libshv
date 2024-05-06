#include <shv/chainpack/datachange.h>

#include <necrolog.h>

namespace shv::chainpack {

//================================================================
// ValueChange
//================================================================
DataChange::MetaType::MetaType()
	: Super("ValueChange")
{
	m_tags = {
		RPC_META_TAG_DEF(DateTime),
		RPC_META_TAG_DEF(ShortTime),
		RPC_META_TAG_DEF(ValueFlags),
		RPC_META_TAG_DEF(SpecialListValue),
	};
}

void DataChange::MetaType::registerMetaType()
{
	static bool is_init = false;
	if(!is_init) {
		is_init = true;
		static MetaType s;
		shv::chainpack::meta::registerType(shv::chainpack::meta::GlobalNS::ID, MetaType::ID, &s);
	}
}

const char *DataChange::valueFlagToString(DataChange::ValueFlag flag)
{
	switch (flag) {
	case ValueFlag::Snapshot: return "Snapshot";
	case ValueFlag::Spontaneous: return "Spontaneous";
	case ValueFlag::Provisional: return "Provisional";
	case ValueFlag::ValueFlagCount: return "ValueFlagCount";
	}
	return "???";
}

std::string DataChange::valueFlagsToString(ValueFlags st)
{
	std::string ret;
	auto append_str = [&ret](const char *str) {
		if(!ret.empty())
			ret += ',';
		ret += str;
	};
	for (int flag = 0; flag < ValueFlagCount; ++flag) {
		unsigned mask = 1 << flag;
		if(st & mask)
			append_str(valueFlagToString(static_cast<ValueFlag>(flag)));
	}
	return ret;
}

DataChange::DataChange(const RpcValue &val, const RpcValue::DateTime &date_time, int short_time)
	: m_dateTime(date_time)
	, m_shortTime(short_time)
{
	setValue(val);
}

DataChange::DataChange(const RpcValue &val, unsigned short_time)
	: m_shortTime(static_cast<int>(short_time))
{
	setValue(val);
}

bool DataChange::isValid() const
{
	return m_value.isValid();
}

bool DataChange::isDataChange(const RpcValue &rv)
{
	return rv.metaTypeId() == MetaType::ID
			&& rv.metaTypeNameSpaceId() == shv::chainpack::meta::GlobalNS::ID;
}

RpcValue DataChange::value() const
{
	return m_value;
}

void DataChange::setValue(const RpcValue &val)
{
	m_value = val;
	if(isDataChange(val))
		nWarning() << "Storing ValueChange to value (ValueChange inside ValueChange):" << val.toCpon();
}

DataChange DataChange::fromRpcValue(const RpcValue &val)
{
	if(isDataChange(val)) {
		DataChange ret;
		if(val.isList()) {
			const RpcValue::List &lst = val.asList();
			if(lst.size() == 1) {
				RpcValue wrapped_val = lst.value(0);
				if(!wrapped_val.metaData().isEmpty() && !val.metaValue(MetaType::Tag::SpecialListValue).isValid()) {
					ret.setValue(wrapped_val);
					goto set_meta_data;
				}
			}
		}
		{
			auto raw_val = val;
			raw_val.takeMeta();
			ret.setValue(raw_val);
		}
set_meta_data:
		ret.setDateTime(val.metaValue(MetaType::Tag::DateTime));
		ret.setShortTime(val.metaValue(MetaType::Tag::ShortTime));
		int st = val.metaValue(MetaType::Tag::ValueFlags).toInt();
		ret.setValueFlags(static_cast<shv::chainpack::DataChange::ValueFlags>(st));
		return ret;
	}
	return DataChange(val, RpcValue::DateTime());
}

RpcValue DataChange::toRpcValue() const
{
	RpcValue ret;
	if(m_value.isValid()) {
		if(m_value.metaData().isEmpty()) {
			ret = m_value;
			const RpcValue::List &lst = m_value.asList();
			if(lst.size() == 1 && !lst[0].metaData().isEmpty()) {
				// [<meta>value] will be packed as <dc-meta>[<meta>value]
				// what will be unpacked to DataChange as <meta>value
				// mark such a rare case with tag SpecialListValue
				// to avoid this mis-interpretation
				ret.setMetaValue(MetaType::Tag::SpecialListValue, true);
			}
		}
		else {
			ret = RpcValue::List{m_value};
		}
	}
	else {
		ret = RpcValue{nullptr};
	}
	ret.setMetaValue(chainpack::meta::Tag::MetaTypeId, MetaType::ID);
	if(hasDateTime())
		ret.setMetaValue(MetaType::Tag::DateTime, m_dateTime);
	if(hasShortTime())
		ret.setMetaValue(MetaType::Tag::ShortTime, static_cast<unsigned>(m_shortTime));
	if(valueFlags() != 0)
		ret.setMetaValue(MetaType::Tag::ValueFlags, static_cast<int>(valueFlags()));
	return ret;
}

bool DataChange::hasDateTime() const
{
	return m_dateTime.msecsSinceEpoch() > 0;
}

RpcValue DataChange::dateTime() const
{
	return hasDateTime()? RpcValue(m_dateTime): RpcValue();
}

void DataChange::setDateTime(const RpcValue &dt)
{
	m_dateTime = dt.isDateTime()? dt.toDateTime(): RpcValue::DateTime();
}

int64_t DataChange::epochMSec() const
{
	return m_dateTime.msecsSinceEpoch();
}

bool DataChange::hasShortTime() const
{
	return m_shortTime > NO_SHORT_TIME;
}

RpcValue DataChange::shortTime() const
{
	return hasShortTime()? RpcValue(static_cast<unsigned>(m_shortTime)): RpcValue();
}

void DataChange::setShortTime(const RpcValue &st)
{
	m_shortTime = (st.isUInt() || (st.isInt() && st.toInt() >= 0))? st.toInt(): NO_SHORT_TIME;
}

bool DataChange::hasValueflags() const
{
	return m_valueFlags != NO_VALUE_FLAGS;
}

void DataChange::setValueFlags(ValueFlags st)
{
	m_valueFlags = st;
}

DataChange::ValueFlags DataChange::valueFlags() const
{
	return m_valueFlags;
}

bool DataChange::isProvisional() const
{
	return testBit(m_valueFlags, ValueFlag::Provisional);
}

void DataChange::setProvisional(bool b)
{
	setBit(m_valueFlags, ValueFlag::Provisional, b);
}


bool DataChange::isSpontaneous() const
{
	return testBit(m_valueFlags, ValueFlag::Spontaneous);
}

void DataChange::setSpontaneous(bool b)
{
	setBit(m_valueFlags, ValueFlag::Spontaneous, b);
}


bool DataChange::isSnapshotValue() const
{
	return testBit(m_valueFlags, ValueFlag::Snapshot);
}

void DataChange::setIsSnaptshotValue(bool b)
{
	setBit(m_valueFlags, ValueFlag::Snapshot, b);
}


bool DataChange::testBit(const unsigned &n, int pos)
{
	unsigned mask = 1 << pos;
	return n & mask;
}

void DataChange::setBit(unsigned &n, int pos, bool b)
{
	unsigned mask = 1 << pos;
	n &= ~mask;
	if(b)
		n |= mask;
}

} // namespace shv
