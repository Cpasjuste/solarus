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
#ifndef SOLARUS_SOUND_H
#define SOLARUS_SOUND_H

#include "solarus/audio/SoundPtr.h"
#include "solarus/core/Common.h"
#include "solarus/lua/ExportableToLua.h"
#include <string>
#include <list>
#include <map>
#include <al.h>
#include <alc.h>
#include <vorbis/vorbisfile.h>

namespace Solarus {

class Arguments;
class ResourceProvider;
class SoundBuffer;

/**
 * \brief Represents a sound effect that can be played in the program.
 *
 * Represents the state of plyaing the sound effect.
 * This class also handles the initialization of the whole audio system.
 */
class SOLARUS_API Sound: public ExportableToLua {

  public:

    static SoundPtr create(const SoundBuffer& data);
    ~Sound();

    const std::string& get_id() const;
    bool start();
    void stop();
    void set_paused(bool pause);

    static bool exists(const std::string& sound_id);
    static void play(const std::string& sound_id, ResourceProvider& resource_provider);
    static void pause_all();
    static void resume_all();

    static void initialize(const Arguments& args);
    static void quit();
    static bool is_initialized();
    static void update();
    static bool check_openal_clean_state(const std::string& function_name);

    static int get_volume();
    static void set_volume(int volume);

    const std::string& get_lua_type_name() const override;

  private:

    explicit Sound(const SoundBuffer& data);
    bool update_playing();

    const SoundBuffer& data;                     /**< The loaded sound data. */
    ALuint source;                               /**< the source currently playing this sound */

    static ALCdevice* device;
    static ALCcontext* context;

    static std::list<SoundPtr>
        current_sounds;                          /**< the sounds currently playing */

    static bool initialized;                     /**< indicates that the audio system is initialized */
    static float volume;                         /**< the volume of sound effects (0.0 to 1.0) */

    static bool pc_play;                         /**< Whether playing performance counter is used. */
};

}

#endif

