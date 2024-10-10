#include "vmuManager.h"

#include "../constants/vmuIcons.h"

VmuManager::VmuManager(){
    resetImage();
}

VmuManager::~VmuManager(){
}

void VmuManager::displayImage(const char *xmp){
    maple_device_t * vmu = maple_enum_type(0, MAPLE_FUNC_LCD);
    if(vmu == NULL) return;
    vmu_draw_lcd_xbm(vmu, xmp);
}

void VmuManager::resetImage(){
    displayImage(vmuNULL);
}
