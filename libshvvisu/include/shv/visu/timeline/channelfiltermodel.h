#pragma once

#include "../shvvisuglobal.h"

#include <QStandardItemModel>
#include <shv/core/stringview.h>

namespace shv { namespace chainpack { class RpcMessage; class RpcValue; }}

namespace shv {
namespace visu {
namespace timeline {

class SHVVISU_DECL_EXPORT ChannelFilterModel : public QStandardItemModel
{
	Q_OBJECT
private:
	using Super = QStandardItemModel;

public:
	ChannelFilterModel(QObject *parent = nullptr);
	~ChannelFilterModel() Q_DECL_OVERRIDE;

	void createNodes(const QSet<QString> &channels, QMap<QString, QString> localized_paths);

	QSet<QString> permittedChannels();
	void setPermittedChannels(const QSet<QString> &channels);
	void setItemCheckState(const QModelIndex &mi, Qt::CheckState check_state);
	void fixCheckBoxesIntegrity();
protected:
	enum UserData {ValidLogEntry = Qt::UserRole + 1, ShvSubPath};

	QString shvPathFromIndex(const QModelIndex &mi);
	QModelIndex shvPathToIndex(const QString &shv_path);

	QString shvPathFromItem(QStandardItem *it) const;
	QStandardItem* shvPathToItem(const QString &shv_path, QStandardItem *it);

	void createNodesForPath(const QString &path, QMap<QString, QString> localized_paths = {});

	void selectedChannels_helper(QSet<QString> *channels, QStandardItem *it);
	bool setData ( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
	void setChildItemsCheckedState(QStandardItem *it, Qt::CheckState check_state);
	void fixChildItemsCheckBoxesIntegrity(QStandardItem *it);
	bool hasCheckedChild(QStandardItem *it);

	QStandardItem* topVisibleParentItem(QStandardItem *it);
};

}
}
}

