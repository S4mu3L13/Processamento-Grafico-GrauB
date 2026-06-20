#ifndef AV3_GAME_TYPES_H
#define AV3_GAME_TYPES_H

#include <glad/glad.h>

struct Vec2 {
    float x;
    float y;
};

struct TextureInfo {
    GLuint id;
    int width;
    int height;
};

struct SpriteSheet {
    TextureInfo texture;
    int frameWidth;
    int frameHeight;
    int frameCount;
    int maxFrame;
    bool horizontal;
};

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

#endif
