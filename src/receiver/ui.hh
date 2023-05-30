#pragma once

namespace ui {
    namespace telnet {
        namespace commands {
            const unsigned char WILL = 251;
            const unsigned char DO   = 253;
            const unsigned char IAC  = 255;
        }
        
        namespace options {
            const unsigned char ECHO     = 1;
            const unsigned char NAOFFD   = 13;
            const unsigned char LINEMODE = 34;
        }

        const char* newline = "\r\n";
    }

    namespace keys {
        const char* ARROW_UP   = "\e[A";
        const char* ARROW_DOWN = "\e[B";
    }

    namespace display {
        const char* CLEAR            = "\ec\e[3J";
        const char* NO_COLOR         = "\e[0m";
        const char* YELOW_BACKGROUND = "\e[48;5;226m";
        const char* CYAN             = "\e[1;96m";
    }
}

#define HIGHLIGHT(msg) msg
//TODO:
// #define HIGHLIGHT(msg)               \
//     ui::display::YELOW_BACKGROUND << \
//     ui::display::CYAN <<             \
//     msg <<                           \
//     ui::display::NO_COLOR
