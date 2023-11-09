#include "channelfiltermodel.h"

#include <shv/chainpack/rpcvalue.h>
#include <shv/coreqt/log.h>
#include <shv/core/utils/shvpath.h>

#include <QSet>

namespace shv::visu::timeline {

ChannelFilterModel::ChannelFilterModel(QObject *parent)
	: Super(parent)
{
	setColumnCount(1);
	setHorizontalHeaderLabels(QStringList{QString()});
}

ChannelFilterModel::~ChannelFilterModel() = default;

void ChannelFilterModel::createNodes(const QSet<QString> &channels)
{
	beginResetModel();

	QStringList sorted_channels = QStringList(channels.begin(), channels.end());
	sorted_channels.sort(Qt::CaseInsensitive);

	for (const auto &p: sorted_channels) {
		createNodesForPath(p);
	}

	endResetModel();
}

QSet<QString> ChannelFilterModel::permittedChannels()
{
	QSet<QString> channels;
	selectedChannels_helper(&channels, invisibleRootItem());
	return channels;
}

void ChannelFilterModel::selectedChannels_helper(QSet<QString> *channels, QStandardItem *it)
{
	if ((it != invisibleRootItem()) && (it->data(UserData::ValidLogEntry).toBool()) && (it->checkState() == Qt::CheckState::Checked)){
		channels->insert(shvPathFromItem(it));
	}

	for (int row = 0; row < it->rowCount(); row++) {
		QStandardItem *child = it->child(row);
		selectedChannels_helper(channels, child);
	}
}

void ChannelFilterModel::setPermittedChannels(const QSet<QString> &channels)
{
	setChildItemsCheckedState(invisibleRootItem(), Qt::CheckState::Unchecked);

	for (const auto &ch: channels) {
		QStandardItem *it = shvPathToItem(ch, invisibleRootItem());

		if (it) {
			it->setCheckState(Qt::CheckState::Checked);
		}
	}

	fixChildItemsCheckBoxesIntegrity(invisibleRootItem());
}

void ChannelFilterModel::setItemCheckState(const QModelIndex &mi, Qt::CheckState check_state)
{
	if (mi.isValid()) {
		QStandardItem *it = itemFromIndex(mi);

		if (it) {
			it->setCheckState(check_state);
		}
	}
}

void ChannelFilterModel::fixCheckBoxesIntegrity()
{
	fixChildItemsCheckBoxesIntegrity(invisibleRootItem());
}

QString ChannelFilterModel::shvPathFromItem(QStandardItem *it) const
{
	QString path;
	QStandardItem *parent_it = it->parent();

	if (parent_it && parent_it != invisibleRootItem()) {
		path = shvPathFromItem(parent_it) + '/';
	}

	path.append(it->text());

	return path;
}


QString ChannelFilterModel::shvPathFromIndex(const QModelIndex &mi)
{
	QStandardItem *it = itemFromIndex(mi);
	QString document_path;

	if (it != nullptr){
		document_path = shvPathFromItem(it);
	}

	return document_path;
}

QModelIndex ChannelFilterModel::shvPathToIndex(const QString &shv_path)
{
	return indexFromItem(shvPathToItem(shv_path, invisibleRootItem()));
}

QStandardItem *ChannelFilterModel::shvPathToItem(const QString &shv_path, QStandardItem *it)
{
	QStringList sl = shv_path.split("/");
	if (!sl.empty() && it){
		QString sub_path = sl.first();

		sl.removeFirst();

		for (int r = 0; r < it->rowCount(); r++){
			QStandardItem *child_it = it->child(r, 0);

			if (sub_path == child_it->text()){
				return (sl.empty()) ? child_it : shvPathToItem(sl.join("/"), child_it);
			}
		}
	}

	return nullptr;
}

void ChannelFilterModel::createNodesForPath(const QString &path)
{
	QStringList path_list = path.split("/");
	QStandardItem *parent_item = invisibleRootItem();

	for(int i = 0; i < path_list.size(); i++) {
		QString sub_path = path_list.mid(0, i+1).join("/");
		QStandardItem *it = shvPathToItem(sub_path, invisibleRootItem());

		if (!it) {
			auto *item = new QStandardItem(path_list.at(i));
			item->setCheckable(true);
			item->setFlags(item->flags() & ~Qt::ItemIsEditable);
			bool has_valid_log_entry = (sub_path == path);
			item->setData(has_valid_log_entry, UserData::ValidLogEntry);
			parent_item->appendRow(item);
			parent_item = item;
		}
		else {
			if (sub_path == path) {
				it->setData(true, UserData::ValidLogEntry);
			}
			parent_item = it;
		}
	}
}

bool ChannelFilterModel::setData(const QModelIndex &ix, const QVariant &val, int role)
{
	bool ret = Super::setData(ix, val, role);

	if (role == Qt::CheckStateRole) {
		QStandardItem *it = itemFromIndex(ix);
		setChildItemsCheckedState(it, static_cast<Qt::CheckState>(val.toInt()));
		fixChildItemsCheckBoxesIntegrity(topVisibleParentItem(it));
	}

	return ret;
}

void ChannelFilterModel::setChildItemsCheckedState(QStandardItem *it, Qt::CheckState check_state)
{
	for (int row = 0; row < it->rowCount(); row++) {
		QStandardItem *child = it->child(row);
		child->setCheckState(check_state);
		setChildItemsCheckedState(child, check_state);
	}
}

void ChannelFilterModel::fixChildItemsCheckBoxesIntegrity(QStandardItem *it)
{
	if (it != invisibleRootItem()) {
		Qt::CheckState check_state = (hasCheckedChild(it)) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked;

		if (it->checkState() != check_state) {
			it->setCheckState(check_state);
		}
	}

	for (int row = 0; row < it->rowCount(); row++) {
		fixChildItemsCheckBoxesIntegrity(it->child(row));
	}
}

bool ChannelFilterModel::hasCheckedChild(QStandardItem *it)
{
	if ((it->data(UserData::ValidLogEntry).toBool()) && (it->checkState() == Qt::CheckState::Checked)) {
		return true;
	}

	for(int row = 0; row < it->rowCount(); row++) {
		QStandardItem *child = it->child(row);

		if (hasCheckedChild(child)) {
			return true;
		}
	}

	return false;
}

QStandardItem *ChannelFilterModel::topVisibleParentItem(QStandardItem *it)
{
	return (it->parent() == nullptr) ? it : topVisibleParentItem(it->parent());
}

}
