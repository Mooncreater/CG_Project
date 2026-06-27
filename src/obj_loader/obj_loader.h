#pragma once

#include <string>
#include <vector>
#include "../base/vertex.h"
#include "../base/gl_utility.h"

struct Element;

namespace ObjLoader {
    Element Load(const std::string& filepath);

}

