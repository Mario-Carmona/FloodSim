
#include "core/snapshot/Snapshot.hpp"

#include "logging/Logger.hpp"

namespace danasim {

    void Snapshot::set(std::chrono::sys_seconds time, const MapGrid& grid) {
        time_ = time;

        const auto& waterDepthGrid = grid.getLayer<float>("water_depth")->getData();

        if (waterDepth_.empty()) {
            waterDepth_.resize(waterDepthGrid.size(), 0.0f);
        }

        std::memcpy(waterDepth_.data(), waterDepthGrid.data(), waterDepthGrid.size() * sizeof(float));
    }

    void Snapshot::set(size_t size) {
        time_ = std::chrono::sys_seconds::min();
        waterDepth_.resize(size, 0.0f);
    }

    void Snapshot::swap(Snapshot& other) noexcept {
        if (this->waterDepth_.empty() && other.waterDepth_.empty()) {
            LOG_ERROR("Ambos snapshots vacios.");
        }

        if (this->waterDepth_.empty()) {
            this->waterDepth_.resize(other.waterDepth_.size(), 0.0f);
        }

        if (other.waterDepth_.empty()) {
            other.waterDepth_.resize(this->waterDepth_.size(), 0.0f);
        }

        using std::swap;
        swap(this->time_, other.time_);
        swap(this->waterDepth_, other.waterDepth_);
    }

    size_t Snapshot::size() const noexcept {
        return waterDepth_.size();
    }

    float* Snapshot::data() noexcept {
        return waterDepth_.data();
    }

} // namespace danasim

namespace std {
    template<> void swap(danasim::Snapshot& a, danasim::Snapshot& b) noexcept {
        a.swap(b);
    }
}
