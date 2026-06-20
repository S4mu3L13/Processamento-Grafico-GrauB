#ifndef AV3_GL_UTILS_H
#define AV3_GL_UTILS_H

#include "GameTypes.h"

#include <glad/glad.h>
#include <string>

bool parseFileIntoString(const char* fileName, std::string& shaderSource);
GLuint createShaderProgramFromFiles(const char* vertexFileName, const char* fragmentFileName);
bool loadTexture(TextureInfo& texture, const std::string& filePath);
bool loadImageFromKnownPaths(TextureInfo& texture, const std::string& relativePath);
bool loadSheetFromKnownPaths(SpriteSheet& sheet, const std::string& relativePath, int frameWidth, int frameHeight, bool horizontal);
TextureInfo loadTextureFromKnownPaths();

#endif
