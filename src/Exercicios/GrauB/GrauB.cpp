#include "DiamondView.h"
#include "GameTypes.h"
#include "TileMap.h"
#include "gl_utils.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace {

constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 800;
constexpr int kCrystalsToWin = 4;
constexpr float kMoveCooldownSeconds = 0.16f;
constexpr float kSceneScale = 0.80f;
constexpr float kTreeVisualScale = 1.20f;

static DiamondView g_tileView;
static int gMapCols = 15;
static int gMapRows = 15;
static float gTileW = 74.88f;
static float gTileH = 37.44f;
static int gScreenW = kWindowWidth;
static int gScreenH = kWindowHeight;

struct GridPos {
    int col;
    int row;
    int type = 1;
};

struct MushroomConfig {
    float startCol;
    float startRow;
    float endCol;
    float endRow;
    float speed;
    float progress;
    float direction;
};

struct GameConfig {
    std::string tilesetPath;
    int tilesetCols = 3;
    int tilesetRows = 6;
    std::string visitedTilesetPath;
    int visitedTilesetCols = 3;
    int visitedTilesetRows = 6;
    int visitedTileIndex = 1;
    TileMap map = TileMap(1, 1, 0);
    std::vector<unsigned char> walkable;
    GridPos witchStart = { 0, 0 };
    std::vector<GridPos> trees;
    std::vector<GridPos> crystals;
    std::vector<MushroomConfig> mushrooms;
};

static int indexOf(int col, int row) {
    return col + row * gMapCols;
}

static bool inBounds(int col, int row) {
    return col >= 0 && col < gMapCols && row >= 0 && row < gMapRows;
}

static std::string trim(const std::string& text) {
    const std::size_t first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    const std::size_t last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

static bool readConfigLine(std::istream& input, std::string& line) {
    while (std::getline(input, line)) {
        line = trim(line);
        if (!line.empty() && line[0] != '#') {
            return true;
        }
    }
    return false;
}

static bool readMatrix(std::istream& input, int width, int height, std::vector<int>& values) {
    values.clear();
    std::string line;

    for (int row = 0; row < height; ++row) {
        if (!readConfigLine(input, line)) {
            return false;
        }

        std::istringstream rowStream(line);
        for (int col = 0; col < width; ++col) {
            int value = 0;
            if (!(rowStream >> value)) {
                return false;
            }
            values.push_back(value);
        }
    }

    return static_cast<int>(values.size()) == width * height;
}

static bool hasTree(const GameConfig& config, int col, int row) {
    return std::any_of(config.trees.begin(), config.trees.end(), [col, row](const GridPos& tree) {
        return tree.col == col && tree.row == row;
    });
}

static bool isTileWalkable(const GameConfig& config, int col, int row) {
    return inBounds(col, row) &&
        config.walkable[indexOf(col, row)] != 0 &&
        !hasTree(config, col, row);
}

static bool loadGameConfig(GameConfig& config, const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file) {
        return false;
    }

    std::string line;
    while (readConfigLine(file, line)) {
        std::istringstream stream(line);
        std::string keyword;
        stream >> keyword;

        if (keyword == "TILESET") {
            stream >> std::quoted(config.tilesetPath) >> config.tilesetCols >> config.tilesetRows >> gTileW >> gTileH;
        } else if (keyword == "VISITED_TILESET") {
            stream >> std::quoted(config.visitedTilesetPath) >> config.visitedTilesetCols >> config.visitedTilesetRows >> config.visitedTileIndex;
        } else if (keyword == "SIZE") {
            stream >> gMapCols >> gMapRows;
            config.map = TileMap(gMapCols, gMapRows, 0);
            config.walkable.assign(gMapCols * gMapRows, 1);
        } else if (keyword == "MAP") {
            std::vector<int> values;
            if (!readMatrix(file, gMapCols, gMapRows, values)) {
                std::cerr << "Erro lendo matriz MAP em " << filePath << std::endl;
                return false;
            }
            for (int row = 0; row < gMapRows; ++row) {
                for (int col = 0; col < gMapCols; ++col) {
                    config.map.setTile(col, row, static_cast<unsigned char>(values[indexOf(col, row)]));
                }
            }
        } else if (keyword == "WALKABLE") {
            std::vector<int> values;
            if (!readMatrix(file, gMapCols, gMapRows, values)) {
                std::cerr << "Erro lendo matriz WALKABLE em " << filePath << std::endl;
                return false;
            }
            config.walkable.resize(gMapCols * gMapRows);
            for (std::size_t i = 0; i < values.size(); ++i) {
                config.walkable[i] = values[i] == 0 ? 0 : 1;
            }
        } else if (keyword == "WITCH") {
            stream >> config.witchStart.col >> config.witchStart.row;
        } else if (keyword == "TREES") {
            int count = 0;
            stream >> count;
            config.trees.clear();
            for (int i = 0; i < count; ++i) {
                if (!readConfigLine(file, line)) {
                    return false;
                }
                std::istringstream item(line);
                GridPos tree = { 0, 0 };
                item >> tree.col >> tree.row;
                if (!(item >> tree.type)) {
                    tree.type = 1;
                }
                tree.type = std::max(1, std::min(5, tree.type));
                if (inBounds(tree.col, tree.row)) {
                    config.trees.push_back(tree);
                }
            }
        } else if (keyword == "CRYSTALS") {
            int count = 0;
            stream >> count;
            config.crystals.clear();
            for (int i = 0; i < count; ++i) {
                if (!readConfigLine(file, line)) {
                    return false;
                }
                std::istringstream item(line);
                GridPos crystal = { 0, 0 };
                item >> crystal.col >> crystal.row;
                if (isTileWalkable(config, crystal.col, crystal.row)) {
                    config.crystals.push_back(crystal);
                } else {
                    std::cerr << "Cristal ignorado em tile bloqueado: "
                              << crystal.col << ", " << crystal.row << std::endl;
                }
            }
        } else if (keyword == "MUSHROOMS") {
            int count = 0;
            stream >> count;
            config.mushrooms.clear();
            for (int i = 0; i < count; ++i) {
                if (!readConfigLine(file, line)) {
                    return false;
                }
                std::istringstream item(line);
                MushroomConfig mushroom = {};
                item >> mushroom.startCol >> mushroom.startRow >> mushroom.endCol >> mushroom.endRow
                     >> mushroom.speed >> mushroom.progress >> mushroom.direction;
                config.mushrooms.push_back(mushroom);
            }
        }
    }

    if (gMapCols < 15 || gMapRows < 15) {
        std::cerr << "O mapa precisa ter pelo menos 15x15 tiles." << std::endl;
        return false;
    }

    if (config.crystals.empty()) {
        std::cerr << "Nenhum cristal valido foi configurado." << std::endl;
        return false;
    }

    for (const GridPos& tree : config.trees) {
        config.walkable[indexOf(tree.col, tree.row)] = 0;
    }

    return isTileWalkable(config, config.witchStart.col, config.witchStart.row);
}

static bool loadConfigFromKnownPaths(GameConfig& config) {
    const std::array<std::string, 4> paths = {
        "src/Exercicios/GrauB/map.txt",
        "../src/Exercicios/GrauB/map.txt",
        "../../src/Exercicios/GrauB/map.txt",
        "map.txt"
    };

    for (const std::string& path : paths) {
        if (loadGameConfig(config, path)) {
            std::cout << "Configuracao carregada: " << path << std::endl;
            return true;
        }
    }

    std::cerr << "Nao foi possivel carregar map.txt do GrauB." << std::endl;
    return false;
}

static void framebufferSizeCallback(GLFWwindow*, int width, int height) {
    gScreenW = width;
    gScreenH = height;
    glViewport(0, 0, width, height);
}

static Vec2 tileToScreen(float col, float row) {
    float x = 0.0f;
    float y = 0.0f;
    const float scaledTileW = gTileW * kSceneScale;
    const float scaledTileH = gTileH * kSceneScale;
    const float originX = (static_cast<float>(gScreenW) * 0.5f)
        - ((static_cast<float>(gMapCols - gMapRows) * scaledTileW) * 0.25f);
    const float diamondCenterYOffset = (static_cast<float>(gMapCols + gMapRows - 2) * scaledTileH) * 0.25f;
    const float originY = (static_cast<float>(gScreenH) * 0.5f) - diamondCenterYOffset;
    g_tileView.computeDrawPosition(col, row, scaledTileW, scaledTileH, originX, originY, x, y);
    return { x, y };
}

static Vec2 tileToScreen(int col, int row) {
    return tileToScreen(static_cast<float>(col), static_cast<float>(row));
}

static void setTileUv(std::vector<float>& vertices, int tileIndex, const TextureInfo& texture, int tilesetCols, int tilesetRows) {
    const int safeTilesetCols = std::max(1, tilesetCols);
    const int safeTilesetRows = std::max(1, tilesetRows);
    const int tilesetCol = tileIndex % safeTilesetCols;
    const int tilesetRow = std::min(tileIndex / safeTilesetCols, safeTilesetRows - 1);

    const float cellW = static_cast<float>(texture.width) / static_cast<float>(safeTilesetCols);
    const float cellH = static_cast<float>(texture.height) / static_cast<float>(safeTilesetRows);
    const float halfTexelU = 0.5f / static_cast<float>(texture.width);
    const float halfTexelV = 0.5f / static_cast<float>(texture.height);

    const float u0 = (static_cast<float>(tilesetCol) * cellW) / static_cast<float>(texture.width) + halfTexelU;
    const float v0 = (static_cast<float>(tilesetRow) * cellH) / static_cast<float>(texture.height) + halfTexelV;
    const float u1 = (static_cast<float>(tilesetCol + 1) * cellW) / static_cast<float>(texture.width) - halfTexelU;
    const float v1 = (static_cast<float>(tilesetRow + 1) * cellH) / static_cast<float>(texture.height) - halfTexelV;

    const float scaledTileW = gTileW * kSceneScale;
    const float scaledTileH = gTileH * kSceneScale;

    vertices = {
        -scaledTileW * 0.5f, -scaledTileH * 0.5f, u0, v0,
         scaledTileW * 0.5f, -scaledTileH * 0.5f, u1, v0,
         scaledTileW * 0.5f,  scaledTileH * 0.5f, u1, v1,
        -scaledTileW * 0.5f,  scaledTileH * 0.5f, u0, v1
    };
}

static void setSpriteFrameVertices(std::vector<float>& vertices, const Actor& actor) {
    const SpriteSheet& sheet = *actor.sheet;
    const int lastFrame = std::min(sheet.maxFrame, sheet.frameCount - 1);
    const int frame = std::min(actor.frame, lastFrame);
    const int cols = std::max(1, sheet.frameCols);
    const int frameCol = sheet.horizontal ? frame % cols : 0;
    const int frameRow = sheet.horizontal ? frame / cols : frame;

    const float halfTexelU = 0.5f / static_cast<float>(sheet.texture.width);
    const float halfTexelV = 0.5f / static_cast<float>(sheet.texture.height);
    float u0 = 0.0f;
    float v0 = 0.0f;
    float u1 = 1.0f;
    float v1 = 1.0f;

    if (sheet.frameCols > 1 && sheet.frameRows > 1) {
        const int x0 = static_cast<int>(std::round((static_cast<float>(frameCol) * sheet.texture.width) / sheet.frameCols));
        const int y0 = static_cast<int>(std::round((static_cast<float>(frameRow) * sheet.texture.height) / sheet.frameRows));
        const int x1 = static_cast<int>(std::round((static_cast<float>(frameCol + 1) * sheet.texture.width) / sheet.frameCols));
        const int y1 = static_cast<int>(std::round((static_cast<float>(frameRow + 1) * sheet.texture.height) / sheet.frameRows));

        u0 = x0 / static_cast<float>(sheet.texture.width) + halfTexelU;
        v0 = y0 / static_cast<float>(sheet.texture.height) + halfTexelV;
        u1 = x1 / static_cast<float>(sheet.texture.width) - halfTexelU;
        v1 = y1 / static_cast<float>(sheet.texture.height) - halfTexelV;
    } else {
        u0 = (frameCol * sheet.frameWidth) / static_cast<float>(sheet.texture.width) + halfTexelU;
        v0 = (frameRow * sheet.frameHeight) / static_cast<float>(sheet.texture.height) + halfTexelV;
        u1 = ((frameCol + 1) * sheet.frameWidth) / static_cast<float>(sheet.texture.width) - halfTexelU;
        v1 = ((frameRow + 1) * sheet.frameHeight) / static_cast<float>(sheet.texture.height) - halfTexelV;
    }

    if (actor.flipX) {
        std::swap(u0, u1);
    }

    const float left = -actor.anchorX * kSceneScale;
    const float top = -actor.anchorY * kSceneScale;
    const float right = (actor.drawWidth - actor.anchorX) * kSceneScale;
    const float bottom = (actor.drawHeight - actor.anchorY) * kSceneScale;

    vertices = {
        left,  top,    u0, v0,
        right, top,    u1, v0,
        right, bottom, u1, v1,
        left,  bottom, u0, v1
    };
}

static void setGridCellVertices(
    std::vector<float>& vertices,
    const Actor& actor,
    int gridCol,
    int gridRow,
    int totalCols,
    int totalRows
) {
    const SpriteSheet& sheet = *actor.sheet;
    const float halfTexelU = 0.5f / static_cast<float>(sheet.texture.width);
    const float halfTexelV = 0.5f / static_cast<float>(sheet.texture.height);

    const int x0 = static_cast<int>(std::round((static_cast<float>(gridCol) * sheet.texture.width) / totalCols));
    const int y0 = static_cast<int>(std::round((static_cast<float>(gridRow) * sheet.texture.height) / totalRows));
    const int x1 = static_cast<int>(std::round((static_cast<float>(gridCol + 1) * sheet.texture.width) / totalCols));
    const int y1 = static_cast<int>(std::round((static_cast<float>(gridRow + 1) * sheet.texture.height) / totalRows));

    float u0 = x0 / static_cast<float>(sheet.texture.width) + halfTexelU;
    float v0 = y0 / static_cast<float>(sheet.texture.height) + halfTexelV;
    float u1 = x1 / static_cast<float>(sheet.texture.width) - halfTexelU;
    float v1 = y1 / static_cast<float>(sheet.texture.height) - halfTexelV;

    if (actor.flipX) {
        std::swap(u0, u1);
    }

    const float left = -actor.anchorX * kSceneScale;
    const float top = -actor.anchorY * kSceneScale;
    const float right = (actor.drawWidth - actor.anchorX) * kSceneScale;
    const float bottom = (actor.drawHeight - actor.anchorY) * kSceneScale;

    vertices = {
        left,  top,    u0, v0,
        right, top,    u1, v0,
        right, bottom, u1, v1,
        left,  bottom, u0, v1
    };
}

static void setSolidQuadVertices(std::vector<float>& vertices, float width, float height) {
    vertices = {
        0.0f,  0.0f,   0.0f, 0.0f,
        width, 0.0f,   1.0f, 0.0f,
        width, height, 1.0f, 1.0f,
        0.0f,  height, 0.0f, 1.0f
    };
}

static void setTextureQuadVertices(std::vector<float>& vertices, float width, float height) {
    setSolidQuadVertices(vertices, width, height);
}

static void updateActorAnimation(Actor& actor, float dt, bool holdLastFrame = false) {
    actor.animTimer += dt;
    while (actor.animTimer >= actor.frameTime) {
        actor.animTimer -= actor.frameTime;
        if (holdLastFrame && actor.frame == actor.sheet->frameCount - 1) {
            return;
        }
        actor.frame = holdLastFrame
            ? std::min(actor.frame + 1, actor.sheet->maxFrame)
            : (actor.frame + 1) % std::max(1, actor.sheet->frameCount);
    }
}

static void updateActorAnimationRange(Actor& actor, float dt, int firstFrame, int lastFrame) {
    actor.frame = std::max(firstFrame, std::min(actor.frame, lastFrame));
    actor.animTimer += dt;
    while (actor.animTimer >= actor.frameTime) {
        actor.animTimer -= actor.frameTime;
        actor.frame = actor.frame >= lastFrame ? firstFrame : actor.frame + 1;
    }
}

static void updateFrameIndex(float& timer, int& frame, float dt, float frameTime, int lastFrame) {
    timer += dt;
    while (timer >= frameTime) {
        timer -= frameTime;
        frame = frame >= lastFrame ? 0 : frame + 1;
    }
}

static void drawActor(
    const Actor& actor,
    GLuint shaderProgram,
    GLuint vbo,
    GLint offsetLoc,
    GLint alphaLoc,
    std::vector<float>& vertices,
    float alpha
) {
    const Vec2 screen = tileToScreen(actor.col, actor.row);
    setSpriteFrameVertices(vertices, actor);
    glBindTexture(GL_TEXTURE_2D, actor.sheet->texture.id);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * vertices.size(), vertices.data());
    glUniform2f(offsetLoc, screen.x, screen.y);
    glUniform1f(alphaLoc, alpha);
    glUniform1i(glGetUniformLocation(shaderProgram, "uUseTexture"), 1);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}

static void drawTreeActor(
    const Actor& actor,
    GLuint shaderProgram,
    GLuint vbo,
    GLint offsetLoc,
    GLint alphaLoc,
    std::vector<float>& vertices,
    float sourceStartY,
    float sourceEndY
) {
    const Vec2 screen = tileToScreen(actor.col, actor.row);
    const SpriteSheet& sheet = *actor.sheet;
    const float sourceFrameHeight = static_cast<float>(sheet.frameHeight);
    const bool usesHorizontalFrames = sheet.frameCount > 3;
    const int treeAnimationCol = usesHorizontalFrames
        ? std::max(0, std::min(actor.frame, sheet.frameCols - 1))
        : 0;
    const int treeAnimationRow = usesHorizontalFrames
        ? 0
        : std::max(0, std::min(actor.frame, sheet.frameRows - 1));
    const float halfTexelU = 0.5f / static_cast<float>(sheet.texture.width);
    const float halfTexelV = 0.5f / static_cast<float>(sheet.texture.height);

    const int cellX0 = static_cast<int>(std::round((static_cast<float>(treeAnimationCol) * sheet.texture.width) / sheet.frameCols));
    const int cellY0 = static_cast<int>(std::round((static_cast<float>(treeAnimationRow) * sheet.texture.height) / sheet.frameRows));
    const int cellX1 = static_cast<int>(std::round((static_cast<float>(treeAnimationCol + 1) * sheet.texture.width) / sheet.frameCols));
    const int cellY1 = static_cast<int>(std::round((static_cast<float>(treeAnimationRow + 1) * sheet.texture.height) / sheet.frameRows));

    const float u0 = cellX0 / static_cast<float>(sheet.texture.width) + halfTexelU;
    const float v0 = (cellY0 + sourceStartY) / static_cast<float>(sheet.texture.height) + halfTexelV;
    const float u1 = cellX1 / static_cast<float>(sheet.texture.width) - halfTexelU;
    const float v1 = (cellY0 + sourceEndY) / static_cast<float>(sheet.texture.height) - halfTexelV;

    const float destStartY = (sourceStartY / sourceFrameHeight) * actor.drawHeight;
    const float destEndY = (sourceEndY / sourceFrameHeight) * actor.drawHeight;

    const float left = -actor.anchorX * kSceneScale;
    const float top = (destStartY - actor.anchorY) * kSceneScale;
    const float right = (actor.drawWidth - actor.anchorX) * kSceneScale;
    const float bottom = (destEndY - actor.anchorY) * kSceneScale;

    vertices = {
        left,  top,    u0, v0,
        right, top,    u1, v0,
        right, bottom, u1, v1,
        left,  bottom, u0, v1
    };

    glBindTexture(GL_TEXTURE_2D, actor.sheet->texture.id);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * vertices.size(), vertices.data());
    glUniform2f(offsetLoc, screen.x, screen.y);
    glUniform1f(alphaLoc, 1.0f);
    glUniform1i(glGetUniformLocation(shaderProgram, "uUseTexture"), 1);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}

static void drawScreenRect(
    GLuint shaderProgram,
    GLuint vbo,
    GLint offsetLoc,
    GLint alphaLoc,
    GLint useTextureLoc,
    GLint colorLoc,
    std::vector<float>& vertices,
    float x,
    float y,
    float width,
    float height,
    float r,
    float g,
    float b,
    float a
) {
    setSolidQuadVertices(vertices, width, height);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * vertices.size(), vertices.data());
    glUniform2f(offsetLoc, x, y);
    glUniform1f(alphaLoc, 1.0f);
    glUniform1i(useTextureLoc, 0);
    glUniform4f(colorLoc, r, g, b, a);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}

static void drawTextureScreen(
    const TextureInfo& texture,
    GLuint vbo,
    GLint offsetLoc,
    GLint alphaLoc,
    GLint useTextureLoc,
    std::vector<float>& vertices,
    float x,
    float y,
    float width,
    float height,
    float alpha
) {
    setTextureQuadVertices(vertices, width, height);
    glBindTexture(GL_TEXTURE_2D, texture.id);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * vertices.size(), vertices.data());
    glUniform2f(offsetLoc, x, y);
    glUniform1f(alphaLoc, alpha);
    glUniform1i(useTextureLoc, 1);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}

static void drawDigit(
    int value,
    GLuint shaderProgram,
    GLuint vbo,
    GLint offsetLoc,
    GLint alphaLoc,
    GLint useTextureLoc,
    GLint colorLoc,
    std::vector<float>& vertices,
    float x,
    float y,
    float scale
) {
    static const bool segments[10][7] = {
        { true, true, true, true, true, true, false },
        { false, true, true, false, false, false, false },
        { true, true, false, true, true, false, true },
        { true, true, true, true, false, false, true },
        { false, true, true, false, false, true, true },
        { true, false, true, true, false, true, true },
        { true, false, true, true, true, true, true },
        { true, true, true, false, false, false, false },
        { true, true, true, true, true, true, true },
        { true, true, true, true, false, true, true }
    };

    value = std::max(0, std::min(9, value));
    const float t = 4.0f * scale;
    const float w = 22.0f * scale;
    const float h = 38.0f * scale;
    const float rects[7][4] = {
        { x + t,      y,              w - 2 * t, t },
        { x + w - t,  y + t,          t,         h * 0.5f - t },
        { x + w - t,  y + h * 0.5f,   t,         h * 0.5f - t },
        { x + t,      y + h - t,      w - 2 * t, t },
        { x,          y + h * 0.5f,   t,         h * 0.5f - t },
        { x,          y + t,          t,         h * 0.5f - t },
        { x + t,      y + h * 0.5f - t * 0.5f, w - 2 * t, t }
    };

    for (int i = 0; i < 7; ++i) {
        if (segments[value][i]) {
            drawScreenRect(shaderProgram, vbo, offsetLoc, alphaLoc, useTextureLoc, colorLoc, vertices,
                rects[i][0], rects[i][1], rects[i][2], rects[i][3], 0.95f, 0.18f, 0.22f, 1.0f);
        }
    }
}

static void drawCrystalCounter(
    int collected,
    int total,
    GLuint shaderProgram,
    GLuint vbo,
    GLint offsetLoc,
    GLint alphaLoc,
    GLint useTextureLoc,
    GLint colorLoc,
    std::vector<float>& vertices
) {
    drawDigit(collected, shaderProgram, vbo, offsetLoc, alphaLoc, useTextureLoc, colorLoc, vertices, 24.0f, 24.0f, 1.38f);
    drawScreenRect(shaderProgram, vbo, offsetLoc, alphaLoc, useTextureLoc, colorLoc, vertices,
        66.0f, 50.0f, 22.0f, 5.0f, 0.95f, 0.95f, 0.95f, 1.0f);
    drawDigit(total, shaderProgram, vbo, offsetLoc, alphaLoc, useTextureLoc, colorLoc, vertices, 100.0f, 24.0f, 1.38f);
}

static void updateMushroom(Mushroom& mushroom, float dt) {
    mushroom.progress += mushroom.direction * mushroom.speed * dt;

    if (mushroom.progress >= 1.0f) {
        mushroom.progress = 1.0f;
        mushroom.direction = -1.0f;
    } else if (mushroom.progress <= 0.0f) {
        mushroom.progress = 0.0f;
        mushroom.direction = 1.0f;
    }

    mushroom.actor.col = mushroom.startCol + (mushroom.endCol - mushroom.startCol) * mushroom.progress;
    mushroom.actor.row = mushroom.startRow + (mushroom.endRow - mushroom.startRow) * mushroom.progress;
    mushroom.actor.flipX = ((mushroom.endCol - mushroom.startCol) * mushroom.direction) > 0.0f;
}

static bool isColliding(const Actor& a, const Actor& b) {
    const float dx = a.col - b.col;
    const float dy = a.row - b.row;
    return std::sqrt(dx * dx + dy * dy) < 0.75f;
}

static bool tryMove(
    const GameConfig& config,
    TileMap& map,
    std::vector<unsigned char>& visitedTiles,
    Actor& witch,
    int deltaCol,
    int deltaRow
) {
    const int currentCol = static_cast<int>(std::round(witch.col));
    const int currentRow = static_cast<int>(std::round(witch.row));
    const int nextCol = currentCol + deltaCol;
    const int nextRow = currentRow + deltaRow;

    if (!isTileWalkable(config, nextCol, nextRow)) {
        return false;
    }

    witch.col = static_cast<float>(nextCol);
    witch.row = static_cast<float>(nextRow);
    witch.flipX = deltaCol < 0;
    map.setTile(nextCol, nextRow, static_cast<unsigned char>(config.visitedTileIndex));
    visitedTiles[indexOf(nextCol, nextRow)] = 1;
    return true;
}

static bool handleMovement(
    GLFWwindow* window,
    const GameConfig& config,
    TileMap& map,
    std::vector<unsigned char>& visitedTiles,
    Actor& witch
) {
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        return tryMove(config, map, visitedTiles, witch, 0, -1);
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        return tryMove(config, map, visitedTiles, witch, 0, 1);
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        return tryMove(config, map, visitedTiles, witch, 1, 0);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        return tryMove(config, map, visitedTiles, witch, -1, 0);
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        return tryMove(config, map, visitedTiles, witch, 1, -1);
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        return tryMove(config, map, visitedTiles, witch, -1, -1);
    }
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        return tryMove(config, map, visitedTiles, witch, 1, 1);
    }
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
        return tryMove(config, map, visitedTiles, witch, -1, 1);
    }
    return false;
}

static void drawMap(
    const GameConfig& config,
    const TileMap& map,
    const std::vector<unsigned char>& visitedTiles,
    const TextureInfo& floorTexture,
    const TextureInfo& visitedTexture,
    GLuint vbo,
    GLint offsetLoc,
    GLint alphaLoc,
    GLint useTextureLoc,
    std::vector<float>& tileVertices
) {
    glUniform1f(alphaLoc, 1.0f);
    glUniform1i(useTextureLoc, 1);

    for (int sum = 0; sum <= (gMapCols - 1) + (gMapRows - 1); ++sum) {
        for (int row = 0; row < gMapRows; ++row) {
            const int col = sum - row;
            if (!inBounds(col, row)) {
                continue;
            }

            if (visitedTiles[indexOf(col, row)] != 0) {
                glBindTexture(GL_TEXTURE_2D, visitedTexture.id);
                setTileUv(tileVertices, config.visitedTileIndex, visitedTexture, config.visitedTilesetCols, config.visitedTilesetRows);
            } else {
                glBindTexture(GL_TEXTURE_2D, floorTexture.id);
                setTileUv(tileVertices, map.getTile(col, row), floorTexture, config.tilesetCols, config.tilesetRows);
            }

            const Vec2 screen = tileToScreen(col, row);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * tileVertices.size(), tileVertices.data());
            glUniform2f(offsetLoc, screen.x, screen.y);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        }
    }
}

static std::vector<Crystal> makeCrystals(const GameConfig& config) {
    std::vector<Crystal> crystals;
    for (const GridPos& pos : config.crystals) {
        crystals.push_back({ static_cast<float>(pos.col), static_cast<float>(pos.row), false });
    }
    return crystals;
}

static std::vector<Mushroom> makeMushrooms(const GameConfig& config, SpriteSheet& mushroomRun) {
    std::vector<Mushroom> mushrooms;
    for (const MushroomConfig& item : config.mushrooms) {
        Mushroom mushroom = {
            { item.startCol, item.startRow, &mushroomRun, 92.0f, 73.6f, 46.0f, 64.4f, 0.08f, 0.0f, 0, false },
            item.startCol,
            item.startRow,
            item.endCol,
            item.endRow,
            item.progress,
            item.direction,
            item.speed
        };
        updateMushroom(mushroom, 0.0f);
        mushrooms.push_back(mushroom);
    }
    return mushrooms;
}

}

int main() {
    GameConfig config;
    if (!loadConfigFromKnownPaths(config)) {
        return 1;
    }

    if (!glfwInit()) {
        std::cerr << "Nao foi possivel iniciar GLFW." << std::endl;
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(kWindowWidth, kWindowHeight, "Grau B - Tilemap Isometrico", nullptr, nullptr);
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

    TextureInfo floorTexture;
    TextureInfo visitedTexture;
    TextureInfo backgroundTexture;
    TextureInfo moonOverlayTexture;
    if (!loadImageFromKnownPaths(floorTexture, config.tilesetPath) ||
        !loadImageFromKnownPaths(visitedTexture, config.visitedTilesetPath) ||
        !loadImageFromKnownPaths(backgroundTexture, "assets/tex/craftpix-net-942044-free-moon-pixel-game-backgrounds/1 background/1.png") ||
        !loadImageFromKnownPaths(moonOverlayTexture, "assets/tex/craftpix-net-942044-free-moon-pixel-game-backgrounds/1 background/2.png")) {
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

    SpriteSheet witchRun;
    SpriteSheet witchDeath;
    SpriteSheet mushroomRun;
    std::array<SpriteSheet, 5> treeSheets;
    std::array<SpriteSheet, 4> crystalSheets;
    TextureInfo victoryImage;
    TextureInfo defeatImage;
    TextureInfo restartExitImage;

    if (!loadSheetFromKnownPaths(witchRun, "assets/sprites/Blue Witch/Blue_witch/B_witch_run.png", 32, 48, false) ||
        !loadSheetFromKnownPaths(witchDeath, "assets/sprites/Blue Witch/Blue_witch/B_witch_death.png", 32, 40, false) ||
        !loadSheetFromKnownPaths(mushroomRun, "assets/sprites/Forest_Monsters_FREE/Forest_Monsters_FREE/Mushroom/Mushroom with VFX/Mushroom-Run.png", 80, 64, true) ||
        !loadImageFromKnownPaths(treeSheets[0].texture, "assets/tex/craftpix-net-695666-free-undead-tileset-top-down-pixel-art/PNG/Animation1.png") ||
        !loadImageFromKnownPaths(treeSheets[1].texture, "assets/tex/craftpix-net-695666-free-undead-tileset-top-down-pixel-art/PNG/Animation2.png") ||
        !loadImageFromKnownPaths(treeSheets[2].texture, "assets/tex/craftpix-net-695666-free-undead-tileset-top-down-pixel-art/PNG/Animation3.png") ||
        !loadImageFromKnownPaths(treeSheets[3].texture, "assets/tex/craftpix-net-695666-free-undead-tileset-top-down-pixel-art/PNG/Animation4.png") ||
        !loadImageFromKnownPaths(treeSheets[4].texture, "assets/tex/craftpix-net-695666-free-undead-tileset-top-down-pixel-art/PNG/Animation5.png") ||
        !loadSheetFromKnownPaths(crystalSheets[0], "assets/sprites/Crystal_Animation/Red/red_crystal_0000.png", 64, 64, true) ||
        !loadSheetFromKnownPaths(crystalSheets[1], "assets/sprites/Crystal_Animation/Red/red_crystal_0001.png", 64, 64, true) ||
        !loadSheetFromKnownPaths(crystalSheets[2], "assets/sprites/Crystal_Animation/Red/red_crystal_0002.png", 64, 64, true) ||
        !loadSheetFromKnownPaths(crystalSheets[3], "assets/sprites/Crystal_Animation/Red/red_crystal_0003.png", 64, 64, true) ||
        !loadImageFromKnownPaths(victoryImage, "src/Exercicios/GrauB/win.png") ||
        !loadImageFromKnownPaths(defeatImage, "src/Exercicios/GrauB/game-over.png") ||
        !loadImageFromKnownPaths(restartExitImage, "src/Exercicios/GrauB/restart-exit.png")) {
        glfwTerminate();
        return 1;
    }
    witchDeath.maxFrame = witchDeath.frameCount - 1;
    for (std::size_t i = 0; i < treeSheets.size(); ++i) {
        SpriteSheet& treeSheet = treeSheets[i];
        treeSheet.frameCols = 6;
        treeSheet.frameRows = 3;
        treeSheet.frameWidth = treeSheet.texture.width / 6;
        treeSheet.frameHeight = treeSheet.texture.height / 3;
        treeSheet.frameCount = i >= 3 ? 6 : 3;
        treeSheet.maxFrame = treeSheet.frameCount - 1;
        treeSheet.horizontal = i >= 3;
    }

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

    TileMap map = config.map;
    std::vector<unsigned char> visitedTiles(gMapCols * gMapRows, 0);
    visitedTiles[indexOf(config.witchStart.col, config.witchStart.row)] = 1;
    map.setTile(config.witchStart.col, config.witchStart.row, static_cast<unsigned char>(config.visitedTileIndex));

    std::vector<float> tileVertices;
    std::vector<float> spriteVertices;
    const GLint screenSizeLoc = glGetUniformLocation(shaderProgram, "uScreenSize");
    const GLint offsetLoc = glGetUniformLocation(shaderProgram, "uOffset");
    const GLint alphaLoc = glGetUniformLocation(shaderProgram, "uAlpha");
    const GLint useTextureLoc = glGetUniformLocation(shaderProgram, "uUseTexture");
    const GLint colorLoc = glGetUniformLocation(shaderProgram, "uColor");

    Actor witch = {
        static_cast<float>(config.witchStart.col),
        static_cast<float>(config.witchStart.row),
        &witchRun,
        62.4f, 78.0f,
        31.2f, 71.5f,
        0.13f, 0.0f, 0,
        false
    };

    Actor treeActor = {
        0.0f, 0.0f,
        &treeSheets[0],
        99.0f, 128.0f,
        49.0f, 120.0f,
        0.18f, 0.0f, 0,
        false
    };

    Actor crystalActor = {
        0.0f, 0.0f,
        &crystalSheets[0],
        54.6f, 54.6f,
        27.3f, 44.2f,
        0.15f, 0.0f, 0,
        false
    };

    std::vector<Crystal> crystals = makeCrystals(config);
    std::vector<Mushroom> mushrooms = makeMushrooms(config, mushroomRun);
    WitchState witchState = WITCH_ALIVE;
    GameResult gameResult = GAME_PLAYING;
    float deathTimer = 0.0f;
    float crystalTimer = 0.0f;
    float moveCooldown = 0.0f;
    float treeAnimTimer = 0.0f;
    float cursedTreeAnimTimer = 0.0f;
    int crystalFrame = 0;
    int treeAnimationFrame = 0;
    int cursedTreeAnimationFrame = 0;
    int collectedCrystals = 0;
    const int crystalsToWin = std::min(kCrystalsToWin, static_cast<int>(crystals.size()));
    double previousTime = glfwGetTime();

    std::cout << "Controles: setas/WASD para N/S/L/O; Q/E/Z/C para diagonais. ESC sai, R reinicia." << std::endl;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        const double currentTime = glfwGetTime();
        const float dt = static_cast<float>(currentTime - previousTime);
        previousTime = currentTime;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        if (gameResult != GAME_PLAYING && glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            map = config.map;
            visitedTiles.assign(gMapCols * gMapRows, 0);
            visitedTiles[indexOf(config.witchStart.col, config.witchStart.row)] = 1;
            map.setTile(config.witchStart.col, config.witchStart.row, static_cast<unsigned char>(config.visitedTileIndex));
            crystals = makeCrystals(config);
            mushrooms = makeMushrooms(config, mushroomRun);
            collectedCrystals = 0;
            gameResult = GAME_PLAYING;
            witchState = WITCH_ALIVE;
            witch.sheet = &witchRun;
            witch.col = static_cast<float>(config.witchStart.col);
            witch.row = static_cast<float>(config.witchStart.row);
            witch.drawWidth = 62.4f;
            witch.drawHeight = 78.0f;
            witch.anchorX = 31.2f;
            witch.anchorY = 71.5f;
            witch.frame = 0;
            witch.animTimer = 0.0f;
            witch.frameTime = 0.13f;
            treeAnimTimer = 0.0f;
            cursedTreeAnimTimer = 0.0f;
            treeAnimationFrame = 0;
            cursedTreeAnimationFrame = 0;
            deathTimer = 0.0f;
            glfwSetWindowTitle(window, "Grau B - Tilemap Isometrico");
        }

        moveCooldown -= dt;

        if (gameResult == GAME_PLAYING) {
            crystalTimer += dt;
            if (crystalTimer >= 0.15f) {
                crystalTimer = 0.0f;
                crystalFrame = (crystalFrame + 1) % 4;
            }
        }

        if (gameResult == GAME_PLAYING && witchState == WITCH_ALIVE && moveCooldown <= 0.0f) {
            if (handleMovement(window, config, map, visitedTiles, witch)) {
                moveCooldown = kMoveCooldownSeconds;
            }
        }

        if (gameResult == GAME_PLAYING || gameResult == GAME_WON) {
            updateFrameIndex(treeAnimTimer, treeAnimationFrame, dt, 0.18f, 2);
            updateFrameIndex(cursedTreeAnimTimer, cursedTreeAnimationFrame, dt, 0.12f, 5);
        }

        if (gameResult == GAME_PLAYING) {
            for (Mushroom& mushroom : mushrooms) {
                updateMushroom(mushroom, dt);
                updateActorAnimation(mushroom.actor, dt);
            }
        }

        if (gameResult == GAME_PLAYING && witchState == WITCH_ALIVE) {
            witch.sheet = &witchRun;
            witch.frameTime = 0.13f;
            updateActorAnimation(witch, dt);

            bool hitEnemy = false;
            for (const Mushroom& mushroom : mushrooms) {
                if (isColliding(witch, mushroom.actor)) {
                    hitEnemy = true;
                    break;
                }
            }

            if (hitEnemy) {
                witchState = WITCH_DYING;
                witch.sheet = &witchDeath;
                witch.drawWidth = 62.4f;
                witch.drawHeight = 70.2f;
                witch.anchorX = 31.2f;
                witch.anchorY = 63.7f;
                witch.frameTime = 0.22f;
                witch.frame = 0;
                witch.animTimer = 0.0f;
                deathTimer = 0.0f;
                crystalFrame = 0;
            } else {
                for (Crystal& crystal : crystals) {
                    Actor collisionCrystal = crystalActor;
                    collisionCrystal.col = crystal.col;
                    collisionCrystal.row = crystal.row;

                    if (!crystal.collected && isColliding(witch, collisionCrystal)) {
                        crystal.collected = true;
                        collectedCrystals++;
                        if (collectedCrystals >= crystalsToWin) {
                            gameResult = GAME_WON;
                            crystalFrame = 0;
                            glfwSetWindowTitle(window, "Grau B - Vitoria! Cristais coletados");
                        }
                        break;
                    }
                }
            }
        } else if (witchState == WITCH_DYING) {
            deathTimer += dt;
            updateActorAnimation(witch, dt, true);

            if (deathTimer >= 3.4f) {
                gameResult = GAME_LOST;
                collectedCrystals = 0;
                crystalFrame = 0;
                glfwSetWindowTitle(window, "Grau B - Derrota! Pressione R para reiniciar");
            }
        } else if (gameResult == GAME_WON) {
            updateActorAnimation(witch, dt);
        }

        int screenW = 0;
        int screenH = 0;
        glfwGetFramebufferSize(window, &screenW, &screenH);

        glClearColor(0.10f, 0.14f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glUniform2f(screenSizeLoc, static_cast<float>(screenW), static_cast<float>(screenH));
        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(vao);

        drawTextureScreen(backgroundTexture, vbo, offsetLoc, alphaLoc, useTextureLoc, spriteVertices,
            0.0f, 0.0f, static_cast<float>(screenW), static_cast<float>(screenH), 1.0f);

        const float moonOverlayW = static_cast<float>(moonOverlayTexture.width);
        const float moonOverlayH = static_cast<float>(moonOverlayTexture.height);
        drawTextureScreen(moonOverlayTexture, vbo, offsetLoc, alphaLoc, useTextureLoc, spriteVertices,
            static_cast<float>(screenW) - moonOverlayW, 0.0f, moonOverlayW, moonOverlayH, 1.0f);

        drawMap(config, map, visitedTiles, floorTexture, visitedTexture, vbo, offsetLoc, alphaLoc, useTextureLoc, tileVertices);

        for (const Crystal& crystal : crystals) {
            if (!crystal.collected) {
                crystalActor.col = crystal.col;
                crystalActor.row = crystal.row;
                crystalActor.sheet = &crystalSheets[crystalFrame];
                drawActor(crystalActor, shaderProgram, vbo, offsetLoc, alphaLoc, spriteVertices, 1.0f);
            }
        }

        for (const Mushroom& mushroom : mushrooms) {
            (void)mushroom;
        }

        for (const GridPos& tree : config.trees) {
            const int treeTypeIndex = std::max(0, std::min(4, tree.type - 1));
            treeActor.sheet = &treeSheets[treeTypeIndex];
            treeActor.frame = treeTypeIndex >= 3 ? cursedTreeAnimationFrame : treeAnimationFrame;
            treeActor.drawWidth = static_cast<float>(treeActor.sheet->frameWidth) * kTreeVisualScale;
            treeActor.drawHeight = static_cast<float>(treeActor.sheet->frameHeight) * kTreeVisualScale;
            treeActor.anchorX = treeActor.drawWidth * 0.5f;
            treeActor.anchorY = treeActor.drawHeight - 8.0f;
            const float treeSplitY = static_cast<float>(treeActor.sheet->frameHeight) * 0.62f;
            treeActor.col = static_cast<float>(tree.col);
            treeActor.row = static_cast<float>(tree.row);
            drawTreeActor(
                treeActor,
                shaderProgram,
                vbo,
                offsetLoc,
                alphaLoc,
                spriteVertices,
                treeSplitY,
                static_cast<float>(treeActor.sheet->frameHeight)
            );
        }

        for (const Mushroom& mushroom : mushrooms) {
            drawActor(mushroom.actor, shaderProgram, vbo, offsetLoc, alphaLoc, spriteVertices, 1.0f);
        }

        drawActor(witch, shaderProgram, vbo, offsetLoc, alphaLoc, spriteVertices, 1.0f);

        for (const GridPos& tree : config.trees) {
            const int treeTypeIndex = std::max(0, std::min(4, tree.type - 1));
            treeActor.sheet = &treeSheets[treeTypeIndex];
            treeActor.frame = treeTypeIndex >= 3 ? cursedTreeAnimationFrame : treeAnimationFrame;
            treeActor.drawWidth = static_cast<float>(treeActor.sheet->frameWidth) * kTreeVisualScale;
            treeActor.drawHeight = static_cast<float>(treeActor.sheet->frameHeight) * kTreeVisualScale;
            treeActor.anchorX = treeActor.drawWidth * 0.5f;
            treeActor.anchorY = treeActor.drawHeight - 8.0f;
            const float treeSplitY = static_cast<float>(treeActor.sheet->frameHeight) * 0.62f;
            treeActor.col = static_cast<float>(tree.col);
            treeActor.row = static_cast<float>(tree.row);
            drawTreeActor(treeActor, shaderProgram, vbo, offsetLoc, alphaLoc, spriteVertices, 0.0f, treeSplitY);
        }

        drawCrystalCounter(std::min(collectedCrystals, crystalsToWin), crystalsToWin, shaderProgram, vbo, offsetLoc, alphaLoc, useTextureLoc, colorLoc, spriteVertices);

        if (gameResult == GAME_WON || gameResult == GAME_LOST) {
            drawScreenRect(shaderProgram, vbo, offsetLoc, alphaLoc, useTextureLoc, colorLoc, spriteVertices,
                0.0f, 0.0f, static_cast<float>(screenW), static_cast<float>(screenH), 1.0f, 1.0f, 1.0f, 0.86f);

            const bool isVictoryScreen = gameResult == GAME_WON;
            const TextureInfo& endImage = gameResult == GAME_WON ? victoryImage : defeatImage;
            const float maxW = 430.0f;
            const float maxH = 430.0f;
            const float frameX = (static_cast<float>(screenW) - maxW) * 0.5f;
            const float frameY = 58.0f;
            const float baseScale = std::min(maxW / static_cast<float>(endImage.width), maxH / static_cast<float>(endImage.height));
            const float imageScale = isVictoryScreen ? baseScale * 0.7f : baseScale;
            const float imageW = static_cast<float>(endImage.width) * imageScale;
            const float imageH = static_cast<float>(endImage.height) * imageScale;
            const float imageX = frameX + (maxW - imageW) * 0.5f;
            const float imageY = frameY + (maxH - imageH) * 0.5f;

            drawTextureScreen(endImage, vbo, offsetLoc, alphaLoc, useTextureLoc, spriteVertices,
                imageX, imageY, imageW, imageH, 1.0f);

            const float restartMaxW = 380.0f;
            const float restartMaxH = 140.0f;
            const float restartScale = std::min(
                restartMaxW / static_cast<float>(restartExitImage.width),
                restartMaxH / static_cast<float>(restartExitImage.height)
            );
            const float restartW = static_cast<float>(restartExitImage.width) * restartScale;
            const float restartH = static_cast<float>(restartExitImage.height) * restartScale;
            const float restartX = (static_cast<float>(screenW) - restartW) * 0.5f;
            const float restartY = frameY + maxH + 28.0f;

            drawTextureScreen(restartExitImage, vbo, offsetLoc, alphaLoc, useTextureLoc, spriteVertices,
                restartX, restartY, restartW, restartH, 1.0f);
        }

        glfwSwapBuffers(window);
    }

    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shaderProgram);
    glDeleteTextures(1, &floorTexture.id);
    glDeleteTextures(1, &visitedTexture.id);
    glDeleteTextures(1, &backgroundTexture.id);
    glDeleteTextures(1, &moonOverlayTexture.id);
    glDeleteTextures(1, &witchRun.texture.id);
    glDeleteTextures(1, &witchDeath.texture.id);
    glDeleteTextures(1, &mushroomRun.texture.id);
    for (SpriteSheet& treeSheet : treeSheets) {
        glDeleteTextures(1, &treeSheet.texture.id);
    }
    glDeleteTextures(1, &victoryImage.id);
    glDeleteTextures(1, &defeatImage.id);
    glDeleteTextures(1, &restartExitImage.id);
    for (SpriteSheet& crystalSheet : crystalSheets) {
        glDeleteTextures(1, &crystalSheet.texture.id);
    }

    glfwTerminate();
    return 0;
}
