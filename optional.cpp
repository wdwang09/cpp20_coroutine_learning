#include <coroutine>
#include <future>
#include <iostream>
#include <optional>
#include <vector>

using std::cout;
using std::endl;

// https://en.cppreference.com/w/cpp/coroutine/coroutine_traits

struct AsCoroutine {};

template <typename T, typename... Args>
struct std::coroutine_traits<std::future<std::optional<T>>, AsCoroutine,
                             Args...> {
  struct promise_type {
    std::promise<std::optional<T>> p;

    std::future<std::optional<T>> get_return_object() { return p.get_future(); }

    auto initial_suspend() { return std::suspend_never{}; }

    auto final_suspend() noexcept { return std::suspend_never{}; }

    void unhandled_exception() {}

    struct Awaiter {
      std::optional<T> opt;

      explicit Awaiter(const std::optional<T>& opt) : opt(opt) {}
      explicit Awaiter(std::optional<T>&& opt) : opt(std::move(opt)) {}

      bool await_ready() {
        // true: await_resume()
        // false: await_suspend()
        return opt.has_value();
      }

      // for "auto XXX = co_yield YYY;"
      T await_resume() {
        // If with value, return it.
        return std::move(opt.value());
      }

      void await_suspend(std::coroutine_handle<> handle) {
        // If nullopt, stop running coroutine.
        handle.destroy();
      }
    };

    // co_yield
    auto yield_value(const std::optional<T>& opt) {
      if (!opt.has_value()) {
        p.set_value(std::nullopt);  // avoid std::future_error
      }
      return Awaiter{opt};
    }

    auto yield_value(std::optional<T>&& opt) {
      if (!opt.has_value()) {
        p.set_value(std::nullopt);  // avoid std::future_error
      }
      return Awaiter{std::move(opt)};
    }

    // co_return expr;
    // return value is in get_return_object() rather than return_value()
    void return_value(T&& value) { p.set_value(std::forward<T>(value)); }
  };
};

std::optional<int> get_odd(int i) {
  if (i % 2 == 1) {
    return i;
  } else {
    return std::nullopt;
  }
}

std::future<std::optional<int>> compute(AsCoroutine, int a, int b) {
  int x = co_yield get_odd(a);
  cout << "a is valid." << endl;
  int y = co_yield get_odd(b);
  cout << "b is valid." << endl;
  co_return x + y;
}

std::future<std::optional<std::vector<int>>> test1(AsCoroutine) {
  // For lvalue and rvalue test.
  using v = std::vector<int>;
  using ov = std::optional<std::vector<int>>;
  co_yield ov(v());
  ov lv;
  lv.emplace(v());
  co_yield lv;
  auto x = co_yield ov(v());
  co_return x;
  co_return v();
  v abc;
  co_return abc;
  co_return co_yield ov({});
}

std::future<std::optional<std::unique_ptr<int>>> test2(AsCoroutine) {
  using ou = std::optional<std::unique_ptr<int>>;

  // co_yield ou();
  // co_yield std::nullopt;
  ou lv(std::make_unique<int>(3));
  co_yield std::move(lv);
  ou lv2(std::make_unique<int>(3));
  co_return std::move(lv2.value());
}

int main() {
  for (int b : {1, 2}) {
    for (int a : {1, 2}) {
      cout << "===" << a << " " << b << "===" << endl;
      auto c = compute({}, a, b).get();
      cout << c.value_or(-1) << endl;
      cout << "=========" << endl << endl;
    }
  }
  test1({}).get();
  test2({}).get();
  return 0;
}
