#include <coroutine>
#include <iostream>
#include <tuple>
#include <utility>

using std::cout;
using std::endl;

namespace self {

struct suspend_always {
  bool await_ready() const noexcept {
    cout << "A await_ready" << endl;
    return false;
  }

  void await_suspend(std::coroutine_handle<> ch) const noexcept {
    cout << "A await_suspend" << endl;
  }

  void await_resume() const noexcept { cout << "A await_resume" << endl; }
};

struct suspend_never {
  bool await_ready() const noexcept {
    cout << "N await_ready" << endl;
    return true;
  }

  void await_suspend(std::coroutine_handle<>) const noexcept {
    cout << "N await_suspend" << endl;
  }

  void await_resume() const noexcept { cout << "N await_resume" << endl; }
};

}  // namespace self

struct Generator {
  // using suspend_always = std::suspend_always;
  // using suspend_never = std::suspend_never;
  using suspend_always = self::suspend_always;
  using suspend_never = self::suspend_never;

  struct promise_type {
    Generator get_return_object() {
      cout << "get_return_object" << endl;
      return handle::from_promise(
          *this);  // use implicit constructor to get Generator
    }

    auto initial_suspend() noexcept {
      cout << "initial_suspend" << endl;
      return suspend_never{};
      // return suspend_always{};
    }

    auto final_suspend() noexcept {
      cout << "final_suspend" << endl;
      return suspend_always{};
    }

    void unhandled_exception() {
      cout << "unhandled_exception" << endl;
      std::terminate();
    }

    void return_void() { cout << "return_void" << endl; }

    auto yield_value(int value) {
      cout << "yield_value(" << value << ")" << endl;
      current_value_ = value;
      return suspend_always{};
    }

    int current_value_ = 0;
  };

  using handle = std::coroutine_handle<promise_type>;
  void next() {
    cout << "next" << endl;
    return handle_.resume();
  }

  bool done() {
    cout << "done? " << handle_.done() << endl;
    return handle_.done();
  }

  int current_value() {
    cout << "current_value: " << handle_.promise().current_value_ << endl;
    return handle_.promise().current_value_;
  }

  // Generator(Generator &&rhs) noexcept
  //     : handle_(std::exchange(rhs.handle_, {})) {
  //   cout << "Generator move constructor" << endl;
  // }

  ~Generator() {
    cout << "Generator destructor begin" << endl;
    if (handle_) {
      cout << "handle_.destroy()" << endl;
      handle_.destroy();
      // Will coroutine_handle be auto-destroyed without the explicit
      // destructor?
      // coroutine_handle doesn't have explicit destructor.
    }
    cout << "Generator destructor end" << endl;
  }

 private:
  Generator(handle h) : handle_(h) {  // should not have explicit keyword.
    cout << "Generator handle constructor" << endl;
  }
  handle handle_;
};

Generator fib() {
  // co_await initial_suspend:
  // never (continue to run fib());
  // always (suspend itself and continue to run main());
  cout << "fib() begin" << endl;
  int a = 1, b = 1;
  while (a < 5) {
    cout << "fib() before co_yield" << endl;
    co_yield a;
    // co_await yield_value(a);
    // co_yield's behavior is user-defined (by yield_value() function).
    // If yield_value(a) return always, suspend itself;
    // else (return never) continue to run fib().
    cout << "fib() after co_yield" << endl;
    std::tie(a, b) = std::make_tuple(b, a + b);
  }
  cout << "fib() end" << endl;
  co_return;
  // co_await final_suspend
}

int main() {
  cout << "Main function starts." << endl;
  auto f = fib();

  // If initial_suspend() return suspend_never,
  // it will run fib() until co_yield or co_return. 1 1 2 3

  // If initial_suspend() return suspend_always,
  // it will not run fib() (start to run fib() when f.next()). 0 1 1 2 3

  cout << "after auto f = fib();" << endl;
  while (!f.done()) {
    cout << "Main function before f.current_value()." << endl;
    auto value = f.current_value();
    cout << "Main function after f.current_value()." << endl;
    cout << value << endl;
    cout << "Main function before f.next()." << endl;
    f.next();
    cout << "Main function after f.next()." << endl;
  }
  cout << "Main function ends." << endl;
  return 0;
}
