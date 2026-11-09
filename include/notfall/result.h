// SPDX-License-Identifier: Apache-2.0

#ifndef NOTFALL_RESULT_H_
#define NOTFALL_RESULT_H_

#include <type_traits>
#include <memory>
#include <stdexcept>
#include <utility>

namespace notfall {

template <typename T>
class Some {
 public:
  explicit constexpr Some(const T& value) noexcept(std::is_nothrow_copy_constructible_v<T>)
      : value_(value) {}

  explicit constexpr Some(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>)
      : value_(std::move(value)) {}

 private:
  template <typename T2, typename E> friend class Result;

  T value_;
};

template <typename E>
class Error {
 public:
  explicit constexpr Error(const E& error) noexcept(std::is_nothrow_copy_constructible_v<E>)
      : error_(error) {}

  explicit constexpr Error(E&& error) noexcept(std::is_nothrow_move_constructible_v<E>)
      : error_(std::move(error)) {}

 private:
  template <typename T, typename E2> friend class Result;

  E error_;
};

template <typename T, typename E>
class Result {
 public:
  static_assert(std::is_destructible_v<T> && std::is_destructible_v<E>,
      "T and E both must be destructible types!");

  static_assert(!std::is_void_v<T> && !std::is_void_v<E>, "Neither T or E should be void!");
  static_assert(!std::is_reference_v<T> && !std::is_reference_v<E>,
      "Neither T or E should be a reference!");

  constexpr Result() noexcept(std::is_nothrow_default_constructible_v<T>) 
      requires std::is_default_constructible_v<T> {
    std::construct_at(std::addressof(value_)); // Default construction
    state_ = State::kValue;
  }

  template <typename T2 = T>
  constexpr Result(Some<T2>&& value) noexcept(std::is_nothrow_constructible_v<T, T2&&>) {
    std::construct_at(std::addressof(value_), std::forward<T2>(value.value_));
    state_ = State::kValue;
  }

  template <typename E2 = E>
  constexpr Result(Error<E2>&& error) noexcept(std::is_nothrow_constructible_v<E, E2&&>) {
    std::construct_at(std::addressof(error_), std::forward<E2>(error.error_));
    state_ = State::kError;
  }

  constexpr Result(const Result<T, E>& other)
      noexcept(std::is_nothrow_copy_constructible_v<T> &&
               std::is_nothrow_copy_constructible_v<E>) {
    if (other.state_ == State::kValue) {
      std::construct_at(std::addressof(value_), other.Value());
    } else if (other.state_ == State::kError) {
      std::construct_at(std::addressof(error_), other.Error());
    }

    state_ = other.state_;
  }

  constexpr Result(Result<T, E>&& other)
      noexcept(std::is_nothrow_move_constructible_v<T> &&
               std::is_nothrow_move_constructible_v<E>) {
    if (other.state_ == State::kValue) {
      std::construct_at(std::addressof(value_), std::move(other.Value()));
    } else if (other.state_ == State::kError) {
      std::construct_at(std::addressof(error_), std::move(other.Error()));
    }

    state_ = other.state_;
    other.state_ = State::kEmpty;
  }

  constexpr ~Result() { Destroy(); }

  template <typename T2 = T>
  constexpr Result<T, E>& operator=(Some<T2>&& value)
      noexcept(std::is_nothrow_constructible_v<T, T2&&>) {
    Destroy();
    std::construct_at(std::addressof(value_), std::forward<T2>(value.value_));
    state_ = State::kValue;
    return *this;
  }
  
  template <typename E2 = E>
  constexpr Result<T, E>& operator=(Error<E2>&& error)
      noexcept(std::is_nothrow_constructible_v<E, E2&&>) {
    Destroy();
    std::construct_at(std::addressof(error_), std::forward<E2>(error.error_));
    state_ = State::kError;
    return *this;
  }

  constexpr Result<T, E>& operator=(const Result<T, E>& other)
      noexcept(std::is_nothrow_copy_constructible_v<T> &&
               std::is_nothrow_copy_constructible_v<E> &&
               std::is_nothrow_copy_assignable_v<T> &&
               std::is_nothrow_copy_assignable_v<E>) {
    if (this != std::addressof(other)) {
      if (other.state_ == State::kValue && state_ == State::kValue) {
        value_ = other.Value();
      } else if (other.state_ == State::kError && state_ == State::kError) {
        error_ = other.Error();
      } else {
        Destroy();
        if (other.state_ == State::kValue) {
          std::construct_at(std::addressof(value_), other.Value());
        } else if (other.state_ == State::kError) {
          std::construct_at(std::addressof(error_), other.Error());
        }

        state_ = other.state_;
      }
    }

    return *this;
  }

  constexpr Result<T, E>& operator=(Result<T, E>&& other)
      noexcept(std::is_nothrow_move_constructible_v<T> &&
               std::is_nothrow_move_constructible_v<E>) {
    if (this != std::addressof(other)) {
      Destroy();
      if (other.state_ == State::kValue) {
        std::construct_at(std::addressof(value_), std::move(other.Value()));
      } else if (other.state_ == State::kError) {
        std::construct_at(std::addressof(error_), std::move(other.Error()));
      }

      state_ = other.state_;
      other.state_ = State::kEmpty;
    }

    return *this;
  }

  constexpr bool HasValue() const noexcept { return state_ == State::kValue; }

  constexpr bool HasError() const noexcept { return state_ == State::kError; }

  constexpr explicit operator bool() const noexcept { return HasValue(); }

  constexpr T& Value() & {
    if (state_ != State::kValue) {
      throw std::runtime_error("Result<T, E> value access violation");
    }

    return *std::launder(std::addressof(value_));
  }

  constexpr const T& Value() const & {
    if (state_ != State::kValue) {
      throw std::runtime_error("Result<T, E> value access violation");
    }

    return *std::launder(std::addressof(value_));
  }

  constexpr T&& Value() && {
    if (state_ != State::kValue) {
      throw std::runtime_error("Result<T, E> value access violation");
    }

    return std::move(*std::launder(std::addressof(value_)));
  }

  constexpr const T&& Value() const && {
    if (state_ != State::kValue) {
      throw std::runtime_error("Result<T, E> value access violation");
    }

    return std::move(*std::launder(std::addressof(value_)));
  }

  constexpr T* operator->() { return std::addressof(Value()); }

  constexpr const T* operator->() const { return std::addressof(Value()); }

  constexpr T& operator*() & { return Value(); }

  constexpr const T& operator*() const & { return Value(); }

  constexpr T&& operator*() && { return std::move(*this).Value(); }

  constexpr const T&& operator*() const && {
    return std::move(*this).Value();
  }

  constexpr E& Error() & {
    if (state_ != State::kError) {
      throw std::runtime_error("Result<T, E> error value access violation");
    }

    return *std::launder(std::addressof(error_));
  }

  constexpr const E& Error() const & {
    if (state_ != State::kError) {
      throw std::runtime_error("Result<T, E> error value access violation");
    }

    return *std::launder(std::addressof(error_));
  }

  constexpr E&& Error() && {
    if (state_ != State::kError) {
      throw std::runtime_error("Result<T, E> error value access violation");
    }

    return std::move(*std::launder(std::addressof(error_)));
  }

  constexpr const E&& Error() const && {
    if (state_ != State::kError) {
      throw std::runtime_error("Result<T, E> error value access violation");
    }

    return std::move(*std::launder(std::addressof(error_)));
  }

 private:
  enum class State {
    kEmpty,
    kValue,
    kError
  };
  
  constexpr void Destroy() {
    if (state_ == State::kValue) {
      std::destroy_at(std::addressof(value_));
    } else if (state_ == State::kError) {
      std::destroy_at(std::addressof(error_));
    }

    state_ = State::kEmpty;
  }

  State state_ = State::kEmpty;
  union {
    T value_;
    E error_;
  };
};

}  // namespace notfall

#endif  // NOTFALL_RESULT_H_
