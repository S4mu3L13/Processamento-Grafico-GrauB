#ifndef AV3_TILE_MAP_H
#define AV3_TILE_MAP_H

#include <vector>

class TileMap {
    int width;
    int height;
    std::vector<unsigned char> map;

public:
    TileMap(int w, int h, unsigned char initWith)
        : width(w), height(h), map(w * h, initWith) {
    }

    int getWidth() const {
        return width;
    }

    int getHeight() const {
        return height;
    }

    unsigned char getTile(int col, int row) const {
        return map[col + row * width];
    }

    void setTile(int col, int row, unsigned char tile) {
        map[col + row * width] = tile;
    }
};

#endif
