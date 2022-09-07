#ifndef GDEMU_H
#define GDEMU_H

#include <Godot.hpp>
#include <Sprite.hpp>
#include <PoolArrays.hpp>
#include <ResourceLoader.hpp>
#include <File.hpp>

//#include "common.h"
//#include "config.h"
//#include "vm/vm.h"
//#include "x1.h"

#include "emulator/common.h"
#include "emulator/config.h"
#include "emulator/vm/vm.h"

class EMU;

namespace godot {

class GDEmu : public Sprite {
    GODOT_CLASS(GDEmu, Sprite)

private:
    float time_passed;
    unsigned char *data;
    PoolByteArray *pScrArray;
    // VM_TEMPLATE *x1;
    // emulator core
    EMU* emu;

    int frame_counter;
    int sound_play_point;
    String drivea_path;
    String driveb_path;
public:
    static void _register_methods();

    GDEmu();
    ~GDEmu();

    void _init(); // our initializer called by Godot

    void _process(float delta);

    PoolByteArray get_data();
    PoolVector2Array get_sound_buffer();

    void setup();

    int get_sound_samples();
    void set_play_point(int offset);

    void set_drivea_path(String path);
    String get_drivea_path();

    void set_driveb_path(String path);
    String get_driveb_path();

	void key_down(int scancode, bool repeat);
	void key_up(int scancode, bool repeat);

    void update_joystate(int num, int joystate);

    void open_floppy_disk(int drv, String file_path, int bank);
    void open_cart(int drv, String str);
	void open_hard_disk(int drv, String file_path);

    int is_floppy_disk_accessed();

    void set_app_path(String path);
};

}

#endif