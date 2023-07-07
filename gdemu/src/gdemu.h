#ifndef GDEMU_H
#define GDEMU_H

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/binder_common.hpp>

//#include <Godot.hpp>
#include <godot_cpp/classes/sprite2d.hpp>
// #include <PoolArrays.hpp>
#include <godot_cpp/variant/typed_array.hpp>
//#include <ResourceLoader.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
//#include <File.hpp>
#include <godot_cpp/classes/file_access.hpp>

//#include "common.h"
//#include "config.h"
//#include "vm/vm.h"
//#include "x1.h"

#include "emulator/common.h"
#include "emulator/config.h"
#include "emulator/vm/vm.h"
#include "emulator/vm/disk.h"

class EMU;

namespace godot {

class GDEmu : public Sprite2D {
    GDCLASS(GDEmu, Sprite2D);

protected:
        static void _bind_methods();

private:
    double time_passed;
    unsigned char *data;
    PackedByteArray *pScrArray;
    // VM_TEMPLATE *x1;
    // emulator core
    EMU* emu;

    int frame_counter;
    int sound_play_point;
    String drivea_path;
    String driveb_path;
public:
//    static void _bind_methods();

    GDEmu();
    ~GDEmu();

    void init();

    virtual void _process(double delta) override;

    PackedByteArray get_data();
    PackedVector2Array get_sound_buffer();

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

    void set_disk_encrypt(bool flag) { DISK::is_encrypt = flag; }
};

}

#endif
