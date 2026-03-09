#include "engine/Engine.h"
#include "engine/utility/files/Files.h"

int main() {
    Engine engine;
    if (!engine.Initialize(SHADERS_DIR)) {
        return EXIT_FAILURE;
    }

    const Camera camera;
    engine.SetMainCamera(camera);

    for (int x = 0; x < 16; x++) {
        for (int z = 0; z < 16; z++) {
            engine.CreateBlock({x, 10, z}, BlockInfo::Grass);
        }
    }

    for (int y = 0; y < 10; y++) {
        for (int x = 0; x < 16; x++) {
            for (int z = 0; z < 16; z++) {
                engine.CreateBlock({x, y, z}, BlockInfo::Stone);
            }
        }
    }

    engine.Run();
    return EXIT_SUCCESS;
}
