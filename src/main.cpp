#include "engine/Engine.h"
#include "engine/utility/files/Files.h"

int main() {
    Engine engine;
    if (!engine.Initialize(SHADERS_DIR)) {
        return EXIT_FAILURE;
    }

    const RenderLayerConfig renderLayerConfig{
        .shaderPath = "shader.spv",
        .vertexShaderEntry = "vertMain",
        .fragmentShaderEntry = "fragMain",
        .descriptorSetConfigs = {
                {
                    .bindings = {
                        vk::DescriptorSetLayoutBinding{
                            .binding = 0,
                            .descriptorType = vk::DescriptorType::eUniformBuffer,
                            .descriptorCount = 1,
                            .stageFlags = vk::ShaderStageFlagBits::eVertex,
                        },
                        vk::DescriptorSetLayoutBinding{
                            .binding = 1,
                            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                            .descriptorCount = 1,
                            .stageFlags = vk::ShaderStageFlagBits::eFragment,
                        },
                    },
                },
            },
        };
    engine.AddRenderLayer(renderLayerConfig);

    const Camera camera({-5.0f, 15.0f, -5.0f}, 135, -30);
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
