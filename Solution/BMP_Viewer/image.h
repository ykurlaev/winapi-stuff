#ifndef Y_IMAGE_H
#define Y_IMAGE_H

#include <cstdint>
#include <istream>
#include <ostream>

struct Image
{
    enum Channel { B = 0, G = 1, R = 2, A = 3 };
    Image() : width(0), height(0), pixels(NULL) {}
    uint32_t width;
    uint32_t height;
	typedef uint8_t pixel[4];
    pixel *pixels;
    pixel *operator[](uint32_t y) { return pixels + width * y; }
    friend std::istream& operator>>(std::istream& stream, Image& image);
    friend std::ostream& operator<<(std::ostream& stream, Image& image);
};

std::istream& operator>>(std::istream& stream, Image& image);

std::ostream& operator<<(std::ostream& stream, Image& image);

#endif //Y_IMAGE_H
