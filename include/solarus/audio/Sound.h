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

#include "solarus/core/Common.h"
#include "solarus/lua/ExportableToLua.h"
#include <string>
#include <list>
#include <map>
#include <mutex>
#include <al.h>
#include <alc.h>
#include <vorbis/vorbisfile.h>

namespace Solarus {

class Arguments;
class ResourceProvider;

/**
 * \brief Represents a sound effect that can be played in the program.
 *
 * This class also handles the initialization of the whole audio system.
 * To create a sound, prefer the Sound::play() method
 * rather than calling directly the constructor of Sound.
 * This class is the only one that depends on the sound decoding library (libsndfile).
 * This class and the Music class are the only ones that depend on the audio mixer library (OpenAL).
 */
class SOLARUS_API Sound: public ExportableToLua {

  public:

    /**
     * \brief Buffer containing an encoded sound file.
     */
    struct SoundFromMemory {
      std::string data;         /**< The OGG encded data. */
      size_t position;          /**< Current position in the buffer. */
      bool loop;                /**< \c true to restart the sound if it finishes. */
    };

    // functions to load the encoded sound from memory
    static ov_callbacks ogg_callbacks;           /**< vorbisfile object used to load the encoded sound from memory */

    Sound();
    explicit Sound(const std::string& sound_id);
    ~Sound();
    const std::string & get_id() const;
    bool is_loaded() const;
    void load();
    bool start();
    void set_paused(bool pause);

    static bool exists(const std::string& sound_id);
    static void play(const std::string& sound_id, ResourceProvider& resource_provider);
    static void pause_all();
    static void resume_all();

    static void initialize(const Arguments& args);
    static void quit();
    static bool is_initialized();
    static void update();

    static int get_volume();
    static void set_volume(int volume);

    const std::string& get_lua_type_name() const override;

  private:

    ALuint decode_file(const std::string& file_name);
    bool update_playing();

    static ALCdevice* device;
    static ALCcontext* context;

    std::string id;                              /**< id of this sound */
    ALuint buffer;                               /**< the OpenAL buffer containing the PCM decoded data of this sound */
    std::list<ALuint> sources;                   /**< the sources currently playing this sound */
    bool loaded;                                 /**< Whether the sound is loaded. */
    std::mutex load_mutex;                       /**< Lock to protect concurrent sound loading. */

    static std::list<Sound*> current_sounds;     /**< the sounds currently playing */

    static bool initialized;                     /**< indicates that the audio system is initialized */
    static float volume;                         /**< the volume of sound effects (0.0 to 1.0) */

    static bool pc_play;                         /**< Whether playing performance counter is used. */
};

}

#endif

