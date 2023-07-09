#include "osd.h"

#ifndef _WIN32
#include "../../vkdef.h"
#endif

void OSD::initialize_input()
{
	// initialize status
	memset(key_status, 0, sizeof(key_status));
#ifdef USE_JOYSTICK
	memset(joy_status, 0, sizeof(joy_status));
	memset(joy_to_key_status, 0, sizeof(joy_to_key_status));
#endif
#ifdef USE_MOUSE
	memset(mouse_status, 0, sizeof(mouse_status));
#endif
	dinput_key_available = false;

	// initialize keycode convert table
	FILEIO* fio = new FILEIO();
	if(fio->Fopen(create_local_path(_T("keycode.cfg")), FILEIO_READ_BINARY)) {
		fio->Fread(keycode_conv, sizeof(keycode_conv), 1);
		fio->Fclose();
	} else {
		for(int i = 0; i < 256; i++) {
			keycode_conv[i] = i;
		}
	}
	delete fio;


#ifdef USE_AUTO_KEY
	now_auto_key = false;
#endif
}

void OSD::release_input()
{
}


// common input
void OSD::update_input()
{
	// release keys
#ifdef USE_AUTO_KEY
	if(lost_focus && !now_auto_key) {
#else
	if(lost_focus) {
#endif
		// we lost key focus so release all pressed keys
		for(int i = 0; i < 256; i++) {
			if(key_status[i] & 0x80) {
				key_status[i] &= 0x7f;
				if(!key_status[i]) {
					vm->key_up(i);
				}
			}
		}
	} else {
		for(int i = 0; i < 256; i++) {
			if(key_status[i] & 0x7f) {
				key_status[i] = (key_status[i] & 0x80) | ((key_status[i] & 0x7f) - 1);
				if(!key_status[i]) {
					vm->key_up(i);
				}
			}
		}
	}
	lost_focus = false;
	
	// VK_$00 should be 0
	key_status[0] = 0;
}

void OSD::set_joystate(int num, int state)
{
#ifdef USE_JOYSTICK
	joy_status[num] = state;
#endif
}

void OSD::key_down(int code, bool extended, bool repeat)
{
	//printf("osd key down: %d\n", code);
#ifdef USE_AUTO_KEY
	if(!now_auto_key && !config.romaji_to_kana) {
#endif
		if(!dinput_key_available) {
			if(code == VK_SHIFT) {
				bool isShiftPressing = true; // Input::get_singleton()->is_key_pressed(16777237);
 
				if(!(key_status[VK_RSHIFT] & 0x80) && isShiftPressing /*(GetAsyggncKeyState(VK_RSHIFT) & 0x8000)*/) {
					key_down_native(VK_RSHIFT, repeat);
				}
				if(!(key_status[VK_LSHIFT] & 0x80) && isShiftPressing) {
					code = VK_LSHIFT;
				} else {
					return;
				}
			} else if(code == VK_CONTROL) {
				bool isCtrlPressing = true; // Input::get_singleton()->is_key_pressed(16777238);

				if(!(key_status[VK_RCONTROL] & 0x80) && isCtrlPressing) {
					key_down_native(VK_RCONTROL, repeat);
				}
				if(!(key_status[VK_LCONTROL] & 0x80) && isCtrlPressing) {
					code = VK_LCONTROL;
				} else {
					return;
				}
			}
			// else if(code == VK_MENU) {
			// 	if(!(key_status[VK_RMENU] & 0x80) && (GetAsyncKeyState(VK_RMENU) & 0x8000)) {
			// 		key_down_native(VK_RMENU, repeat);
			// 	}
			// 	if(!(key_status[VK_LMENU] & 0x80) && (GetAsyncKeyState(VK_LMENU) & 0x8000)) {
			// 		code = VK_LMENU;
			// 	} else {
			// 		return;
			// 	}
			// }
//			if(code == VK_LSHIFT || code == VK_RSHIFT) {
			if(code == VK_LSHIFT) {
				key_shift_pressed = true;
				return;
			}
			if(!extended) {
				switch(code) {
				case VK_INSERT:
					if(key_shift_pressed || key_shift_released || key_status[VK_NUMPAD0]) {
						code = VK_NUMPAD0;
						key_shift_pressed = true;
					}
					break;
				case VK_END:
					if(key_shift_pressed || key_shift_released || key_status[VK_NUMPAD1]) {
						code = VK_NUMPAD1;
						key_shift_pressed = true;
					}
					break;
				case VK_DOWN:
					if(key_shift_pressed || key_shift_released || key_status[VK_NUMPAD2]) {
						code = VK_NUMPAD2;
						key_shift_pressed = true;
					}
					break;
				case VK_NEXT:
					if(key_shift_pressed || key_shift_released || key_status[VK_NUMPAD3]) {
						code = VK_NUMPAD3;
						key_shift_pressed = true;
					}
					break;
				case VK_LEFT:
					if(key_shift_pressed || key_shift_released || key_status[VK_NUMPAD4]) {
						code = VK_NUMPAD4;
						key_shift_pressed = true;
					}
					break;
				case VK_CLEAR:
					if(key_shift_pressed || key_shift_released || key_status[VK_NUMPAD5]) {
						code = VK_NUMPAD5;
						key_shift_pressed = true;
					}
					break;
				case VK_RIGHT:
					if(key_shift_pressed || key_shift_released || key_status[VK_NUMPAD6]) {
						code = VK_NUMPAD6;
						key_shift_pressed = true;
					}
					break;
				case VK_HOME:
					if(key_shift_pressed || key_shift_released || key_status[VK_NUMPAD7]) {
						code = VK_NUMPAD7;
						key_shift_pressed = true;
					}
					break;
				case VK_UP:
					if(key_shift_pressed || key_shift_released || key_status[VK_NUMPAD8]) {
						code = VK_NUMPAD8;
						key_shift_pressed = true;
					}
					break;
				case VK_PRIOR:
					if(key_shift_pressed || key_shift_released || key_status[VK_NUMPAD9]) {
						code = VK_NUMPAD9;
						key_shift_pressed = true;
					}
					break;
				}
			}
			key_down_native(code, repeat);
		} else {
			if(repeat || code == 0xf0 || code == 0xf1 || code == 0xf2 || code == 0xf3 || code == 0xf4) {
				key_down_native(code, repeat);
			}
		}
#ifdef USE_AUTO_KEY
	}
#endif
}



void OSD::key_up(int code, bool extended)
{
	//printf("osd key up: %d\n", code);
#ifdef USE_AUTO_KEY
	if(!now_auto_key && !config.romaji_to_kana) {
#endif
		if(!dinput_key_available) {
			if(code == VK_SHIFT) {
				bool isShiftPressing = false; // Input::get_singleton()->is_key_pressed(16777237);
				if((key_status[VK_RSHIFT] & 0x80) && !isShiftPressing) {
					key_up_native(VK_RSHIFT);
				}
				if((key_status[VK_LSHIFT] & 0x80) && !isShiftPressing) {
					code = VK_LSHIFT;
				} else {
					return;
				}
			} else if(code == VK_CONTROL) {
				bool isCtrlPressing = false; // Input::get_singleton()->is_key_pressed(16777238);
				if((key_status[VK_RCONTROL] & 0x80) && !isCtrlPressing) {
					key_up_native(VK_RCONTROL);
				}
				if((key_status[VK_LCONTROL] & 0x80) && !isCtrlPressing) {
					code = VK_LCONTROL;
				} else {
					return;
				}
			}
			// else if(code == VK_MENU) {
			//	if((key_status[VK_RMENU] & 0x80) && !(GetAsyncKeyState(VK_RMENU) & 0x8000)) {
			//		key_up_native(VK_RMENU);
			//	}
			//	if((key_status[VK_LMENU] & 0x80) && !(GetAsyncKeyState(VK_LMENU) & 0x8000)) {
			//		code = VK_LMENU;
			//	} else {
			//		return;
			//	}
			//}
//			if(code == VK_LSHIFT || code == VK_RSHIFT) {
			if(code == VK_LSHIFT) {
				key_shift_pressed = false;
				key_shift_released = true;
				return;
			}
			if(!extended) {
				switch(code) {
				case VK_NUMPAD0: case VK_INSERT:
					key_up_native(VK_NUMPAD0);
					key_up_native(VK_INSERT);
					return;
				case VK_NUMPAD1: case VK_END:
					key_up_native(VK_NUMPAD1);
					key_up_native(VK_END);
					return;
				case VK_NUMPAD2: case VK_DOWN:
					key_up_native(VK_NUMPAD2);
					key_up_native(VK_DOWN);
					return;
				case VK_NUMPAD3: case VK_NEXT:
					key_up_native(VK_NUMPAD3);
					key_up_native(VK_NEXT);
					return;
				case VK_NUMPAD4: case VK_LEFT:
					key_up_native(VK_NUMPAD4);
					key_up_native(VK_LEFT);
					return;
				case VK_NUMPAD5: case VK_CLEAR:
					key_up_native(VK_NUMPAD5);
					key_up_native(VK_CLEAR);
					return;
				case VK_NUMPAD6: case VK_RIGHT:
					key_up_native(VK_NUMPAD6);
					key_up_native(VK_RIGHT);
					return;
				case VK_NUMPAD7: case VK_HOME:
					key_up_native(VK_NUMPAD7);
					key_up_native(VK_HOME);
					return;
				case VK_NUMPAD8: case VK_UP:
					key_up_native(VK_NUMPAD8);
					key_up_native(VK_UP);
					return;
				case VK_NUMPAD9: case VK_PRIOR:
					key_up_native(VK_NUMPAD9);
					key_up_native(VK_PRIOR);
					return;
				}
			}
			key_up_native(code);
		}
#ifdef USE_AUTO_KEY
	}
#endif
}

void OSD::key_down_native(int code, bool repeat)
{
	bool keep_frames = false;
	
	if(code == 0xf0) {
		code = VK_CAPITAL;
		keep_frames = true;
	} else if(code == 0xf1 || code == 0xf2) {
		code = VK_KANA;
		keep_frames = true;
	} else if(code == 0xf3 || code == 0xf4) {
		code = VK_KANJI;
		keep_frames = true;
	}
	if(!(code == VK_LSHIFT || code == VK_RSHIFT || code == VK_LCONTROL || code == VK_RCONTROL || code == VK_LMENU || code == VK_RMENU)) {
		code = keycode_conv[code];
	}
	if(key_status[code] == 0 || keep_frames) {
		repeat = false;
	}
#ifdef DONT_KEEEP_KEY_PRESSED
	if(!(code == VK_LSHIFT || code == VK_RSHIFT || code == VK_LCONTROL || code == VK_RCONTROL || code == VK_LMENU || code == VK_RMENU)) {
		key_status[code] = KEY_KEEP_FRAMES;
	} else
#endif
	key_status[code] = keep_frames ? KEY_KEEP_FRAMES : 0x80;
	
	uint8_t prev_shift = key_status[VK_SHIFT];
	uint8_t prev_control = key_status[VK_CONTROL];
	uint8_t prev_menu = key_status[VK_MENU];
	
	key_status[VK_SHIFT] = key_status[VK_LSHIFT] | key_status[VK_RSHIFT];
	key_status[VK_CONTROL] = key_status[VK_LCONTROL] | key_status[VK_RCONTROL];
	key_status[VK_MENU] = key_status[VK_LMENU] | key_status[VK_RMENU];
	
	if(code == VK_LSHIFT || code == VK_RSHIFT) {
		if(prev_shift == 0 && key_status[VK_SHIFT] != 0) {
			vm->key_down(VK_SHIFT, repeat);
		}
	} else if(code == VK_LCONTROL|| code == VK_RCONTROL) {
		if(prev_control == 0 && key_status[VK_CONTROL] != 0) {
			vm->key_down(VK_CONTROL, repeat);
		}
	} else if(code == VK_LMENU|| code == VK_RMENU) {
		if(prev_menu == 0 && key_status[VK_MENU] != 0) {
			vm->key_down(VK_MENU, repeat);
		}
	}
	vm->key_down(code, repeat);
}

void OSD::key_up_native(int code)
{
	if(!(code == VK_LSHIFT || code == VK_RSHIFT || code == VK_LCONTROL || code == VK_RCONTROL || code == VK_LMENU || code == VK_RMENU)) {
		code = keycode_conv[code];
	}
	if(key_status[code] == 0) {
		return;
	}
	if((key_status[code] &= 0x7f) != 0) {
		return;
	}
	vm->key_up(code);

	uint8_t prev_shift = key_status[VK_SHIFT];
	uint8_t prev_control = key_status[VK_CONTROL];
	uint8_t prev_menu = key_status[VK_MENU];
	
	key_status[VK_SHIFT] = key_status[VK_LSHIFT] | key_status[VK_RSHIFT];
	key_status[VK_CONTROL] = key_status[VK_LCONTROL] | key_status[VK_RCONTROL];
	key_status[VK_MENU] = key_status[VK_LMENU] | key_status[VK_RMENU];
	
	if(code == VK_LSHIFT || code == VK_RSHIFT) {
		if(prev_shift != 0 && key_status[VK_SHIFT] == 0) {
			vm->key_up(VK_SHIFT);
		}
	} else if(code == VK_LCONTROL|| code == VK_RCONTROL) {
		if(prev_control != 0 && key_status[VK_CONTROL] == 0) {
			vm->key_up(VK_CONTROL);
		}
	} else if(code == VK_LMENU || code == VK_RMENU) {
		if(prev_menu != 0 && key_status[VK_MENU] == 0) {
			vm->key_up(VK_MENU);
		}
	}
}