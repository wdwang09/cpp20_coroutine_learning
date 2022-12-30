#include <coroutine>
#include <iostream>
#include <memory>

using std::cout;
using std::endl;

// https://en.cppreference.com/w/cpp/coroutine/noop_coroutine

template <typename T>
struct Task {
  struct promise_type {
    Task<T> get_return_object() {
      return Task<T>(std::coroutine_handle<promise_type>::from_promise(*this));
    }

    auto initial_suspend() { return std::suspend_always{}; }

    auto final_suspend() noexcept {
      struct FinalAwaiter {
        bool await_ready() noexcept {
          return false;  // run await_suspend()
        }

        // Never run.
        void await_resume() noexcept {}

        std::coroutine_handle<> await_suspend(
            std::coroutine_handle<promise_type> h) noexcept {
          cout << ((uint64_t)h.address() & (uint64_t)0xfff)
               << " in final: await_suspend" << endl;
          auto p = h.promise().parent_;
          if (p) {
            cout << "resume " << ((uint64_t)p.address() & (uint64_t)0xfff)
                 << endl;
            return p;  // resume parent coroutine
          }
          cout << "No parent coroutine." << endl;
          return std::noop_coroutine();
        }
      };

      return FinalAwaiter{};
    }

    void unhandled_exception() { throw; }

    // co_return
    void return_value(T&& value) { result_ = std::forward<T>(value); }

    T result_{};
    std::coroutine_handle<> parent_;
  };

  // ===

  auto operator co_await() {
    struct Awaiter {
      std::coroutine_handle<promise_type> h_;

      bool await_ready() {
        return false;  // run await_suspend()
      }

      // co_await's return value
      T await_resume() {
        cout << ((uint64_t)h_.address() & (uint64_t)0xfff)
             << " in co_await: await_resume (to get return value)" << endl;
        return std::move(h_.promise().result_);
      }

      auto await_suspend(std::coroutine_handle<> parent) {
        cout << ((uint64_t)parent.address() & (uint64_t)0xfff) << " co_await "
             << ((uint64_t)h_.address() & (uint64_t)0xfff);
        cout << " await_suspend" << endl;
        h_.promise().parent_ = parent;
        return h_;
        // (A co_await B;) Resume B with B's await_resume(). A's handle is
        // stored in B (resume A in B's final_suspend()).
      }
    };

    return Awaiter{h_};
  }

  T operator()() {
    h_.resume();
    return std::move(h_.promise().result_);
  }

  explicit Task(std::coroutine_handle<promise_type> h) : h_(h) {
    cout << "Task(" << ((uint64_t)h_.address() & (uint64_t)0xfff) << ")"
         << endl;
  }

  ~Task() {
    cout << "~Task(" << ((uint64_t)h_.address() & (uint64_t)0xfff) << ")"
         << endl;
    if (h_) h_.destroy();
  }

 private:
  std::coroutine_handle<promise_type> h_;
};

Task<int> get_int() {
  cout << "get_int()" << endl;
  co_return 1;
}

Task<int> test() {
  cout << "Task<int> test()" << endl;
  Task<int> u = get_int();
  cout << "u" << endl;
  int v = co_await u;
  cout << "v: " << v << endl;
  Task<int> w = get_int();
  co_return v + co_await w + co_await get_int();
};

Task<std::unique_ptr<int>> get_unique_ptr() {
  co_return std::make_unique<int>(1);
}

Task<std::unique_ptr<int>> test2() {
  auto u = get_unique_ptr();
  auto v = co_await u;
  co_return co_await get_unique_ptr();
}

int main() {
  auto gi = get_int();
  int giv = gi();
  cout << "giv: " << giv << endl;
  cout << "===" << endl;

  auto t = test();
  int result = t();
  cout << "result: " << result << endl;
  cout << "===" << endl;

  auto t2 = test2();
  auto result2 = t2();
  cout << "result2: " << std::boolalpha << bool(result2) << endl;
  return 0;
}
