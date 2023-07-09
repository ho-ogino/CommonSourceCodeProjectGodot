/*
	Skelton for retropc emulator

	Author : Hiroshi.Ogino
	Date   : 2022.08.27-

	[ godot dependent ]
*/

#ifndef _GODOT_OSD_H_
#define _GODOT_OSD_H_

#include "../vm/vm.h"
//#include "../emu.h"
#include "../common.h"
#include "../config.h"

#if defined(USE_ZLIB) && !defined(USE_VCPKG)				// zlib not installed by vcpkg
	// relative path from *.vcproj/*.vcxproj, not from this directory :-(
	#ifdef _M_AMD64
		// these zlibstat.lib were built with VC++2017 (thanks Marukun)
		#ifdef _DEBUG
			#pragma comment(lib, "src/emulator/zlib-1.2.11/debug/x64/zlibstat.lib")
		#else
			#pragma comment(lib, "src/emulator/zlib-1.2.11/release/x64/zlibstat.lib")
		#endif
	#else
		// these zlibstat.lib were built with VC++2008
		#ifdef _DEBUG
			#pragma comment(lib, "src/emulator/zlib-1.2.11/debug/zlibstat.lib")
		#else
			#pragma comment(lib, "src/emulator/zlib-1.2.11/release/zlibstat.lib")
		#endif
		// for vsnprintf() and snprintf() in zlibstat.lib
		#if defined(_MSC_VER) && (_MSC_VER >= 1900)
			#pragma comment(lib, "legacy_stdio_definitions.lib")
		#endif
	#endif
#endif

// osd common

#define OSD_CONSOLE_BLUE	1 // text color contains blue
#define OSD_CONSOLE_GREEN	2 // text color contains green
#define OSD_CONSOLE_RED		4 // text color contains red
#define OSD_CONSOLE_INTENSITY	8 // text color is intensified

namespace godot {
class PackedByteArray;
class PackedVector2Array;
}

typedef struct bitmap_s
{
	// common
	inline bool initialized()
	{
		return (pBitmapArray != NULL);
	}
	inline scrntype_t* get_buffer(int y)
	{
		return lpBmp + width * (height - y - 1);
	}
	int width, height;
	scrntype_t* lpBmp;
	godot::PackedByteArray* pBitmapArray;
} bitmap_t;

typedef struct font_s
{
	// common
	inline bool initialized()
	{
		return (hFont != NULL);
	}
	_TCHAR family[64];
	int width, height, rotate;
	bool bold, italic;
	unsigned char*hFont;
} font_t;

typedef struct pen_s {
	// common
	inline bool initialized()
	{
		return true; // (hPen != NULL);
	}
	int width;
	uint8_t r, g, b;
} pen_t;

class OSD
{
private:
	int lock_count;
	
	// console
	void initialize_console();
	void release_console();
	
	/// HANDLE hStdIn, hStdOut;
	int console_count;
	
	void open_telnet(const _TCHAR* title);
	void close_telnet();
	void send_telnet(const char* buffer);
	
	bool use_telnet, telnet_closed;
	int svr_socket, cli_socket;
	
	// input
	void initialize_input();
	void release_input();
	
	bool dinput_key_available;
//	bool dinput_joy_available;
	
	uint8_t keycode_conv[256];
	uint8_t key_status[256];	// windows key code mapping
	uint8_t key_dik[256];
	uint8_t key_dik_prev[256];
	bool key_shift_pressed, key_shift_released;
	bool key_caps_locked;
	bool lost_focus;
	
	godot::PackedVector2Array* pSoundBuffer;
#ifdef USE_JOYSTICK
	// bit0-3	up,down,left,right
	// bit4-19	button #1-#16
	// bit20-21	z-axis pos
	// bit22-23	r-axis pos
	// bit24-25	u-axis pos
	// bit26-27	v-axis pos
	// bit28-31	pov pos
	uint32_t joy_status[4];
	int joy_num;
	struct {
		UINT wNumAxes;
		DWORD dwXposLo, dwXposHi;
		DWORD dwYposLo, dwYposHi;
		DWORD dwZposLo, dwZposHi;
		DWORD dwRposLo, dwRposHi;
		DWORD dwUposLo, dwUposHi;
		DWORD dwVposLo, dwVposHi;
		DWORD dwButtonsMask;
	} joy_caps[4];
	bool joy_to_key_status[256];
#endif
	
#ifdef USE_MOUSE
	int32_t mouse_status[3];	// x, y, button (b0 = left, b1 = right)
	bool mouse_enabled;
#endif
	
	// screen
	void initialize_screen();
	void release_screen();
	void initialize_screen_buffer(bitmap_t *buffer, int width, int height, int mode);
	void release_screen_buffer(bitmap_t *buffer);
#ifdef USE_SCREEN_FILTER
	void apply_rgb_filter_to_screen_buffer(bitmap_t *source, bitmap_t *dest);
	void apply_rgb_filter_x3_y3(bitmap_t *source, bitmap_t *dest);
	void apply_rgb_filter_x3_y2(bitmap_t *source, bitmap_t *dest);
	void apply_rgb_filter_x2_y3(bitmap_t *source, bitmap_t *dest);
	void apply_rgb_filter_x2_y2(bitmap_t *source, bitmap_t *dest);
	void apply_rgb_filter_x1_y1(bitmap_t *source, bitmap_t *dest);
#endif
	int add_video_frames();
	
	bitmap_t vm_screen_buffer;
#ifdef USE_SCREEN_FILTER
	bitmap_t filtered_screen_buffer;
	bitmap_t tmp_filtered_screen_buffer;
#endif
//#ifdef USE_SCREEN_ROTATE
	bitmap_t rotated_screen_buffer;
//#endif
	bitmap_t stretched_screen_buffer;
	bitmap_t shrinked_screen_buffer;
	bitmap_t reversed_screen_buffer;
	bitmap_t video_screen_buffer;
	
	bitmap_t* draw_screen_buffer;
	
	int host_window_width, host_window_height;
	bool host_window_mode;
	int vm_screen_width, vm_screen_height;
	int vm_window_width, vm_window_height;
	int vm_window_width_aspect, vm_window_height_aspect;
	int draw_screen_width, draw_screen_height;
	
	_TCHAR video_file_path[_MAX_PATH];
	int rec_video_fps;
	double rec_video_run_frames;
	double rec_video_frames;
	
	// LPBITMAPINFO lpDibRec;
	// PAVIFILE pAVIFile;
	// PAVISTREAM pAVIStream;
	// PAVISTREAM pAVICompressed;
	// AVICOMPRESSOPTIONS AVIOpts;
	// DWORD dwAVIFileSize;
	// LONG lAVIFrames;
	// HANDLE hVideoThread;
	// rec_video_thread_param_t rec_video_thread_param;
	
	bool first_draw_screen;
	bool first_invalidate;
	bool self_invalidate;
	
	// sound
	void initialize_sound(int rate, int samples);
	void release_sound();
	
	int sound_rate, sound_samples;
	bool sound_available, sound_started, sound_muted;
	int sound_play_point;
	
	// LPDIRECTSOUND lpds;
	// LPDIRECTSOUNDBUFFER lpdsPrimaryBuffer, lpdsSecondaryBuffer;
	bool sound_first_half;
	
	_TCHAR sound_file_path[_MAX_PATH];
	FILEIO* rec_sound_fio;
	int rec_sound_bytes;
	int rec_sound_buffer_ptr;
	
	// socket
#ifdef USE_SOCKET
	void initialize_socket();
	void release_socket();
	
	SOCKET soc[SOCKET_MAX];
	bool is_tcp[SOCKET_MAX];
	struct sockaddr_in udpaddr[SOCKET_MAX];
	int socket_delay[SOCKET_MAX];
	char recv_buffer[SOCKET_MAX][SOCKET_BUFFER_MAX];
	int recv_r_ptr[SOCKET_MAX], recv_w_ptr[SOCKET_MAX];
#endif
	
	//midi
#ifdef USE_MIDI
	void initialize_midi();
	void release_midi();
	
	HANDLE hMidiThread;
	midi_thread_params_t midi_thread_params;
#endif
	
public:
	OSD()
	{
		lock_count = 0;
	}
	~OSD() {}
	
	// common
	VM_TEMPLATE* vm;
	
	void initialize(int rate, int samples);
	void release();
	void power_off();
	void suspend();
	void restore();
	void lock_vm();
	void unlock_vm();
	bool is_vm_locked()
	{
		return (lock_count != 0);
	}
	void force_unlock_vm();
	void sleep(uint32_t ms);
	
	// common debugger
#ifdef USE_DEBUGGER
	void start_waiting_in_debugger();
	void finish_waiting_in_debugger();
	void process_waiting_in_debugger();
#endif
	
	// common console
	void open_console(int width, int height, const _TCHAR* title);
	void close_console();
	unsigned int get_console_code_page();
	void set_console_text_attribute(unsigned short attr);
	void write_console(const _TCHAR* buffer, unsigned int length);
	int read_console_input(_TCHAR* buffer, unsigned int length);
	bool is_console_key_pressed(int vk);
	bool is_console_closed();
	void close_debugger_console();
	
	// common input
	void update_input();
	void key_down(int code, bool extended, bool repeat);
	void key_up(int code, bool extended);
	void key_down_native(int code, bool repeat);
	void key_up_native(int code);
	void key_lost_focus()
	{
		lost_focus = true;
	}
#ifdef USE_MOUSE
	void enable_mouse();
	void disable_mouse();
	void toggle_mouse();
	bool is_mouse_enabled()
	{
		return mouse_enabled;
	}
#endif
	uint8_t* get_key_buffer()
	{
		return key_status;
	}
#ifdef USE_JOYSTICK
	uint32_t* get_joy_buffer()
	{
		return joy_status;
	}
#endif
#ifdef USE_MOUSE
	int32_t* get_mouse_buffer()
	{
		return mouse_status;
	}
#endif
#ifdef USE_AUTO_KEY
	bool now_auto_key;
#endif
	
	// common screen
	double get_window_mode_power(int mode);
	int get_window_mode_width(int mode);
	int get_window_mode_height(int mode);
	void set_host_window_size(int window_width, int window_height, bool window_mode);
	void set_vm_screen_size(int screen_width, int screen_height, int window_width, int window_height, int window_width_aspect, int window_height_aspect);
	void set_vm_screen_lines(int lines);
	int get_vm_window_width()
	{
		return vm_window_width;
	}
	int get_vm_window_height()
	{
		return vm_window_height;
	}
	int get_vm_window_width_aspect()
	{
		return vm_window_width_aspect;
	}
	int get_vm_window_height_aspect()
	{
		return vm_window_height_aspect;
	}
	scrntype_t* get_vm_screen_buffer(int y);
	int draw_screen();
#ifdef ONE_BOARD_MICRO_COMPUTER
	void reload_bitmap()
	{
		first_invalidate = true;
	}
#endif
	void capture_screen();
	bool start_record_video(int fps);
	void stop_record_video();
	void restart_record_video();
	void add_extra_frames(int extra_frames);
	bool now_record_video;
#ifdef USE_SCREEN_FILTER
	bool screen_skip_line;
#endif
	
	// common sound
	void update_sound(int* extra_frames);
	void mute_sound();
	void stop_sound();
	void start_record_sound();
	void stop_record_sound();
	void restart_record_sound();
	bool now_record_sound;
#ifdef _UNITY	// MARU
	int16_t	* get_sound_buffer();
#endif // !_UNITY
	
	// common video device
#if defined(USE_MOVIE_PLAYER) || defined(USE_VIDEO_CAPTURE)
	void get_video_buffer();
	void mute_video_dev(bool l, bool r);
#endif
#ifdef USE_MOVIE_PLAYER
	bool open_movie_file(const _TCHAR* file_path);
	void close_movie_file();
	void play_movie();
	void stop_movie();
	void pause_movie();
	double get_movie_frame_rate()
	{
		return movie_frame_rate;
	}
	int get_movie_sound_rate()
	{
		return movie_sound_rate;
	}
	void set_cur_movie_frame(int frame, bool relative);
	uint32_t get_cur_movie_frame();
	bool now_movie_play, now_movie_pause;
#endif
#ifdef USE_VIDEO_CAPTURE
	int get_cur_capture_dev_index()
	{
		return cur_capture_dev_index;
	}
	int get_num_capture_devs()
	{
		return num_capture_devs;
	}
	_TCHAR* get_capture_dev_name(int index)
	{
		return capture_dev_name[index];
	}
	void open_capture_dev(int index, bool pin);
	void close_capture_dev();
	void show_capture_dev_filter();
	void show_capture_dev_pin();
	void show_capture_dev_source();
	void set_capture_dev_channel(int ch);
#endif
	
	// common printer
#ifdef USE_PRINTER
	void create_bitmap(bitmap_t *bitmap, int width, int height);
	void release_bitmap(bitmap_t *bitmap);
	void create_font(font_t *font, const _TCHAR *family, int width, int height, int rotate, bool bold, bool italic);
	void release_font(font_t *font);
	void create_pen(pen_t *pen, int width, uint8_t r, uint8_t g, uint8_t b);
	void release_pen(pen_t *pen);
	void clear_bitmap(bitmap_t *bitmap, uint8_t r, uint8_t g, uint8_t b);
	int get_text_width(bitmap_t *bitmap, font_t *font, const char *text);
	void draw_text_to_bitmap(bitmap_t *bitmap, font_t *font, int x, int y, const char *text, uint8_t r, uint8_t g, uint8_t b);
	void draw_line_to_bitmap(bitmap_t *bitmap, pen_t *pen, int sx, int sy, int ex, int ey);
	void draw_rectangle_to_bitmap(bitmap_t *bitmap, int x, int y, int width, int height, uint8_t r, uint8_t g, uint8_t b);
	void draw_point_to_bitmap(bitmap_t *bitmap, int x, int y, uint8_t r, uint8_t g, uint8_t b);
	void stretch_bitmap(bitmap_t *dest, int dest_x, int dest_y, int dest_width, int dest_height, bitmap_t *source, int source_x, int source_y, int source_width, int source_height);
#endif
	void write_bitmap_to_file(bitmap_t *bitmap, const _TCHAR *file_path);
	
	// common socket
#ifdef USE_SOCKET
	SOCKET get_socket(int ch)
	{
		return soc[ch];
	}
	void notify_socket_connected(int ch);
	void notify_socket_disconnected(int ch);
	void update_socket();
	bool initialize_socket_tcp(int ch);
	bool initialize_socket_udp(int ch);
	bool connect_socket(int ch, uint32_t ipaddr, int port);
	void disconnect_socket(int ch);
	bool listen_socket(int ch);
	void send_socket_data_tcp(int ch);
	void send_socket_data_udp(int ch, uint32_t ipaddr, int port);
	void send_socket_data(int ch);
	void recv_socket_data(int ch);
#endif
	
	// common midi
#ifdef USE_MIDI
	void send_to_midi(uint8_t data);
	bool recv_from_midi(uint8_t *data);
#endif

	godot::PackedVector2Array* get_sound_buffer()
	{
		return pSoundBuffer;
	}
	
//	// win32 dependent
//	void invalidate_screen();
//	void update_screen(HDC hdc);
//	HWND main_window_handle;
//	HINSTANCE instance_handle;
//	bool vista_or_later;
	void set_sound_play_point(int ofs)
	{
		sound_play_point = ofs;
	}
	void set_joystate(int num, int state);
};

#endif
