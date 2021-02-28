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
#include "solarus/core/Logger.h"
#include "solarus/core/System.h"
#include <fstream>
#include <iostream>
#include <SDL_log.h>

namespace Solarus {

namespace Logger {

namespace {

#ifdef SOLARUS_FILE_LOGGING
  const std::string error_log_file_name = "error.txt";
  std::ofstream error_log_file;

  /**
   * \brief Returns the error log file.
   *
   * Opens it the first time this function is called.
   */
  std::ofstream& get_error_log_file() {

    if (!error_log_file.is_open()) {
      error_log_file.open(error_log_file_name.c_str());
    }
    return error_log_file;
  }
#endif
}

/**
 * \brief Logs a message on the given output stream.
 *
 * The message is prepended by "[Solarus] [t] " where t is the current
 * simulated time.
 *
 * \param message The message to log.
 * \param out The output stream.
 */
SOLARUS_API void print(const std::string& message, std::ostream& out) {
#ifdef ANDROID
  SDL_Log("%s",message.c_str());
#else
  uint32_t simulated_time = System::now_ms();
  out << "[Solarus] [" << simulated_time << "] " << message << std::endl;
#endif
}

/**
 * \brief Logs a debug message on stdout.
 * \param message The message to log.
 */
SOLARUS_API void debug(const std::string& message) {

  print("Debug: " + message);
}

/**
 * \brief Logs an information message on stdout.
 * \param message The message to log.
 */
SOLARUS_API void info(const std::string& message) {

  print("Info: " + message);
}

/**
 * \brief Logs a warning message on stdout and error.txt (if configured).
 * \param message The message to log.
 */
SOLARUS_API void warning(const std::string& message) {

  std::string prefixed_message = "Warning: " + message;
  print(prefixed_message);
#ifdef SOLARUS_FILE_LOGGING
  print(prefixed_message, get_error_log_file());
#endif
}

/**
 * \brief Logs an error message on stdout and error.txt (if configured).
 * \param message The message to log.
 */
SOLARUS_API void error(const std::string& message) {

  std::string prefixed_message = "Error: " + message;
  print(prefixed_message);
#ifdef SOLARUS_FILE_LOGGING
  print(prefixed_message, get_error_log_file());
#endif
}

/**
 * \brief Logs a fatal error message on stdout and error.txt (if configured).
 * \param message The message to log.
 */
SOLARUS_API void fatal(const std::string& message) {

  std::string prefixed_message = "Fatal: " + message;
  print(prefixed_message);
#ifdef SOLARUS_FILE_LOGGING
  print(prefixed_message, get_error_log_file());
#endif
}

}  // namespace Logger

}  // namespace Solarus
