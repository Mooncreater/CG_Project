#include "map.h"
#include <algorithm>

GameMap::GameMap() {
    _grid.resize(GameConst::GRID_COLS * GameConst::GRID_ROWS, GameConst::Empty);
}

int GameMap::getCell(int col, int row) const {
    if (!isInBounds(col, row)) return GameConst::Blocked;
    return _grid[row * GameConst::GRID_COLS + col];
}

bool GameMap::isInBounds(int col, int row) const {
    return col >= 0 && col < GameConst::GRID_COLS && row >= 0 && row < GameConst::GRID_ROWS;
}

void GameMap::setCell(int col, int row, int type) {
    if (isInBounds(col, row))
        _grid[row * GameConst::GRID_COLS + col] = type;
}

glm::vec3 GameMap::cellToWorld(int col, int row) const {
    float x = GameConst::GRID_OFFSET_X + (col + 0.5f) * GameConst::CELL_SIZE;
    float z = GameConst::GRID_OFFSET_Z + (row + 0.5f) * GameConst::CELL_SIZE;
    return glm::vec3(x, 0.0f, z);
}

void GameMap::worldToCell(const glm::vec3& pos, int& col, int& row) const {
    col = (int)((pos.x - GameConst::GRID_OFFSET_X) / GameConst::CELL_SIZE);
    row = (int)((pos.z - GameConst::GRID_OFFSET_Z) / GameConst::CELL_SIZE);
}

bool GameMap::isBuildable(int col, int row) const {
    return getCell(col, row) == GameConst::Buildable || getCell(col, row) == GameConst::Empty;
}

void GameMap::placeTower(int col, int row) {
    // Mark cell and surrounding as occupied variant
    setCell(col, row, GameConst::Blocked);
}

void GameMap::addPath(const std::vector<PathNode>& nodes) {
    _paths.push_back(nodes);
}

void GameMap::buildStraightPath(int col0, int row0, int col1, int row1) {
    if (col0 == col1) {
        int r0 = std::min(row0, row1), r1 = std::max(row0, row1);
        for (int r = r0; r <= r1; r++) setCell(col0, r, GameConst::Path);
    } else if (row0 == row1) {
        int c0 = std::min(col0, col1), c1 = std::max(col0, col1);
        for (int c = c0; c <= c1; c++) setCell(c, row0, GameConst::Path);
    }
}

// ============================================================
//  Build the default tower defense map
//  Path: S-shaped winding path from left to right
// ============================================================
void GameMap::buildDefault() {
    const int C = GameConst::GRID_COLS;
    const int R = GameConst::GRID_ROWS;

    // Mark all cells as buildable initially
    for (int i = 0; i < C * R; i++) _grid[i] = GameConst::Buildable;

    // === Path layout (S-shape) ===
    // Enter from left (col 0, row 2) -> right (col 18, row 2)
    // -> down to row 6 -> left (col 1, row 6) -> down to row 10
    // -> right (col 18, row 10) -> carrot at col 18, row 11
    buildStraightPath(0, 2, 16, 2);   // top horizontal
    buildStraightPath(16, 2, 16, 6);  // right vertical down
    buildStraightPath(16, 6, 2, 6);   // middle horizontal left
    buildStraightPath(2, 6, 2, 10);   // left vertical down
    buildStraightPath(2, 10, 17, 10); // bottom horizontal right

    // Spawn point
    setCell(0, 2, GameConst::Spawn);

    // Carrot
    setCell(18, 10, GameConst::Carrot);
    _carrotPos = cellToWorld(18, 10) + glm::vec3(0, 0.5f, 0);

    // Mark cells adjacent to path as buildable (already are)
    // Widen path area to 2 cells thick (visual only - use decorations)

    // === Create path nodes for enemy movement ===
    std::vector<PathNode> pathNodes;
    pathNodes.push_back(PathNode(cellToWorld(0, 2)));
    pathNodes.push_back(PathNode(cellToWorld(16, 2)));
    pathNodes.push_back(PathNode(cellToWorld(16, 6)));
    pathNodes.push_back(PathNode(cellToWorld(2, 6)));
    pathNodes.push_back(PathNode(cellToWorld(2, 10)));
    pathNodes.push_back(PathNode(cellToWorld(18, 10)));
    addPath(pathNodes);

    // Second path variant (for variety)
    std::vector<PathNode> pathNodes2;
    pathNodes2.push_back(PathNode(cellToWorld(0, 2)));
    pathNodes2.push_back(PathNode(cellToWorld(10, 2)));
    pathNodes2.push_back(PathNode(cellToWorld(10, 4)));
    pathNodes2.push_back(PathNode(cellToWorld(2, 4)));
    pathNodes2.push_back(PathNode(cellToWorld(2, 8)));
    pathNodes2.push_back(PathNode(cellToWorld(12, 8)));
    pathNodes2.push_back(PathNode(cellToWorld(12, 10)));
    pathNodes2.push_back(PathNode(cellToWorld(18, 10)));
    addPath(pathNodes2);

    // === Decorate path tiles (visual) ===
    for (int r = 0; r < R; r++) {
        for (int c = 0; c < C; c++) {
            if (_grid[r * C + c] == GameConst::Path ||
                _grid[r * C + c] == GameConst::Spawn ||
                _grid[r * C + c] == GameConst::Carrot) {
                MapObject tile;
                tile.position = cellToWorld(c, r) + glm::vec3(0, -0.08f, 0);
                tile.size = glm::vec3(GameConst::CELL_SIZE * 0.95f, 0.12f, GameConst::CELL_SIZE * 0.95f);
                tile.color = glm::vec3(0.55f, 0.50f, 0.42f); // sandy path
                tile.meshType = 0;
                _pathTiles.push_back(tile);
            }

            if (_grid[r * C + c] == GameConst::Buildable) {
                // Add random grass decorations
                MapObject grass;
                grass.position = cellToWorld(c, r) + glm::vec3(0, -0.06f, 0);
                grass.size = glm::vec3(GameConst::CELL_SIZE * 0.98f, 0.08f, GameConst::CELL_SIZE * 0.98f);
                // Slight color variation
                float g = 0.55f + ((c * 7 + r * 13) % 10) * 0.01f;
                grass.color = glm::vec3(0.25f, g, 0.22f);
                grass.meshType = 0;
                _decorations.push_back(grass);
            }
        }
    }

    // === Decorative trees/rocks near edges ===
    for (int r = 0; r < R; r++) {
        for (int c : {0, C-1}) {
            if (_grid[r * C + c] == GameConst::Buildable) {
                MapObject tree;
                tree.position = cellToWorld(c, r) + glm::vec3(0, 0.6f, 0);
                tree.size = glm::vec3(0.5f, 1.2f, 0.5f);
                tree.color = glm::vec3(0.15f, 0.45f, 0.15f);
                tree.meshType = 2; // cylinder trunk
                _decorations.push_back(tree);

                MapObject canopy;
                canopy.position = cellToWorld(c, r) + glm::vec3(0, 1.2f, 0);
                canopy.size = glm::vec3(1.2f, 1.0f, 1.2f);
                canopy.color = glm::vec3(0.1f, 0.55f, 0.1f);
                canopy.meshType = 1; // sphere canopy
                _decorations.push_back(canopy);
            }
        }
    }
}
