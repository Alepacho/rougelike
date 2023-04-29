#ifndef MATH_H
#define MATH_H

#include <iostream>
#include <string>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <valarray>
#include <random>

extern bool running;

#define PI 3.14159265
#define D2R PI / 180.0
#define R2D 180.0 / PI

typedef struct {
    double r;       // ∈ [0, 1]
    double g;       // ∈ [0, 1]
    double b;       // ∈ [0, 1]
} rgb;

typedef struct {
    double h;       // ∈ [0, 360]
    double s;       // ∈ [0, 1]
    double v;       // ∈ [0, 1]
} hsv;

struct Mouse { 
    struct MouseButton {
        bool pressed, unpressed, pressing;
    }; MouseButton left, middle, right;
    int x, y;

    Mouse() {
        left  .pressing = false;
        middle.pressing = false;
        right .pressing = false;
    }
};

inline void trace(std::string msg) {
    std::cout << msg << std::endl;
}

inline void error(std::string type, std::string msg) {
    std::cout << "[" << type << "]: " << msg << std::endl;
    running = false;
}

inline int get_string_len(std::string text) {
    int len = 0;
    const char *c = text.c_str();
    while (*c) len += (*c++ & 0xc0) != 0x80;
    return len;
}

inline double lerp(double a, double b, double f) {
    return a + f * (b - a);
}

inline int diff(int v1, int v2) {
    int maxv = std::max(v1, v2);
    int minv = std::min(v1, v2);

    return maxv - minv;
}

inline double angle(double x1, double y1, double x2, double y2) {
    double x = x2 - x1;
    double y = y2 - y1;
    return std::atan2(y, x);
}

inline double dist(double x1, double y1, double x2, double y2) {
    double x = x2 - x1;
    double y = y2 - y1;
    return std::sqrt(x * x + y * y);
}

/*
template<class T>
inline const T& clamp(const T& x, const T& upper, const T& lower) {
    return std::min(upper, std::max(x, lower));
}
*/

inline rgb hsv2rgb(hsv HSV)
{
    rgb RGB;
    double H = HSV.h, S = HSV.s, V = HSV.v,
            P, Q, T,
            fract;

    (H == 360.)?(H = 0.):(H /= 60.);
    fract = H - floor(H);

    P = V*(1. - S);
    Q = V*(1. - S*fract);
    T = V*(1. - S*(1. - fract));

    if      (0. <= H && H < 1.)
        RGB = (rgb){.r = V, .g = T, .b = P};
    else if (1. <= H && H < 2.)
        RGB = (rgb){.r = Q, .g = V, .b = P};
    else if (2. <= H && H < 3.)
        RGB = (rgb){.r = P, .g = V, .b = T};
    else if (3. <= H && H < 4.)
        RGB = (rgb){.r = P, .g = Q, .b = V};
    else if (4. <= H && H < 5.)
        RGB = (rgb){.r = T, .g = P, .b = V};
    else if (5. <= H && H < 6.)
        RGB = (rgb){.r = V, .g = P, .b = Q};
    else
        RGB = (rgb){.r = 0., .g = 0., .b = 0.};

    return RGB;
}


const int primes_max = 10;
const int primes[primes_max][3] = {
    { 995615039, 600173719, 701464987 },
    { 831731269, 162318869, 136250887 },
    { 174329291, 946737083, 245679977 },
    { 362489573, 795918041, 350777237 },
    { 457025711, 880830799, 909678923 },
    { 787070341, 177340217, 593320781 },
    { 405493717, 291031019, 391950901 },
    { 458904767, 676625681, 424452397 },
    { 531736441, 939683957, 810651871 },
    { 997169939, 842027887, 423882827 }
};
static int num_octaves = 9;
static int prime_index = 3;
static double persistence = 0.65;

inline double noise(int i, int x, int y) {
    int n = x + y * 57;
    n = (n << 13) ^ n;
    int a = primes[i][0], b = primes[i][1], c = primes[i][2];
    int t = (n * (n * n * a + b) + c) & 0x7fffffff;
    return 1.0 - (double)(t)/1073741824.0;
}

inline double smoothed_noise(int i, int x, int y) {
    double corners = (noise(i, x-1, y-1) + noise(i, x+1, y-1) +
                        noise(i, x-1, y+1) + noise(i, x+1, y+1)) / 16,
            sides = (noise(i, x-1, y) + noise(i, x+1, y) + noise(i, x, y-1) +
                    noise(i, x, y+1)) / 8,
            center = noise(i, x, y) / 4;
    return corners + sides + center;
}

inline double interpolate(double a, double b, double x) {  // cosine interpolation
    double ft = x * 3.1415927,
            f = (1 - cos(ft)) * 0.5;
    return  a*(1-f) + b*f;
}

inline double interpolated_noise(int i, double x, double y) {
    int integer_X = x;
    double fractional_X = x - integer_X;
    int integer_Y = y;
    double fractional_Y = y - integer_Y;

    double v1 = smoothed_noise(i, integer_X, integer_Y),
            v2 = smoothed_noise(i, integer_X + 1, integer_Y),
            v3 = smoothed_noise(i, integer_X, integer_Y + 1),
            v4 = smoothed_noise(i, integer_X + 1, integer_Y + 1),
            i1 = interpolate(v1, v2, fractional_X),
            i2 = interpolate(v3, v4, fractional_X);
    return interpolate(i1, i2, fractional_Y);
}

inline double value_noise(double x, double y) {
    double total = 0,
            frequency = pow(2, num_octaves),
            amplitude = 1;
    for (int i = 0; i < num_octaves; ++i) {
        frequency /= 2;
        amplitude *= persistence;
        total += interpolated_noise((prime_index + i) % primes_max,
            x / frequency, y / frequency) * amplitude;
    }
    return total / frequency;
}

inline std::vector<std::vector<double>> noise_perlin(int width, int height, double seed, int noct, int pri, double per) {
    num_octaves = noct;
    prime_index = pri;
    persistence = per;

    std::vector<std::vector<double>> data;

    data.resize(height);
    for (int y = 0; y < height; y++) {
        data[y].resize(width);
        for (int x = 0; x < width; x++) {
            data[y][x] = per * 2 + value_noise(x / 5.0 + seed, y / 5.0 + seed) * 2;//(rand() % 2) ? 7 : 8;
            // std::cout << "[" << data[y][x] << "]";

        }
    }

    return data;
}

#endif // MATH_H