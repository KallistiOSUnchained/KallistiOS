<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> d5839872 (Add raylib raytris example with changes to code formatting)
/* KallistiOS ##version##
   examples/dreamcast/raylib/raytris/src/grid/grid.cpp
   Copyright (C) 2024 Cole Hall
*/

<<<<<<< HEAD
#include "grid.h"
#include "../colors/colors.h"
#include "../constants/constants.h"
#include <iostream>

#include <kos.h>
=======
=======
>>>>>>> d5839872 (Add raylib raytris example with changes to code formatting)
#include "grid.h"
#include "../colors/colors.h"
#include "../constants/constants.h"
<<<<<<< HEAD
>>>>>>> bed13a85 (Rename example to raytris)
=======
#include <iostream>

#include <kos.h>
>>>>>>> d5839872 (Add raylib raytris example with changes to code formatting)

Grid::Grid(){
    numRows = 20;
    numCols = 10;
    cellSize = Constants::cellSize;
    Initialize();
    colors = GetCellColors();
}

void Grid::Initialize(){
    for(int row = 0; row < numRows; row++){
        for(int column = 0; column < numCols; column++){
            grid[row][column] = 0;
        }
    }
}

void Grid::Print(){
    printf("Printing Grid");
    for(int row =0; row < numRows; row++){
        for(int column = 0; column < numCols; column++){
<<<<<<< HEAD
<<<<<<< HEAD
            std::cout << grid[row][column] << " ";
        }
        std::cout << std::endl;
=======
            printf("%d ", grid[row][column]);
        }
        printf("\n");
>>>>>>> bed13a85 (Rename example to raytris)
=======
            std::cout << grid[row][column] << " ";
        }
        std::cout << std::endl;
>>>>>>> d5839872 (Add raylib raytris example with changes to code formatting)
    }
}

void Grid::Draw(){
    for (int row = 0; row < numRows; row++){
        for (int column = 0; column < numCols; column++){
            int cellValue = grid[row][column];
            DrawRectangle(
                column * cellSize + Constants::gridOffset, 
                row * cellSize + 11, 
                cellSize - 1, 
                cellSize - 1, 
                colors[cellValue]
<<<<<<< HEAD
<<<<<<< HEAD
            );
        }
    }
=======
                );
=======
            );
>>>>>>> d5839872 (Add raylib raytris example with changes to code formatting)
        }
    }
<<<<<<< HEAD
    
>>>>>>> bed13a85 (Rename example to raytris)
=======
>>>>>>> d5839872 (Add raylib raytris example with changes to code formatting)
}

bool Grid::IsCellOutside(int row, int column){
    if(row >= 0 && row < numRows && column >= 0 && column < numCols){
        return false;
    }
    return true;
}

<<<<<<< HEAD
<<<<<<< HEAD
bool Grid::isCellEmpty(int row, int column){
=======
bool Grid::isCellEmpty(int row, int column)
{
>>>>>>> bed13a85 (Rename example to raytris)
=======
bool Grid::isCellEmpty(int row, int column){
>>>>>>> d5839872 (Add raylib raytris example with changes to code formatting)
    if(grid[row][column] == 0){
        return true;
    }
    return false;
}

int Grid::ClearFullRows(){
    int completed = 0;
    for(int row = numRows-1; row >= 0; row--){
        if(IsRowFull(row)){
            ClearRow(row);
            completed++;
        }else if(completed > 0){
            MoveRowDown(row, completed);
        }
    }
    return completed;
}

bool Grid::IsRowFull(int row){
    for (int column = 0; column < numCols; column++){
        if(grid[row][column] == 0){
            return false;
        }
    }
    return true;
<<<<<<< HEAD
<<<<<<< HEAD

=======
    
>>>>>>> bed13a85 (Rename example to raytris)
=======

>>>>>>> d5839872 (Add raylib raytris example with changes to code formatting)
}

void Grid::ClearRow(int row){
    for (int column = 0; column < numCols; column++){
        grid[row][column] = 0;
    }
}

void Grid::MoveRowDown(int row, int numRows){
<<<<<<< HEAD
<<<<<<< HEAD
    for (int column = 0; column < numCols; column++){
        grid[row + numRows][column] = grid[row][column];
        grid[row][column] = 0;
    }
}
<<<<<<< HEAD
=======
    for (int column = 0; column < numCols; column++)
    {
        grid[row + numRows][column] = grid[row][column];
        grid[row][column] = 0;
    }
    
}
>>>>>>> bed13a85 (Rename example to raytris)
=======
    for (int column = 0; column < numCols; column++){
        grid[row + numRows][column] = grid[row][column];
        grid[row][column] = 0;
    }
}
>>>>>>> d5839872 (Add raylib raytris example with changes to code formatting)
=======
>>>>>>> 6dffd11d (Added newlines to EOF that github likes to complain about)
