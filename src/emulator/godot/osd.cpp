/*
	Skelton for retropc emulator

	Author : Hiroshi.Ogino
	Date   : 2022.08.27-

	[ godot dependent ]
*/

#include "osd.h"

#ifndef _WIN32
#include "../../vkdef.h"
#define COLORONCOLOR 0
#endif

void OSD::initialize(int rate, int samples)
{
	printf("rate: %d samples: %d\n", rate, samples);
	initialize_screen();
	initialize_sound(rate, samples);
	initialize_input();
}
void OSD::release()
{
	release_screen();
	release_sound();
	release_input();
}

void OSD::power_off()
{
}

void OSD::suspend()
{
}

void OSD::restore()
{
}

void OSD::lock_vm()
{
	lock_count++;
}
void OSD::unlock_vm()
{
	if(--lock_count <= 0) {
		force_unlock_vm();
	}
}
void OSD::force_unlock_vm()
{
	lock_count = 0;
}
void OSD::sleep(uint32_t ms)
{

}

#ifdef USE_DEBUGGER
void OSD::start_waiting_in_debugger()
{
}
void OSD::finish_waiting_in_debugger()
{

}
void OSD::process_waiting_in_debugger()
{
}
#endif
	
// common console
void OSD::open_console(int width, int height, const _TCHAR* title)
{
}

void OSD::close_console()
{
}

unsigned int OSD::get_console_code_page()
{
	return 0;
}

void OSD::set_console_text_attribute(unsigned short attr)
{

}

void OSD::write_console(const _TCHAR* buffer, unsigned int length)
{

}

int OSD::read_console_input(_TCHAR* buffer, unsigned int length)
{
	return 0;
}

bool OSD::is_console_key_pressed(int vk)
{
	return false;
}

bool OSD::is_console_closed()
{
	return false;
}

void OSD::close_debugger_console()
{
}

#ifdef USE_MOUSE
void OSD::enable_mouse()
{
}

void OSD::disable_mouse()
{

}
void OSD::toggle_mouse()
{

}

#endif
