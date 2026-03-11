
#pragma once

#include "core/grid/Layer.hpp"
#include "adapters/input/readers/StaticReader.hpp"

#include <string>
#include <memory>
#include <chrono>
#include <stdexcept>

namespace danasim {

	template <typename T>
	class StaticLayer : public Layer<T> {
	public:
		StaticLayer(std::string name, LayerRole role);
		virtual ~StaticLayer() = default;

		inline bool isStatic() const { return true; };
		inline bool isDerived() const { return false; };

		void setReader(std::unique_ptr<Reader> reader, std::chrono::system_clock::time_point currentTime);

		void update(std::chrono::system_clock::time_point currentTime);
	};


    template <typename T>
    StaticLayer<T>::StaticLayer(std::string name, LayerRole role)
        : Layer<T>(name, role)
    {

    }

    template <typename T>
    void StaticLayer<T>::setReader(std::unique_ptr<Reader> reader, std::chrono::system_clock::time_point /* currentTime */) {
        reader->read(this->data_);
    }

    template <typename T>
    void StaticLayer<T>::update(std::chrono::system_clock::time_point /* currentTime */) {
        
    }

} // namespace danasim