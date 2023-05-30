#pragma once

namespace ui {
    namespace telnet {
        namespace commands {
            const int WILL = 251;
            const int DO   = 253;
            const int IAC  = 255;
        }
        
        namespace options {
            const int ECHO     = 1;
            const int NAOFFD   = 13;
            const int LINEMODE = 34;
        }
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

#define HIGHLIGHT(msg)  msg
//TODO:
// #define HIGHLIGHT(msg)               \
//     ui::display::YELOW_BACKGROUND << \
//     ui::display::CYAN <<             \
//     msg <<                           \
//     ui::display::NO_COLOR
