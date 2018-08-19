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
#ifndef SOLARUS_GUI_QUESTS_MODEL_H
#define SOLARUS_GUI_QUESTS_MODEL_H

#include "solarus/gui/gui_common.h"
#include "solarus/core/QuestProperties.h"
#include <QAbstractTableModel>
#include <QIcon>
#include <QMetaType>

namespace SolarusGui {

/**
 * @brief List of quests added to Solarus.
 */
class SOLARUS_GUI_API QuestsModel : public QAbstractTableModel {

public:

    /**
     * @brief Info of a quest from the list.
     */
    struct QuestInfo {
      QString path;               /**< Path to the quest directory. */
      QString directory_name;     /**< Name of the quest directory. */
      QIcon icon;                 /**< Icon of the quest. */
      QPixmap logo;               /**< Logo of the quest (recommended: 200x140). */
      Solarus::QuestProperties
          properties;             /**< All properties from quest.dat. */
    };

  explicit QuestsModel(QObject* parent = nullptr);

  // QAbstractTableModel API
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant headerData(int section,
                      Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

  // QuestsModel API
  int path_to_index(const QString& quest_path) const;
  QString index_to_path(int quest_index) const;

  bool has_quest(const QString& quest_path);
  bool add_quest(const QString& quest_path);
  bool remove_quest(int index);
  QStringList get_paths() const;

  Solarus::QuestProperties get_quest_properties(int quest_index) const;

  const QPixmap& get_quest_default_logo() const;
  const QPixmap& get_quest_logo(int index) const;

  const QIcon& get_quest_default_icon() const;

private:

  void load_icon(int quest_index) const;

  mutable std::vector<QuestInfo>
      quests;                   /**< Info of each quest in the list. */
};

}

// Allow to use QuestInfo in QVariant
Q_DECLARE_METATYPE(SolarusGui::QuestsModel::QuestInfo)

#endif
