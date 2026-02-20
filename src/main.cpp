#include <iostream>

#include "engine/Engine.h"
#include "engine/utility/files/Files.h"

int main() {
    try {
        Engine engine;
        engine.Initialize(SHADERS_DIR);
        engine.Run();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
