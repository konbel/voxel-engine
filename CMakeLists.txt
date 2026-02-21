cmake_minimum_required(VERSION 4.1)
project(voxel_engine)

set(CMAKE_CXX_STANDARD 20)

# ================================================================================
# Build Options
# ================================================================================
option(ENABLE_CPP20_MODULE "Enable C++ 20 module support for Vulkan" OFF)

# ================================================================================
# External Dependencies
# ================================================================================
find_package(Vulkan REQUIRED)
find_package(slang CONFIG HINTS "$ENV{VULKAN_SDK}/lib/cmake")

include(FetchContent)

# GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 0af55ccecd98d4e5a8d1fad7de25ba429d60e863 #refs/tags/1.0.1
)
FetchContent_MakeAvailable(glm)

# GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4 # Latest stable as of Feb 2026
)
FetchContent_MakeAvailable(glfw)

# ==============================================================================
# Vulkan C++ Module Configuration
# ==============================================================================
# Set up Vulkan C++ module only if enabled
if(ENABLE_CPP20_MODULE)
    add_compile_definitions(USE_CPP20_MODULES)

    add_library(VulkanCppModule)
    add_library(Vulkan::cppm ALIAS VulkanCppModule)

    target_compile_definitions(VulkanCppModule
        PUBLIC VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
    )
    target_include_directories(VulkanCppModule
        PUBLIC
        "${Vulkan_INCLUDE_DIR}"
    )
    target_link_libraries(VulkanCppModule
        PUBLIC
        Vulkan::Vulkan
    )

    set_target_properties(VulkanCppModule PROPERTIES
        CXX_STANDARD 20
        CXX_SCAN_FOR_MODULES ON
    )

    # MSVC-specific compiler options for proper C++ module support
    if(MSVC)
        target_compile_options(VulkanCppModule PRIVATE
            /std:c++latest      # Use latest C++ standard for better module support
            /permissive-        # Standards conformance mode
            /Zc:__cplusplus     # Enable correct __cplusplus macro
            /EHsc               # Enable C++ exception handling
            /Zc:preprocessor    # Use conforming preprocessor
            /translateInclude   # Automatically translate #include to import for standard library
        )
    endif()

    target_sources(VulkanCppModule
        PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        BASE_DIRS
        "${Vulkan_INCLUDE_DIR}"
        FILES
        "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm"
    )

    # Add the vulkan.cppm file directly as a source file
    target_sources(VulkanCppModule
        PRIVATE
        "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm"
    )
else()
    # Create a dummy interface library when C++ 20 module is disabled
    add_library(VulkanCppModule INTERFACE)
    add_library(Vulkan::cppm ALIAS VulkanCppModule)
    target_link_libraries(VulkanCppModule INTERFACE Vulkan::Vulkan)
    target_compile_definitions(VulkanCppModule
        INTERFACE VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
    )
endif()

# ================================================================================
# Shader Compilation
# ================================================================================
set(SHADERS_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SHADERS_OUT_DIR ${CMAKE_BINARY_DIR}/shaders)

file(GLOB SLANG_SHADER_SOURCES ${SHADERS_SRC_DIR}/*.slang)

set(SLANG_SPV_OUTPUTS)
foreach(SHADER_SOURCE ${SLANG_SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER_SOURCE} NAME_WE)
    set(SPV_OUTPUT ${SHADERS_OUT_DIR}/${SHADER_NAME}.spv)
    add_custom_command(
        OUTPUT ${SPV_OUTPUT}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADERS_OUT_DIR}
        COMMAND ${SLANGC_EXECUTABLE} ${SHADER_SOURCE} -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name -entry vertMain -entry fragMain -o ${SPV_OUTPUT}
        DEPENDS ${SHADER_SOURCE}
        COMMENT "Compiling Slang shader: ${SHADER_NAME}.slang -> ${SHADER_NAME}.spv"
        VERBATIM
    )
    list(APPEND SLANG_SPV_OUTPUTS ${SPV_OUTPUT})
endforeach()

# ================================================================================
# Target Configuration
# ================================================================================
file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(voxel_engine ${SOURCES})

if(ENABLE_CPP20_MODULE)
    set_target_properties(voxel_engine PROPERTIES
        CXX_SCAN_FOR_MODULES ON
    )
endif()

target_include_directories(voxel_engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(voxel_engine PRIVATE
    glm::glm
    glfw
    Vulkan::cppm
)

add_custom_target(voxel_engine_shaders DEPENDS ${SLANG_SPV_OUTPUTS})
add_dependencies(voxel_engine voxel_engine_shaders)
target_compile_definitions(voxel_engine PRIVATE SHADERS_DIR="${SHADERS_OUT_DIR}")
