#include "MapLoader.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>

bool loadMap(
    const char* filename,
    TileMap& map,
    MapConfig& config
) {
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr
            << "Nao foi possivel abrir o mapa: "
            << filename
            << std::endl;

        return false;
    }

    std::string command;

    int mapWidth = 0;
    int mapHeight = 0;

    bool foundMapSection = false;

    while (file >> command) {
        // Ignora linhas de comentário iniciadas por #.
        if (!command.empty() && command[0] == '#') {
            file.ignore(
                std::numeric_limits<std::streamsize>::max(),
                '\n'
            );

            continue;
        }

        if (command == "TILESET") {
            file
                >> config.tilesetPath
                >> config.tileCount
                >> config.tileWidth
                >> config.tileHeight;
        }
        else if (command == "LAVA_TEXTURE") {
            file >> config.lavaTexturePath;
        }
        else if (command == "PLAYER") {
            file
                >> config.playerStartCol
                >> config.playerStartRow;
        }
        else if (command == "WALKABLE") {
            int amount = 0;
            file >> amount;

            if (amount < 0) {
                std::cerr
                    << "Quantidade de tiles caminhaveis invalida."
                    << std::endl;

                return false;
            }

            config.walkableTiles.clear();

            for (int i = 0; i < amount; ++i) {
                int tile = 0;

                if (!(file >> tile)) {
                    std::cerr
                        << "Erro lendo tile caminhavel."
                        << std::endl;

                    return false;
                }

                if (tile < 0 || tile > 255) {
                    std::cerr
                        << "Tile caminhavel invalido: "
                        << tile
                        << std::endl;

                    return false;
                }

                config.walkableTiles.push_back(
                    static_cast<unsigned char>(tile)
                );
            }
        }
        else if (command == "MAP_SIZE") {
            file >> mapWidth >> mapHeight;
        }
        else if (command == "MAP") {
            foundMapSection = true;
            break;
        }
        else {
            std::cerr
                << "Comando desconhecido no map.txt: "
                << command
                << std::endl;

            return false;
        }

        if (!file) {
            std::cerr
                << "Erro lendo a configuracao do mapa."
                << std::endl;

            return false;
        }
    }

    if (!foundMapSection) {
        std::cerr
            << "Secao MAP nao encontrada."
            << std::endl;

        return false;
    }

    if (mapWidth <= 0 || mapHeight <= 0) {
        std::cerr
            << "Dimensoes invalidas do mapa: "
            << mapWidth
            << "x"
            << mapHeight
            << std::endl;

        return false;
    }

    TileMap loadedMap(
        mapWidth,
        mapHeight,
        0
    );

    for (int row = 0; row < mapHeight; ++row) {
        for (int col = 0; col < mapWidth; ++col) {
            int tile = 0;

            if (!(file >> tile)) {
                std::cerr
                    << "Faltou tile na posicao ("
                    << col
                    << ", "
                    << row
                    << ")."
                    << std::endl;

                return false;
            }

            if (tile < 0 || tile > 255) {
                std::cerr
                    << "Tile invalido na posicao ("
                    << col
                    << ", "
                    << row
                    << "): "
                    << tile
                    << std::endl;

                return false;
            }

            loadedMap.setTile(
                col,
                row,
                static_cast<unsigned char>(tile)
            );
        }
    }

    if (config.playerStartCol < 0 ||
        config.playerStartCol >= mapWidth ||
        config.playerStartRow < 0 ||
        config.playerStartRow >= mapHeight) {

        std::cerr
            << "Posicao inicial do jogador fora do mapa."
            << std::endl;

        return false;
    }

    if (config.walkableTiles.empty()) {
        std::cerr
            << "Nenhum tile caminhavel configurado."
            << std::endl;

        return false;
    }

    map = loadedMap;

    return true;
}

bool isWalkableTile(
    const MapConfig& config,
    unsigned char tile
) {
    return std::find(
        config.walkableTiles.begin(),
        config.walkableTiles.end(),
        tile
    ) != config.walkableTiles.end();
}