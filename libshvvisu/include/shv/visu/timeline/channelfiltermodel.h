#pragma once

#include <shv/visu/shvvisuglobal.h>
#include <shv/visu/timeline/graph.h>

#include <QStandardItemModel>
#include <shv/core/stringview.h>

namespace shv::chainpack { class RpcMessage; class RpcValue; }

namespace shv::visu::timeline {

class SHVVISU_DECL_EXPORT ChannelFilterModel : public QStandardItemModel
{
	Q_OBJECT
private:
	using Super = QStandardItemModel;

public:
	ChannelFilterModel(QObject *parent, Graph *graph);
	~ChannelFilterModel() Q_DECL_OVERRIDE;

	void createNodes();
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	bool setData ( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

	QSet<QString> permittedChannels();
	void setPermittedChannels(const QSet<QString> &channels);
	void setItemCheckState(const QModelIndex &mi, Qt::CheckState check_state);
	void fixCheckBoxesIntegrity();
protected:
	enum UserData {ValidLogEntry = Qt::UserRole + 1, DirName, LocalizedDirName};

	QString shvPathFromIndex(const QModelIndex &mi);
	QModelIndex shvPathToIndex(const QString &shv_path);

	QString shvPathFromItem(QStandardItem *it) const;
	QStandardItem* shvPathToItem(const QString &shv_path, QStandardItem *it);

	void createNodesForPath(const QString &path, const QMap<QString, QStringList> &localized_paths);

	void selectedChannels_helper(QSet<QString> *channels, QStandardItem *it);
	void setChildItemsCheckedState(QStandardItem *it, Qt::CheckState check_state);
	void fixChildItemsCheckBoxesIntegrity(QStandardItem *it);
	bool hasCheckedChild(QStandardItem *it);

	QStandardItem* topVisibleParentItem(QStandardItem *it);
	Graph *m_graph = nullptr;
};

}
