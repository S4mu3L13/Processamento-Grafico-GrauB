#ifndef M6_CONFIG_H
#define M6_CONFIG_H

// Configurações gerais da janela
static const int WINDOW_WIDTH = 1280;
static const int WINDOW_HEIGHT = 720;

// Dimensões lógicas do tilemap
static const int MAP_COLS = 16;
static const int MAP_ROWS = 16;

// Tamanho em pixels usado para desenhar cada tile isométrico
static const float TILE_W = 74.88f;
static const float TILE_H = 37.44f;

// Quantidade de cristais necessária para vencer
static const int CRYSTALS_TO_WIN = 4;

// Organização do tileset de pisos: 3 colunas por 6 linhas
static const int TILESET_COLS = 3;
static const int TILESET_ROWS = 6;

#endif
