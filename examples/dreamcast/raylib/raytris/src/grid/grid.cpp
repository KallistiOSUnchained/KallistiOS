<<<<<<< HEAD
/* KallistiOS ##version##
   examples/dreamcast/raylib/raytris/src/grid/grid.cpp
   Copyright (C) 2024 Cole Hall
*/

#include "grid.h"
#include "../colors/colors.h"
#include "../constants/constants.h"
#include <iostream>

#include <kos.h>
=======
#include "grid.h"
#include <kos.h>
#include "../colors/colors.h"
#include "../constants/constants.h"
>>>>>>> bed13a85 (Rename example to raytris)

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
            std::cout << grid[row][column] << " ";
        }
        std::cout << std::endl;
=======
            printf("%d ", grid[row][column]);
        }
        printf("\n");
>>>>>>> bed13a85 (Rename example to raytris)
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
            );
        }
    }
=======
                );
        }
        
    }
    
>>>>>>> bed13a85 (Rename example to raytris)
}

bool Grid::IsCellOutside(int row, int column){
    if(row >= 0 && row < numRows && column >= 0 && column < numCols){
        return false;
    }
    return true;
}

<<<<<<< HEAD
bool Grid::isCellEmpty(int row, int column){
=======
bool Grid::isCellEmpty(int row, int column)
{
>>>>>>> bed13a85 (Rename example to raytris)
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

=======
    
>>>>>>> bed13a85 (Rename example to raytris)
}

void Grid::ClearRow(int row){
    for (int column = 0; column < numCols; column++){
        grid[row][column] = 0;
    }
}

void Grid::MoveRowDown(int row, int numRows){
<<<<<<< HEAD
    for (int column = 0; column < numCols; column++){
        grid[row + numRows][column] = grid[row][column];
        grid[row][column] = 0;
    }
}
=======
    for (int column = 0; column < numCols; column++)
    {
        grid[row + numRows][column] = grid[row][column];
        grid[row][column] = 0;
    }
    
}
>>>>>>> bed13a85 (Rename example to raytris)
