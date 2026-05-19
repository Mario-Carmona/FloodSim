
#pragma once

#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include <omp.h>

#include "app/core/grid/LayerTypes.hpp"
#include "app/io/readers/Reader.hpp"

namespace danasim {

	struct Tile {
		int64_t core_start_x;
		int64_t core_start_y;
		int64_t core_width;   // Puede ser < 128 en los bordes del mapa
		int64_t core_height;  // Puede ser < 128 en los bordes del mapa
	};

	struct BoundingBox {
		int64_t minX;
		int64_t minY;
		int64_t maxX;
		int64_t maxY;
	};

	class ScratchpadBase {
	public:
		virtual ~ScratchpadBase() = default;

		virtual void resize(size_t newSize) = 0;

		virtual size_t size() const = 0;
	};

	template <typename T>
	class Scratchpad : public ScratchpadBase {
	public:
		Scratchpad() = default;
		virtual ~Scratchpad() = default;

		std::vector<T>& getData() { return data_; }
		const std::vector<T>& getData() const { return data_; }

		void resize(size_t newSize) { data_.resize(newSize); };

		size_t size() const { return data_.size(); };

	protected:
		std::vector<T> data_;
	};



	// 1. LA NUEVA INTERFAZ BASE
	class LayerBase {
	public:
		virtual ~LayerBase() = default;

		virtual std::string getName() const = 0;

		virtual bool isStatic() const = 0;

		virtual std::unique_ptr<ScratchpadBase> generateScratchpad() const = 0;

		virtual void extractTilesData(ScratchpadBase* const scratchpadBase, const std::vector<Tile>& activeTiles, GridIndexType gridCols, GridIndexType gridRows,
			int64_t haloSize, int64_t tensorWidth, int64_t tensorHeight) const = 0;
		virtual void updateTilesData(const ScratchpadBase* const scratchpadBase, const std::vector<Tile>& activeTiles, GridIndexType gridCols, int64_t haloSize, int64_t tensorWidth, int64_t tensorHeight) = 0;

		virtual void clear() = 0;

		virtual void resize(size_t newSize) = 0;

		virtual void setReader(const GridMetadata& mainMetadata, std::unique_ptr<Reader> reader, std::chrono::system_clock::time_point currentTime) = 0;
		virtual void update(std::chrono::system_clock::time_point currentTime) = 0;
	};

	// 2. LA CLASE TEMPLATIZADA HEREDA DE LA BASE
	template <typename T>
	class Layer : public LayerBase {
	public:
		Layer(std::string name);
		virtual ~Layer() = default;

		// Implementamos los métodos virtuales de la base
		inline std::string getName() const override { return name_; };

		std::unique_ptr<ScratchpadBase> generateScratchpad() const { return std::make_unique<Scratchpad<T>>(); }

		void extractTilesData(ScratchpadBase* const scratchpadBase, const std::vector<Tile>& activeTiles, GridIndexType gridCols, GridIndexType gridRows,
			int64_t haloSize, int64_t tensorWidth, int64_t tensorHeight) const;
		void updateTilesData(const ScratchpadBase* const scratchpadBase, const std::vector<Tile>& activeTiles, GridIndexType gridCols, int64_t haloSize, int64_t tensorWidth, int64_t tensorHeight);

		// ESTE ES EL ÚNICO MÉTODO QUE DEPENDE DE 'T'
		std::vector<T>& getData() { return data_; }
		const std::vector<T>& getData() const { return data_; }

		void clear() override { data_.clear(); };

		void resize(size_t newSize) override { data_.resize(newSize); };

	protected:
		std::string name_;
		std::vector<T> data_;
	};


	template <typename T>
	Layer<T>::Layer(std::string name)
		: name_(name)
	{

	}

	template <typename T>
	void Layer<T>::extractTilesData(ScratchpadBase* const scratchpadBase, const std::vector<Tile>& activeTiles, GridIndexType gridCols, GridIndexType gridRows,
			int64_t haloSize, int64_t tensorWidth, int64_t tensorHeight) const {
		
		auto start = std::chrono::high_resolution_clock::now();

		// 1. Asignación robusta y segura por tipos (Type-Safe) del valor de padding
		T padValue;
		if constexpr (std::is_same_v<T, float>) {
			if (name_ == "topo_bathy") {
				padValue = 9999.0f; // Muro virtual para que el agua no escape del mapa
			}
			else {
				padValue = 0.0f;    // water_depth, rainfall, etc.
			}
		}
		else if constexpr (std::is_same_v<T, int8_t>) {
			padValue = static_cast<int8_t>(0); // cell_state, land_cover
		}
		else {
			padValue = static_cast<T>(0);
		}

		Scratchpad<T>* const scratchpad = dynamic_cast<Scratchpad<T>*const>(scratchpadBase);

		T* dest_base_ptr = scratchpad->getData().data();
		const T* src_base_ptr = data_.data();

		#pragma omp parallel for
		for (int64_t b = 0; b < static_cast<int64_t>(activeTiles.size()); ++b) {
			const auto& tile = activeTiles[b];
			T* batch_dest_ptr = dest_base_ptr + (b * tensorWidth * tensorHeight);

			// 1. Rellenar todo el tensor 130x130 con el valor por defecto (Padding)
			std::fill(batch_dest_ptr, batch_dest_ptr + (tensorWidth * tensorHeight), padValue);

			// 2. Calcular los límites globales reales (evitando salir del mapa)
			int64_t start_global_x = tile.core_start_x - haloSize;
			int64_t start_global_y = tile.core_start_y - haloSize;

			int64_t valid_min_x = std::max(int64_t(0), start_global_x);
			int64_t valid_max_x = std::min((int64_t)gridCols, tile.core_start_x + tile.core_width + haloSize);

			int64_t valid_min_y = std::max(int64_t(0), start_global_y);
			int64_t valid_max_y = std::min((int64_t)gridRows, tile.core_start_y + tile.core_height + haloSize);

			int64_t valid_width = valid_max_x - valid_min_x;


			for (int64_t gy = valid_min_y; gy < valid_max_y; ++gy) {
				int64_t local_y = gy - start_global_y;
				int64_t local_x = valid_min_x - start_global_x;

				int64_t global_offset = gy * gridCols + valid_min_x;
				int64_t local_offset = local_y * tensorWidth + local_x;

				std::memcpy(batch_dest_ptr + local_offset, src_base_ptr + global_offset, valid_width * sizeof(T));
			}
		}

		auto end = std::chrono::high_resolution_clock::now();
		LOG_DEBUG("extractTilesData time: {}s", std::chrono::duration<double>(end - start).count());
	}

	template <typename T>
	void Layer<T>::updateTilesData(const ScratchpadBase* const scratchpadBase, const std::vector<Tile>& activeTiles, GridIndexType gridCols, int64_t haloSize, int64_t tensorWidth, int64_t tensorHeight) {
		auto start = std::chrono::high_resolution_clock::now();

		const Scratchpad<T>* const scratchpad = dynamic_cast<const Scratchpad<T>*const>(scratchpadBase);

		T* dest_base_ptr = data_.data();
		const T* src_base_ptr = scratchpad->getData().data();

		#pragma omp parallel for
		for (int64_t b = 0; b < static_cast<int64_t>(activeTiles.size()); ++b) {
			const auto& tile = activeTiles[b];
			const T* batch_src_ptr = src_base_ptr + (b * tensorWidth * tensorHeight);

			// ATENCIÓN: Solo iteramos sobre el CORE. Ignoramos el Halo por completo.
			for (int64_t core_y = 0; core_y < tile.core_height; ++core_y) {

				int64_t global_y = tile.core_start_y + core_y;
				int64_t local_y = core_y + haloSize; // Saltamos el halo superior

				int64_t globalOffset = (global_y * gridCols) + tile.core_start_x;
				int64_t localOffset = (local_y * tensorWidth) + haloSize; // Saltamos el halo izquierdo

				std::memcpy(dest_base_ptr + globalOffset, batch_src_ptr + localOffset, tile.core_width * sizeof(T));
			}
		}

		auto end = std::chrono::high_resolution_clock::now();
		LOG_DEBUG("updateTilesData time: {}s", std::chrono::duration<double>(end - start).count());
	}

} // namespace danasim