
#pragma once

namespace danasim {

    struct GridMetadata {
        int width;
        int height;
        int cellCount;

        float cellSize;

        double minX;
        double maxX;
        double minY;
        double maxY;

        std::string crs;
        std::string units;
        float unitDist;
    };
}