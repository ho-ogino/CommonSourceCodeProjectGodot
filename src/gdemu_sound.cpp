#include "gdemu.h"

#include "./emulator/emu.h"

using namespace godot;

void GDEmu::set_play_point(int offset)
{
    // ↓いらんかな
    sound_play_point = offset;

    emu->get_osd()->set_sound_play_point(offset);
}

int GDEmu::get_sound_samples()
{
    return emu->get_sound_samples();
}

PoolVector2Array GDEmu::get_sound_buffer()
{
    return *emu->get_osd()->get_sound_buffer();
}
