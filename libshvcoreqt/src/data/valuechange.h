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
		ValueX(TimeStamp value);
		ValueX(int value);
		ValueX(double value);
		ValueX();

		double toDouble(ValueType stored_type) const;
		int toInt(ValueType stored_type) const;

		TimeStamp timeStamp;
		int intValue;
		double doubleValue;
	};
	ValueX valueX;

	union ValueY {
		ValueY(bool value);
		ValueY(int value);
		ValueY(double value);
		ValueY(CustomData *value);
		ValueY();

		double toDouble(ValueType stored_type) const;
		int toInt(ValueType stored_type) const;
		bool toBool(ValueType stored_type) const;

		bool boolValue;
		int intValue;
		double doubleValue;
		CustomData *pointerValue;
	} valueY;

	ValueChange(ValueX value_x, ValueY value_y);
	ValueChange(TimeStamp value_x, ValueY value_y);
	ValueChange(TimeStamp value_x, bool value_y);
	ValueChange(TimeStamp value_x, int value_y);
	ValueChange(TimeStamp value_x, double value_y);
	ValueChange(TimeStamp value_x, CustomData *value_y);
	ValueChange();
};

struct SHVCOREQT_DECL_EXPORT ValueXInterval
{
	ValueXInterval();
	ValueXInterval(ValueChange min_, ValueChange max_, ValueType type_);
	ValueXInterval(ValueChange::ValueX min_, ValueChange::ValueX max_, ValueType type_);
	ValueXInterval(int min_, int max_);
	ValueXInterval(ValueChange::TimeStamp min_, ValueChange::TimeStamp max_);
	ValueXInterval(double min_, double max_);

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

	SerieData();
	SerieData(ValueType x_type, ValueType y_type);

	const_iterator upper_bound(ValueChange::ValueX value_x) const;
	const_iterator upper_bound(const_iterator begin, const_iterator end, ValueChange::ValueX value_x) const;
	const_iterator lower_bound(ValueChange::ValueX value_x) const;
	const_iterator lower_bound(const_iterator begin, const_iterator end, ValueChange::ValueX value_x) const;
	const_iterator findMinYValue(const_iterator begin, const_iterator end, const ValueChange::ValueX x_value) const;
	const_iterator findMinYValue(const ValueChange::ValueX x_value) const;

	const_iterator lessOrEqualIterator(ValueChange::ValueX value_x) const;
	QPair<const_iterator, const_iterator> intersection(const ValueChange::ValueX &start, const ValueChange::ValueX &end, bool &valid) const;

	ValueType xType() const;
	ValueType yType() const;

	ValueXInterval range() const;
	bool addValueChange(const ValueChange &value);
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
