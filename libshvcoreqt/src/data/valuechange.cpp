#include <shv/coreqt/data/valuechange.h>

#include <shv/coreqt/exception.h>

#include <algorithm>

namespace shv::coreqt::data {
ValueChange::ValueX::ValueX(TimeStamp value)
	: timeStamp(value)
{
}

ValueChange::ValueX::ValueX(int value)
	: intValue(value)
{
}

ValueChange::ValueX::ValueX(double value)
	: doubleValue(value)
{
}

ValueChange::ValueX::ValueX()
	: intValue(0)
{
}

double ValueChange::ValueX::toDouble(ValueType stored_type) const
{
	switch (stored_type) {
		case ValueType::Int: return intValue;
		case ValueType::Double: return doubleValue;
		case ValueType::TimeStamp: return static_cast<double>(timeStamp);
		default: Q_ASSERT_X(false,"valueX", "Unsupported conversion"); return 0;
	}
}

int ValueChange::ValueX::toInt(ValueType stored_type) const{
	switch (stored_type) {
		case ValueType::Int: return intValue;
		case ValueType::Double: return qRound(doubleValue);
		case ValueType::TimeStamp: return static_cast<int>(timeStamp);
		default: Q_ASSERT_X(false,"valueX", "Unsupported conversion"); return 0;
	}
}

ValueChange::ValueY::ValueY(bool value)
	: boolValue(value)
{
}

ValueChange::ValueY::ValueY(int value)
	: intValue(value)
{
}

ValueChange::ValueY::ValueY(double value)
	: doubleValue(value)
{
}

ValueChange::ValueY::ValueY(CustomData *value)
	: pointerValue(value)
{
}

ValueChange::ValueY::ValueY()
	: intValue(0)
{
}

double ValueChange::ValueY::toDouble(ValueType stored_type) const
{
	switch (stored_type) {
		case ValueType::Int: return intValue;
		case ValueType::Double: return doubleValue;
		case ValueType::Bool: return boolValue;
		default: Q_ASSERT_X(false,"valueY", "Unsupported conversion"); return 0;
	}
}

int ValueChange::ValueY::toInt(ValueType stored_type) const
{
	switch (stored_type) {
		case ValueType::Int: return intValue;
		case ValueType::Double: return qRound(doubleValue);
		case ValueType::Bool: return boolValue;
		default: Q_ASSERT_X(false,"valueY", "Unsupported conversion"); return 0;
	}
}

bool ValueChange::ValueY::toBool(ValueType stored_type) const
{
	switch (stored_type) {
		case ValueType::Int: return (intValue > 0);
		case ValueType::Double: return (doubleValue > 0.0);
		case ValueType::Bool: return boolValue;
		default: Q_ASSERT_X(false,"valueY", "Unsupported conversion"); return false;
	}
}

ValueChange::ValueChange(ValueX value_x, ValueY value_y)
	: valueX(value_x), valueY(value_y)
{
}

ValueChange::ValueChange(TimeStamp value_x, ValueY value_y)
	: ValueChange(ValueX(value_x), value_y)
{
}

ValueChange::ValueChange(TimeStamp value_x, bool value_y)
	: ValueChange(value_x, ValueY(value_y))
{
}

ValueChange::ValueChange(TimeStamp value_x, int value_y)
	: ValueChange(value_x, ValueY(value_y))
{
}

ValueChange::ValueChange(TimeStamp value_x, double value_y)
	: ValueChange(value_x, ValueY(value_y))
{
}

ValueChange::ValueChange(TimeStamp value_x, CustomData *value_y)
	: ValueChange(value_x, ValueY(value_y))
{
}

ValueChange::ValueChange() = default;

SerieData::const_iterator SerieData::lessOrEqualIterator(ValueChange::ValueX value_x) const
{
	auto it = lower_bound(value_x);

	if (it == cend()) {
		if (!empty()) {
			it--;
		}
	}
	else {
		if (!compareValueX(it->valueX, value_x, m_xType)) {
			if (it == cbegin()) {
				it = cend();
			}
			else {
				it--;
			}
		}
	}
	return it;
}

QPair<SerieData::const_iterator, SerieData::const_iterator> SerieData::intersection(const ValueChange::ValueX &start, const ValueChange::ValueX &end, bool &valid) const
{
	QPair<const_iterator, const_iterator> result;
	result.first = lessOrEqualIterator(start);
	result.second = lessOrEqualIterator(end);

	if (result.first == cend() && !empty()){
		result.first = cbegin();
	}

	valid = (result.second != cend());
	return result;
}

ValueType SerieData::xType() const
{
	return m_xType;
}

ValueType SerieData::yType() const
{
	return m_yType;
}

ValueXInterval SerieData::range() const
{
	if (!empty()) {
		return ValueXInterval(at(0).valueX, back().valueX, xType());
	}

	switch (xType()) {
	case ValueType::Double:
		return ValueXInterval(0.0, 0.0);
	case ValueType::Int:
		return ValueXInterval(0, 0);
	case ValueType::TimeStamp:
		return ValueXInterval(0LL, 0LL);
	default:
		SHV_EXCEPTION("Invalid type on X axis");
	}
}

bool SerieData::addValueChange(const ValueChange &value)
{
	int sz = static_cast<int>(size());
	if (sz == 0) {
		push_back(value);
		return true;
	}

	const ValueChange &last = at(static_cast<size_t>(sz - 1));
	if (!compareValueX(last, value, xType()) && !compareValueY(last, value, yType())) {
		push_back(value);
		return true;
	}
	return false;
}

SerieData::iterator SerieData::insertValueChange(const_iterator position, const ValueChange &value)
{
	return insert(position, value);
}

void SerieData::updateValueChange(const_iterator position, const ValueChange &new_value)
{
	Q_ASSERT(position >= cbegin());
	unsigned long index = static_cast<unsigned long>(position - cbegin());
	ValueChange &old_value = at(index);
	if (!compareValueX(old_value, new_value, m_xType)) {
		if ((index > 0 && (compareValueX(new_value, at(index - 1), m_xType) || lessThenValueX(new_value, at(index - 1), m_xType)))
			||
			(index < size() - 1 && (compareValueX(new_value, at(index + 1), m_xType) || greaterThenValueX(new_value, at(index + 1), m_xType)))) {
			SHV_EXCEPTION("updateValueChange: requested change of ValueX would break time sequence");
		}
	}
	old_value = new_value;
}

void SerieData::extendRange(int &min, int &max) const
{
	if (!empty()) {
		min = std::min(at(0).valueX.intValue, min);
		max = std::max(back().valueX.intValue, max);
	}
}

void SerieData::extendRange(double &min, double &max) const
{
	if (!empty()) {
		min = std::min(at(0).valueX.doubleValue, min);
		max = std::max(back().valueX.doubleValue, max);
	}
}

void SerieData::extendRange(ValueChange::TimeStamp &min, ValueChange::TimeStamp &max) const
{
	if (!empty()) {
		min = std::min(at(0).valueX.timeStamp, min);
		max = std::max(back().valueX.timeStamp, max);
	}
}

bool compareValueX(const ValueChange &value1, const ValueChange &value2, ValueType type)
{
	return compareValueX(value1.valueX, value2.valueX, type);
}

bool compareValueX(const ValueChange::ValueX &value1, const ValueChange::ValueX &value2, ValueType type)
{
	switch (type) {
	case ValueType::TimeStamp:
		return value1.timeStamp == value2.timeStamp;
	case ValueType::Double:
		return value1.doubleValue == value2.doubleValue;
	case ValueType::Int:
		return value1.intValue == value2.intValue;
	default:
		SHV_EXCEPTION("Invalid type on valueX");
	}
}

bool compareValueY(const ValueChange &value1, const ValueChange &value2, ValueType type)
{
	return compareValueY(value1.valueY, value2.valueY, type);
}

bool compareValueY(const ValueChange::ValueY &value1, const ValueChange::ValueY &value2, ValueType type)
{
	switch (type) {
	case ValueType::Double:
		return value1.doubleValue == value2.doubleValue;
	case ValueType::Int:
		return value1.intValue == value2.intValue;
	case ValueType::Bool:
		return value1.boolValue == value2.boolValue;
	case ValueType::CustomDataPointer:
		return value1.pointerValue == value2.pointerValue;
	default:
		SHV_EXCEPTION("Invalid type on valueY");
	}
}

SerieData::SerieData()
	: m_xType(ValueType::Int), m_yType(ValueType::Int)
{
}

SerieData::SerieData(ValueType x_type, ValueType y_type)
	: m_xType(x_type), m_yType(y_type)
{
}

SerieData::const_iterator SerieData::upper_bound(ValueChange::ValueX value_x) const
{
	return upper_bound(cbegin(), cend(), value_x);
}

SerieData::const_iterator SerieData::upper_bound(const_iterator begin, const_iterator end, ValueChange::ValueX value_x) const
{
	auto it = cend();
	if (m_xType == ValueType::TimeStamp) {
		it = std::upper_bound(begin, end, value_x.timeStamp, [](ValueChange::TimeStamp x, ValueChange val) {
			return val.valueX.timeStamp > x;
		});
	}
	else if (m_xType == ValueType::Int) {
		it = std::upper_bound(begin, end, value_x.intValue, [](int x, ValueChange val) {
			return val.valueX.intValue > x;
		});
	}
	else if (m_xType == ValueType::Double) {
		it = std::upper_bound(begin, end, value_x.doubleValue, [](double x, ValueChange val) {
			return val.valueX.doubleValue > x;
		});
	}
	return it;
}

SerieData::const_iterator SerieData::lower_bound(ValueChange::ValueX value_x) const
{
	return lower_bound(cbegin(), cend(), value_x);
}

SerieData::const_iterator SerieData::lower_bound(SerieData::const_iterator begin, SerieData::const_iterator end, ValueChange::ValueX value_x) const
{
	auto it = cend();
	if (m_xType == ValueType::TimeStamp) {
		it = std::lower_bound(begin, end, value_x.timeStamp, [](ValueChange val, ValueChange::TimeStamp x) {
			return val.valueX.timeStamp < x;
		});
	}
	else if (m_xType == ValueType::Int) {
		it = std::lower_bound(begin, end, value_x.intValue, [](ValueChange val, int x) {
			return val.valueX.intValue < x;
		});
	}
	else if (m_xType == ValueType::Double) {
		it = std::lower_bound(begin, end, value_x.doubleValue, [](ValueChange val, double x) {
			return val.valueX.doubleValue < x;
		});
	}
	return it;
}

SerieData::const_iterator SerieData::findMinYValue(const_iterator begin, const_iterator end, const ValueChange::ValueX x_value) const
{
	auto it = lower_bound(begin, end, x_value);
	if (it != begin) {
		--it;
	}
	return it;
}

SerieData::const_iterator SerieData::findMinYValue(const ValueChange::ValueX x_value) const
{
	return findMinYValue(cbegin(), cend(), x_value);
}

ValueXInterval::ValueXInterval() = default;

ValueXInterval::ValueXInterval(ValueChange min_, ValueChange max_, ValueType type_)
	: min(min_.valueX), max(max_.valueX), type(type_)
{
}

ValueXInterval::ValueXInterval(ValueChange::ValueX min_, ValueChange::ValueX max_, ValueType type_)
	: min(min_), max(max_), type(type_)
{
}

ValueXInterval::ValueXInterval(int min_, int max_)
	: min(min_), max(max_)
{
}

ValueXInterval::ValueXInterval(ValueChange::TimeStamp min_, ValueChange::TimeStamp max_)
	: min(min_), max(max_), type(ValueType::TimeStamp)
{
}

ValueXInterval::ValueXInterval(double min_, double max_)
	: min(min_), max(max_), type(ValueType::Double)
{
}

bool ValueXInterval::operator==(const ValueXInterval &interval) const
{
	if (type != interval.type) {
		return false;
	}
	return compareValueX(min, interval.min, type) && compareValueX(max, interval.max, type);
}

ValueChange::ValueX ValueXInterval::length() const
{
	if (type == ValueType::TimeStamp) {
		return ValueChange::ValueX(max.timeStamp - min.timeStamp);
	}
	if (type == ValueType::Int) {
		return ValueChange::ValueX(max.intValue - min.intValue);
	}
	if (type == ValueType::Double) {
		return ValueChange::ValueX(max.doubleValue - min.doubleValue);
	}
    SHV_EXCEPTION("Invalid interval type");
}

bool lessThenValueX(const ValueChange &value1, const ValueChange &value2, ValueType type)
{
	return lessThenValueX(value1.valueX, value2.valueX, type);
}

bool lessThenValueX(const ValueChange::ValueX &value1, const ValueChange::ValueX &value2, ValueType type)
{
	switch (type) {
	case ValueType::TimeStamp:
		return value1.timeStamp < value2.timeStamp;
	case ValueType::Double:
		return value1.doubleValue < value2.doubleValue;
	case ValueType::Int:
		return value1.intValue < value2.intValue;
	default:
		SHV_EXCEPTION("Invalid type on valueX");
	}
}

bool greaterThenValueX(const ValueChange &value1, const ValueChange &value2, ValueType type)
{
	return greaterThenValueX(value1.valueX, value2.valueX, type);
}

bool greaterThenValueX(const ValueChange::ValueX &value1, const ValueChange::ValueX &value2, ValueType type)
{
	switch (type) {
	case ValueType::TimeStamp:
		return value1.timeStamp > value2.timeStamp;
	case ValueType::Double:
		return value1.doubleValue > value2.doubleValue;
	case ValueType::Int:
		return value1.intValue > value2.intValue;
	default:
		SHV_EXCEPTION("Invalid type on valueX");
	}
}

}
