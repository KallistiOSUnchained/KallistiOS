#include "game.h"
#include "../constants/constants.h"
#include <random>
#include <kos.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>

// The below moves are in numpad notation because I can't understand them otherwise.
const int Game::moves[15][2] = {
    {1, 0},   // Move 2
    {1, 1},   // Move 1
    {0, 1},   // Move 4
    {1, -1},  // Move 3
    {0, -1},  // Move 6
    {-1, 0},  // Move 8
    {-1, 1},  // Move 7
    {-1, -1}, // Move 9
    {0, -2},  // Move 66 (stuck on a wall fallback)
    {0, 2},   // Move 44 (stuck on a wall fallback)
    {-2, 0},  // Move 88
    {-2, 1},  // Move 87
    {-2, -1}, // Move 89
    {-2, 2},  // Move 77
    {-2, -2}, // Move 99
};

Game::Game(){
    grid = Grid();
    blocks = GetAllBlocks();
    currentBlock = GetRandomBlock();
    nextBlock = GetRandomBlock();
    prev_buttons = 0;
    gameOver = false;
    score = 0;
    lastHeldMoveTime = 0.0;
    vmuManager.resetImage();
}

Block Game::GetRandomBlock(){
    if(blocks.empty()){
        blocks = GetAllBlocks();
    }
    int randomIndex = rand() % blocks.size();
    Block block = blocks[randomIndex];
    blocks.erase(blocks.begin() + randomIndex);
    return block;
}

std::vector<Block> Game::GetAllBlocks(){
    return {IBlock(), JBlock(), LBlock(), OBlock(), SBlock(), TBlock(), ZBlock()};
}

void Game::Draw(){
    grid.Draw();
    currentBlock.Draw(Constants::gridOffset, 11);
}

void Game::DrawBlockAtPosition(Block& block, int offsetX, int offsetY, int offsetXAdjustment, int offsetYAdjustment) {
    if (block.id == -1) return;
    if (block.id == 3 || block.id == 4) {
        block.Draw(offsetX + offsetXAdjustment, offsetY + offsetYAdjustment);
    } else {
        block.Draw(offsetX, offsetY);
    }
}

void Game::DrawHeld(int offsetX, int offsetY) {
    DrawBlockAtPosition(heldBlock, offsetX, offsetY, -15, 0);
}

void Game::DrawNext(int offsetX, int offsetY) {
    DrawBlockAtPosition(nextBlock, offsetX, offsetY, -15, 10);
}

void Game::HandleInput() {
    if ((cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER)) != NULL) {
        state = (cont_state_t *)maple_dev_status(cont);

        if (state == NULL) {
            return;
        }

        uint16_t just_pressed = state->buttons & ~prev_buttons;

        double currentTime = GetTime();  
        prev_buttons = state->buttons; 

        switch (just_pressed) {
            case CONT_START:
                if(gameOver){
                    gameOver = false;
                    Reset();
                }
                break;

            case CONT_DPAD_LEFT:
                MoveBlockLeft();
                lastHeldMoveTime = currentTime + 0.1;
                break;

            case CONT_DPAD_RIGHT:
                MoveBlockRight();
                lastHeldMoveTime = currentTime + 0.1;
                break;

            case CONT_DPAD_DOWN:
                MoveBlockDown();
                UpdateScore(0, 1);
                lastHeldMoveTime = currentTime;
                break;
            
            case CONT_DPAD_UP:
                HardDrop();
                break;

            case CONT_X:
                RotateBlock(false);
                break;
            
            case CONT_B:
                RotateBlock(true);
                break;

            default:
                break;
        }

        if (prev_buttons & (CONT_DPAD_LEFT | CONT_DPAD_RIGHT | CONT_DPAD_DOWN)) {
            if (currentTime - lastHeldMoveTime >= moveThreshold) {
                if (prev_buttons & CONT_DPAD_LEFT) {
                    MoveBlockLeft();
                }
                if (prev_buttons & CONT_DPAD_RIGHT) {
                    MoveBlockRight();
                }
                if (prev_buttons & CONT_DPAD_DOWN) {
                    MoveBlockDown();
                    UpdateScore(0, 1);
                }
                lastHeldMoveTime = currentTime;
            }
        }
        
        int leftTrigger = state->ltrig;
        if (leftTrigger > 10){
            if(!canHoldBlock) return;
            vmuManager.displayImage(currentBlock.vmuIcon);
            HoldBlock();
        }
    }
}

void Game::HardDrop(){
    while(!gameOver){
        currentBlock.Move(1, 0);
        if(IsBlockOutside() || BlockFits() == false){
            currentBlock.Move(-1, 0);
            LockBlock();
            return;
        } else {
            UpdateScore(0, 2);
        }
    }
}

void Game::HoldBlock(){
    currentBlock.Reset();
    canHoldBlock = false;
    Block oldCurrent = currentBlock;
    if(heldBlock.id == NullBlock().id){
        currentBlock = nextBlock;
        heldBlock = oldCurrent;
        nextBlock = GetRandomBlock();
        return;
    }
    currentBlock = heldBlock;
    heldBlock = oldCurrent;
}

void Game::MoveBlockLeft(){
    if(gameOver) return;
    currentBlock.Move(0, -1);
    if(IsBlockOutside() || BlockFits() == false){
        currentBlock.Move(0, 1);
    } else {
        timeSinceLastRotation = GetTime();
    }
}

void Game::MoveBlockRight(){
    if(gameOver) return;
    currentBlock.Move(0, 1);
    if(IsBlockOutside() || BlockFits() == false){
        currentBlock.Move(0, -1);
    } else {
        timeSinceLastRotation = GetTime();
    }
}

void Game::MoveBlockDown(){
    if(gameOver) return;
    currentBlock.Move(1, 0);
    if(IsBlockOutside() || BlockFits() == false){
        currentBlock.Move(-1, 0);
        double currentTime = GetTime();
        if(floorContactTime == 0) {
            floorContactTime = currentTime;
        }
        if(currentTime - timeSinceLastRotation >= timerGraceSmall || currentTime - floorContactTime >= timerGraceBig){
            LockBlock();
            timeSinceLastRotation = currentTime; // Stops a player from immediately dying if right at the top
        }
    }
}

bool Game::IsBlockOutside(){
    std::vector<Position> tiles = currentBlock.GetCellPositions();
    for(Position item: tiles){
        if(grid.IsCellOutside(item.row, item.column)){
            return true;
        }
    }
    return false;
}

void Game::RotateBlock(bool clockwise){
    if(gameOver) return;

    if(clockwise) {
        currentBlock.Rotate();
    } else {
        currentBlock.UndoRotation();
    }

    const int moveCount = sizeof(moves) / sizeof(moves[0]);

    if (IsBlockOutside() || !BlockFits()) {
        bool foundFit = false;

        for (int i = 0; i < moveCount; i++) {
            currentBlock.Move(moves[i][0], moves[i][1]);

            if (!IsBlockOutside() && BlockFits()) {
                foundFit = true;
                soundManager.PlayRotateSound();
                timeSinceLastRotation = GetTime();
                break;
            }

            currentBlock.Move(-moves[i][0], -moves[i][1]);
        }

        if (!foundFit) {
            if(clockwise) {
                currentBlock.UndoRotation();
            } else {
                currentBlock.Rotate();
            }
        }
    } else {
        timeSinceLastRotation = GetTime();
        soundManager.PlayRotateSound();
    }
}

void Game::LockBlock(){
    std::vector<Position> tiles = currentBlock.GetCellPositions();
    lastHeldMoveTime = 0;
    for(Position item: tiles){
        grid.grid[item.row][item.column] = currentBlock.id;
    }
    currentBlock = nextBlock;
    if(BlockFits() == false){
        gameOver = true;
    }
    canHoldBlock = true;
    nextBlock = GetRandomBlock();
    floorContactTime = 0;
    int rowsCleared = grid.ClearFullRows();
    if(rowsCleared > 0){
        UpdateScore(rowsCleared, 0);
        soundManager.PlayClearSound();
    }
}

bool Game::BlockFits()
{
    std::vector<Position> tiles = currentBlock.GetCellPositions();
    for(Position item: tiles){
        if(grid.isCellEmpty(item.row, item.column) == false){
            return false;
        }
    }
    return true;
}

void Game::Reset(){
    grid.Initialize();
    blocks = GetAllBlocks();
    currentBlock = GetRandomBlock();
    nextBlock = GetRandomBlock();
    heldBlock = NullBlock();
    vmuManager.resetImage();
    score = 0;
    canHoldBlock = true;
}

void Game::UpdateScore(int linesCleared, int moveDownPoints){
    if(gameOver) return;
    switch(linesCleared){
        case 1:
            score += 100;
            break;
        case 2:
            score += 300;
            break;
        case 3:
            score += 500;
            break;
        case 4:
            score += 1000;
            break;
        default:
            break;
    }

    score += moveDownPoints;
}
