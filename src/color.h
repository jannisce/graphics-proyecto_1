#include <SDL2/SDL.h>
#include <SDL_stdinc.h>

#pragma once

struct Color {
    Uint8 r;
    Uint8 g;
    Uint8 b;
    Uint8 a;

    Color(Uint8 red = 0, Uint8 green = 0, Uint8 blue = 0, Uint8 alpha = 255)
            : r(red), g(green), b(blue), a(alpha) {}

    Color operator*(float scalar) const {
        Color result;
        result.r = static_cast<uint8_t>(r * scalar);
        result.g = static_cast<uint8_t>(g * scalar);
        result.b = static_cast<uint8_t>(b * scalar);
        result.a = static_cast<uint8_t>(a * scalar);
        return result;
    }

    Color operator+(const Color& other) const {
        Color result;
        result.r = static_cast<uint8_t>(r + other.r);
        result.g = static_cast<uint8_t>(g + other.g);
        result.b = static_cast<uint8_t>(b + other.b);
        result.a = static_cast<uint8_t>(a + other.a);
        return result;
    }
};

Color MixColors(const Color& color1, const Color& color2, float t) {
    t = glm::clamp(t, 0.0f, 1.0f); // Asegura que t esté en el rango [0, 1]
    return Color(
            static_cast<uint8_t>((1.0f - t) * color1.r + t * color2.r),
            static_cast<uint8_t>((1.0f - t) * color1.g + t * color2.g),
            static_cast<uint8_t>((1.0f - t) * color1.b + t * color2.b),
            static_cast<uint8_t>((1.0f - t) * color1.a + t * color2.a)
    );
}

