#pragma once

#include "types.h"
#include <vector>
#include <glm/glm.hpp>

class GameMap {
public:
    GameMap();

    // Build the default tower defense map
    void buildDefault();

    // Grid access
    int getCell(int col, int row) const;
    bool isInBounds(int col, int row) const;
    glm::vec3 cellToWorld(int col, int row) const;
    void worldToCell(const glm::vec3& pos, int& col, int& row) const;
    bool isBuildable(int col, int row) const;
    void placeTower(int col, int row);

    // Path
    void addPath(const std::vector<PathNode>& nodes);
    const std::vector<PathNode>& getPath(int id) const { return _paths[id]; }
    int pathCount() const { return (int)_paths.size(); }

    // Grid data
    const std::vector<int>& grid() const { return _grid; }
    glm::vec3 carrotPosition() const { return _carrotPos; }

    // Grid dimensions
    int cols() const { return GameConst::GRID_COLS; }
    int rows() const { return GameConst::GRID_ROWS; }

    // Map decoration objects (for rendering)
    struct MapObject {
        glm::vec3 position;
        glm::vec3 size;
        glm::vec3 color;
        int meshType; // 0=cube, 1=sphere, 2=cylinder, 3=cone
    };
    const std::vector<MapObject>& decorations() const { return _decorations; }
    const std::vector<MapObject>& pathTiles() const { return _pathTiles; }

private:
    std::vector<int> _grid;  // GameConst::CellType per cell
    std::vector<std::vector<PathNode>> _paths;
    std::vector<MapObject> _decorations;
    std::vector<MapObject> _pathTiles;
    glm::vec3 _carrotPos;

    void setCell(int col, int row, int type);
    void buildStraightPath(int col0, int row0, int col1, int row1);
};
