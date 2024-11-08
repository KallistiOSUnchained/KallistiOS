<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> d5839872 (Add raylib raytris example with changes to code formatting)
/* KallistiOS ##version##
   examples/dreamcast/raylib/raytris/src/vmu/vmuManager.cpp
   Copyright (C) 2024 Cole Hall
*/

<<<<<<< HEAD
=======
>>>>>>> bed13a85 (Rename example to raytris)
=======
>>>>>>> d5839872 (Add raylib raytris example with changes to code formatting)
#include "vmuManager.h"

#include "../constants/vmuIcons.h"

VmuManager::VmuManager(){
    resetImage();
}

<<<<<<< HEAD
<<<<<<< HEAD
void VmuManager::displayImage(const char *xmp){
    maple_device_t *vmu = maple_enum_type(0, MAPLE_FUNC_LCD);
    if(vmu == nullptr) return;
=======
VmuManager::~VmuManager(){
}

void VmuManager::displayImage(const char *xmp){
    maple_device_t * vmu = maple_enum_type(0, MAPLE_FUNC_LCD);
    if(vmu == NULL) return;
>>>>>>> bed13a85 (Rename example to raytris)
=======
void VmuManager::displayImage(const char *xmp){
    maple_device_t *vmu = maple_enum_type(0, MAPLE_FUNC_LCD);
    if(vmu == nullptr) return;
>>>>>>> d5839872 (Add raylib raytris example with changes to code formatting)
    vmu_draw_lcd_xbm(vmu, xmp);
}

void VmuManager::resetImage(){
    displayImage(vmuNULL);
<<<<<<< HEAD
<<<<<<< HEAD
}
=======
}
>>>>>>> bed13a85 (Rename example to raytris)
=======
}
>>>>>>> d5839872 (Add raylib raytris example with changes to code formatting)
