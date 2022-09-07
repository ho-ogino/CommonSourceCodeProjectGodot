#include "osd.h"

#ifndef _WIN32
#include "../../vkdef.h"
#define COLORONCOLOR 0
#endif

#undef min
#undef max

#include "PoolArrays.hpp"

void OSD::initialize_screen()
{
	host_window_width = WINDOW_WIDTH;
	host_window_height = WINDOW_HEIGHT;
	host_window_mode = true;
	
	vm_screen_width = SCREEN_WIDTH;
	vm_screen_height = SCREEN_HEIGHT;
	vm_window_width = WINDOW_WIDTH;
	vm_window_height = WINDOW_HEIGHT;
	vm_window_width_aspect = WINDOW_WIDTH_ASPECT;
	vm_window_height_aspect = WINDOW_HEIGHT_ASPECT;
	
	memset(&vm_screen_buffer, 0, sizeof(bitmap_t));
#ifdef USE_SCREEN_FILTER
	memset(&filtered_screen_buffer, 0, sizeof(bitmap_t));
	memset(&tmp_filtered_screen_buffer, 0, sizeof(bitmap_t));
#endif
//#ifdef USE_SCREEN_ROTATE
	memset(&rotated_screen_buffer, 0, sizeof(bitmap_t));
//#endif
	memset(&stretched_screen_buffer, 0, sizeof(bitmap_t));
	memset(&shrinked_screen_buffer, 0, sizeof(bitmap_t));
	memset(&reversed_screen_buffer, 0, sizeof(bitmap_t));
	memset(&video_screen_buffer, 0, sizeof(bitmap_t));
	
	first_draw_screen = false;
	first_invalidate = true;
	self_invalidate = false;
}

void OSD::initialize_screen_buffer(bitmap_t *buffer, int width, int height, int mode)
{
	release_screen_buffer(buffer);

	buffer->lpBmp = (scrntype_t*)malloc(width * height * sizeof(scrntype_t));
	buffer->pBitmapArray = new godot::PoolByteArray();
	buffer->pBitmapArray->resize(width * height * 3);	// R,G,B

	buffer->width = width;
	buffer->height = height;
}

void OSD::release_screen_buffer(bitmap_t *buffer)
{
	// TODO bufferの中身リリース
	if(buffer->initialized())
	{
		free(buffer->lpBmp);
		delete buffer->pBitmapArray;
	}
	memset(buffer, 0, sizeof(bitmap_t));
}
void OSD::release_screen()
{
	if(vm_screen_buffer.initialized())
	{
		release_screen_buffer(&vm_screen_buffer);
	}
}

// common screen
double OSD::get_window_mode_power(int mode)
{
	if(mode + WINDOW_MODE_BASE == 2) {
		return 1.5;
	} else if(mode + WINDOW_MODE_BASE > 2) {
		return mode + WINDOW_MODE_BASE - 1;
	}
	return mode + WINDOW_MODE_BASE;
}

int OSD::get_window_mode_width(int mode)
{
//#ifdef USE_SCREEN_ROTATE
	if(config.rotate_type == 1 || config.rotate_type == 3) {
		return (int)((config.window_stretch_type == 0 ? vm_window_height : vm_window_height_aspect) * get_window_mode_power(mode));
	}
//#endif
	return (int)((config.window_stretch_type == 0 ? vm_window_width : vm_window_width_aspect) * get_window_mode_power(mode));
}

int OSD::get_window_mode_height(int mode)
{
//#ifdef USE_SCREEN_ROTATE
	if(config.rotate_type == 1 || config.rotate_type == 3) {
		return (int)((config.window_stretch_type == 0 ? vm_window_width : vm_window_width_aspect) * get_window_mode_power(mode));
	}
//#endif
	return (int)((config.window_stretch_type == 0 ? vm_window_height : vm_window_height_aspect) * get_window_mode_power(mode));
}

void OSD::set_host_window_size(int window_width, int window_height, bool window_mode)
{
	if(window_width != -1) {
		host_window_width = window_width;
	}
	if(window_height != -1) {
		host_window_height = window_height;
	}
	host_window_mode = window_mode;
	
	first_draw_screen = false;
	first_invalidate = true;
}

void OSD::set_vm_screen_size(int screen_width, int screen_height, int window_width, int window_height, int window_width_aspect, int window_height_aspect)
{
	if(vm_screen_width != screen_width || vm_screen_height != screen_height) {
		if(window_width == -1) {
			window_width = screen_width;
		}
		if(window_height == -1) {
			window_height = screen_height;
		}
		if(window_width_aspect == -1) {
			window_width_aspect = window_width;
		}
		if(window_height_aspect == -1) {
			window_height_aspect = window_height;
		}
		vm_screen_width = screen_width;
		vm_screen_height = screen_height;
		
		if(vm_window_width != window_width || vm_window_height != window_height || vm_window_width_aspect != window_width_aspect || vm_window_height_aspect != window_height_aspect) {
			vm_window_width = window_width;
			vm_window_height = window_height;
			vm_window_width_aspect = window_width_aspect;
			vm_window_height_aspect = window_height_aspect;
		} else {
			// to make sure
			set_host_window_size(-1, -1, host_window_mode);
		}
	}
	if(vm_screen_buffer.width != vm_screen_width || vm_screen_buffer.height != vm_screen_height) {
		if(now_record_video) {
			stop_record_video();
//			stop_record_sound();
		}
		initialize_screen_buffer(&vm_screen_buffer, vm_screen_width, vm_screen_height, COLORONCOLOR);
	}
}

void OSD::set_vm_screen_lines(int lines)
{
}

scrntype_t* OSD::get_vm_screen_buffer(int y)
{
	return vm_screen_buffer.get_buffer(y);
}

int OSD::draw_screen()
{
	// draw screen
	if(vm_screen_buffer.width != vm_screen_width || vm_screen_buffer.height != vm_screen_height) {
		if(now_record_video) {
			stop_record_video();
//			stop_record_sound();
		}
		initialize_screen_buffer(&vm_screen_buffer, vm_screen_width, vm_screen_height, COLORONCOLOR);
		printf("init screen %d,%d\n", vm_screen_width, vm_screen_height);
	}
#ifdef USE_SCREEN_FILTER
	screen_skip_line = false;
#endif
	vm->draw_screen();

	// なんかする？

	return 1;
}

void OSD::capture_screen()
{

}
bool OSD::start_record_video(int fps)
{
	return false;
}
void OSD::stop_record_video()
{

}
void OSD::restart_record_video()
{

}
void OSD::add_extra_frames(int extra_frames)
{

}
