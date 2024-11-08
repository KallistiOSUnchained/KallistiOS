<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> d5839872 (Add raylib raytris example with changes to code formatting)
/* KallistiOS ##version##
   examples/dreamcast/raylib/raytris/src/blocks/block.h
   Copyright (C) 2024 Cole Hall
*/

<<<<<<< HEAD
=======
>>>>>>> bed13a85 (Rename example to raytris)
=======
>>>>>>> d5839872 (Add raylib raytris example with changes to code formatting)
#pragma once

#include <vector>
#include <map>
#include "../position/position.h"
#include "../colors/colors.h"

class Block{

    public:
        Block();
        void Draw(int offsetX, int offsetY);
        void Move(int rows, int columns);
        void Reset();
        std::vector<Position> GetCellPositions();
        void Rotate();
        void UndoRotation();
        int id;
        std::map<int, std::vector<Position>> cells;
        const char* vmuIcon;

    private:
        int cellSize;
        int rotationState;
        std::vector<Color> colors;
        int rowOffset;
        int columnOffset;
<<<<<<< HEAD
<<<<<<< HEAD
};
=======
};
>>>>>>> bed13a85 (Rename example to raytris)
=======
};
>>>>>>> d5839872 (Add raylib raytris example with changes to code formatting)
