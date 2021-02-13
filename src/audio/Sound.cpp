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
#include "solarus/audio/Music.h"
#include "solarus/audio/Sound.h"
#include "solarus/core/Arguments.h"
#include "solarus/core/CurrentQuest.h"
#include "solarus/core/Debug.h"
#include "solarus/core/PerfCounter.h"
#include "solarus/core/QuestFiles.h"
#include "solarus/core/ResourceProvider.h"
#include "solarus/core/String.h"
#include "solarus/lua/LuaContext.h"
#include <algorithm>
#include <cstdio>

namespace Solarus {

ALCdevice* Sound::device = nullptr;
ALCcontext* Sound::context = nullptr;
bool Sound::initialized = false;
float Sound::volume = 1.0;
bool Sound::pc_play = false;
std::list<SoundPtr> Sound::current_sounds;
bool Sound::paused_by_system = false;

/**
 * \brief Creates a new Ogg Vorbis sound.
 * \param data The loaded sound data ready to be played.
 */
Sound::Sound(const SoundBuffer& data):
  data(data),
  source(AL_NONE),
  paused_by_script(false) {
}

/**
 * \brief Destroys the sound.
 */
Sound::~Sound() {

  if (is_initialized() && source != AL_NONE) {
    stop_source();
  }
}

/**
 * \brief Creates a new Ogg Vorbis sound.
 * \param data The loaded sound data ready to be played.
 */
SoundPtr Sound::create(const SoundBuffer& data) {
  Sound* sound = new Sound(data);
  return SoundPtr(sound);
}

/**
 * \brief Initializes the audio (music and sound) system.
 *
 * This method should be called when the application starts.
 * If the argument -no-audio is provided, this function has no effect and
 * there will be no sound.
 * If the argument -perf-sound-play is provided and is "yes", sound
 * playing will be accounted using a performance counter.
 *
 * \param args Command-line arguments.
 */
void Sound::initialize(const Arguments& args) {

  // Check the -no-audio option.
  const bool disable = args.has_argument("-no-audio");
  if (disable) {
    return;
  }

  // Check the -perf-sound-play option.
  pc_play = args.get_argument_value("-perf-sound-play") == "yes";

  // Initialize OpenAL.

  device = alcOpenDevice(nullptr);
  if (!device) {
    Debug::error("Cannot open audio device");
    return;
  }

  context = alcCreateContext(device, nullptr);
  if (!context) {
    Debug::error("Cannot create audio context");
    alcCloseDevice(device);
    return;
  }
  if (!alcMakeContextCurrent(context)) {
    Debug::error("Cannot activate audio context");
    alcDestroyContext(context);
    alcCloseDevice(device);
    return;
  }

  alGenBuffers(0, nullptr);  // Necessary on some systems to avoid errors with the first sound loaded.

  initialized = true;
  set_volume(100);

  // initialize the music system
  Music::initialize();
}

/**
 * \brief Closes the audio (music and sound) system.
 *
 * This method should be called when exiting the application.
 */
void Sound::quit() {

  if (is_initialized()) {

    // uninitialize the music subsystem
    Music::quit();

    // uninitialize OpenAL

    alcMakeContextCurrent(nullptr);
    alcDestroyContext(context);
    context = nullptr;
    alcCloseDevice(device);
    device = nullptr;
    volume = 1.0;

    initialized = false;
  }
}

/**
 * \brief Returns whether the audio (music and sound) system is initialized.
 * \return true if the audio (music and sound) system is initilialized
 */
bool Sound::is_initialized() {
  return initialized;
}

/**
 * \brief Returns the id of this sound.
 * \return The sound id.
 */
const std::string& Sound::get_id() const {
  return data.get_id();
}

/**
 * \brief Returns whether a sound exists.
 * \param sound_id id of the sound to test
 * \return true if the sound exists
 */
bool Sound::exists(const std::string& sound_id) {

  std::ostringstream oss;
  oss << "sounds/" << sound_id << ".ogg";
  return QuestFiles::data_file_exists(oss.str());
}

/**
 * \brief Starts playing the specified sound.
 * \param sound_id Id of the sound to play.
 * \param resource_provider The resource provider.
 */
void Sound::play(const std::string& sound_id, ResourceProvider& resource_provider) {
  if (pc_play) {
    PerfCounter::update("sound-play");
  }

  SoundBuffer& buffer = resource_provider.get_sound(sound_id);
  SoundPtr sound = Sound::create(buffer);
  sound->start();
}

/**
 * \brief Returns the current volume of sound effects.
 * \return the volume (0 to 100)
 */
int Sound::get_volume() {

  return (int) (volume * 100.0 + 0.5);
}

/**
 * \brief Sets the volume of sound effects.
 * \param volume the new volume (0 to 100)
 */
void Sound::set_volume(int volume) {

  volume = std::min(100, std::max(0, volume));
  Sound::volume = volume / 100.0;
}

/**
 * \brief Updates the audio (music and sound) system.
 *
 * This function is called repeatedly by the game.
 */
void Sound::update() {

  // update the playing sounds
  std::list<SoundPtr> sounds_to_remove;
  for (const SoundPtr& sound: current_sounds) {
    if (!sound->update_playing()) {
      sounds_to_remove.push_back(sound);
    }
  }

  for (const SoundPtr& sound: sounds_to_remove) {
    current_sounds.remove(sound);
  }

  // also update the music
  Music::update();
}

/**
 * \brief Updates this sound when it is playing.
 * \return true if the sound is still playing, false if it is finished.
 */
bool Sound::update_playing() {

  check_openal_clean_state("Sound::update_playing");

  // See if this sound is still playing.
  if (source == AL_NONE) {
    return false;
  }

  ALint status;
  alGetSourcei(source, AL_SOURCE_STATE, &status);

  if (status == AL_STOPPED) {
    stop_source();
  }

  return source != AL_NONE;
}

/**
 * \brief Plays the sound.
 * \return \c true if the sound was started successfully, \c false otherwise.
 */
bool Sound::start() {

  if (!is_initialized()) {
    // Sound might be disabled.
    return false;
  }

  if (!check_openal_clean_state("Sound::start")) {
    return false;
  }

  ALuint buffer = data.get_buffer();
  if (buffer == AL_NONE) {
    return false;
  }

  if (source == AL_NONE) {
    // create a source
    alGenSources(1, &source);
    alSourcei(source, AL_BUFFER, buffer);
    alSourcef(source, AL_GAIN, volume);

    // play the sound
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
      std::ostringstream oss;
      oss << "Cannot attach buffer " << buffer
          << " to source " << source << " to play sound '" << get_id() << "': error " << std::hex << error;
      Debug::error(oss.str());
      alDeleteSources(1, &source);
      source = AL_NONE;
      return false;
    }
  }

  SoundPtr shared_this = std::static_pointer_cast<Sound>(shared_from_this());
  current_sounds.remove(shared_this);  // To avoid duplicates.
  current_sounds.push_back(shared_this);
  alSourcePlay(source);
  ALenum error = alGetError();
  if (error != AL_NO_ERROR) {
    std::ostringstream oss;
    oss << "Cannot play sound '" << get_id() << "': error " << std::hex << error;
    Debug::error(oss.str());
    return false;
  }

  return true;
}

/**
 * \brief Stops playing this sound.
 */
void Sound::stop() {

  if (!is_initialized()) {
    return;
  }

  check_openal_clean_state("Sound::stop");

  if (source == AL_NONE) {
    // Nothing to do.
    return;
  }

  ALint status;
  alGetSourcei(source, AL_SOURCE_STATE, &status);
  if (status == AL_PLAYING || status == AL_PAUSED) {
    stop_source();
  }
}

/**
 * \brief Stops playing the sound.
 */
void Sound::stop_source() {

  if (source == AL_NONE) {
    return;
  }

  alSourceStop(source);
  alSourcei(source, AL_BUFFER, 0);
  alDeleteSources(1, &source);

  int error = alGetError();
  if (error != AL_NO_ERROR) {
    std::ostringstream oss;
    oss << "Failed to delete AL source " << source
        << " for sound '" << get_id() << "': error " << std::hex << error;
    Debug::error(oss.str());
  }

  source = AL_NONE;
}

/**
 * \brief Returns whether the sound is currently paused.
 * \return \c true if the sound is paused.
 */
bool Sound::is_paused() const {

  if (!is_initialized()) {
    return false;
  }

  ALint status;
  alGetSourcei(source, AL_SOURCE_STATE, &status);
  return status == AL_PAUSED;
}

/**
 * \brief Pauses or resumes this sound.
 * \param paused \c true to pause the source, \c false to resume it.
 */
void Sound::set_paused(bool paused) {

  if (!is_initialized() || source == AL_NONE) {
    return;
  }

  ALint status;
  alGetSourcei(source, AL_SOURCE_STATE, &status);

  if (paused) {
    if (status == AL_PLAYING) {
      alSourcePause(source);
    }
  }
  else {
    if (status == AL_PAUSED) {
      alSourcePlay(source);
    }
  }
}

/**
 * \brief Returns whether the sound is currently paused by a script.
 * \return \c true if the sound is paused by a script.
 */
bool Sound::is_paused_by_script() const {
  return paused_by_script;
}

/**
 * \brief Pauses or resumes this sound from a script perspective.
 *
 * This is independent of the automatic pause/resume that can happen when
 * the window loses focus.
 *
 * \param paused \c true to pause the sound, \c false to resume it.
 */
void Sound::set_paused_by_script(bool paused_by_script) {

  this->paused_by_script = paused_by_script;
  update_paused();
}

/**
 * \brief Pauses all currently playing sounds.
 */
void Sound::pause_all() {

  paused_by_system = true;
  for (const SoundPtr& sound: current_sounds) {
    sound->update_paused();
  }
}

/**
 * \brief Resumes playing all sounds previously paused.
 */
void Sound::resume_all() {

  paused_by_system = false;
  for (const SoundPtr& sound: current_sounds) {
    sound->update_paused();
  }
}

/**
 * \brief Pauses or resumes the sound depending on the current pause state.
 */
void Sound::update_paused() {

  set_paused(paused_by_script || paused_by_system);
}

/**
 * \brief Prints an error message if there is a current OpenAL error.
 * \param function_name Current function name for debugging purposes.
 * \return \c false if there is an error.
 */
bool Sound::check_openal_clean_state(const std::string& function_name) {

  ALenum error = alGetError();
  if (error != AL_NONE) {
    std::ostringstream oss;
    oss << "Previous audio error not cleaned in " << function_name << ": " << std::hex << error;
    Debug::error(oss.str());
    return false;
  }

  return true;
}


/**
 * \brief Returns the name identifying this type in Lua.
 * \return The name identifying this type in Lua.
 */
const std::string& Sound::get_lua_type_name() const {
  return LuaContext::sound_module_name;
}

}

