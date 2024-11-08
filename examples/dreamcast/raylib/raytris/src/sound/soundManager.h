<<<<<<< HEAD
/* KallistiOS ##version##
   examples/dreamcast/raylib/raytris/src/sound/soundManager.h
   Copyright (C) 2024 Cole Hall
*/

=======
>>>>>>> bed13a85 (Rename example to raytris)
#pragma once

#include <dc/sound/sound.h>
#include <dc/sound/sfxmgr.h>

class SoundManager {
public:
    SoundManager();
    ~SoundManager();
<<<<<<< HEAD

=======
    
>>>>>>> bed13a85 (Rename example to raytris)
    void PlayRotateSound();
    void PlayClearSound();

private:
    sfxhnd_t sndRotate;
    sfxhnd_t sndClear;
<<<<<<< HEAD
};
=======
};
>>>>>>> bed13a85 (Rename example to raytris)
