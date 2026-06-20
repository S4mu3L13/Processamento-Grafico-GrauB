#ifndef MAP_LOADER_H
#define MAP_LOADER_H

#include "TileMap.h"

#include <string>
#include <vector>

struct MapConfig {
    std::string tilesetPath;
    std::string lavaTexturePath;

    int tileCount = 0;
    int tileWidth = 64;
    int tileHeight = 32;

    int playerStartCol = 1;
    int playerStartRow = 1;

    std::vector<unsigned char> walkableTiles;
};

bool loadMap(
    const char* filename,
    TileMap& map,
    MapConfig& config
);

bool isWalkableTile(
    const MapConfig& config,
    unsigned char tile
);

#endif