/**
 * @file InputPort.hpp
 * @brief Interface for data ingestion strategies.
 *
 * @version 1.0
 * @date 2026
 * @copyright Copyright (c) 2026 Danasim
 */

#pragma once

#include <memory>
#include <concepts>

#include "app/adapters/input/readers/Reader.hpp"

namespace danasim {

    /**
     * @interface InputPort
     * @brief Abstract interface for initializing the simulation domain.
     *
     * Implementations of this class are responsible for populating the static
     * and dynamic layers of the MapGrid from external sources (Files, APIs, Generators).
     */
    class InputPort {
    public:
        virtual ~InputPort() = default;

        virtual std::unique_ptr<Reader> generateReader(std::string name, bool isStatic) const = 0;

        virtual bool isStaticLayer(const std::string& name) const = 0;

        virtual const std::string& getDatasetName() const = 0;
    };

} // namespace danasim
