#pragma once

/**
 * @namespace ui
 * @brief Contains user interface-related constants and utilities.
 */
namespace ui {

    /**
     * @namespace telnet
     * @brief Defines constants related to the Telnet protocol.
     */
    namespace telnet {
        /**
         * @namespace commands
         * @brief Telnet command byte values.
         */
        namespace commands {
            inline const unsigned char WILL = 251; ///< Telnet WILL command.
            inline const unsigned char DO   = 253; ///< Telnet DO command.
            inline const unsigned char IAC  = 255; ///< Telnet Interpret as Command (IAC).
        }

        /**
         * @namespace options
         * @brief Telnet option byte values.
         */
        namespace options {
            inline const unsigned char ECHO     = 1;  ///< Telnet ECHO option.
            inline const unsigned char LINEMODE = 34; ///< Telnet LINEMODE option.
        }

        /// Newline sequence for Telnet communication.
        inline const char* newline = "\r\n";
    }

    /**
     * @namespace commands
     * @brief Defines keyboard-related command constants.
     */
    namespace commands {

        /**
         * @enum Key
         * @brief Represents key command values.
         */
        enum Key {
            ESCAPE     = '\33', ///< Escape key.
            DELIM      = '[',   ///< Delimiter for escape sequences.
            ARROW_UP   = 'A',   ///< Up arrow key.
            ARROW_DOWN = 'B'    ///< Down arrow key.
        };

        inline const int MAX_CMD_LEN = 3;  ///< Maximum length of a command sequence.
        inline const char* UP   = "\33[A"; ///< ANSI sequence for up arrow.
        inline const char* DOWN = "\33[B"; ///< ANSI sequence for down arrow.
    }

    /**
     * @namespace display
     * @brief Contains ANSI escape sequences for UI formatting.
     */
    namespace display {
        inline const char* CLEAR       = "\33[2J\33[H";      ///< Clears the screen.

        inline const char* NO_COLOR    = "\e[0m";            ///< Resets text formatting.
        inline const char* GREEN       = "\e[1;92m";         ///< Green-colored text.

        inline const char* START_BLINK = "\e[5m";            ///< Starts blinking text.
        inline const char* STOP_BLINK  = "\e[25m";           ///< Stops blinking text.

        inline const char* START_BOLD  = "\e[1m";            ///< Starts bold text.
        inline const char* STOP_BOLD   = "\e[22m";           ///< Stops bold text.
    }
}

/**
 * @brief Macro to apply blinking effect to a message.
 * @param msg The message to format.
 */
#define BLINK(msg)              \
    ui::display::START_BLINK << \
    msg <<                      \
    ui::display::STOP_BLINK

/**
 * @brief Macro to apply bold effect to a message.
 * @param msg The message to format.
 */
#define BOLD(msg)               \
    ui::display::START_BOLD <<  \
    msg <<                      \
    ui::display::STOP_BOLD

/**
 * @brief Macro to apply green color to a message.
 * @param msg The message to format.
 */
#define GREEN(msg)              \
    ui::display::GREEN <<       \
    msg <<                      \
    ui::display::NO_COLOR
