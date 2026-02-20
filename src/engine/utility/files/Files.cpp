#include "Files.h"

#include <fstream>
#include <stdexcept>

////////////////////////////////////////////////////////////////////////////////
std::vector<char> ReadFile(const std::string &path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + path);
    }

    std::vector<char> buffer(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    file.close();
    return buffer;
}
