#include "obj_loader.h"
#include "../primitives/primitives.h"

#include <fstream>
#include <sstream>
#include <glm/glm.hpp>

Element ObjLoader::Load(const std::string& filepath) {
    Element element;

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;

    struct FaceIdx { int p, t, n; };
    std::vector<FaceIdx> facePool;

    std::ifstream file(filepath);
    if (!file.is_open())
        throw std::runtime_error("Cannot open: " + filepath);

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ss(line);
        std::string type;
        ss >> type;

        if (type == "v") {
            float x, y, z;
            ss >> x >> y >> z;
            positions.emplace_back(x, y, z);
        }
        else if (type == "vn") {
            float x, y, z;
            ss >> x >> y >> z;
            normals.emplace_back(x, y, z);
        }
        else if (type == "vt") {
            float u, v;
            ss >> u >> v;
            texcoords.emplace_back(u, v);
        }
        else if (type == "f") {
            std::vector<FaceIdx> face;
            std::string token;
            while (ss >> token) {
                FaceIdx fv = {0, 0, 0};
                size_t s1 = token.find('/');
                if (s1 == std::string::npos) {
                    fv.p = std::stoi(token);
                } else {
                    fv.p = std::stoi(token.substr(0, s1));
                    size_t s2 = token.find('/', s1 + 1);
                    if (s2 == std::string::npos) {
                        fv.t = std::stoi(token.substr(s1 + 1));
                    } else {
                        if (s2 > s1 + 1)
                            fv.t = std::stoi(token.substr(s1 + 1, s2 - s1 - 1));
                        fv.n = std::stoi(token.substr(s2 + 1));
                    }
                }
                face.push_back(fv);
            }

            // fan triangulation: quad (a,b,c,d) -> (a,b,c), (a,c,d)
            if (face.size() >= 3) {
                uint32_t base = static_cast<uint32_t>(facePool.size());
                for (auto& fv : face)
                    facePool.push_back(fv);
                for (size_t i = 1; i < face.size() - 1; ++i) {
                    element.indices.push_back(base);
                    element.indices.push_back(base + static_cast<uint32_t>(i));
                    element.indices.push_back(base + static_cast<uint32_t>(i + 1));
                }
            }
        }
    }

    // Convert facePool to vertices (OBJ 1-based -> C++ 0-based)
    for (auto& fv : facePool) {
        Vertex v;
        if (fv.p > 0 && fv.p <= (int)positions.size())
            v.position = positions[fv.p - 1];
        if (fv.t > 0 && fv.t <= (int)texcoords.size())
            v.texCoord = texcoords[fv.t - 1];
        if (fv.n > 0 && fv.n <= (int)normals.size())
            v.normal = normals[fv.n - 1];
        element.vertices.push_back(v);
    }

    return element;
}

void ObjLoader::Save(const std::string& filepath, const Element& element, const glm::mat4& transform) {
    std::ofstream file(filepath);
    if (!file.is_open()) throw std::runtime_error("Cannot open for writing: " + filepath);
    file << "# OBJ exported\n";
    for (auto& v : element.vertices) { auto p=transform*glm::vec4(v.position,1); file<<"v "<<p.x<<" "<<p.y<<" "<<p.z<<"\n"; }
    for (auto& v : element.vertices) { auto n=glm::mat3(transform)*v.normal; file<<"vn "<<n.x<<" "<<n.y<<" "<<n.z<<"\n"; }
    for (auto& v : element.vertices) file<<"vt "<<v.texCoord.x<<" "<<v.texCoord.y<<"\n";
    for (size_t i=0;i<element.indices.size();i+=3) { auto a=element.indices[i]+1,b=element.indices[i+1]+1,c=element.indices[i+2]+1; file<<"f "<<a<<"/"<<a<<"/"<<a<<" "<<b<<"/"<<b<<"/"<<b<<" "<<c<<"/"<<c<<"/"<<c<<"\n"; }
}