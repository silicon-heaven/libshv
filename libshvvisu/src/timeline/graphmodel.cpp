#include "graphmodel.h"

#include <shv/chainpack/rpcvalue.h>
#include <shv/coreqt/log.h>

#include <cmath>

namespace shv {
namespace visu {
namespace timeline {

GraphModel::GraphModel(QObject *parent)
	: Super(parent)
{

}

void GraphModel::clear()
{
	m_pathToChannelCache.clear();
	m_samples.clear();
	m_channelsInfo.clear();
}

int GraphModel::count(int channel) const
{
	if(channel < 0 || channel > channelCount())
		return 0;
	return m_samples.at(channel).count();
}

Sample GraphModel::sampleAt(int channel, int ix) const
{
	return m_samples.at(channel).at(ix);
}

Sample GraphModel::sampleValue(int channel, int ix) const
{
	if(channel < 0 || channel >= channelCount())
		return Sample();
	if(ix < 0 || ix >= m_samples[channel].count())
		return Sample();
	return sampleAt(channel, ix);
}

XRange GraphModel::xRange() const
{
	XRange ret;
	for (int i = 0; i < channelCount(); ++i) {
		ret = ret.united(xRange(i));
	}
	return ret;
}

XRange GraphModel::xRange(int channel_ix) const
{
	XRange ret;
	if(count(channel_ix) > 0) {
		ret.min = sampleAt(channel_ix, 0).time;
		ret.max = sampleAt(channel_ix, count(channel_ix) - 1).time;
	}
	return ret;
}

YRange GraphModel::yRange(int channel_ix) const
{
	YRange ret;
	auto type = channelInfo(channel_ix).typeDescr.type();
	for (int i = 0; i < count(channel_ix); ++i) {
		QVariant v = sampleAt(channel_ix, i).value;
		bool ok;
		double d = valueToDouble(v, type, &ok);
		if(ok) {
			ret.min = qMin(ret.min, d);
			ret.max = qMax(ret.max, d);
		}
	}
	return ret;
}

static shv::core::utils::ShvTypeDescr::Type qt_to_shv_type(int meta_type_id)
{
	using Type = shv::core::utils::ShvTypeDescr::Type;
	switch (meta_type_id) {
	case QMetaType::QVariantMap:
		return Type::Map;
	case QMetaType::Double:
		return Type::Double;
	case QMetaType::LongLong:
	case QMetaType::ULongLong:
	case QMetaType::UInt:
	case QMetaType::Int:
		return Type::Int;
	case QMetaType::Bool:
		return Type::Bool;
	case QMetaType::QString:
		return Type::String;
	}
	return Type::Invalid;
}

double GraphModel::valueToDouble(const QVariant v, shv::core::utils::ShvTypeDescr::Type type_id, bool *ok)
{
	using Type = shv::core::utils::ShvTypeDescr::Type;
	if(ok)
		*ok = true;
	if(type_id == Type::Invalid)
		type_id = qt_to_shv_type(v.userType());
	switch (type_id) {
	case Type::Invalid:
	case Type::Map:
		return 0;
	case Type::Double:
		return v.toDouble();
	case Type::Int:
		return v.toLongLong();
	case Type::UInt:
		return v.toULongLong();
	case Type::Decimal:
		return v.toDouble();
	case Type::Bool:
		return v.toBool()? 1: 0;
	case Type::String:
		return v.toString().isEmpty()? 0: 1;
	case Type::Enum:
	case Type::BitField:
		// show integer value for now
		return v.toInt();
	default:
		if(ok)
			*ok = false;
		else
			shvWarning() << "cannot convert variant:" << v.typeName() << "to double";
		return 0;
	}
}

int GraphModel::lessTimeIndex(int channel, timemsec_t time) const
{
	int ix = lessOrEqualTimeIndex(channel, time);
	Sample s = sampleValue(channel, ix);
	if(s.time == time)
		return ix - 1;
	return ix;
}

int GraphModel::lessOrEqualTimeIndex(int channel, timemsec_t time) const
{
	if(channel < 0 || channel > channelCount())
		return -1;

	int first = 0;
	int cnt = count(channel);
	bool found = false;
	while (cnt > 0) {
		int step = cnt / 2;
		int pivot = first + step;
		if (sampleAt(channel, pivot).time <= time) {
			first = pivot;
			if(step)
				cnt -= step;
			else
				cnt = 0;
			found = true;
		}
		else {
			cnt = step;
			found = false;
		}
	};
	int ret = found? first: -1;
	//shvInfo() << time << "-->" << ret;
	return ret;
}

int GraphModel::greaterTimeIndex(int channel, timemsec_t time) const
{
	int ix = lessOrEqualTimeIndex(channel, time);
	return ix + 1;
}

int GraphModel::greaterOrEqualTimeIndex(int channel, timemsec_t time) const
{
	int ix = lessOrEqualTimeIndex(channel, time);
	Sample s = sampleValue(channel, ix);
	if(s.time == time)
		return ix;
	return ix + 1;
}

void GraphModel::beginAppendValues()
{
	m_begginAppendXRange = xRange();
}

void GraphModel::endAppendValues()
{
	XRange xr = xRange();
	if(xr.max > m_begginAppendXRange.max)
		emit xRangeChanged(xr);
	m_begginAppendXRange = XRange();
	for (int i = 0; i < channelCount(); ++i) {
		ChannelInfo &chi = channelInfo(i);
		if(!chi.typeDescr.isValid()) {
			auto type_name = guessTypeName(i);
			chi.typeDescr = typeInfo().typeDescriptionForName(type_name.toStdString());
		}
	}
}

void GraphModel::appendValue(int channel, Sample &&sample)
{
	//shvInfo() << channel << sample.time << sample.value.toString();
	if(channel < 0 || channel > channelCount()) {
		shvError() << "Invalid channel index:" << channel;
		return;
	}
	if(sample.time <= 0) {
		shvWarning() << "ignoring value with timestamp <= 0, timestamp:" << sample.time;
		return;
	}
	ChannelSamples &samples = m_samples[channel];
	if(!samples.isEmpty() && samples.last().time > sample.time) {
		shvWarning() << channelInfo(channel).shvPath << "channel:" << channel
					 << "ignoring value with lower timestamp than last value (check possibly wrong short-time correction):"
					 << samples.last().time << shv::chainpack::RpcValue::DateTime::fromMSecsSinceEpoch(samples.last().time).toIsoString()
					 << "val:"
					 << sample.time << shv::chainpack::RpcValue::DateTime::fromMSecsSinceEpoch(sample.time).toIsoString();
		return;
	}
	if (!samples.isEmpty() &&
		channelInfo(channel).typeDescr.sampleType() == shv::core::utils::ShvTypeDescr::SampleType::Continuous &&
		samples.last().value == sample.value) {
		return;
	}
	//m_appendSince = qMin(sampleAt.time, m_appendSince);
	//m_appendUntil = qMax(sampleAt.time, m_appendUntil);
	samples.push_back(std::move(sample));
}

void GraphModel::appendValueShvPath(const std::string &shv_path, Sample &&sample)
{
	int ch_ix = pathToChannelIndex(shv_path);
	if(ch_ix < 0) {
		if(isAutoCreateChannels()) {
			auto type_name = m_typeInfo.nodeDescriptionForPath(shv_path).typeName();
			shvMessage() << "Auto append channel:" << shv_path << "type:" << type_name;
			appendChannel(shv_path, std::string(), type_name);
			ch_ix = channelCount() - 1;
		}
		else {
			shvMessage() << "Cannot find channel with shv path:" << shv_path;
			return;
		}
	}
	appendValue(ch_ix, std::move(sample));
}

int GraphModel::pathToChannelIndex(const std::string &path) const
{
	auto it = m_pathToChannelCache.find(path);
	if(it == m_pathToChannelCache.end()) {
		for (int i = 0; i < channelCount(); ++i) {
			const ChannelInfo &chi = channelInfo(i);
			if(chi.shvPath == QLatin1String(path.data(), (int)path.size())) {
				m_pathToChannelCache[path] = i;
				return i;
			}
		}
		return -1;
	}
	return it->second;
}

void GraphModel::appendChannel(const std::string &shv_path, const std::string &name, const core::utils::ShvTypeDescr &type_descr)
{
	m_pathToChannelCache.clear();
	m_channelsInfo.append(ChannelInfo());
	m_samples.append(ChannelSamples());
	auto &chi = m_channelsInfo.last();
	if(!shv_path.empty())
		chi.shvPath = QString::fromStdString(shv_path);
	if(!name.empty())
		chi.name = QString::fromStdString(name);
	chi.typeDescr = type_descr;
	emit channelCountChanged(channelCount());
}

QString GraphModel::typeDescrFieldName(const shv::core::utils::ShvTypeDescr &type_descr, int field_index)
{
	for (const auto &field : type_descr.fields()) {
		if (field_index == field.value().toInt()) {
			return QString::fromStdString(field.name());
		}
	}
	return QString();
}

QString GraphModel::guessTypeName(int channel_ix)
{
	for(int i = 0; i < count(channel_ix); i++) {
		Sample s = sampleValue(channel_ix, i);
		if(s.value.userType() != QMetaType::UnknownType) {
			switch (s.value.userType()) {
			case QMetaType::Bool: return QStringLiteral("Bool");
			case QMetaType::Double: return QStringLiteral("Double");
			case QMetaType::QString: return QStringLiteral("String");
			case QMetaType::Short:
			case QMetaType::Int:
				return QStringLiteral("Int");
			case QMetaType::LongLong:
				return QStringLiteral("Int64");
			case QMetaType::UShort:
			case QMetaType::UInt:
				return QStringLiteral("UInt");
			case QMetaType::ULongLong:
				return QStringLiteral("UInt64");
			default:
				return s.value.typeName();
			}
		}
	}
	return {};
}

}}}
