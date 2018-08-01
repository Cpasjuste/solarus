/*
 * Copyright (C) 2006-2018 Christopho, Solarus - http://www.solarus-games.org
 *
 * Solarus Quest Editor is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Solarus Quest Editor is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "solarus/gui/quests_model.h"
#include "solarus/core/CurrentQuest.h"
#include "solarus/core/QuestFiles.h"
#include "solarus/core/QuestProperties.h"
#include <QApplication>
#include <algorithm>

namespace SolarusGui {

/**
 * @brief Creates a quests view.
 * @param parent Parent object or nullptr.
 */
QuestsModel::QuestsModel(QObject* parent) :
  QAbstractListModel(parent),
  quests() {

}

/**
 * @brief Returns the number of quests.
 * @param parent Parent index.
 * @return The number of quests.
 */
int QuestsModel::rowCount(const QModelIndex& parent) const {

  Q_UNUSED(parent);
  return quests.size();
}

/**
 * @brief Returns the data of an item for the given role.
 * @param index Index of the item to get.
 * @param role Kind of data to get.
 * @return The data for this item and this role.
 */
QVariant QuestsModel::data(const QModelIndex& index, int role) const {

  if (index.row() < 0 || index.row() >= rowCount()) {
    return QVariant();
  }

  switch (role) {

  case Qt::ItemDataRole::DisplayRole:
    load_icon(index.row());
    return QVariant::fromValue(quests.at(index.row()));

  case Qt::ItemDataRole::ToolTipRole:
    return QString::fromStdString(
            quests.at(index.row()).properties.get_title());

  default:
    return QVariant();
  }
}

/**
 * @brief Finds the index of a quest in the list.
 * @param quest_path Path of the quest to look for.
 * @return The quest index, or -1 if it is not in the list.
 */
int QuestsModel::path_to_index(const QString& quest_path) const {

  for (size_t i = 0; i < quests.size(); ++i) {
    if (quests[i].path == quest_path) {
      return i;
    }
  }

  return -1;
}

/**
 * @brief Returns the quest path at the given index.
 * @param quest_index Index of the quest to get.
 * @return The path, or an empty string if the index is invalid.
 */
QString QuestsModel::index_to_path(int quest_index) const {

  if (quest_index < 0 || quest_index >= (int) quests.size()) {
    return QString();
  }

  return quests[quest_index].path;
}

/**
 * @brief Returns whether the model contains the given quest path.
 * @param quest_path Quest path to test.
 * @return @c true if it is in the model.
 */
bool QuestsModel::has_quest(const QString& quest_path) {

  return path_to_index(quest_path) != -1;
}

/**
 * @brief Adds a quest to the model.
 * @param quest_path Path of the quest to add.
 * @return @c true if it was added, @c false if there is no such quest
 * or if it is already there.
 */
bool QuestsModel::add_quest(const QString& quest_path) {

  if (has_quest(quest_path)) {
    return false;
  }

  QuestInfo info;

  // Open the quest to get its quest.dat file.
  QStringList arguments = QApplication::arguments();
  QString program_name = arguments.isEmpty() ? QString() : arguments.first();
  if (!Solarus::QuestFiles::open_quest(program_name.toLocal8Bit().toStdString(),
                                       quest_path.toLocal8Bit().toStdString())) {
    Solarus::QuestFiles::close_quest();
    return false;
  }
  info.properties = Solarus::CurrentQuest::get_properties();
  Solarus::QuestFiles::close_quest();

  const int num_quests = rowCount();
  beginInsertRows(QModelIndex(), num_quests, num_quests);

  info.path = quest_path;
  info.directory_name = quest_path.section('/', -1, -1, QString::SectionSkipEmpty);
  quests.push_back(info);

  endInsertRows();

  return true;
}

/**
 * @brief Removes a quest from this model.
 * @param quest_index Index of the quest to remove.
 * @return @c true if the quest was removed, @c false if there is no quest
 * with this index.
 */
bool QuestsModel::remove_quest(int quest_index) {

  if (quest_index < 0 || quest_index > rowCount()) {
    return false;
  }

  beginRemoveRows(QModelIndex(), quest_index, quest_index);
  quests.erase(quests.begin() + quest_index);
  endRemoveRows();
  return true;
}

/**
 * @brief Returns the list of quests paths in the model.
 * @return The quests paths.
 */
QStringList QuestsModel::get_paths() const {

  QStringList paths;
  for (const QuestInfo& info : quests) {
    paths << info.path;
  }
  return paths;
}

/**
 * @brief Returns the properties of a quest in the model.
 * @param quest_index Index of the quest to get.
 * @return The quest properties.
 */
Solarus::QuestProperties QuestsModel::get_quest_properties(int quest_index) const {

  if (quest_index < 0 || quest_index > rowCount()) {
    return Solarus::QuestProperties();
  }

  return quests[quest_index].properties;
}

/**
 * @brief Returns the default logo for a quest.
 * @return The default logo
 */
const QPixmap& QuestsModel::get_quest_default_logo() const {

  static const QPixmap default_logo(":/images/no_logo.png");

  return default_logo;
}

/**
 * @brief Returns the quest logo.
 * @param quest_index Index of the quest to get.
 * @return The quest logo.
 */
const QPixmap& QuestsModel::get_quest_logo(int quest_index) const {

  if (quest_index < 0 || quest_index > rowCount()) {
    return get_quest_default_logo();
  }

  QuestInfo& quest = quests[quest_index];
  if (quest.logo.isNull()) {
    // Lazily load the logo.
    quest.logo = get_quest_default_logo();

    QStringList arguments = QApplication::arguments();
    QString program_name = arguments.isEmpty() ? QString() : arguments.first();
    if (Solarus::QuestFiles::open_quest(program_name.toLocal8Bit().toStdString(),
                                        quest.path.toLocal8Bit().toStdString())) {
      std::string file_name = "logos/logo.png";
      if (Solarus::QuestFiles::data_file_exists(file_name) &&
          !Solarus::QuestFiles::data_file_is_dir(file_name)) {
        std::string buffer = Solarus::QuestFiles::data_file_read(file_name);
        QPixmap pixmap;
        if (pixmap.loadFromData((const uchar*) buffer.data(), (uint) buffer.size())) {
          quests[quest_index].logo = pixmap;
        }
      }
    }
    Solarus::QuestFiles::close_quest();
  }

  return quest.logo;
}

/**
 * @brief Returns the default icon for a quest.
 * @return The default icon.
 */
const QIcon& QuestsModel::get_quest_default_icon() const {

  static const QIcon default_icon(":/images/default_icon.png");

  return default_icon;
}

/**
 * @brief Loads the icon of the given quest.
 * @param quest_index Index of a quest.
 */
void QuestsModel::load_icon(int quest_index) const {

  if (quest_index < 0 || quest_index > rowCount()) {
    return;
  }

  QuestInfo& quest = quests[quest_index];
  QIcon& icon = quest.icon;
  if (!icon.isNull()) {
    // Already loaded.
    return;
  }

  QStringList arguments = QApplication::arguments();
  QString program_name = arguments.isEmpty() ? QString() : arguments.first();
  if (!Solarus::QuestFiles::open_quest(program_name.toLocal8Bit().toStdString(),
                                       quest.path.toLocal8Bit().toStdString())) {
    Solarus::QuestFiles::close_quest();
    icon = get_quest_default_icon();
    return;
  }

  QStringList file_names = {
    "logos/icon_16.png",
    "logos/icon_24.png",
    "logos/icon_32.png",
    "logos/icon_48.png",
    "logos/icon_64.png",
    "logos/icon_128.png",
    "logos/icon_256.png",
    "logos/icon_512.png",
    "logos/icon_1024.png",
  };
  for (const QString& file_name : file_names) {
    if (Solarus::QuestFiles::data_file_exists(file_name.toLocal8Bit().toStdString())) {
      std::string buffer = Solarus::QuestFiles::data_file_read(file_name.toLocal8Bit().toStdString());
      QPixmap pixmap;
      if (!pixmap.loadFromData((const uchar*) buffer.data(), (uint) buffer.size())) {
        continue;
      }
      icon.addPixmap(pixmap);
    }
  }
  Solarus::QuestFiles::close_quest();

  if (icon.isNull()) {
    icon = get_quest_default_icon();
  }
}

/**
 * @brief Sort the quests and notify the view (more convenient than sort(int, order).
 * @param sortType Way to sort the quests in the list by
 * @param order Order to the quests in the list by
 */
void QuestsModel::sort(QuestSort sortType, Qt::SortOrder order) {

  do_sort(sortType, order);
}

/**
 * @brief Sort the quests and notify the view
 * @param column int value of a QuestSort value, column to sort the quests by
 * @param order Order to the quests in the list by
 */
void QuestsModel::sort(int column, Qt::SortOrder order) {

  do_sort(static_cast<QuestSort>(column), order);
}

/**
 * @brief Do the actual sort, without notifying the view
 * @param sortType Way to sort the quests in the list by
 * @param order Order to sort the quests in the list by
 */
void QuestsModel::do_sort(QuestSort sortType, Qt::SortOrder order) {

  std::sort(quests.begin(), quests.end(),
    [sortType, order](const QuestInfo& a, const QuestInfo& b) {
      const bool ascending = order == Qt::AscendingOrder;
      switch (sortType) {
      case QuestSort::SortByAuthor:
        return ascending ? a.properties.get_author() < b.properties.get_author()
                         : a.properties.get_author() > b.properties.get_author();
      case QuestSort::SortByDate:
        return ascending ? a.properties.get_release_date() < b.properties.get_release_date()
                         : a.properties.get_release_date() > b.properties.get_release_date();
      case QuestSort::SortByName:
      default:
        return ascending ? a.properties.get_title() < b.properties.get_title()
                         : a.properties.get_title() > b.properties.get_title();
      }
    });
}

} // namespace SolarusGui
