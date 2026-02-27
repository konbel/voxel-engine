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

    engine.Run();
    return EXIT_SUCCESS;
}
