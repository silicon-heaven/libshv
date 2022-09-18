#pragma once

#include "../shvcoreqtglobal.h"

#include <QPair>
#include <QVector>
#include <math.h>
#include <vector>

namespace shv {
namespace coreqt {
namespace data {


class SHVCOREQT_DECL_EXPORT CustomData
{
public:
};

enum class ValueType { TimeStamp, Int, Double, Bool, CustomDataPointer };

struct SHVCOREQT_DECL_EXPORT ValueChange
{
	using TimeStamp = qint64;
	union ValueX {
		ValueX(TimeStamp value) : timeStamp(value) {}
		ValueX(int value) : intValue(value) {}
		ValueX(double value) : doubleValue(value) {}
		ValueX() : intValue(0) {}

		double toDouble(ValueType stored_type) const{
			switch (stored_type) {
				case ValueType::Int: return intValue;
				case ValueType::Double: return doubleValue;
				case ValueType::TimeStamp: return timeStamp;
				default: Q_ASSERT_X(false,"valueX", "Unsupported conversion"); return 0;
			}
		}

		int toInt(ValueType stored_type) const{
			switch (stored_type) {
				case ValueType::Int: return intValue;
				case ValueType::Double: return qRound(doubleValue);
				case ValueType::TimeStamp: return timeStamp;
				default: Q_ASSERT_X(false,"valueX", "Unsupported conversion"); return 0;
			}
		}

		TimeStamp timeStamp;
		int intValue;
		double doubleValue;
	} valueX;

	union ValueY {
		ValueY(bool value) : boolValue(value) {}
		ValueY(int value) : intValue(value) {}
		ValueY(double value) : doubleValue(value) {}
		ValueY(CustomData *value) : pointerValue(value) {}
		ValueY() : intValue(0) {}

		double toDouble(ValueType stored_type) const{
			switch (stored_type) {
				case ValueType::Int: return intValue;
				case ValueType::Double: return doubleValue;
				case ValueType::Bool: return boolValue;
				default: Q_ASSERT_X(false,"valueY", "Unsupported conversion"); return 0;
			}
		}

		int toInt(ValueType stored_type) const{
			switch (stored_type) {
				case ValueType::Int: return intValue;
				case ValueType::Double: return qRound(doubleValue);
				case ValueType::Bool: return boolValue;
				default: Q_ASSERT_X(false,"valueY", "Unsupported conversion"); return 0;
			}
		}

		bool toBool(ValueType stored_type) const{
			switch (stored_type) {
				case ValueType::Int: return (intValue > 0);
				case ValueType::Double: return (doubleValue > 0.0);
				case ValueType::Bool: return boolValue;
				default: Q_ASSERT_X(false,"valueY", "Unsupported conversion"); return false;
			}
		}

		bool boolValue;
		int intValue;
		double doubleValue;
		CustomData *pointerValue;
	} valueY;

	ValueChange(ValueX value_x, ValueY value_y) : valueX(value_x), valueY(value_y) {}
	ValueChange(TimeStamp value_x, ValueY value_y) : ValueChange(ValueX(value_x), value_y) {}
	ValueChange(TimeStamp value_x, bool value_y) : ValueChange(value_x, ValueY(value_y)) {}
	ValueChange(TimeStamp value_x, int value_y) : ValueChange(value_x, ValueY(value_y)) {}
	ValueChange(TimeStamp value_x, double value_y) : ValueChange(value_x, ValueY(value_y)) {}
	ValueChange(TimeStamp value_x, CustomData *value_y) : ValueChange(value_x, ValueY(value_y)) {}
	ValueChange() {}
};

struct SHVCOREQT_DECL_EXPORT ValueXInterval
{
	ValueXInterval() = default;
	inline ValueXInterval(ValueChange min, ValueChange max, ValueType type) : min(min.valueX), max(max.valueX), type(type) {}
	inline ValueXInterval(ValueChange::ValueX min, ValueChange::ValueX max, ValueType type) : min(min), max(max), type(type) {}
	inline ValueXInterval(int min, int max) : min(min), max(max), type(ValueType::Int) {}
	inline ValueXInterval(ValueChange::TimeStamp min, ValueChange::TimeStamp max) : min(min), max(max), type(ValueType::TimeStamp) {}
	inline ValueXInterval(double min, double max) : min(min), max(max), type(ValueType::Double) {}

	bool operator==(const ValueXInterval &interval) const;

	ValueChange::ValueX length() const;
	ValueChange::ValueX min;
	ValueChange::ValueX max;
	ValueType type = ValueType::Int;
};

SHVCOREQT_DECL_EXPORT bool compareValueX(const ValueChange &value1, const ValueChange &value2, ValueType type);
SHVCOREQT_DECL_EXPORT bool compareValueX(const ValueChange::ValueX &value1, const ValueChange::ValueX &value2, ValueType type);

SHVCOREQT_DECL_EXPORT bool lessThenValueX(const ValueChange &value1, const ValueChange &value2, ValueType type);
SHVCOREQT_DECL_EXPORT bool lessThenValueX(const ValueChange::ValueX &value1, const ValueChange::ValueX &value2, ValueType type);

SHVCOREQT_DECL_EXPORT bool greaterThenValueX(const ValueChange &value1, const ValueChange &value2, ValueType type);
SHVCOREQT_DECL_EXPORT bool greaterThenValueX(const ValueChange::ValueX &value1, const ValueChange::ValueX &value2, ValueType type);

SHVCOREQT_DECL_EXPORT bool compareValueY(const ValueChange &value1, const ValueChange &value2, ValueType type);
SHVCOREQT_DECL_EXPORT bool compareValueY(const ValueChange::ValueY &value1, const ValueChange::ValueY &value2, ValueType type);

class SHVCOREQT_DECL_EXPORT SerieData : public std::vector<ValueChange>
{
	using Super = std::vector<ValueChange>;

public:
	class Interval
	{
	public:
		const_iterator begin;
		const_iterator end;
	};

	SerieData() : m_xType(ValueType::Int), m_yType(ValueType::Int)	{}
	SerieData(ValueType x_type, ValueType y_type) : m_xType(x_type), m_yType(y_type) {}

	const_iterator upper_bound(ValueChange::ValueX value_x) const;
	const_iterator upper_bound(const_iterator begin, const_iterator end, ValueChange::ValueX value_x) const;
	const_iterator lower_bound(ValueChange::ValueX value_x) const;
	const_iterator lower_bound(const_iterator begin, const_iterator end, ValueChange::ValueX value_x) const;
	const_iterator findMinYValue(const_iterator begin, const_iterator end, const ValueChange::ValueX x_value) const;
	const_iterator findMinYValue(const ValueChange::ValueX x_value) const;

	const_iterator lessOrEqualIterator(ValueChange::ValueX value_x) const;
	QPair<const_iterator, const_iterator> intersection(const ValueChange::ValueX &start, const ValueChange::ValueX &end, bool &valid) const;

	ValueType xType() const	{ return m_xType; }
	ValueType yType() const	{ return m_yType; }

	ValueXInterval range() const;
	bool addValueChange(const ValueChange &value, bool check_changes = true);
	iterator insertValueChange(const_iterator position, const ValueChange &value);
	void updateValueChange(const_iterator position, const ValueChange &new_value);

	void extendRange(int &min, int &max) const;
	void extendRange(double &min, double &max) const;
	void extendRange(ValueChange::TimeStamp &min, ValueChange::TimeStamp &max) const;

private:
	ValueType m_xType;
	ValueType m_yType;
};

class SHVCOREQT_DECL_EXPORT SerieDataList : public QVector<SerieData>
{
};

} //namespace data
} //namespace coreqt
} //namespace shv
