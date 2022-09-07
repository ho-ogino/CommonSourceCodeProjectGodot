/*
	SONY SMC-70 Emulator 'eSMC-70'
	SONY SMC-777 Emulator 'eSMC-777'

	Author : Takeda.Toshiya
	Date   : 2015.08.13-

	[ memory and i/o bus ]
*/

#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define SIG_MEMORY_DATAREC_IN	0
#define SIG_MEMORY_CRTC_DISP	1
#define SIG_MEMORY_CRTC_VSYNC	2
#define SIG_MEMORY_FDC_DRQ	3
#define SIG_MEMORY_FDC_IRQ	4
#if defined(_SMC70)
#define SIG_MEMORY_RTC_DATA	5
#define SIG_MEMORY_RTC_BUSY	6
#endif

typedef struct key_table_s {
	int vk;
	uint8_t code;
	uint8_t code_shift;
	uint8_t code_ctrl;
	uint8_t code_kana;
	uint8_t code_kana_shift;
	int flag;
} key_table_t;

static const key_table_t key_table_base[] = {
	{0x70, 0x01, 0x15, 0x1a, 0x01, 0x15, -1},	// F1
	{0x71, 0x02, 0x18, 0x10, 0x02, 0x18, -1},	// F2
	{0x72, 0x04, 0x12, 0x13, 0x04, 0x12, -1},	// F3
	{0x73, 0x06, 0x05, 0x07, 0x06, 0x05, -1},	// F4
	{0x74, 0x0b, 0x03, 0x0c, 0x0b, 0x03, -1},	// F5
	{0x7a, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, -1},	// F11 -> H
	{0x23, 0x0e, 0x0e, 0x0e, 0x0e, 0x0e, -1},	// END -> CLR
	{0x2e, 0x11, 0x11, 0x11, 0x11, 0x11, -1},	// DEL
	{0x2d, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, -1},	// INS
	{0x24, 0x14, 0x14, 0x14, 0x14, 0x14, -1},	// HOME
	{0x25, 0x16, 0x16, 0x16, 0x16, 0x16, -1},	// LEFT
	{0x26, 0x17, 0x17, 0x17, 0x17, 0x17, -1},	// UP
	{0x28, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, -1},	// DOWN
	{0x27, 0x19, 0x19, 0x19, 0x19, 0x19, -1},	// RIGHT
	{0x1b, 0x1b, 0x1b, 0x1b, 0x1b, 0x1b, -1},	// ESC
	{0x31, 0x31, 0x21, 0x00, 0xb1, 0xa7, -1},	// '1'
	{0x32, 0x32, 0x40, 0x00, 0xb2, 0xa8, -1},	// '2'
	{0x33, 0x33, 0x23, 0x00, 0xb3, 0xa9, -1},	// '3'
	{0x34, 0x34, 0x24, 0x00, 0xb4, 0xaa, -1},	// '4'
	{0x35, 0x35, 0x25, 0x00, 0xb5, 0xab, -1},	// '5'
	{0x36, 0x36, 0x5e, 0x00, 0xc5, 0xc5, -1},	// '6'
	{0x37, 0x37, 0x26, 0x00, 0xc6, 0xc6, -1},	// '7'
	{0x38, 0x38, 0x2a, 0x00, 0xc7, 0xc7, -1},	// '8'
	{0x39, 0x39, 0x28, 0x00, 0xc8, 0xc8, -1},	// '9'
	{0x30, 0x30, 0x29, 0x00, 0xc9, 0xc9, -1},	// '0'
	{0x61, 0x31, 0x21, 0x00, 0xb1, 0xa7,  0},	// NUMPAD '1'
	{0x62, 0x32, 0x40, 0x00, 0xb2, 0xa8,  0},	// NUMPAD '2'
	{0x63, 0x33, 0x23, 0x00, 0xb3, 0xa9,  0},	// NUMPAD '3'
	{0x64, 0x34, 0x24, 0x00, 0xb4, 0xaa,  0},	// NUMPAD '4'
	{0x65, 0x35, 0x25, 0x00, 0xb5, 0xab,  0},	// NUMPAD '5'
	{0x66, 0x36, 0x5e, 0x00, 0xc5, 0xc5,  0},	// NUMPAD '6'
	{0x67, 0x37, 0x26, 0x00, 0xc6, 0xc6,  0},	// NUMPAD '7'
	{0x68, 0x38, 0x2a, 0x00, 0xc7, 0xc7,  0},	// NUMPAD '8'
	{0x69, 0x39, 0x28, 0x00, 0xc8, 0xc8,  0},	// NUMPAD '9'
	{0x60, 0x30, 0x29, 0x00, 0xc9, 0xc9,  0},	// NUMPAD '0'
	{0x6a, 0x38, 0x2a, 0x00, 0xc7, 0xc7,  1},	// NUMPAD '*' -> SHIFT + '8'
	{0x6b, 0x3d, 0x2b, 0x00, 0xd8, 0xd8,  1},	// NUMPAD '+' -> SHIFT + '^'
	{0x6c, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, -1},	// NUMPAD ENTER
	{0x6d, 0x2d, 0x5f, 0x00, 0xd7, 0xd7,  0},	// NUMPAD '-'
	{0x6e, 0x2e, 0x3e, 0x1e, 0xdc, 0xa4,  0},	// NUMPAD '.'
	{0x6f, 0x2f, 0x3f, 0x1f, 0xa6, 0xa1,  0},	// NUMPAD '/'
	{0xbd, 0x2d, 0x5f, 0x00, 0xd7, 0xd7, -1},	// '-'
	{0xde, 0x3d, 0x2b, 0x00, 0xd8, 0xd8, -1},	// '^' -> '='
	{0xdc, 0x7f, 0x7f, 0x7f, 0xd9, 0xd9, -1},	// '\' -> RUB OUT
	{0x08, 0x08, 0x08, 0x08, 0x08, 0x08, -1},	// BS
	{0x51, 0x71, 0x51, 0x11, 0xb6, 0xb6, -1},	// 'Q'
	{0x57, 0x77, 0x57, 0x17, 0xb7, 0xb7, -1},	// 'W'
	{0x45, 0x65, 0x45, 0x05, 0xb8, 0xb8, -1},	// 'E'
	{0x52, 0x72, 0x52, 0x12, 0xb9, 0xb9, -1},	// 'R'
	{0x54, 0x74, 0x54, 0x14, 0xba, 0xba, -1},	// 'T'
	{0x59, 0x79, 0x59, 0x19, 0xca, 0xca, -1},	// 'Y'
	{0x55, 0x75, 0x55, 0x15, 0xcb, 0xcb, -1},	// 'U'
	{0x49, 0x69, 0x49, 0x09, 0xcc, 0xcc, -1},	// 'I'
	{0x4f, 0x6f, 0x4f, 0x0f, 0xcd, 0xcd, -1},	// 'O'
	{0x50, 0x70, 0x50, 0x10, 0xce, 0xce, -1},	// 'P'
	{0xc0, 0x5b, 0x7b, 0x1b, 0xda, 0xda, -1},	// '['
	{0xdb, 0x5d, 0x7d, 0x1d, 0xdb, 0xb0, -1},	// ']'
	{0x7b, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, -1},	// F12 -> LF
	{0x41, 0x61, 0x41, 0x01, 0xbb, 0xbb, -1},	// 'A'
	{0x53, 0x73, 0x53, 0x13, 0xbc, 0xbc, -1},	// 'S'
	{0x44, 0x64, 0x44, 0x04, 0xbd, 0xbd, -1},	// 'D'
	{0x46, 0x66, 0x46, 0x06, 0xbe, 0xbe, -1},	// 'F'
	{0x47, 0x67, 0x47, 0x07, 0xbf, 0xbf, -1},	// 'G'
	{0x48, 0x68, 0x48, 0x08, 0xcf, 0xcf, -1},	// 'H'
	{0x4a, 0x6a, 0x4a, 0x0a, 0xd0, 0xd0, -1},	// 'J'
	{0x4b, 0x6b, 0x4b, 0x0b, 0xd1, 0xd1, -1},	// 'K'
	{0x4c, 0x6c, 0x4c, 0x0c, 0xd2, 0xd2, -1},	// 'L'
	{0xbb, 0x3b, 0x3a, 0x00, 0xd3, 0xd3, -1},	// ';'
	{0xba, 0x27, 0x22, 0x00, 0xde, 0xa2, -1},	// ','
	{0xdd, 0x60, 0x7e, 0x00, 0xdf, 0xa3, -1},	// '`'
	{0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, -1},	// ENTER
	{0x5a, 0x7a, 0x5a, 0x1a, 0xc0, 0xc0, -1},	// 'Z'
	{0x58, 0x78, 0x58, 0x18, 0xc1, 0xc1, -1},	// 'X'
	{0x43, 0x63, 0x43, 0x03, 0xc2, 0xaf, -1},	// 'C'
	{0x56, 0x76, 0x56, 0x16, 0xc3, 0xc3, -1},	// 'V'
	{0x42, 0x62, 0x42, 0x02, 0xc4, 0xc4, -1},	// 'B'
	{0x4e, 0x6e, 0x4e, 0x0e, 0xd4, 0xac, -1},	// 'N'
	{0x4d, 0x6d, 0x4d, 0x0d, 0xd5, 0xad, -1},	// 'M'
	{0xbc, 0x2c, 0x3c, 0x00, 0xd6, 0xae, -1},	// ','
	{0xbe, 0x2e, 0x3e, 0x1e, 0xdc, 0xa4, -1},	// '.'
	{0xbf, 0x2f, 0x3f, 0x1f, 0xa6, 0xa1, -1},	// '/'
	{0xe2, 0x5c, 0x7c, 0x1c, 0xdd, 0xa5, -1},	// '\'
	{0x09, 0x09, 0x09, 0x09, 0x09, 0x09, -1},	// TAB
	{0x20, 0x20, 0x20, 0x20, 0x20, 0x20, -1},	// SPACE
};

class MEMORY : public DEVICE
{
private:
	// contexts
	DEVICE *d_cpu, *d_crtc, *d_drec, *d_fdc, *d_pcm;
#if defined(_SMC70)
	DEVICE *d_rtc;
#elif defined(_SMC777)
	DEVICE *d_psg;
#endif
	uint8_t* crtc_regs;
	const uint8_t* key_stat;
	const uint32_t* joy_stat;
	
	// memory
	uint8_t ram[0x10000];
#if defined(_SMC70)
	uint8_t rom[0x8000];
#else
	uint8_t rom[0x4000];
#endif
	uint8_t cram[0x800];
	uint8_t aram[0x800];
	uint8_t pcg[0x800];
	uint8_t gram[0x8000];
	
	uint8_t wdmy[0x4000];
	uint8_t rdmy[0x4000];
	uint8_t* wbank[4];
	uint8_t* rbank[4];
	
	bool rom_selected;
	int rom_switch_wait;
	int ram_switch_wait;
	
	// keyboard
	key_table_t key_table[array_length(key_table_base)];
	uint8_t key_code;
	uint8_t key_status;
	uint8_t key_cmd;
	int key_repeat_start;
	int key_repeat_interval;
	int key_repeat_event;
	int funckey_code;
	int funckey_index;
	bool caps, kana;
	void initialize_key();
	void update_key();
	
	// display
	uint8_t gcw;
	bool vsup;
	bool vsync, disp, blink;
	int cblink;
#if defined(_SMC777)
	bool use_palette_text;
	bool use_palette_graph;
	struct {
		int r, g, b;
	} pal[16];
#endif
	uint8_t text[200][640];
	uint8_t graph[400][640];
	scrntype_t palette_pc[16 + 16];	// color generator + palette board
#if defined(_SMC70)
	scrntype_t palette_bw_pc[2];
#else
	scrntype_t palette_line_text_pc[200][16];
	scrntype_t palette_line_graph_pc[200][16];
#endif
	
	void draw_text_80x25(int v);
	void draw_text_40x25(int v);
	void draw_graph_640x400(int v);
	void draw_graph_640x200(int v);
	void draw_graph_320x200(int v);
	void draw_graph_160x100(int v);
	
	// kanji rom
	uint8_t kanji[0x23400];
#if defined(_SMC70)
	uint8_t basic[0x8000];
#endif
	int kanji_hi, kanji_lo;
	
	// misc
	bool ief_key, ief_vsync;
	bool vsync_irq;
	bool fdc_irq, fdc_drq;
	bool drec_in;
#if defined(_SMC70)
	uint8_t rtc_data;
	bool rtc_busy;
#endif
	
public:
	MEMORY(VM_TEMPLATE* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu)
	{
		set_device_name(_T("Memory Bus"));
	}
	~MEMORY() {}
	
	// common functions
	void initialize();
	void reset();
	void write_data8(uint32_t addr, uint32_t data);
	uint32_t read_data8(uint32_t addr);
	uint32_t fetch_op(uint32_t addr, int *wait);
	void write_io8(uint32_t addr, uint32_t data);
	uint32_t read_io8(uint32_t addr);
#ifdef _IO_DEBUG_LOG
	uint32_t read_io8_debug(uint32_t addr);
#endif
	void write_signal(int id, uint32_t data, uint32_t mask);
	void event_callback(int event_id, int err);
	void event_frame();
	void event_vline(int v, int clock);
	bool process_state(FILEIO* state_fio, bool loading);
	
	// unique functions
	void set_context_cpu(DEVICE* device)
	{
		d_cpu = device;
	}
	void set_context_crtc(DEVICE* device, uint8_t* ptr)
	{
		d_crtc = device;
		crtc_regs = ptr;
	}
	void set_context_drec(DEVICE* device)
	{
		d_drec = device;
	}
	void set_context_fdc(DEVICE* device)
	{
		d_fdc = device;
	}
	void set_context_pcm(DEVICE* device)
	{
		d_pcm = device;
	}
#if defined(_SMC70)
	void set_context_rtc(DEVICE* device)
	{
		d_rtc = device;
	}
#elif defined(_SMC777)
	void set_context_psg(DEVICE* device)
	{
		d_psg = device;
	}
#endif
	bool get_caps_locked()
	{
		return caps;
	}
	bool get_kana_locked()
	{
		return kana;
	}
	void key_down(int code);
	void draw_screen();
	bool warm_start;
};

#endif
