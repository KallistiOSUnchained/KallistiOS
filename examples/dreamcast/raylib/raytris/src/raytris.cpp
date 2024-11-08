<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> d5839872 (Add raylib raytris example with changes to code formatting)
/* KallistiOS ##version##
   examples/dreamcast/raylib/raytris/src/raytris.cpp
   Copyright (C) 2024 Cole Hall
*/

<<<<<<< HEAD
=======
>>>>>>> bed13a85 (Rename example to raytris)
=======
>>>>>>> d5839872 (Add raylib raytris example with changes to code formatting)
#include <raylib.h>
#include "game/game.h"
#include "constants/constants.h"
#include "colors/colors.h"
<<<<<<< HEAD
<<<<<<< HEAD
#include <iostream>
#include <string>
#include <iostream> 
=======
#include "system/cd.h"
#include <iostream>
>>>>>>> bed13a85 (Rename example to raytris)
=======
#include <iostream>
#include <string>
#include <iostream> 
>>>>>>> d5839872 (Add raylib raytris example with changes to code formatting)

#include <kos/init.h>
#include <kos/dbgio.h>
#include <dc/sound/stream.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <wav/sndwav.h>

<<<<<<< HEAD
<<<<<<< HEAD
static double lastUpdateTime = 0;
=======
double lastUpdateTime = 0;
>>>>>>> bed13a85 (Rename example to raytris)
=======
static double lastUpdateTime = 0;
>>>>>>> d5839872 (Add raylib raytris example with changes to code formatting)

static bool EventTriggered(double interval){
    double currentTime = GetTime();
    if(currentTime - lastUpdateTime >= interval){
        lastUpdateTime = currentTime;
        return true;
    }
    return false;
}

<<<<<<< HEAD
<<<<<<< HEAD
int main(int argc, char* argv[]){
=======
static bool check_btn_combo(void){
    MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st)

    // Check if the required buttons (START, A, B, X, Y) are all pressed.
    const uint32_t exit_combo = CONT_START | CONT_A | CONT_B | CONT_X | CONT_Y;

    if((st->buttons & exit_combo) == exit_combo)
        return true;

    MAPLE_FOREACH_END()
    return false;
}

int main(){
>>>>>>> bed13a85 (Rename example to raytris)
=======
int main(int argc, char* argv[]){
>>>>>>> d5839872 (Add raylib raytris example with changes to code formatting)
    const int screenWidth = 640;
    const int screenHeight = 480;
    wav_stream_hnd_t bgm;

    InitWindow(screenWidth, screenHeight, "Block stacking puzzle game in KOS!");
    SetTargetFPS(60);

    snd_stream_init();
    wav_init();

    bgm = wav_create("/rd/assets/sound/bgm.adpcm", 1);
    wav_volume(bgm, 255);
    wav_play(bgm);

    Game game = Game();

    int TextUIDistance = Constants::gridWidthWithOffset + UIPadding::large;
    int scorePaddingHeight = UIPadding::medium;
    int scoreBoxPaddingHeight = scorePaddingHeight + 15 + UIPadding::small;
    int nextPaddingHeight = scoreBoxPaddingHeight + 60 + UIPadding::large;
    int nextBoxPaddingHeight = nextPaddingHeight + UIFont::medium + UIPadding::small;
    int gameOverPaddingHeight = nextBoxPaddingHeight + 180 + UIPadding::large;

<<<<<<< HEAD
<<<<<<< HEAD
    while(game.Running()){
        game.HandleInput();

=======
    while(!check_btn_combo()){
        game.HandleInput();
>>>>>>> bed13a85 (Rename example to raytris)
=======
    while(game.Running()){
        game.HandleInput();

>>>>>>> d5839872 (Add raylib raytris example with changes to code formatting)
        if(EventTriggered(0.2)){
            game.MoveBlockDown();
        }
        BeginDrawing();
        ClearBackground(darkBlue);
        game.Draw();
        DrawText("Hold", Constants::gridOffset - UIPadding::medium * 4, nextPaddingHeight, UIFont::medium, WHITE);
        DrawRectangleRounded({UIPadding::medium, (float)nextBoxPaddingHeight, Constants::gridOffset - UIPadding::large, 170}, 0.3, 6, lightBlue);

        DrawText("Score", TextUIDistance, scorePaddingHeight, UIFont::medium, WHITE);
        DrawRectangleRounded({Constants::gridWidthWithOffset + UIPadding::medium, (float)scoreBoxPaddingHeight, 170, 60}, 0.3, 6, lightBlue);

<<<<<<< HEAD
<<<<<<< HEAD
        std::string scoreText = std::to_string(game.score);
        Vector2 textSize = MeasureTextEx(GetFontDefault(), scoreText.c_str(), UIFont::medium, 0);

        DrawText(scoreText.c_str(), TextUIDistance + (170 - textSize.x)/2, scoreBoxPaddingHeight + UIPadding::medium, UIFont::medium, WHITE);
=======
        char scoreText[10];
        sprintf(scoreText, "%d", game.score);
        Vector2 textSize = MeasureTextEx(GetFontDefault(), scoreText, UIFont::medium, 0);

        DrawText(scoreText, TextUIDistance + (170 - textSize.x)/2, scoreBoxPaddingHeight + UIPadding::medium, UIFont::medium, WHITE);
>>>>>>> bed13a85 (Rename example to raytris)
=======
        std::string scoreText = std::to_string(game.score);
        Vector2 textSize = MeasureTextEx(GetFontDefault(), scoreText.c_str(), UIFont::medium, 0);

        DrawText(scoreText.c_str(), TextUIDistance + (170 - textSize.x)/2, scoreBoxPaddingHeight + UIPadding::medium, UIFont::medium, WHITE);
>>>>>>> d5839872 (Add raylib raytris example with changes to code formatting)

        DrawText("Next", TextUIDistance,  nextPaddingHeight, UIFont::medium, WHITE);
        DrawRectangleRounded({Constants::gridWidthWithOffset + UIPadding::medium, (float)nextBoxPaddingHeight, 170, 180}, 0.3, 6, lightBlue);
        if(game.gameOver){
            DrawText("GAME OVER\nPress start!", TextUIDistance, gameOverPaddingHeight, UIFont::medium, WHITE);
        }
        game.DrawNext(TextUIDistance - 20, nextBoxPaddingHeight + UIPadding::large * 1.5);
<<<<<<< HEAD
<<<<<<< HEAD

=======
        
>>>>>>> bed13a85 (Rename example to raytris)
=======

>>>>>>> d5839872 (Add raylib raytris example with changes to code formatting)
        game.DrawHeld(-20, nextBoxPaddingHeight + UIPadding::large * 1.5);
        EndDrawing();
    }

<<<<<<< HEAD
<<<<<<< HEAD
    std::cout << "Finishing - Cleaning up\n";  
    wav_stop(bgm);
    wav_shutdown();
    snd_stream_shutdown();
    std::cout << "Finished - Cleaning up\n";  

    return 0;
}
=======
    printf("Finishing - Cleaning up\n");
=======
    std::cout << "Finishing - Cleaning up\n";  
>>>>>>> d5839872 (Add raylib raytris example with changes to code formatting)
    wav_stop(bgm);
    wav_shutdown();
    snd_stream_shutdown();
    std::cout << "Finished - Cleaning up\n";  

    return 0;
<<<<<<< HEAD
}
>>>>>>> bed13a85 (Rename example to raytris)
=======
}
>>>>>>> d5839872 (Add raylib raytris example with changes to code formatting)
