#ifndef M6_TILE_MAP_H
#define M6_TILE_MAP_H

#include <vector>

// Guarda a matriz de índices do cenário
class TileMap {
    int width;
    int height;
    std::vector<unsigned char> map;

public:
    // Cria um mapa preenchido com o tile inicial informado
    TileMap(int w, int h, unsigned char initWith)
        : width(w), height(h), map(w * h, initWith) {
    }

    // Retorna a largura do mapa em tiles
    int getWidth() const {
        return width;
    }

    // Retorna a altura do mapa em tiles
    int getHeight() const {
        return height;
    }

    // Consulta o índice do tile em uma posição da matriz
    unsigned char getTile(int col, int row) const {
        return map[col + row * width];
    }

    // Define o índice do tile em uma posição da matriz
    void setTile(int col, int row, unsigned char tile) {
        map[col + row * width] = tile;
    }
};

#endif
