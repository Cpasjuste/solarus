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
#include "solarus/movements/FallingOnFloorMovement.h"

namespace Solarus {

namespace {

/**
 * \brief Y move at each time frame.
 */
const std::string trajectories[4] = {

    "", // none

    "0 -1  0 1", // low

    "0 -1  0 -1  0 -1  0 -1  0 1  0 1  0 1  0 1  0 -1  0 -1  0 1  0 1", // medium

    "0 -4  0 -3  0 -3  0 -2  0 -2  0 -1  0 1  0 2  0 2  0 3  0 3  0 4  "
    "0 -2  0 -1  0 -1  0 -1  0 -1  0 1  0 1  0 1  0 1  0 2  0 -1  0 -1  0 1  0 1" // high
};

}

/**
 * \brief Creates a movement falling object.
 * \param height Height the entity will fall from.
 */
FallingOnFloorMovement::FallingOnFloorMovement(FallingHeight height):
  PixelMovement(trajectories[height], 30, false, true) {

}

}

