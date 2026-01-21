#include <shv/visu/timeline/channelfiltermodel.h>

#include <shv/chainpack/rpcvalue.h>
#include <shv/coreqt/log.h>
#include <shv/core/utils/shvpath.h>

#include <QSet>

namespace shv::visu::timeline {

ChannelFilterModel::ChannelFilterModel(QObject *parent, Graph *graph)
	: Super(parent)
{
	m_graph = graph;
	setColumnCount(1);
	setHorizontalHeaderLabels(QStringList{QString()});
}

ChannelFilterModel::~ChannelFilterModel() = default;

void ChannelFilterModel::createNodes()
{
	beginResetModel();

	const auto channels = m_graph->channelPaths();
	QStringList sorted_channels = QStringList(channels.begin(), channels.end());
	sorted_channels.sort(Qt::CaseInsensitive);

	QMap<QString, QStringList> localized_channel_paths;

	if (m_graph->style().isLocalizeShvPath())
		localized_channel_paths = m_graph->localizedChannelPaths();

	for (const auto &p: sorted_channels) {
		createNodesForPath(p, localized_channel_paths);
	}

	endResetModel();
}

QVariant ChannelFilterModel::data(const QModelIndex &index, int role) const
{
	switch (role) {
	case Qt::DisplayRole: {
		if (index.isValid()) {
			QStandardItem *it = itemFromIndex(index);

			if (!m_graph->style().isLocalizeShvPath()) {
				return it->data(UserData::DirName);
			}

			auto locname = it->data(UserData::LocalizedDirName).toString();
			return (locname.isEmpty())? it->data(UserData::DirName): locname;
		}
	}
	}
	return Super::data(index, role);
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

	path.append(it->data(UserData::DirName).toString());

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

		for (int r = 0; r < it->rowCount(); r++) {
			QStandardItem *child_it = it->child(r, 0);

			if (sub_path == child_it->data(UserData::DirName).toString()) {
				return (sl.empty()) ? child_it : shvPathToItem(sl.join("/"), child_it);
			}
		}
	}

	return nullptr;
}

void ChannelFilterModel::createNodesForPath(const QString &path, const QMap<QString, QStringList> &localized_paths)
{
	QStringList path_list = path.split("/");
	QStringList localized_path_list = localized_paths.value(path);
	QStandardItem *parent_item = invisibleRootItem();

	for(int i = 0; i < path_list.size(); i++) {
		QString sub_path = path_list.mid(0, i+1).join("/");
		QStandardItem *it = shvPathToItem(sub_path, invisibleRootItem());

		if (!it) {
			const auto& dir_name = path_list.at(i);
			auto *item = new QStandardItem(dir_name);
			item->setData(dir_name, UserData::DirName);

			if (path_list.size() == localized_path_list.size()) {
				item->setData(localized_path_list.at(i), UserData::LocalizedDirName);
			}

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
