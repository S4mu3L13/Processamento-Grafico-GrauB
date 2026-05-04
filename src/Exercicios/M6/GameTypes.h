#ifndef M6_GAME_TYPES_H
#define M6_GAME_TYPES_H

#include <glad/glad.h>

// Estrutura de vetor 2D
struct Vec2 {
    float x;
    float y;
};

// Guarda o identificador OpenGL e as dimensões de uma textura
struct TextureInfo {
    GLuint id;
    int width;
    int height;
};

// Descreve uma spritesheet e como seus frames estão organizados
struct SpriteSheet {
    TextureInfo texture;
    int frameWidth;
    int frameHeight;
    int frameCount;
    int maxFrame;
    bool horizontal;
};

// Entidade desenhável no tilemap
struct Actor {
    float col;
    float row;
    SpriteSheet* sheet;
    float drawWidth;
    float drawHeight;
    float anchorX;
    float anchorY;
    float frameTime;
    float animTimer;
    int frame;
    bool flipX;
};

// Obstáculo móvel com caminho linear entre dois pontos do mapa
struct Mushroom {
    Actor actor;
    float startCol;
    float startRow;
    float endCol;
    float endRow;
    float progress;
    float direction;
    float speed;
};

// Coletável posicionado no mapa
struct Crystal {
    float col;
    float row;
    bool collected;
};

// Estado local da bruxa
enum WitchState {
    WITCH_ALIVE,
    WITCH_DYING
};

// Estado geral da partida
enum GameResult {
    GAME_PLAYING,
    GAME_WON,
    GAME_LOST
};

#endif
