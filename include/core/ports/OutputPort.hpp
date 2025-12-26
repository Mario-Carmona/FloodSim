
#pragma once

#include <string>

#include "core/snapshot/SnapshotManager.hpp"

namespace danasim {

    /**
     * @brief Interface for simulation output consumers.
     */
    class OutputPort {
    public:
        virtual ~OutputPort() = default;

        /**
         * @brief Runs the output consumer loop.
         * This method is expected to be executed in its own thread.
         */
        virtual void run(SnapshotManager& snapshotManager) = 0;

        virtual std::string getThreadName() const = 0;
    };

} // namespace danasim
