#include <chrono>
#include <coroutine>
#include <iostream>
#include <stdexcept>
#include <thread>

using std::cout;
using std::endl;
using namespace std::chrono_literals;

// https://en.cppreference.com/w/cpp/language/coroutines

auto switch_to_new_thread(std::jthread& out) {
  struct Awaitable {
    std::jthread* p_out;

    bool await_ready() {
      cout << "await_ready" << endl;
      // Suspend itself and run await_suspend().
      return false;
      // Don't suspend. Don't run await_suspend(). Run await_resume().
      return true;
    }

    void await_suspend(std::coroutine_handle<> h) {
      // same as "bool await_suspend(h) {...; return true;}"
      // h is parent coroutine_handle
      std::jthread& out = *p_out;
      // https://en.cppreference.com/w/cpp/thread/jthread/joinable
      // Checks if the std::jthread object identifies an active thread of
      // execution. Not joinable: no thread; has been joined
      if (out.joinable()) {
        throw std::runtime_error("'out' shouldn't have an active thread.");
      }
      out = std::jthread([h] {
        h.resume();  // run await_resume();
        // Parent resumes in another thread.
      });
      cout << "New thread ID: " << out.get_id() << endl;
      // Don't use p_out->get_id():
      // Note that because the coroutine is fully suspended before entering
      // awaiter.await_suspend(), that function is free to transfer the
      // coroutine handle across threads, with no additional synchronization.
      // For example, it can put it inside a callback, scheduled to run on a
      // thread pool when async I/O operation completes. In that case, since the
      // current coroutine may have been resumed and thus executed the awaiter
      // object's destructor, all concurrently as await_suspend() continues its
      // execution on the current thread, await_suspend() should treat *this as
      // destroyed and not access it after the handle was published to other
      // threads.
    }

    void await_resume() {
      // The condition which will run await_Resume():
      // await_ready() return true;
      // A await_suspend() runs arg_handle.resume() explicitly.
      // A bool await_suspend() return false.
      // A await_suspend() returns a coroutine. Resume this coroutine.
      cout << "await_resume thread ID: " << std::this_thread::get_id() << endl;
      // cout << "await_resume sleeping..." << endl;
      // std::this_thread::sleep_for(200ms);
      // cout << "await_resume done." << endl;
    }
  };

  cout << "switch_to_new_thread(out)" << endl;
  return Awaitable{.p_out = &out};
}

struct Task {
  struct promise_type {
    Task get_return_object() {
      // using CoHandle = std::coroutine_handle<promise_type>;
      // cout << CoHandle::from_promise(*this).address() << endl;
      return {};
    }

    auto initial_suspend() {
      cout << "initial_suspend" << endl;
      return std::suspend_never{};  // continue resuming_on_new_thread()
      // return std::suspend_always{};  // continue main()
    }

    auto final_suspend() noexcept {
      cout << "final_suspend" << endl;
      return std::suspend_never{};
    }

    void return_void() { cout << "return_void" << endl; }

    void unhandled_exception() {}
  };
};

Task resuming_on_new_thread(std::jthread& out) {
  // Compiler creates a coroutine frame.
  cout << "Coroutine started on thread: " << std::this_thread::get_id() << endl;
  // currently 'out' is null
  // Parent coroutine await an Awaiter object (by running its await_ready()
  // function).
  // for example: co_await std::suspend_always{}; NOT co_await 1;
  co_await switch_to_new_thread(out);
  cout << "Coroutine resumed on thread: " << std::this_thread::get_id() << endl;
  // return_void and final_suspend
}

int main() {
  // https://en.cppreference.com/w/cpp/thread/jthread
  std::jthread out;  // currently no thread
  resuming_on_new_thread(out);
  cout << "main() end" << endl;
  return 0;
}
