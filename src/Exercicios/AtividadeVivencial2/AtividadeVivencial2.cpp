// Grupo: Júlia Faccio Zanette e Samuel de Oliveira Pasquali

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <array>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using std::cerr;
using std::cout;
using std::endl;
using std::string;
using std::vector;

static const int WINDOW_W = 1000;
static const int WINDOW_H = 700;

struct TextureInfo {
    GLuint id = 0;
    int width = 0;
    int height = 0;
};

struct ParallaxLayer {
    TextureInfo tex;
    float factorX = 0.0f;
    float factorY = 0.0f;
};

static bool checkShader(GLuint shader, const char* name) {
    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok == GL_TRUE) return true;

    char log[1024];
    glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
    cerr << "Erro ao compilar shader (" << name << "): " << log << endl;
    return false;
}

static bool checkProgram(GLuint program) {
    GLint ok = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (ok == GL_TRUE) return true;

    char log[1024];
    glGetProgramInfoLog(program, sizeof(log), nullptr, log);
    cerr << "Erro ao linkar programa: " << log << endl;
    return false;
}

static TextureInfo loadTexture(const char* path, bool repeat) {
    TextureInfo out;
    int channels = 0;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* pixels = stbi_load(path, &out.width, &out.height, &channels, 4);
    if (!pixels) {
        cerr << "Falha ao carregar textura: " << path << endl;
        return out;
    }

    glGenTextures(1, &out.id);
    glBindTexture(GL_TEXTURE_2D, out.id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, out.width, out.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(pixels);
    return out;
}

static string resolveAssetPath(const string& relFromAssets) {
    const std::array<string, 4> prefixes = {
        "../assets/",
        "assets/",
        "../../assets/",
        "../../../assets/"
    };

    for (const string& prefix : prefixes) {
        string full = prefix + relFromAssets;
        std::ifstream f(full.c_str(), std::ios::binary);
        if (f.good()) return full;
    }
    return string("../assets/") + relFromAssets;
}

int main() {
    if (!glfwInit()) {
        cerr << "Falha ao iniciar GLFW." << endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WINDOW_W, WINDOW_H, "Atividade Vivencial 2 - Parallax", nullptr, nullptr);
    if (!window) {
        cerr << "Falha ao criar janela." << endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cerr << "Falha ao iniciar GLAD." << endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    glViewport(0, 0, WINDOW_W, WINDOW_H);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const char* vertexSrc =
        "#version 330 core\n"
        "layout (location = 0) in vec2 aPos;\n"
        "layout (location = 1) in vec2 aUV;\n"
        "uniform mat4 projection;\n"
        "uniform mat4 model;\n"
        "uniform vec2 uvScale;\n"
        "uniform vec2 uvOffset;\n"
        "out vec2 vUV;\n"
        "void main() {\n"
        "  gl_Position = projection * model * vec4(aPos, 0.0, 1.0);\n"
        "  vUV = aUV * uvScale + uvOffset;\n"
        "}\n";

    const char* fragmentSrc =
        "#version 330 core\n"
        "in vec2 vUV;\n"
        "uniform sampler2D uTex;\n"
        "out vec4 FragColor;\n"
        "void main() {\n"
        "  vec4 c = texture(uTex, vUV);\n"
        "  if (c.a < 0.02) discard;\n"
        "  FragColor = c;\n"
        "}\n";

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexSrc, nullptr);
    glCompileShader(vs);
    if (!checkShader(vs, "vertex")) return -1;

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentSrc, nullptr);
    glCompileShader(fs);
    if (!checkShader(fs, "fragment")) return -1;

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    if (!checkProgram(program)) return -1;
    glDeleteShader(vs);
    glDeleteShader(fs);

    float quad[] = {
        0.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f
    };
    unsigned int idx[] = {0, 1, 2, 2, 3, 0};

    GLuint vao = 0, vbo = 0, ebo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    string baseTex = resolveAssetPath("tex/craftpix-665532-free-fairy-tale-game-backgrounds/_PNG/3/");
    if (!baseTex.empty() && baseTex.back() != '/' && baseTex.back() != '\\') {
        baseTex += "/";
    }
    vector<ParallaxLayer> layers;
    layers.reserve(7);
    const std::array<int, 7> drawOrder = {7, 6, 5, 4, 3, 2, 1}; // 7 = mais distante, 1 = mais proxima
    const std::array<float, 7> factorsX = {0.04f, 0.08f, 0.14f, 0.22f, 0.32f, 0.46f, 0.64f};
    const std::array<float, 7> factorsY = {0.02f, 0.04f, 0.06f, 0.09f, 0.12f, 0.16f, 0.22f};
    for (size_t i = 0; i < drawOrder.size(); i++) {
        string path = baseTex + std::to_string(drawOrder[i]) + ".png";
        ParallaxLayer layer;
        layer.tex = loadTexture(path.c_str(), true);
        layer.factorX = factorsX[i];
        layer.factorY = factorsY[i];
        if (layer.tex.id != 0) {
            layers.push_back(layer);
        }
    }

    TextureInfo playerTex = loadTexture(
        resolveAssetPath("sprites/craftpix-net-529677-free-wizard-sprite-sheets-pixel-art/Lightning Mage/Run.png").c_str(),
        false
    );

    if (layers.empty() || playerTex.id == 0) {
        cerr << "Texturas obrigatorias nao foram carregadas. Encerrando." << endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // A spritesheet tem uma unica linha de frames quadrados
    int frameWidthPx = playerTex.height;
    int frameCount = (frameWidthPx > 0) ? (playerTex.width / frameWidthPx) : 1;
    if (frameCount < 1) frameCount = 1;

    glm::vec2 playerPos(400.0f, 120.0f);
    glm::vec2 playerStart = playerPos;
    glm::vec2 playerSize(110.0f, 110.0f);
    float playerSpeed = 260.0f;
    bool facingRight = true;
    int frame = 0;
    float frameTimer = 0.0f;
    const float frameDelay = 0.08f;

    glm::mat4 projection = glm::ortho(0.0f, (float)WINDOW_W, 0.0f, (float)WINDOW_H, -1.0f, 1.0f);

    GLint locProj = glGetUniformLocation(program, "projection");
    GLint locModel = glGetUniformLocation(program, "model");
    GLint locUvScale = glGetUniformLocation(program, "uvScale");
    GLint locUvOffset = glGetUniformLocation(program, "uvOffset");
    GLint locTex = glGetUniformLocation(program, "uTex");

    float prevTime = (float)glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        float now = (float)glfwGetTime();
        float dt = now - prevTime;
        prevTime = now;

        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        glm::vec2 velocity(0.0f, 0.0f);
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) velocity.x = -playerSpeed;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) velocity.x = playerSpeed;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) velocity.y = -playerSpeed;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) velocity.y = playerSpeed;

        if (velocity.x < 0.0f) facingRight = false;
        else if (velocity.x > 0.0f) facingRight = true;

        bool moving = (velocity.x != 0.0f || velocity.y != 0.0f);
        if (moving) {
            playerPos += velocity * dt;
            frameTimer += dt;
            if (frameTimer >= frameDelay) {
                frame = (frame + 1) % frameCount;
                frameTimer = 0.0f;
            }
        } else {
            frame = 1 % frameCount;
            frameTimer = 0.0f;
        }

        playerPos.x = glm::clamp(playerPos.x, 40.0f, WINDOW_W - 40.0f);
        playerPos.y = glm::clamp(playerPos.y, 60.0f, WINDOW_H - 60.0f);

        glm::vec2 delta = playerPos - playerStart;

        glClearColor(0.08f, 0.10f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program);
        glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform1i(locTex, 0);
        glBindVertexArray(vao);

        for (const ParallaxLayer& layer : layers) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, layer.tex.id);

            glm::mat4 model(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
            model = glm::scale(model, glm::vec3((float)WINDOW_W, (float)WINDOW_H, 1.0f));

            glm::vec2 uvOffset(
                (-delta.x * layer.factorX) / (float)layer.tex.width,
                (-delta.y * layer.factorY) / (float)layer.tex.height
            );

            glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
            glUniform2f(locUvScale, 1.0f, 1.0f);
            glUniform2f(locUvOffset, uvOffset.x, uvOffset.y);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, playerTex.id);
        glm::mat4 playerModel(1.0f);
        float drawX = playerPos.x - playerSize.x * 0.5f;
        float drawY = playerPos.y - playerSize.y * 0.5f;
        float sx = playerSize.x;
        if (!facingRight) {
            drawX += playerSize.x;
            sx = -playerSize.x;
        }
        playerModel = glm::translate(playerModel, glm::vec3(drawX, drawY, 0.0f));
        playerModel = glm::scale(playerModel, glm::vec3(sx, playerSize.y, 1.0f));

        float frameW = 1.0f / (float)frameCount;
        glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(playerModel));
        glUniform2f(locUvScale, frameW, 1.0f);
        glUniform2f(locUvOffset, frameW * frame, 0.0f);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
    }

    for (const ParallaxLayer& layer : layers) {
        if (layer.tex.id != 0) glDeleteTextures(1, &layer.tex.id);
    }
    if (playerTex.id != 0) glDeleteTextures(1, &playerTex.id);
    glDeleteProgram(program);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteVertexArrays(1, &vao);




    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
