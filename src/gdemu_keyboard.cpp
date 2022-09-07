#include "gdemu.h"

#include "./emulator/emu.h"
#include "./emulator/fifo.h"
#include "./emulator/fileio.h"
#include "./emulator/vm/disk.h"

#include <map>

#include <OS.hpp>
#include <Directory.hpp>

using namespace godot;


// romaji to kana table
static std::map<int, int> godot_to_char_map =
{
    {16777220, 0x08},   // KEY_BACKSPACE
    {16777218, 0x09},   // KEY_TAB
    {16777221, 0x0d},   // KEY_ENTER
    {16777217, 0x1b},   // KEY_ESCAPE
    {32, 0x20},         // KEY_SPACE

    // NUMBER
    {48, 48},   // 0
    {49, 49},   // 1
    {50, 50},   // 2
    {51, 51},   // 3
    {52, 52},   // 4
    {53, 53},   // 5
    {54, 54},   // 6
    {55, 55},   // 7
    {56, 56},   // 8
    {57, 57},   // 9

    // ASCII
    {65, 65},   // A
    {65, 65},   // B
    {66, 66},
    {67, 67},
    {68, 68},
    {69, 69},
    {70, 70},
    {71, 71},
    {72, 72},
    {73, 73},
    {74, 74},
    {75, 75},
    {76, 76},
    {77, 77},
    {78, 78},
    {79, 79},
    {80, 80},
    {81, 81},
    {82, 82},
    {83, 83},
    {84, 84},
    {85, 85},
    {86, 86},
    {87, 87},
    {88, 88},
    {89, 89},
    {90, 90},   // Z
};

// scancode to VK map
static std::map<int, int> godot_to_vkey_map =
{
    {16777220, VK_BACK},
    {16777218, VK_TAB},
    {16777221, VK_RETURN},
    {16777225, VK_PAUSE},
    {16777217, VK_ESCAPE},
    {32,       VK_SPACE},

    // {16777240, VK_KANA}, // ALTをカナキーにする
    {16777240, VK_F13},     // ALTをToggle romaji to kanaのトグルにする(F13に割り当てている)

    {16777241, VK_CAPITAL}, //CapsLock

    {16777244, VK_F1},
    {16777245, VK_F2},
    {16777246, VK_F3},
    {16777247, VK_F4},
    {16777248, VK_F5},

    {39, VK_OEM_7}, // APOSTROPHE
    {59, VK_OEM_1}, // SEMICOLON
    {61, VK_OEM_PLUS}, // =+
    {91, VK_OEM_4}, // [
    {92, VK_OEM_5}, // BACKSLASH
    {93, VK_OEM_6}, // ]
    {96, VK_OEM_3}, // `

    {33554431 ,VK_OEM_102}, // \_ TODO UNKNOWN! to OEM 102

    {16777232, 0x40000000 | VK_UP},
    {16777234, 0x40000000 | VK_DOWN},
    {16777231, 0x40000000 | VK_LEFT},
    {16777233, 0x40000000 | VK_RIGHT},

    {16777237, VK_SHIFT},
    {16777238, VK_CONTROL},

    {44, VK_OEM_COMMA},
    {45 , VK_OEM_MINUS},
    {46, VK_OEM_PERIOD},
    {47, VK_OEM_2},     // /

    // TENKEY NUMBER
    {16777350, VK_NUMPAD0},
    {16777351, VK_NUMPAD1},
    {16777352, VK_NUMPAD2},
    {16777353, VK_NUMPAD3},
    {16777354, VK_NUMPAD4},
    {16777355, VK_NUMPAD5},
    {16777356, VK_NUMPAD6},
    {16777357, VK_NUMPAD7},
    {16777358, VK_NUMPAD8},
    {16777359, VK_NUMPAD9},

    // NUMBER
    {48, 48},   // 0
    {49, 49},   // 1
    {50, 50},   // 2
    {51, 51},   // 3
    {52, 52},   // 4
    {53, 53},   // 5
    {54, 54},   // 6
    {55, 55},   // 7
    {56, 56},   // 8
    {57, 57},   // 9

    // ASCII
    {65, 65},   // A
    {65, 65},   // B
    {66, 66},
    {67, 67},
    {68, 68},
    {69, 69},
    {70, 70},
    {71, 71},
    {72, 72},
    {73, 73},
    {74, 74},
    {75, 75},
    {76, 76},
    {77, 77},
    {78, 78},
    {79, 79},
    {80, 80},
    {81, 81},
    {82, 82},
    {83, 83},
    {84, 84},
    {85, 85},
    {86, 86},
    {87, 87},
    {88, 88},
    {89, 89},
    {90, 90},   // Z
};

static int scancode_to_vkey(int scancode, bool *extended)
{
    if(godot_to_vkey_map.count(scancode) > 0)
    {
        *extended = (godot_to_vkey_map[scancode] & 0x40000000) ? true : false;
        return godot_to_vkey_map[scancode] & 0xffff;
    }
    return 0;
}

void GDEmu::key_down(int scancode, bool repeat)
{
    bool extended = false;
    int code = scancode_to_vkey(scancode, &extended);


    if(code == VK_F13)
    {
		if(!config.romaji_to_kana) {
			emu->set_auto_key_char(1); // start
			config.romaji_to_kana = true;
		} else {
			emu->set_auto_key_char(0); // end
			config.romaji_to_kana = false;
		}
        return;
    }

    if(config.romaji_to_kana)
    {
        if(godot_to_char_map.count(scancode) > 0)
        {
            emu->key_char(godot_to_char_map[scancode]);
        }
    }

    emu->key_down(code, extended, repeat);
}

void GDEmu::key_up(int scancode, bool repeat)
{
    bool extended = false;
    int code = scancode_to_vkey(scancode, &extended);
    if(code == VK_F13)
    {
        return;
    }

    emu->key_up(code, extended);
}
