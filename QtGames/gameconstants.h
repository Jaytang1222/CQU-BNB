#ifndef GAMECONSTANTS_H
#define GAMECONSTANTS_H

// Game constants (ASCII only)
namespace GameConstants {
    inline constexpr int UNIT_SIZE = 8;
    inline constexpr int LOGIC_UNIT = 1;
    inline constexpr int BLOCK_SIZE = 4;
    inline constexpr int MAP_GRID_COUNT = 25;
    inline constexpr int MAP_SIZE_UNITS = 25 * 4;          // 100
    inline constexpr int MAP_SIZE_PIXELS = MAP_SIZE_UNITS * UNIT_SIZE; // 800
}

#endif // GAMECONSTANTS_H
