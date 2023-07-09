
#include "osd.h"

#undef min
#undef max

#define DSOUND_BUFFER_SIZE (DWORD)(sound_samples * 2)
#define DSOUND_BUFFER_HALF (DWORD)(sound_samples)

#include <godot_cpp/variant/typed_array.hpp>
//#include "PoolArrays.hpp"


// common sound
void OSD::initialize_sound(int rate, int samples)
{
	sound_rate = rate;
	sound_samples = samples;
	sound_available = sound_started = sound_muted = now_record_sound = false;
	rec_sound_buffer_ptr = 0;

	pSoundBuffer = new godot::PackedVector2Array();
	pSoundBuffer->resize(DSOUND_BUFFER_SIZE);

	sound_available = sound_first_half = true;
}

void OSD::release_sound()
{
	delete pSoundBuffer;
}

void OSD::update_sound(int* extra_frames)
{
	static int count = 0;
	*extra_frames = 0;

	sound_muted = false;
	
	if(sound_available) {
		DWORD offset;

		if(!sound_started) {
			sound_started = true;
			return;
		}

		if(sound_first_half) {
			if(sound_play_point < DSOUND_BUFFER_HALF) {
				return;
			}
			offset = 0;
		} else {
			if(sound_play_point >= DSOUND_BUFFER_HALF) {
				return;
			}
			offset = DSOUND_BUFFER_HALF;
		}
		uint16_t* sound_buffer = vm->create_sound(extra_frames);

		for(int i = 0; i < DSOUND_BUFFER_HALF; i++)
		{
			uint16_t left  = sound_buffer[i*2];
			uint16_t right = sound_buffer[i*2+1];

			real_t leftr  = (real_t)((int16_t)left) / 32768.0;
			real_t rightr = (real_t)((int16_t)right) / 32768.0;
			if(leftr >  1.0)leftr = 1.0;
			if(leftr < -1.0)leftr = -1.0;
			if(rightr >  1.0)rightr = 1.0;
			if(rightr < -1.0)rightr = -1.0;

			pSoundBuffer->set(offset + i, godot::Vector2(leftr, rightr));
		}
		sound_first_half = !sound_first_half;
	}
}

void OSD::mute_sound()
{

}

void OSD::stop_sound()
{

}

void OSD::start_record_sound()
{

}

void OSD::stop_record_sound()
{

}

void OSD::restart_record_sound()
{

}

void OSD::write_bitmap_to_file(bitmap_t *bitmap, const _TCHAR *file_path)
{
}
