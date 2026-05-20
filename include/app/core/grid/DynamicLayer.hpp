
#pragma once

#include <iostream>
#include <chrono>
#include <sstream>
#include <string>

#include "app/core/grid/Layer.hpp"
#include "app/io/readers/DynamicReader.hpp"
#include "Types.hpp"

namespace danasim {

	template <typename T>
	class DynamicLayer : public Layer<T> {
	public:
		DynamicLayer(std::string name);
		virtual ~DynamicLayer() = default;

		inline bool isStatic() const { return false; };

		void setReader(const GridMetadata& mainMetadata, std::unique_ptr<Reader> reader, std::chrono::system_clock::time_point currentTime);

		void update(std::chrono::system_clock::time_point currentTime);

	protected:
		std::unique_ptr<DynamicReader> reader_;
		std::vector<T> buffer_;
		std::vector<FlatVectorIndexType> indexMap_;
		std::vector<bool> insideMap_;
	};


    template <typename T>
    DynamicLayer<T>::DynamicLayer(std::string name)
        : Layer<T>(name)
    {

    }

    template <typename T>
    void DynamicLayer<T>::setReader(const GridMetadata& mainMetadata, std::unique_ptr<Reader> reader, std::chrono::system_clock::time_point currentTime) {
        this->data_.assign(mainMetadata.cellCount, T{});
        
        reader_ = std::unique_ptr<DynamicReader>(static_cast<DynamicReader*>(reader.release()));

        reader_->update(currentTime);

        unsigned int downgradeFactor = reader_->getDowngradeFactor();

        if (downgradeFactor > 1) {
            GridMetadata metadata = reader_->readMetadata();

            indexMap_.resize(mainMetadata.cellCount);
            insideMap_.resize(mainMetadata.cellCount, false);

            // Iterate over every cell in the Simulation Grid
            for (GridIndexType r = 0; r < mainMetadata.height; ++r) {
                for (GridIndexType c = 0; c < mainMetadata.width; ++c) {
                    // Map Sim(r,c) -> HDF5(hr, hc) using integer division (Downscaling)
                    GridIndexType hr = r / downgradeFactor;
                    GridIndexType hc = c / downgradeFactor;

                    // Boundary Check:
                    // Ensure the calculated HDF5 coordinate is valid.
                    if (hr < metadata.height && hc < metadata.width) {
                        // Store the HDF5 index in the map
                        FlatVectorIndexType idx = r * mainMetadata.width + c;
                        indexMap_[idx] = hr * metadata.width + hc;
                        insideMap_[idx] = true;
                    }
                }
            }

            buffer_.resize(metadata.cellCount, T{});

            reader_->read(buffer_);

            for (FlatVectorIndexType i = 0; i < insideMap_.size(); ++i) {
                if (insideMap_[i]) {
                    // Map the value from the Coarse Buffer -> Fine Cell
                    this->data_[i] = buffer_[indexMap_[i]];
                }
                else {
                    // Cell is outside the coverage area
                    this->data_[i] = T{};
                }
            }
        }
        else {
            reader_->read(this->data_);
        }
    }

    template <typename T>
    void DynamicLayer<T>::update(std::chrono::system_clock::time_point currentTime) {
        bool updated = reader_->update(currentTime);

        if (updated) {
            if (reader_->getDowngradeFactor() > 1) {
                reader_->read(buffer_);

                for (FlatVectorIndexType i = 0; i < insideMap_.size(); ++i) {
                    if (insideMap_[i]) {
                        // Map the value from the Coarse Buffer -> Fine Cell
                        this->data_[i] = buffer_[indexMap_[i]];
                    }
                    else {
                        // Cell is outside the coverage area
                        this->data_[i] = T{};
                    }
                }
            }
            else {
                reader_->read(this->data_);
            }
        }
    }

} // namespace danasim