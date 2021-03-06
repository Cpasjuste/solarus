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
#ifndef SOLARUS_HQ2X_FILTER_H
#define SOLARUS_HQ2X_FILTER_H

#include "solarus/core/Common.h"
#include "solarus/graphics/SoftwarePixelFilter.h"
#include <cstdint>

namespace Solarus {

/**
 * \brief Wrapper to the hq2x algorithm.
 */
class Hq2xFilter: public SoftwarePixelFilter {

  public:

    Hq2xFilter();

    virtual int get_scaling_factor() const override;
    virtual void filter(
        const uint32_t* src,
        int src_width,
        int src_height,
        uint32_t* dst
    ) const override;

};

}

#endif

