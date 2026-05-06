
#pragma once

#include <iostream>
#include <string>
#include <sstream>

namespace danasim {

	class ScalarBase {
	public:
		virtual ~ScalarBase() = default;

		virtual std::string getName() const = 0;

		virtual void setValue(const std::string& s) = 0;
	};

	template <typename T>
	class Scalar : public ScalarBase {
	public:
		Scalar(std::string name);
		virtual ~Scalar() = default;

		// Implementamos los métodos virtuales de la base
		inline std::string getName() const override { return name_; }

		void setValue(const T& value);
		void setValue(const std::string& s);

		// ESTE ES EL ÚNICO MÉTODO QUE DEPENDE DE 'T'
		inline T& getValue() { return value_; }
		inline const T& getValue() const { return value_; }

		inline bool isSetted() const { return setted_; }

	protected:
		std::string name_;
		bool setted_;
		T value_;
	};


	template <typename T>
	Scalar<T>::Scalar(std::string name)
		: name_(name), setted_(false)
	{

	}

	template <typename T>
	void Scalar<T>::setValue(const T& value) 
	{ 
		value_ = value;
		setted_ = true;
	}

	template <typename T>
	void Scalar<T>::setValue(const std::string& s) {
		std::stringstream ss(s);

		if (!(ss >> value_)) {
			// Manejo de error si la conversión falla (ej. s="hola" y T=int)
			throw std::runtime_error("Error de conversión: " + s);
		}

		setted_ = true;
	}

} // namespace danasim