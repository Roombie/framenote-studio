#pragma once

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace Framenote {
namespace ShapeRasterizer {

template <typename PlotFn>
void rasterizeLine(int x0, int y0, int x1, int y1, PlotFn&& plot) {
    int dx = std::abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;

    int dy = -std::abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;

    int err = dx + dy;

    while (true) {
        plot(x0, y0);

        if (x0 == x1 && y0 == y1)
            break;

        int e2 = 2 * err;

        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }

        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

template <typename PlotFn>
void rasterizeEllipse(int x0, int y0, int x1, int y1, bool filled, PlotFn&& plot) {
    if (x0 > x1)
        std::swap(x0, x1);

    if (y0 > y1)
        std::swap(y0, y1);

    int width  = x1 - x0 + 1;
    int height = y1 - y0 + 1;

    if (width <= 0 || height <= 0)
        return;

    if (width == 1 && height == 1) {
        plot(x0, y0);
        return;
    }

    if (width == 1) {
        rasterizeLine(x0, y0, x1, y1, plot);
        return;
    }

    if (height == 1) {
        rasterizeLine(x0, y0, x1, y1, plot);
        return;
    }

    const double cx = (static_cast<double>(x0) + static_cast<double>(x1)) * 0.5;
    const double cy = (static_cast<double>(y0) + static_cast<double>(y1)) * 0.5;

    const double rx = static_cast<double>(width) * 0.5;
    const double ry = static_cast<double>(height) * 0.5;

    const double rx2 = rx * rx;
    const double ry2 = ry * ry;

    auto inside = [&](int px, int py) -> bool {
        double dx = static_cast<double>(px) - cx;
        double dy = static_cast<double>(py) - cy;

        double v = (dx * dx) / rx2 + (dy * dy) / ry2;
        return v <= 1.0;
    };

    for (int py = y0; py <= y1; ++py) {
        for (int px = x0; px <= x1; ++px) {
            if (!inside(px, py))
                continue;

            if (filled) {
                plot(px, py);
                continue;
            }

            bool edge =
                !inside(px - 1, py) ||
                !inside(px + 1, py) ||
                !inside(px, py - 1) ||
                !inside(px, py + 1);

            if (edge)
                plot(px, py);
        }
    }
}

} // namespace ShapeRasterizer
} // namespace Framenote