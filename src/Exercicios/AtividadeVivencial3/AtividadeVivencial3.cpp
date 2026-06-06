#include "DiamondView.h"
#include "TileMap.h"
#include "gl_utils.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace {

constexpr int kWindowWidth = 1000;
constexpr int kWindowHeight = 800;
constexpr int kMapCols = 12;
constexpr int kMapRows = 12;
constexpr float kTileWidth = 74.88f;
constexpr float kTileHeight = 37.44f;
constexpr int kTilesetCols = 3;
constexpr int kTilesetRows = 6;
constexpr float kMoveCooldownSeconds = 0.18f;

static DiamondView g_tileView;

static void framebufferSizeCallback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

static TileMap createInitialMap() {
    const int initialMap[kMapRows][kMapCols] = {
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }
    };

    TileMap map(kMapCols, kMapRows, 0);
    for (int row = 0; row < kMapRows; ++row) {
        for (int col = 0; col < kMapCols; ++col) {
            map.setTile(col, row, static_cast<unsigned char>(initialMap[row][col]));
        }
    }

    return map;
}

static Vec2 tileToScreen(int col, int row) {
    float x = 0.0f;
    float y = 0.0f;
    const float originX = (static_cast<float>(kWindowWidth) * 0.5f)
        - ((static_cast<float>(kMapCols - kMapRows) * kTileWidth) * 0.25f);
    const float originY = (static_cast<float>(kWindowHeight) * 0.5f)
        - ((static_cast<float>(kMapCols + kMapRows - 2) * kTileHeight) * 0.25f);
    g_tileView.computeDrawPosition(
        static_cast<float>(col),
        static_cast<float>(row),
        kTileWidth,
        kTileHeight,
        originX,
        originY,
        x,
        y
    );
    return { x, y };
}

static void setTileUv(std::vector<float>& vertices, int tileIndex, const TextureInfo& texture) {
    const int tilesetCol = tileIndex % kTilesetCols;
    const int tilesetRow = tileIndex / kTilesetCols;
    const float cellW = static_cast<float>(texture.width) / static_cast<float>(kTilesetCols);
    const float cellH = static_cast<float>(texture.height) / static_cast<float>(kTilesetRows);
    const float halfTexelU = 0.5f / static_cast<float>(texture.width);
    const float halfTexelV = 0.5f / static_cast<float>(texture.height);

    const float u0 = (static_cast<float>(tilesetCol) * cellW) / static_cast<float>(texture.width) + halfTexelU;
    const float v0 = (static_cast<float>(tilesetRow) * cellH) / static_cast<float>(texture.height) + halfTexelV;
    const float u1 = (static_cast<float>(tilesetCol + 1) * cellW) / static_cast<float>(texture.width) - halfTexelU;
    const float v1 = (static_cast<float>(tilesetRow + 1) * cellH) / static_cast<float>(texture.height) - halfTexelV;

    vertices = {
        -kTileWidth * 0.5f, -kTileHeight * 0.5f, u0, v0,
         kTileWidth * 0.5f, -kTileHeight * 0.5f, u1, v0,
         kTileWidth * 0.5f,  kTileHeight * 0.5f, u1, v1,
        -kTileWidth * 0.5f,  kTileHeight * 0.5f, u0, v1
    };
}

static void setSpriteFrameVertices(std::vector<float>& vertices, const Actor& actor) {
    const SpriteSheet& sheet = *actor.sheet;
    const int lastFrame = std::min(sheet.maxFrame, sheet.frameCount - 1);
    const int frame = std::min(actor.frame, lastFrame);
    const int frameCol = sheet.horizontal ? frame : 0;
    const int frameRow = sheet.horizontal ? 0 : frame;

    const float halfTexelU = 0.5f / static_cast<float>(sheet.texture.width);
    const float halfTexelV = 0.5f / static_cast<float>(sheet.texture.height);
    float u0 = (frameCol * sheet.frameWidth) / static_cast<float>(sheet.texture.width) + halfTexelU;
    float v0 = (frameRow * sheet.frameHeight) / static_cast<float>(sheet.texture.height) + halfTexelV;
    float u1 = ((frameCol + 1) * sheet.frameWidth) / static_cast<float>(sheet.texture.width) - halfTexelU;
    float v1 = ((frameRow + 1) * sheet.frameHeight) / static_cast<float>(sheet.texture.height) - halfTexelV;

    if (actor.flipX) {
        std::swap(u0, u1);
    }

    const float left = -actor.anchorX;
    const float top = -actor.anchorY;
    const float right = actor.drawWidth - actor.anchorX;
    const float bottom = actor.drawHeight - actor.anchorY;

    vertices = {
        left,  top,    u0, v0,
        right, top,    u1, v0,
        right, bottom, u1, v1,
        left,  bottom, u0, v1
    };
}

static void updateActorAnimation(Actor& actor, float dt) {
    actor.animTimer += dt;
    while (actor.animTimer >= actor.frameTime) {
        actor.animTimer -= actor.frameTime;
        actor.frame = (actor.frame + 1) % std::max(1, actor.sheet->frameCount);
    }
}

static void drawActor(
    const Actor& actor,
    GLuint vbo,
    GLint offsetLoc,
    GLint alphaLoc,
    GLint useTextureLoc,
    std::vector<float>& vertices
) {
    const Vec2 screen = tileToScreen(static_cast<int>(std::round(actor.col)), static_cast<int>(std::round(actor.row)));
    setSpriteFrameVertices(vertices, actor);
    glBindTexture(GL_TEXTURE_2D, actor.sheet->texture.id);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * vertices.size(), vertices.data());
    glUniform2f(offsetLoc, screen.x, screen.y);
    glUniform1f(alphaLoc, 1.0f);
    glUniform1i(useTextureLoc, 1);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}

static void paintTile(TileMap& map, int col, int row, unsigned char tileValue) {
    map.setTile(col, row, tileValue);
}

static void updateWindowTitle(GLFWwindow* window, int col, int row, unsigned char tileIndex, unsigned char selectedTile) {
    std::ostringstream title;
    title << "Atividade Vivencial 3 - Diamond | Pos (" << col << ", " << row << ") | Tile "
          << static_cast<int>(tileIndex)
          << " | Pintura "
          << static_cast<int>(selectedTile)
          << " | 0-7 + Arrows/WASD + Q/E/Z/C";
    glfwSetWindowTitle(window, title.str().c_str());
}

static bool tryMove(TileMap& map, Actor& witch, unsigned char selectedTile, int deltaCol, int deltaRow, GLFWwindow* window) {
    const int currentCol = static_cast<int>(std::round(witch.col));
    const int currentRow = static_cast<int>(std::round(witch.row));
    const int nextCol = currentCol + deltaCol;
    const int nextRow = currentRow + deltaRow;

    if (nextCol < 0 || nextCol >= kMapCols || nextRow < 0 || nextRow >= kMapRows) {
        return false;
    }

    witch.col = static_cast<float>(nextCol);
    witch.row = static_cast<float>(nextRow);
    paintTile(map, nextCol, nextRow, selectedTile);
    updateWindowTitle(window, nextCol, nextRow, map.getTile(nextCol, nextRow), selectedTile);
    return true;
}

static void drawMap(
    const TileMap& map,
    const TextureInfo& texture,
    GLuint vbo,
    GLint offsetLoc,
    GLint alphaLoc,
    GLint useTextureLoc,
    std::vector<float>& tileVertices
) {
    glBindTexture(GL_TEXTURE_2D, texture.id);
    glUniform1f(alphaLoc, 1.0f);
    glUniform1i(useTextureLoc, 1);

    for (int sum = 0; sum <= (kMapCols - 1) + (kMapRows - 1); ++sum) {
        for (int row = 0; row < kMapRows; ++row) {
            const int col = sum - row;
            if (col < 0 || col >= kMapCols) {
                continue;
            }

            setTileUv(tileVertices, map.getTile(col, row), texture);
            const Vec2 screen = tileToScreen(col, row);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * tileVertices.size(), tileVertices.data());
            glUniform2f(offsetLoc, screen.x, screen.y);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        }
    }
}

static bool handleMovement(GLFWwindow* window, TileMap& map, Actor& witch, unsigned char selectedTile) {
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        return tryMove(map, witch, selectedTile, 0, -1, window);
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        return tryMove(map, witch, selectedTile, 0, 1, window);
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        return tryMove(map, witch, selectedTile, 1, 0, window);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        return tryMove(map, witch, selectedTile, -1, 0, window);
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        return tryMove(map, witch, selectedTile, 1, -1, window);
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        return tryMove(map, witch, selectedTile, -1, -1, window);
    }
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        return tryMove(map, witch, selectedTile, 1, 1, window);
    }
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
        return tryMove(map, witch, selectedTile, -1, 1, window);
    }

    return false;
}

static bool handlePaintSelection(GLFWwindow* window, unsigned char& selectedTile) {
    for (int value = 0; value <= 7; ++value) {
        const int key = GLFW_KEY_0 + value;
        if (glfwGetKey(window, key) == GLFW_PRESS) {
            selectedTile = static_cast<unsigned char>(value);
            return true;
        }
    }

    return false;
}

} // namespace

int main() {
    if (!glfwInit()) {
        std::cerr << "Nao foi possivel iniciar GLFW." << std::endl;
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        kWindowWidth,
        kWindowHeight,
        "Atividade Vivencial 3 - Diamond",
        nullptr,
        nullptr
    );
    if (!window) {
        std::cerr << "Nao foi possivel criar a janela." << std::endl;
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSwapInterval(1);

    if (!gladLoadGL()) {
        std::cerr << "Nao foi possivel carregar OpenGL com GLAD." << std::endl;
        glfwTerminate();
        return 1;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(0, 0, kWindowWidth, kWindowHeight);

    TextureInfo tileset = loadTextureFromKnownPaths();
    if (tileset.id == 0) {
        glfwTerminate();
        return 1;
    }

    SpriteSheet witchRun;
    if (!loadSheetFromKnownPaths(witchRun, "assets/sprites/Blue Witch/Blue_witch/B_witch_run.png", 32, 48, false)) {
        glfwTerminate();
        return 1;
    }

    GLuint shaderProgram = createShaderProgramFromFiles("_geral_vs.glsl", "_geral_fs.glsl");
    if (shaderProgram == 0) {
        glfwTerminate();
        return 1;
    }

    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "uTexture"), 0);

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    const unsigned int indices[] = { 0, 1, 2, 0, 2, 3 };
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 16, nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    TileMap map = createInitialMap();
    std::vector<float> tileVertices;
    std::vector<float> spriteVertices;
    const GLint screenSizeLoc = glGetUniformLocation(shaderProgram, "uScreenSize");
    const GLint offsetLoc = glGetUniformLocation(shaderProgram, "uOffset");
    const GLint alphaLoc = glGetUniformLocation(shaderProgram, "uAlpha");
    const GLint useTextureLoc = glGetUniformLocation(shaderProgram, "uUseTexture");

    Actor witch = {
        0.0f, 0.0f,
        &witchRun,
        62.4f, 78.0f,
        31.2f, 71.5f,
        0.13f, 0.0f, 0,
        false
    };

    unsigned char selectedTile = 1;
    paintTile(map, 0, 0, selectedTile);
    updateWindowTitle(window, 0, 0, map.getTile(0, 0), selectedTile);
    std::cout << "Controls: 0-7 selects paint tile; arrows/WASD + Q/E/Z/C move. ESC quits." << std::endl;

    float moveCooldown = 0.0f;
    double previousTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        const double currentTime = glfwGetTime();
        const float dt = static_cast<float>(currentTime - previousTime);
        previousTime = currentTime;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        handlePaintSelection(window, selectedTile);

        moveCooldown -= dt;
        if (moveCooldown <= 0.0f && handleMovement(window, map, witch, selectedTile)) {
            moveCooldown = kMoveCooldownSeconds;
            updateWindowTitle(window, static_cast<int>(std::round(witch.col)), static_cast<int>(std::round(witch.row)), map.getTile(static_cast<int>(std::round(witch.col)), static_cast<int>(std::round(witch.row))), selectedTile);
        }

        int screenW = 0;
        int screenH = 0;
        glfwGetFramebufferSize(window, &screenW, &screenH);

        glClearColor(0.11f, 0.16f, 0.19f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glUniform2f(screenSizeLoc, static_cast<float>(screenW), static_cast<float>(screenH));
        glUniform1i(useTextureLoc, 1);
        glBindVertexArray(vao);
        drawMap(map, tileset, vbo, offsetLoc, alphaLoc, useTextureLoc, tileVertices);
        updateActorAnimation(witch, dt);
        drawActor(witch, vbo, offsetLoc, alphaLoc, useTextureLoc, spriteVertices);

        glfwSwapBuffers(window);
    }

    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shaderProgram);
    glDeleteTextures(1, &tileset.id);

    glfwTerminate();
    return 0;
}
