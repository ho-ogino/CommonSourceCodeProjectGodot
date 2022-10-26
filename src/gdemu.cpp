#include "gdemu.h"

#include "./emulator/emu.h"
#include "./emulator/fifo.h"
#include "./emulator/fileio.h"
#include "./emulator/vm/disk.h"

#include <map>

#include <OS.hpp>
#include <Directory.hpp>
#include <AESContext.hpp>
#include "emulator/zlib-1.2.11/zlib.h"

using namespace godot;

void GDEmu::_register_methods() {
    register_method("_process", &GDEmu::_process);
    register_method("setup", &GDEmu::setup);
    register_method("get_data", &GDEmu::get_data);
    register_method("get_sound_buffer", &GDEmu::get_sound_buffer);
    register_method("get_sound_samples", &GDEmu::get_sound_samples);
    register_method("set_play_point", &GDEmu::set_play_point);
    register_method("update_joystate", &GDEmu::update_joystate);

    register_method("open_floppy_disk", &GDEmu::open_floppy_disk);
#if USE_CART
    register_method("open_cart", &GDEmu::open_cart);
#endif
#if USE_HARD_DISK
    register_method("open_hard_disk", &GDEmu::open_hard_disk);
#endif

    register_method("is_floppy_disk_accessed", &GDEmu::is_floppy_disk_accessed);

    register_method("key_down", &GDEmu::key_down);
    register_method("key_up", &GDEmu::key_up);

    register_method("set_disk_encrypt", &GDEmu::set_disk_encrypt);

    // プロパティを外に追い出す場合はこんな感じ……(とりあえず無しにしておく)
    // register_property<GDEmu, String>("drivea_path", &GDEmu::set_drivea_path, &GDEmu::get_drivea_path, "");
    // register_property<GDEmu, String>("driveb_path", &GDEmu::set_driveb_path, &GDEmu::get_driveb_path, "");
}


void GDEmu::set_app_path(String path)
{
    char* cstr = path.alloc_c_string();
    set_application_path(cstr);
    api->godot_free(cstr);
}

void GDEmu::update_joystate(int num, int joystate)
{
    emu->get_osd()->set_joystate(num, joystate);
}

static void file_copy(String from, String to)
{
    Ref<File> file = File::_new();
    Error err = file->open(from, File::ModeFlags::READ);
    if(err == Error::OK)
    {
        PoolByteArray data = file->get_buffer(file->get_len());
        file->close();

        file->open(to, File::ModeFlags::WRITE);
        file->store_buffer(data);
        file->close();
    }
}


void GDEmu::setup()
{
    emu->reset();
}

PoolByteArray GDEmu::get_data()
{
    return *pScrArray;
}

GDEmu::GDEmu() {
}

GDEmu::~GDEmu() {
    delete emu;
    emu = NULL;
    delete pScrArray;
}

void GDEmu::_init() {
    frame_counter = 0;

    // initialize any variables here
    time_passed = 0.0;

    // 事前にアプリケーション用パスを保存(書き換え可能なユーザーディレクトリにしておく)
    String app_path = OS::get_singleton()->get_user_data_dir() + "/";
    char* pAppPath = app_path.alloc_c_string();
    printf("appPath: %s\n", pAppPath);
    set_application_path(pAppPath);
    api->godot_free(pAppPath);

    // 各種リソースを読み書き可能な領域(user dir)にコピーする
#ifdef _X1
    // X1 IPL & FONT
    // ※毎回コピーするが気にしない
    file_copy("res://IPLROM.X1", "user://IPLROM.X1");
    file_copy("res://IPLROM.X1", "user://IPLROM.X1T");
    // X1Tがあれば使う。無ければX1をX1Tのかわりに使う(不気味)
    file_copy("res://IPLROM.X1T", "user://IPLROM.X1T");
    file_copy("res://FNT0808.X1", "user://FNT0808.X1");
#endif

#if defined(_MSX1) || defined(_MSX2) || defined(_MSX2P)
    // MSX BIOS
    file_copy("res://MSXJ.ROM", "user://MSXJ.ROM");
#endif

#if defined(_PC8801MA)
    file_copy("res://N88.ROM", "user://N88.ROM");
    file_copy("res://DISK.ROM", "user://DISK.ROM");
    file_copy("res://KANJI1.ROM", "user://KANJI1.ROM");
    file_copy("res://KANJI2.ROM", "user://KANJI2.ROM");

    // ここからは実機のROM？
    file_copy("res://PC88.ROM", "user://PC88.ROM");
    file_copy("res://N88_0.ROM", "user://N88_0.ROM");
    file_copy("res://N88_1.ROM", "user://N88_1.ROM");
    file_copy("res://N88_2.ROM", "user://N88_2.ROM");
    file_copy("res://N88_3.ROM", "user://N88_3.ROM");
    file_copy("res://N80.ROM", "user://N80.ROM");
#endif

    // フロッピーアクセス音
    file_copy("res://FDDSEEK.BIN", "user://FDDSEEK.WAV");

    initialize_config();

	emu = new EMU();
	emu->set_host_window_size(WINDOW_WIDTH, WINDOW_HEIGHT, true);

#ifdef _X1
    // FDD noise -10dB
	emu->set_sound_device_volume(5, -30, -30);
#endif

//    X1turboで2HDから起動したい場合はこちらを有効にする事
//    config.drive_type = 2;        // 0=2D 1=2DD 2=2HD
//    config.monitor_type = 1;      // 0=High 1=Standard
//    emu->get_vm()->update_dipswitch();

    // 真っ黒で画面をクリアしておく
    pScrArray = new PoolByteArray();
    int scrw = SCREEN_WIDTH + 16;
    int scrh = SCREEN_HEIGHT + 16;
    pScrArray->resize(scrw * scrh * 3);
    for(int i = 0; i < scrw * scrh * 3; i+= 3)
    {
        pScrArray->set(i,   0);
        pScrArray->set(i+1, 0);
        pScrArray->set(i+2, 0);
    }
}


void GDEmu::_process(float delta) {
    time_passed += delta;

    // 誤差は……気にしない……(気になる)
    float interval = (float)emu->get_frame_interval() / 1000000.0;
    while(time_passed >= interval)
    {
        int run_frames = emu->run();
        frame_counter += run_frames > 0 ? run_frames - 1 : 0;

        time_passed -= interval;
    }
    emu->draw_screen();

    OSD* osd = emu->get_osd();

    // 遅そう……
    for(int i = 0; i < SCREEN_HEIGHT; i++)
    {
        scrntype_t *scr = osd->get_vm_screen_buffer(i);
        int idx = ((i + 8) * (SCREEN_WIDTH + 16) + 8) * 3;
        for(int j = 0; j < SCREEN_WIDTH; j++)
        {
            scrntype_t col = *scr;

            pScrArray->set(idx++, (col >> 16) & 0xff);
            pScrArray->set(idx++, (col >> 8) & 0xff);
            pScrArray->set(idx++, col & 0xff);

            scr++;
        }
    }
}
