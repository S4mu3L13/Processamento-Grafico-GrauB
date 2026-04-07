#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

// Estrutura para armazenar cores em RGB [0.0, 1.0]
struct Color {
	float r;
	float g;
	float b;
};

// ===== CONFIGURAÇÕES DA TELA E GRADE =====
static const int WIDTH = 1280;
static const int HEIGHT = 720;
static const int ROWS = 5;  
static const int COLS = 4;  
static const int TOTAL_CELLS = ROWS * COLS;

// Dimensões de cada retângulo em coordenadas normalizadas (-1 a 1)
static const float HALF_W = 0.23f;  
static const float HALF_H = 0.12f;  
static const float STEP_X = 0.46f;  
static const float STEP_Y = 0.24f;  
static const float START_X = -0.69f;  
static const float START_Y = 0.48f;  

// Tolerância para detectar cores similares 
static const float SIMILARITY_TOLERANCE = 0.17f;

// Paleta com as 20 cores dos retângulos
static std::vector<Color> gPalette = {
	{0.11f, 0.62f, 0.89f}, {0.17f, 0.80f, 0.76f}, {0.95f, 0.09f, 0.03f}, {0.08f, 0.67f, 0.58f},
	{0.08f, 0.47f, 0.69f}, {0.95f, 0.82f, 0.19f}, {0.82f, 0.09f, 0.51f}, {0.00f, 0.50f, 0.00f},
	{0.93f, 0.54f, 0.51f}, {0.48f, 0.91f, 0.27f}, {0.40f, 0.40f, 0.40f}, {0.42f, 0.83f, 0.78f},
	{0.06f, 0.33f, 0.53f}, {0.87f, 0.50f, 0.72f}, {0.99f, 0.38f, 0.30f}, {0.95f, 0.47f, 0.00f},
	{0.93f, 0.24f, 0.61f}, {0.08f, 0.47f, 0.69f}, {0.91f, 0.87f, 0.31f}, {0.33f, 0.66f, 0.86f}
};

// ===== ESTADO DO JOGO (VARIÁVEIS GLOBAIS) =====
static std::vector<bool> gRemoved(TOTAL_CELLS, false);

static int gTotalScore = 0;  // Pontos acumulados
static int gRemainingCells = TOTAL_CELLS;  // Retângulos ainda visíveis
static bool gGameFinished = false;  // Indica se o jogo acabou

// Calcula a distância entre duas cores usando Euclidiana em RGB
static float colorDistance(const Color& a, const Color& b)
{
	// Diferença em cada canal de cor
	const float dr = a.r - b.r;
	const float dg = a.g - b.g;
	const float db = a.b - b.b;
	const float dist = std::sqrt(dr * dr + dg * dg + db * db);
	return dist / std::sqrt(3.0f);
}

// Converte o índice de um retângulo (0-19) para suas coordenadas no centro
static void cellCenterByIndex(int index, float& cx, float& cy)
{
	const int row = index / COLS;  // linha (0-4)
	const int col = index % COLS;  //  coluna (0-3)
	cx = START_X + static_cast<float>(col) * STEP_X;  // X
	cy = START_Y - static_cast<float>(row) * STEP_Y;  // Y
}

static void executeAttempt(int selectedIndex);

// Verifica qual retângulo foi clicado nas coordenadas (ndcX, ndcY)
static int cellAtNDC(float ndcX, float ndcY)
{
	for (int i = 0; i < TOTAL_CELLS; ++i) {
		if (gRemoved[i]) {
			continue;
		}

		float cx = 0.0f;
		float cy = 0.0f;
		cellCenterByIndex(i, cx, cy);  // Pega posição do retângulo

		// Verifica se o clique está dentro do retângulo
		if (ndcX >= cx - HALF_W && ndcX <= cx + HALF_W &&
			ndcY >= cy - HALF_H && ndcY <= cy + HALF_H) {
			return i;  // Retorna índice do retângulo clicado
		}
	}

	return -1;  // Nenhum retângulo foi clicado
}

// Reinicia o jogo quando a tecla R é pressionada
static void resetGame()
{
	std::fill(gRemoved.begin(), gRemoved.end(), false);  // Mostra todos os retângulos novamente
	gTotalScore = 0;  // Zera pontuação
	gRemainingCells = TOTAL_CELLS;  // Restaura 20 retângulos
	gGameFinished = false;  // Jogo continua
	std::cout << "===== JOGO REINICIADO =====\n";
}

// Executa a tentativa quando um retângulo é clicado
static void executeAttempt(int selectedIndex)
{
	// Valida se a tentativa é válida
	if (gGameFinished || selectedIndex < 0 || selectedIndex >= TOTAL_CELLS || gRemoved[selectedIndex]) {
		return;
	}

	const Color selected = gPalette[selectedIndex];  // Cor clicada
	int removedNow = 0;

	// Loop por todos os retângulos
	for (int i = 0; i < TOTAL_CELLS; ++i) {
		if (gRemoved[i]) {
			continue;  // Pula se já foi removido
		}

		// Calcula distância entre a cor clicada e a cor atual
		const float normalizedDist = colorDistance(selected, gPalette[i]);
		
		// Se cores são similares, remove o retângulo
		if (normalizedDist <= SIMILARITY_TOLERANCE) {
			gRemoved[i] = true;
			++removedNow;
		}
	}

	gRemainingCells -= removedNow;  // Atualiza quantidade de retângulos restantes

	const int pointsPerRect = 1;  // 1 ponto por retângulo removido
	const int gained = removedNow * pointsPerRect;
	gTotalScore += gained; 

	// Mostra resultado no console
	std::cout << "Removidos: " << removedNow
		<< " | Ganho: " << gained
		<< " | Total: " << gTotalScore << "\n";

	// Verifica se jogo acabou (todos os retângulos removidos)
	if (gRemainingCells <= 0) {
		gGameFinished = true;  // Marca jogo como finalizado
		std::cout << "\n===== FIM DE JOGO =====\n";
		std::cout << "Pontuacao Final: " << gTotalScore << "\n";
		std::cout << "Pressione R para jogar novamente ou ESC para sair.\n";
		std::cout << "===== FIM DE JOGO =====\n\n";
	}
}

// Callback para o mouse
static void mouseClick(GLFWwindow* window, int button, int action, int)
{
	if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) {
		return;
	}

	// Pega posição do mouse em pixels
	double mx = 0.0;
	double my = 0.0;
	glfwGetCursorPos(window, &mx, &my);

	int fbw = 1;
	int fbh = 1;
	glfwGetFramebufferSize(window, &fbw, &fbh);  // Tamanho da janela

	// Converte píxels para coordenadas normalizadas (-1 a 1)
	const float ndcX = (2.0f * static_cast<float>(mx)) / static_cast<float>(fbw) - 1.0f;
	const float ndcY = 1.0f - (2.0f * static_cast<float>(my)) / static_cast<float>(fbh);

	const int selectedIndex = cellAtNDC(ndcX, ndcY);  // Descobre qual retângulo foi clicado
	executeAttempt(selectedIndex);  // Processa a tentativa
}

// Callback para quando uma tecla é pressionada
static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action != GLFW_PRESS) {
		return;
	}

	if (key == GLFW_KEY_R) {
		resetGame();  // R = Reinicia o jogo
	} else if (key == GLFW_KEY_ESCAPE) {
		glfwSetWindowShouldClose(window, GLFW_TRUE);  // ESC = Sai do jogo
	}
}

// Verifica se o shader compilou sem erros
static bool checkShaderCompile(GLuint shader, const char* label)
{
	GLint success = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		char info[1024];
		glGetShaderInfoLog(shader, 1024, nullptr, info);
		std::cout << "Erro compilando shader (" << label << "): " << info << "\n";
		return false;
	}
	return true;
}

// Verifica se o programa OpenGL foi linkado sem erros
static bool checkProgramLink(GLuint program, const char* label)
{
	GLint success = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		char info[1024];
		glGetProgramInfoLog(program, 1024, nullptr, info);
		std::cout << "Erro linkando programa (" << label << "): " << info << "\n";
		return false;
	}
	return true;
}

// ===== FUNÇÃO PRINCIPAL =====
int main()
{
	// Inicializa GLFW
	if (!glfwInit()) {
		std::cout << "Erro inicializando GLFW\n";
		return -1;
	}

	// Configura OpenGL versão 3.3 Core Profile
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Cria a janela
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Desafio M3", nullptr, nullptr);
	if (!window) {
		std::cout << "Erro criando janela\n";
		glfwTerminate();
		return -1;
	}

	// Ativa o contexto OpenGL da janela
	glfwMakeContextCurrent(window);

	// Carrega as funções OpenGL (usando GLAD)
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Erro inicializando GLAD\n";
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}

	// Define a área de renderização
	glViewport(0, 0, WIDTH, HEIGHT);
	
	// Registra callbacks para mouse e teclado
	glfwSetMouseButtonCallback(window, mouseClick);
	glfwSetKeyCallback(window, keyCallback);

	// Exibe instruções no console
	std::cout << "\n";
	std::cout << "===== DESAFIO M3 - JOGO DE CORES =====\n";
	std::cout << "Objetivo: Clique em um retangulo para remover todos os retangulos com cores similares.\n";
	std::cout << "\nControles:\n";
	std::cout << "  - MOUSE: Clique para selecionar uma cor\n";
	std::cout << "  - R: Reiniciar o jogo\n";
	std::cout << "  - ESC: Sair\n";
	std::cout << "\nVerifique o titulo da janela para ver sua pontuacao em tempo real!\n";
	std::cout << "========================================\n\n";

	// ===== SHADERS =====
	const char* vertexShaderSource =
		"#version 330 core\n"
		"layout (location = 0) in vec2 aPosition;\n"
		"uniform vec2 uOffset;\n"
		"void main() {\n"
		"    vec2 p = aPosition + uOffset;\n"
		"    gl_Position = vec4(p, 0.0, 1.0);\n"
		"}\n";

	// Fragment Shader (define cor dos retângulos)
	const char* fragmentShaderSource =
		"#version 330 core\n"
		"uniform vec3 uColor;\n"
		"out vec4 FragColor;\n"
		"void main() {\n"
		"    FragColor = vec4(uColor, 1.0);\n"
		"}\n";

	// Compila o Vertex Shader
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &vertexShaderSource, nullptr);
	glCompileShader(vs);
	if (!checkShaderCompile(vs, "VS")) {
		glDeleteShader(vs);
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}

	// Compila o Fragment Shader
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &fragmentShaderSource, nullptr);
	glCompileShader(fs);
	if (!checkShaderCompile(fs, "FS")) {
		glDeleteShader(vs);
		glDeleteShader(fs);
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}

	// Linka os shaders em um programa
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vs);
	glAttachShader(shaderProgram, fs);
	glLinkProgram(shaderProgram);

	// Deleta os shaders
	glDeleteShader(vs);
	glDeleteShader(fs);

	if (!checkProgramLink(shaderProgram, "M3Program")) {
		glDeleteProgram(shaderProgram);
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}

	// ===== GEOMETRIA =====
	// Define um retângulo usando dois triângulos
	const float rectangleVertices[] = {
		-HALF_W,  HALF_H,  
		-HALF_W, -HALF_H,  
		 HALF_W, -HALF_H, 

		-HALF_W,  HALF_H,  
		 HALF_W, -HALF_H,  
		 HALF_W,  HALF_H   
	};

	// ===== CONFIGURAÇÃO DO OPENGL (VAO e VBO) =====
	GLuint vao = 0;  // Vertex Array Object
	GLuint vbo = 0;  // Vertex Buffer Object

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);

	glBindVertexArray(vao);

	// Envia vértices para a GPU
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(rectangleVertices), rectangleVertices, GL_STATIC_DRAW);

	// Define como interpretar os dados de posição
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// Localização das variáveis de uniforme no shader
	const GLint offsetLoc = glGetUniformLocation(shaderProgram, "uOffset");  // Posição
	const GLint colorLoc = glGetUniformLocation(shaderProgram, "uColor");    // Cor

	// ===== LOOP PRINCIPAL =====
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();  // Processa eventos (cliques, teclas, etc)

		glClearColor(0.90f, 0.90f, 0.90f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// Atualiza o título da janela com estatísticas em tempo real
		std::string title = "M3 - Restantes: " + std::to_string(gRemainingCells) +
			" | Total: " + std::to_string(gTotalScore);
		if (gGameFinished) {
			title += " | FIM DE JOGO - Pontuacao Final: " + std::to_string(gTotalScore);
		}
		glfwSetWindowTitle(window, title.c_str());

		// Prepara shader e geometria para desenho
		glUseProgram(shaderProgram);
		glBindVertexArray(vao);

		// Desenha cada retângulo ainda visível
		for (int i = 0; i < TOTAL_CELLS; ++i) {
			if (gRemoved[i]) {
				continue;  // Pula retângulos removidos
			}

			// Calcula posição do retângulo
			float dx = 0.0f;
			float dy = 0.0f;
			cellCenterByIndex(i, dx, dy);

			// Define cor e posição via uniformes
			const Color& c = gPalette[i];
			glUniform2f(offsetLoc, dx, dy);  // Passa posição pro shader
			glUniform3f(colorLoc, c.r, c.g, c.b);  // Passa cor pro shader

			glDrawArrays(GL_TRIANGLES, 0, 6);  // Desenha os 6 vértices
		}

		glBindVertexArray(0);
		glfwSwapBuffers(window);  // Mostra a imagem na tela
	}




	// Libera recursos OpenGL
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(shaderProgram);

	// Fecha a janela e encerra GLFW
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
