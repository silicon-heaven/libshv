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
	auto mtid = channelInfo(channel_ix).typeDescr.type;
	for (int i = 0; i < count(channel_ix); ++i) {
		QVariant v = sampleAt(channel_ix, i).value;
		bool ok;
		double d = valueToDouble(v, mtid, &ok);
		if(ok) {
			ret.min = qMin(ret.min, d);
			ret.max = qMax(ret.max, d);
		}
	}
	return ret;
}

static shv::core::utils::ShvLogTypeDescr::Type qt_to_shv_type(int meta_type_id)
{
	using Type = shv::core::utils::ShvLogTypeDescr::Type;
	switch (meta_type_id) {
	case QVariant::Map:
		return Type::Map;
	case QVariant::Double:
		return Type::Double;
	case QVariant::LongLong:
	case QVariant::ULongLong:
	case QVariant::UInt:
	case QVariant::Int:
		return Type::Int;
	case QVariant::Bool:
		return Type::Bool;
	case QVariant::String:
		return Type::String;
	}
	return Type::Invalid;
}

double GraphModel::valueToDouble(const QVariant v, shv::core::utils::ShvLogTypeDescr::Type type_id, bool *ok)
{
	using Type = shv::core::utils::ShvLogTypeDescr::Type;
	if(ok)
		*ok = true;
	if(shv::coreqt::Utils::isValueNotAvailable(v))
		return std::numeric_limits<double>::quiet_NaN();
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
	Sample s = sampleValue(channel, ix);
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
	//for (int i = 0; i < channelCount(); ++i) {
	//	ChannelInfo &chi = channelInfo(i);
	//	if(chi.metaTypeId == QMetaType::UnknownType) {
	//		chi.metaTypeId = guessMetaType(i);
	//	}
	//}
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
	ChannelSamples &dat = m_samples[channel];
	if(!dat.isEmpty() && dat.last().time > sample.time) {
		shvWarning() << channelInfo(channel).shvPath << "channel:" << channel
					 << "ignoring value with lower timestamp than last value (check possibly wrong short-time correction):"
					 << dat.last().time << shv::chainpack::RpcValue::DateTime::fromMSecsSinceEpoch(dat.last().time).toIsoString()
					 << "val:"
					 << sample.time << shv::chainpack::RpcValue::DateTime::fromMSecsSinceEpoch(sample.time).toIsoString();
		return;
	}
	//m_appendSince = qMin(sampleAt.time, m_appendSince);
	//m_appendUntil = qMax(sampleAt.time, m_appendUntil);
	dat.push_back(std::move(sample));
}

void GraphModel::appendValueShvPath(const std::string &shv_path, Sample &&sample)
{
	int ch_ix = pathToChannelIndex(shv_path);
	if(ch_ix < 0) {
		if(isAutoCreateChannels()) {
			appendChannel(shv_path, std::string(), m_typeInfo.typeDescription(shv_path));
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

void GraphModel::appendChannel(const std::string &shv_path, const std::string &name, const shv::core::utils::ShvLogTypeDescr &type_descr)
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

QString GraphModel::typeDescrFieldName(const shv::core::utils::ShvLogTypeDescr &type_descr, int field_index)
{
	for (const auto &field : type_descr.fields) {
		if (field_index == field.value.toInt()) {
			return QString::fromStdString(field.name);
		}
	}
	return QString();
}

//int GraphModel::guessMetaType(int channel_ix)
//{
//	Sample s = sampleValue(channel_ix, 0);
//	return s.value.userType();
//}

}}}
