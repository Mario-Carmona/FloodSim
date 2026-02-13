
#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <opencv2/opencv.hpp>

#include "core/ports/OutputPort.hpp"

namespace danasim {

    /**
     * @brief Publishes snapshots to an MQTT broker.
     */
    class ImageOutput : public OutputPort {
    public:
        struct Config {
            std::string units;
            float minVal;       // Valor mínimo FIJO (para consistencia entre frames)
            float maxVal;       // Valor máximo FIJO
            int colormap;
            bool doHillshade;  // Solo para relieve
            bool maskZero;
        };

        ImageOutput();

        ~ImageOutput();

        void run(SnapshotManager& snapshotManager, const std::filesystem::path& outputPath) override;

        void setGrid(const MapGrid& grid) override;

        std::string getThreadName() const override { return "Out_Image"; }

    private:
        int rows_;
        int cols_;

        const float* elevation_;


        void saveSnapshotAsImage(const Snapshot& snapshot, const ChangeList& changes, const std::filesystem::path& imagesOutputPath);

        cv::Mat createTerrainBackground(const Snapshot& snapshot, bool useColorMap);

        void drawUI(cv::Mat& canvas, const Snapshot& snapshot, double baseScale, int thickness, int marginTitle, int sidebarWidth);
        
        std::unordered_map<LayerId, Config> layerConfigs_;
    };

} // namespace danasim
