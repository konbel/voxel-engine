#include <iostream>

#include "engine/Engine.h"
#include "engine/utility/files/Files.h"

int main() {
    Engine engine;
    if (!engine.Initialize(SHADERS_DIR)) {
        return EXIT_FAILURE;
    }

    const Camera camera;
    engine.SetMainCamera(camera);

    for (int x = 0; x < 10; x++) {
        for (int z = 0; z < 10; z++) {
            engine.CreateBlock({x, 0, z});
        }
    }

    engine.Run();
    return EXIT_SUCCESS;
}
