/*
 * Copyright (C) 2006-2019 Christopho, Solarus - http://www.solarus-games.org
 *
 * Solarus is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Solarus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "solarus/gui/about_dialog.h"
#include <QApplication>

namespace SolarusGui {

/**
 * @brief Creates a main window.
 * @param parent Parent object or nullptr.
 */
AboutDialog::AboutDialog(QWidget* parent) :
  QDialog(parent) {

    // Setup up widgets.
    ui.setupUi(this);

    // Window title
    setWindowTitle(tr("About %1").arg(QApplication::applicationDisplayName()));

    // Set app information in labels
    ui.app_title_label->setText(QApplication::applicationName());
    ui.app_version_label->setText(QApplication::applicationVersion());

    // Remove "?" button on Windows.
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
}

} // namespace SolarusGui
