<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> d5839872 (Add raylib raytris example with changes to code formatting)
/* KallistiOS ##version##
   examples/dreamcast/raylib/raytris/src/grid/grid.h
   Copyright (C) 2024 Cole Hall
*/

<<<<<<< HEAD
=======
>>>>>>> bed13a85 (Rename example to raytris)
=======
>>>>>>> d5839872 (Add raylib raytris example with changes to code formatting)
#pragma once
#include <vector>
#include <raylib.h>

class Grid{
    public:
        Grid();
        void Initialize();
        void Print();
        void Draw();
        bool IsCellOutside(int row, int column);
        bool isCellEmpty(int row, int column);
        int ClearFullRows();
        int grid[20][10];
<<<<<<< HEAD
<<<<<<< HEAD

=======
    
>>>>>>> bed13a85 (Rename example to raytris)
=======

>>>>>>> d5839872 (Add raylib raytris example with changes to code formatting)
    private:
        bool IsRowFull(int row);
        void ClearRow(int row);
        void MoveRowDown(int row, int numRows);
        int numRows;
        int numCols;
        int cellSize;
        std::vector<Color> colors;
<<<<<<< HEAD
<<<<<<< HEAD
};
=======
};
>>>>>>> bed13a85 (Rename example to raytris)
=======
};
>>>>>>> d5839872 (Add raylib raytris example with changes to code formatting)
