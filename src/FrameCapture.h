#pragma once
#include <GL/glew.h>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <vector>

// Capture the back buffer before swap. BMP stores rows bottom-up, as OpenGL does.
inline void captureBmp(const std::filesystem::path& path, int width, int height) {
    const std::uint32_t stride = (static_cast<std::uint32_t>(width) * 3u + 3u) & ~3u;
    std::vector<unsigned char> pixels(stride * static_cast<std::uint32_t>(height));
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, pixels.data());
    unsigned char header[54]{};
    auto put = [&](int offset, std::uint32_t value) {
        for (int i = 0; i < 4; ++i) header[offset+i] = static_cast<unsigned char>((value >> (8*i)) & 255u);
    };
    header[0] = 'B'; header[1] = 'M';
    put(2, 54u + static_cast<std::uint32_t>(pixels.size()));
    put(10, 54); put(14, 40); put(18, static_cast<std::uint32_t>(width));
    put(22, static_cast<std::uint32_t>(height)); header[26] = 1; header[28] = 24;
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(header), sizeof(header));
    file.write(reinterpret_cast<const char*>(pixels.data()), static_cast<std::streamsize>(pixels.size()));
    if (!file) throw std::runtime_error("Cannot write screenshot: " + path.string());
}
