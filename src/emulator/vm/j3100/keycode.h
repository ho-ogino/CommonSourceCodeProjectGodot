/*
	TOSHIBA J-3100GT Emulator 'eJ-3100GT'
	TOSHIBA J-3100SL Emulator 'eJ-3100SL'

	Author : Takeda.Toshiya
	Date   : 2011.08.16-

	[ keyboard ]
*/

#ifndef _KEYCODE_H_
#define _KEYCODE_H_

static const int key_table[256] = {
	// NmLock -> Pause
	// PrtScr -> F11
	// SysReq -> F12
	 0, 0, 0, 0, 0, 0, 0, 0,14,15, 0, 0, 0,28, 0, 0,
	42,29,56,69,58,87, 0, 0, 0,86, 0, 1, 0, 0, 0, 0,
	57,73,81,79,71,75,72,77,80, 0, 0, 0, 0,82,83, 0,
	11, 2, 3, 4, 5, 6, 7, 8, 9,10, 0, 0, 0, 0, 0, 0,
	 0,30,48,46,32,18,33,34,35,23,36,37,38,50,49,24,
	25,16,19,31,20,22,47,17,45,21,44, 0, 0, 0, 0, 0,
	88,89,90,91,92,93,94,95,96,97, 0,98, 0,99,100,0,
	59,60,61,62,63,64,65,66,67,68,55,84, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0,70, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,40,39,51,12,52,53,
	26, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,27,43,85,13, 0,
	 0, 0,41, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

#endif

