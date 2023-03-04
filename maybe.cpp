#include <coroutine>
#include <iostream>
#include <optional>

using std::cout;
using std::endl;

template <typename T>
struct maybe : public std::optional<T> {
  using Base = std::optional<T>;
  using Base::Base;  //  Inheriting constructors. Only initialize Base part.
  // https://en.cppreference.com/w/cpp/language/using_declaration

  explicit maybe(maybe*& p) { p = this; }

  // WARNING:
  // If this destructor isn't written explicitly,
  // CLANG (15.0.6) will return a wrong result (std::nullopt).
  // "= default" in CLANG cannot solve this problem.
  ~maybe() {}

  struct promise_type {
    auto initial_suspend() noexcept { return std::suspend_never{}; }

    auto final_suspend() noexcept {
      cout << "final_suspend " << res_->has_value() << " ";
      cout << res_->value_or(-1) << endl;
      return std::suspend_never{};
    }

    void unhandled_exception() {}

    maybe<T> get_return_object() {
      return maybe<T>{res_};

      // CANNOT write below:
      // maybe<T> r{res_};
      // cout << "get_return_object: after " << res_ << endl;
      // return r;

      // NOT "return handle::from_promise(*this);"
    }

    template <typename U>
    void return_value(U&& value) {
      // Use in co_return, which influences get_return_object()
      // by modifying res_.
      cout << "return_value with arg " << value << endl;
      // cout << res_ << endl;
      res_->emplace(std::forward<U>(value));
      cout << "res_->value() " << res_->value() << endl;
    }

    auto yield_value(maybe<T> opt) {
      struct Awaiter {
        explicit Awaiter(maybe<T>&& o) : opt_(std::forward<maybe<T>>(o)) {
          cout << "Construct awaiter with " << opt_.value_or(-1) << endl;
        }

        bool await_ready() {
          cout << "await_ready " << opt_.has_value() << endl;
          return opt_.has_value();
        }

        decltype(auto) await_resume() {
          // If await_ready() is true (opt_ has value), return opt_'s value.
          return opt_.value();  // or "return *opt_;"

          // await_resume() isn't void, so "auto a = co_yield XXX;" is valid.
        }

        void await_suspend(std::coroutine_handle<> handle) {
          // If await_ready() is false (opt is null), destroy the coroutine.
          handle.destroy();

          // 'await_suspend' must return 'void', 'bool' or a coroutine handle
        }

        maybe<T> opt_;
      };

      // Send the value to be yield into awaiter,
      // and let awaiter decide to return the value or destroy.
      return Awaiter{std::move(opt)};
      // return Awaiter(std::move(opt));
    }

   private:
    maybe<T>* res_{};
  };
};

/// OR:
/// https://en.cppreference.com/w/cpp/coroutine/coroutine_traits
// template <typename T, typename... Args>
// struct std::coroutine_traits<maybe<T>, Args...> {
//   struct promise_type {
//     ...
//   };
// };

maybe<int> get_odd(int i) {
  if (i % 2 == 1) {
    return i;
  } else {
    return std::nullopt;
    // Cannot co_return std::nullopt because of 'emplace' function.
    // co_return 1 is OK.
    // co_yield std::nullopt is OK.
    // co_yield 1 is OK.
  }
}

maybe<int> compute(int a, int b) {
  int x = co_yield get_odd(a);
  cout << "a is valid." << endl;
  int y = co_yield get_odd(b);
  cout << "b is valid." << endl;
  co_return x + y;
}

int main() {
  cout << std::boolalpha;
  for (int b : {1, 2}) {
    for (int a : {1, 2}) {
      cout << "===" << a << " " << b << "===" << endl;
      auto c = compute(a, b);
      cout << c.value_or(-1) << endl;
      cout << "=========" << endl << endl;
    }
  }
  return 0;
}
