#include "game/minecraft_game.h"
#include <iostream>

int main(int argc, char* argv[]) {
    Options options;
    options.windowTitle = "Minecraft Clone";
    options.windowWidth = 1280;
    options.windowHeight = 720;
    options.windowResizable = true;
    options.vSync = true;
    options.assetRootDir = "D:/.zju/Professional_courses/CG/final/media";
    options.glVersion = {3, 3};
    options.backgroundColor = glm::vec4(0.08f, 0.12f, 0.22f, 1.0f);

    try {
        MinecraftGame app(options);
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return 0;
}
