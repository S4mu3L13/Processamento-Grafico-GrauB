#ifndef M6_GL_UTILS_H
#define M6_GL_UTILS_H

#include "GameTypes.h"

#include <glad/glad.h>
#include <string>

// Lê um arquivo de texto, usado principalmente para carregar shaders
bool parseFileIntoString(const char* fileName, std::string& shaderSource);

// Compila vertex/fragment shaders externos e cria o programa OpenGL
GLuint createShaderProgramFromFiles(const char* vertexFileName, const char* fragmentFileName);

// Carrega uma textura RGBA, aplicando transparência para fundo magenta
bool loadTexture(TextureInfo& texture, const std::string& filePath);

// Busca e carrega uma imagem a partir dos caminhos comuns do projeto, usado para o tileset e spritesheets
bool loadImageFromKnownPaths(TextureInfo& texture, const std::string& relativePath);

// Carrega uma spritesheet e calcula sua quantidade de frames, usado para os sprites animados do jogo
bool loadSheetFromKnownPaths(SpriteSheet& sheet, const std::string& relativePath, int frameWidth, int frameHeight, bool horizontal);

// Carrega o tileset de pisos usado pelo tilemap
TextureInfo loadTextureFromKnownPaths();

#endif
