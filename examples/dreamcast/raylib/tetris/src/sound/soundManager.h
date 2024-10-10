#pragma once

#include <dc/sound/sound.h>
#include <dc/sound/sfxmgr.h>

class SoundManager {
public:
    SoundManager();
    ~SoundManager();
    
    void PlayRotateSound();
    void PlayClearSound();

private:
    sfxhnd_t sndRotate;
    sfxhnd_t sndClear;
};
