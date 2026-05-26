/**
 * @file Scalar.hpp
 * @brief Defines the base and templated scalar types for simulation primitives.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace floodsim {

/**
 * @class ScalarBase
 * @brief Abstract base class for all scalar configurations or variables.
 *
 * Provides a type-erased interface for retrieving the name and parsing
 * values from their string representations.
 */
class ScalarBase {
public:
	virtual ~ScalarBase() = default;

	/**
	 * @brief Retrieves the name of the scalar.
	 *
	 * @return std::string The scalar's identifier.
	 */
	[[nodiscard]] virtual std::string GetName() const = 0;

	/**
	 * @brief Parses and sets the scalar value from a string.
	 *
	 * @param s The string representation of the value.
	 */
	virtual void SetValue(const std::string& s) = 0;
};

/**
 * @class Scalar
 * @brief Templated scalar variable representing a typed primitive.
 *
 * @tparam T The underlying data type of the scalar (e.g., int, float, double).
 */
template <typename T>
class Scalar : public ScalarBase {
public:
	/**
	 * @brief Constructs a new Scalar with a given name.
	 *
	 * @param name The identifier for this scalar.
	 */
	explicit Scalar(std::string name);

	virtual ~Scalar() override = default;

	/**
	 * @brief Retrieves the name of the scalar.
	 *
	 * @return std::string The identifier string.
	 */
	[[nodiscard]] inline std::string GetName() const override { return name_; }

	/**
	 * @brief Sets the scalar to a directly typed value.
	 *
	 * @param value The value to assign.
	 */
	void SetValue(const T& value);

	/**
	 * @brief Parses a string to set the scalar's value.
	 *
	 * @param s The string representation to parse.
	 */
	void SetValue(const std::string& s) override;

	/**
	 * @brief Retrieves a mutable reference to the underlying value.
	 *
	 * @return T& Reference to the stored data.
	 */
	[[nodiscard]] inline T& GetValue() noexcept { return value_; }

	/**
	 * @brief Retrieves a read-only reference to the underlying value.
	 *
	 * @return const T& Const reference to the stored data.
	 */
	[[nodiscard]] inline const T& GetValue() const noexcept { return value_; }

	/**
	 * @brief Checks if the scalar has been assigned a value.
	 *
	 * @return true If a value was set.
	 * @return false If it remains uninitialized.
	 */
	[[nodiscard]] inline bool IsSet() const noexcept { return is_set_; }

protected:
	std::string name_;  ///< Identifier of the scalar.
	bool is_set_;       ///< Flag indicating if the value has been initialized.
	T value_;           ///< The actual data value.
};

// -----------------------------------------------------------------------------
// Template Implementations
// -----------------------------------------------------------------------------

template <typename T>
Scalar<T>::Scalar(std::string name)
	: name_(std::move(name)),
	is_set_(false),
	value_{} {}

template <typename T>
void Scalar<T>::SetValue(const T& value) {
	value_ = value;
	is_set_ = true;
}

template <typename T>
void Scalar<T>::SetValue(const std::string& s) {
	std::stringstream ss(s);
	if (ss >> value_) {
		is_set_ = true;
	}
	else {
		throw std::runtime_error("Scalar: Failed to parse value from string '" + s + "' for scalar '" + name_ + "'.");
	}
}

} // namespace floodsim