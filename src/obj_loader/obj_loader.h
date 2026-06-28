#pragma once

#include <string>
#include <vector>
#include "../base/vertex.h"
#include "../base/gl_utility.h"

struct Element;

namespace ObjLoader {
    Element Load(const std::string& filepath);
    void Save(const std::string& filepath, const Element& element, const glm::mat4& transform = glm::mat4(1.0f));
}

