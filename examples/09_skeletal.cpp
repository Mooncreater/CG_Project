// 09: Minecraft-style game — thin entry point
#include "../src/game/minecraft_game.h"

int main() {
    Options o;
    o.windowTitle  = "Minecraft Style";
    o.assetRootDir = "media";
    o.windowWidth  = 1280;
    o.windowHeight = 800;
    o.glVersion    = {3, 3};
    MinecraftGame app(o);
    app.run();
    return 0;
}
