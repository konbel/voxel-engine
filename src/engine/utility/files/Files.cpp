#include "Files.h"

#include <fstream>
#include <stdexcept>

#include "engine/utility/logging/Log.h"

////////////////////////////////////////////////////////////////////////////////
std::vector<char> ReadFile(const std::string &path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        Log::Error(("Failed to open file: " + path).c_str());
        return std::vector<char>();
    }

    std::vector<char> buffer(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    file.close();
    return buffer;
}
