#pragma once

namespace ui {
    namespace telnet {
        namespace commands {
            inline const unsigned char WILL = 251;
            inline const unsigned char DO   = 253;
            inline const unsigned char IAC  = 255;
        }
        
        namespace options {
            inline const unsigned char ECHO     = 1;
            inline const unsigned char LINEMODE = 34;
        }

        inline const char* newline = "\r\n";
    }

    namespace commands {
        enum Key {
            ESCAPE     = '\33',
            DELIM      = '[',
            ARROW_UP   = 'A',
            ARROW_DOWN = 'B'
        };

        inline const int MAX_CMD_LEN = 3; 
        inline const char* UP   = "\33[A";
        inline const char* DOWN = "\33[B";
    }

    namespace display {
        inline const char* CLEAR            = "\33[2J\33[H";

        inline const char* NO_COLOR         = "\e[0m";
        inline const char* GREEN            = "\e[1;92m";

        inline const char* START_BLINK = "\e[5m";
        inline const char* STOP_BLINK  = "\e[25m";

        inline const char* START_BOLD  = "\e[1m";
        inline const char* STOP_BOLD   = "\e[22m";
    }
}

#define BLINK(msg)              \
    ui::display::START_BLINK << \
    msg <<                      \
    ui::display::STOP_BLINK

#define BOLD(msg)               \
    ui::display::START_BOLD <<  \
    msg <<                      \
    ui::display::STOP_BOLD

#define GREEN(msg)              \
    ui::display::GREEN <<       \
    msg <<                      \
    ui::display::NO_COLOR
