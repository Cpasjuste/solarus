/*
 * Copyright (C) 2006-2018 Christopho, Solarus - http://www.solarus-games.org
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
#ifndef SOLARUS_SHADERCONTEXT_H
#define SOLARUS_SHADERCONTEXT_H

#include "solarus/core/Common.h"
#include "solarus/graphics/Shader.h"
#include <memory>
#include <string>

namespace Solarus {

/**
 * \brief Shader context management.
 *
 * This class allows to handle the display context and create a shader.
 */
class ShaderContext {

  public:

    static bool initialize();
    static void quit();

    static ShaderPtr create_shader(const std::string& shader_id);
};

}

#endif
