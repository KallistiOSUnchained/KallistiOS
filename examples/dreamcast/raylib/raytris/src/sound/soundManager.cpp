<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> d5839872 (Add raylib raytris example with changes to code formatting)
/* KallistiOS ##version##
   examples/dreamcast/raylib/raytris/src/sound/soundManager.cpp
   Copyright (C) 2024 Cole Hall
*/

<<<<<<< HEAD
=======
>>>>>>> bed13a85 (Rename example to raytris)
=======
>>>>>>> d5839872 (Add raylib raytris example with changes to code formatting)
#include "soundManager.h"

SoundManager::SoundManager(){
    sndRotate = snd_sfx_load("/rd/assets/sound/rotate.wav");
    sndClear = snd_sfx_load("/rd/assets/sound/clear.wav");
}

SoundManager::~SoundManager(){
    snd_sfx_unload(sndRotate);
    snd_sfx_unload(sndClear);
}

void SoundManager::PlayRotateSound(){
    if (sndRotate != SFXHND_INVALID) snd_sfx_play(sndRotate, 255, 128);
}

void SoundManager::PlayClearSound(){
    if (sndClear != SFXHND_INVALID) snd_sfx_play(sndClear, 255, 128);
<<<<<<< HEAD
<<<<<<< HEAD
}
=======
}
>>>>>>> bed13a85 (Rename example to raytris)
=======
}
>>>>>>> d5839872 (Add raylib raytris example with changes to code formatting)
