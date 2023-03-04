#include <coroutine>
#include <iostream>
#include <utility>

using std::cout;
using std::endl;

namespace self {

struct suspend_always {
  bool await_ready() const noexcept {
    cout << "[A] await_ready false" << endl;
    // Suspend itself and run await_suspend().
    return false;
  }

  void await_suspend(std::coroutine_handle<>) const noexcept {
    cout << "[A] await_suspend" << endl;
  }

  void await_resume() const noexcept { cout << "[A] await_resume" << endl; }
};

struct suspend_never {
  bool await_ready() const noexcept {
    cout << "[N] await_ready true" << endl;
    // Don't suspend. Don't run await_suspend(). Run await_resume().
    return true;
  }

  void await_suspend(std::coroutine_handle<>) const noexcept {
    cout << "[N] await_suspend" << endl;
  }

  void await_resume() const noexcept { cout << "[N] await_resume" << endl; }
};

}  // namespace self

struct Generator {
  // using suspend_always = std::suspend_always;
  // using suspend_never = std::suspend_never;
  using suspend_always = self::suspend_always;
  using suspend_never = self::suspend_never;

  struct promise_type {
    Generator get_return_object() {
      cout << "[promise_type::get_return_object]" << endl;
      return Generator(handle::from_promise(*this));
    }

    auto initial_suspend() noexcept {
      cout << "[promise_type::initial_suspend]" << endl;
      return suspend_never{};
      // return suspend_always{};
    }

    auto final_suspend() noexcept {
      cout << "[promise_type::final_suspend]" << endl;
      return suspend_always{};
    }

    void unhandled_exception() {
      cout << "[promise_type::unhandled_exception]" << endl;
      std::terminate();
    }

    void return_void() { cout << "[promise_type::return_void]" << endl; }

    auto yield_value(int value) {
      cout << "[promise_type::yield_value] value: " << value << "" << endl;
      current_value_ = value;
      return suspend_always{};
    }

    int current_value_ = 0;
  };

  using handle = std::coroutine_handle<promise_type>;

  void resume() {
    cout << "[Generator] resume" << endl;
    handle_.resume();
  }

  bool done() {
    cout << "[Generator] done? " << handle_.done() << endl;
    return handle_.done();
  }

  int current_value() {
    cout << "[Generator] current_value: " << handle_.promise().current_value_
         << endl;
    return handle_.promise().current_value_;
  }

  // Generator(Generator &&rhs) noexcept
  //     : handle_(std::exchange(rhs.handle_, {})) {
  //   cout << "Generator move constructor" << endl;
  // }

  ~Generator() {
    cout << "[Generator] destructor begin" << endl;
    if (handle_) {
      cout << "[Generator] handle_.destroy()" << endl;
      handle_.destroy();
      // Will coroutine_handle be auto-destroyed without the explicit
      // destructor?
      // coroutine_handle doesn't have explicit destructor.
    }
    cout << "[Generator] destructor end" << endl;
  }

 private:
  explicit Generator(handle h) : handle_(h) {
    cout << "[Generator] Constructor with handle" << endl;
  }
  handle handle_;
};

Generator fib() {
  /// co_await initial_suspend:
  /// never (await_ready true) (Don't suspend itself, continue to run fib().)
  /// always (await_ready false) (suspend itself and continue to run main().)
  cout << "[user] fib() begin" << endl;
  int a = 1, b = 1;
  while (a < 5) {
    cout << "[user] fib() before co_yield"
         << " a=" << a << " b=" << b << endl;
    co_yield a;
    /// "co_yield a;" is equivalent to "co_await promise.yield_value(a);"
    /// co_yield's behavior is user-defined (by yield_value() function).
    /// Because it's a co_await, so yield_value() will return a Awaiter
    /// rather than int.
    /// If yield_value(a) return always, suspend itself;
    /// else (return never) continue to run fib().
    cout << "[user] fib() after co_yield" << endl;
    a = std::exchange(b, a + b);
  }
  cout << "[user] fib() end" << endl;
  co_return;
  // co_await final_suspend
}

int main() {
  cout << "[main] start" << endl;
  auto f = fib();

  // If initial_suspend() return suspend_never,
  // it will run fib() until co_yield or co_return. 1 1 2 3

  // If initial_suspend() return suspend_always,
  // it will not run fib() (start to run fib() when f.next()). 0 1 1 2 3

  cout << "[main] after auto f = fib();" << endl;
  while (!f.done()) {
    cout << "[main] before f.current_value()" << endl;
    auto value = f.current_value();
    cout << "[main] after f.current_value()" << endl;
    cout << value << endl;
    cout << "[main] before f.resume()" << endl;
    f.resume();
    cout << "[main] after f.resume()" << endl;
  }
  cout << "[main] end" << endl;
  return 0;
}
