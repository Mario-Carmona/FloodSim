
#include "core/snapshot/Snapshot.hpp"

#include "logging/Logger.hpp"

namespace danasim {

    void Snapshot::set(StepType step, float time, const MapGrid& grid) {
        step_ = step;
        time_ = time;

        const auto& waterDepthGrid = grid.getLayer<float>(LayerId::WaterDepth);

        if (waterDepth_.empty()) {
            waterDepth_.resize(waterDepthGrid.size(), 0.0f);
        }

        std::memcpy(waterDepth_.data(), waterDepthGrid.data(), waterDepthGrid.size() * sizeof(float));
    }

    void Snapshot::set(size_t size) {
        step_ = 0;
        time_ = 0.0f;
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
        swap(this->step_, other.step_);
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
