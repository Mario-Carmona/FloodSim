
#pragma once

#include <vector>
#include <string>
#include <memory>
#include <chrono>

#include "core/grid/LayerTypes.hpp"
#include "adapters/input/readers/Reader.hpp"

namespace danasim {

	// 1. LA NUEVA INTERFAZ BASE
	class LayerBase {
	public:
		virtual ~LayerBase() = default;

		virtual std::string getName() const = 0;
		virtual LayerRole getRole() const = 0;
		virtual GridMetadata getMetadata() const = 0;

		virtual bool isStatic() const = 0;
		virtual bool isDerived() const = 0;

		virtual void init(GridMetadata metadata) = 0;
		virtual void setReader(std::unique_ptr<Reader> reader, std::chrono::system_clock::time_point currentTime) = 0;
		virtual void update(std::chrono::system_clock::time_point currentTime) = 0;
	};

	// 2. LA CLASE TEMPLATIZADA HEREDA DE LA BASE
	template <typename T>
	class Layer : public LayerBase {
	public:
		Layer(std::string name, LayerRole role);
		virtual ~Layer() = default;

		// Implementamos los métodos virtuales de la base
		void init(GridMetadata metadata) override;
		inline std::string getName() const override { return name_; };
		inline LayerRole getRole() const override { return role_; };
		inline GridMetadata getMetadata() const override { return gridMetadata_; };

		// Mantenemos los virtuales puros para StaticLayer/DynamicLayer
		virtual bool isStatic() const override = 0;
		virtual bool isDerived() const override = 0;
		virtual void setReader(std::unique_ptr<Reader> reader, std::chrono::system_clock::time_point currentTime) override = 0;
		virtual void update(std::chrono::system_clock::time_point currentTime) override = 0;

		// ESTE ES EL ÚNICO MÉTODO QUE DEPENDE DE 'T'
		std::vector<T>& getData() { return data_; }
		const std::vector<T>& getData() const { return data_; }

	protected:
		GridMetadata gridMetadata_;
		std::string name_;
		LayerRole role_;
		std::vector<T> data_;
	};


	template <typename T>
	Layer<T>::Layer(std::string name, LayerRole role)
		: name_(name), role_(role)
	{

	}

	template <typename T>
	void Layer<T>::init(GridMetadata metadata) {
		gridMetadata_ = metadata;

		data_.assign(gridMetadata_.cellCount, T{});
	}

} // namespace danasim