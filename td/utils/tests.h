#pragma once

#include "td/utils/common.h"
#include "td/utils/Context.h"
#include "td/utils/format.h"
#include "td/utils/List.h"
#include "td/utils/logging.h"
#include "td/utils/port/thread.h"
#include "td/utils/Random.h"
#include "td/utils/optional.h"
#include "td/utils/Slice.h"
#include "td/utils/Status.h"
#include "td/utils/Time.h"

#include <atomic>
#include <map>

#define REGISTER_TESTS(x)                \
  void TD_CONCAT(register_tests_, x)() { \
  }
#define DESC_TESTS(x) void TD_CONCAT(register_tests_, x)()
#define LOAD_TESTS(x) TD_CONCAT(register_tests_, x)()

namespace td {

class RegressionTester {
 public:
  virtual ~RegressionTester() = default;
  static void destroy(CSlice db_path);
  static std::unique_ptr<RegressionTester> create(string db_path, string db_cache_dir = "");

  virtual Status verify_test(Slice name, Slice result) = 0;
  virtual void save_db() = 0;
};

class Test {
 public:
  virtual ~Test() = default;
  virtual void run() {
    while (step()) {
    }
  }
  virtual bool step() {
    run();
    return false;
  }
  Test() = default;
  Test(const Test &) = delete;
  Test &operator=(const Test &) = delete;
  Test(Test &&) = delete;
  Test &operator=(Test &&) = delete;
};

class TestContext : public Context<TestContext> {
 public:
  virtual ~TestContext() = default;
  virtual Slice name() = 0;
  virtual Status verify(Slice data) = 0;
};

class TestsRunner : public TestContext {
 public:
  static TestsRunner &get_default();

  void add_test(string name, std::unique_ptr<Test> test);
  void add_substr_filter(std::string str);
  void set_stress_flag(bool flag);
  void run_all();
  bool run_all_step();
  void set_regression_tester(std::unique_ptr<RegressionTester> regression_tester);

 private:
  struct State {
    size_t it{0};
    bool is_running = false;
    double start;
    size_t end{0};
  };
  bool stress_flag_{false};
  std::vector<std::string> substr_filters_;
  std::vector<std::pair<string, std::unique_ptr<Test>>> tests_;
  State state_;
  std::unique_ptr<RegressionTester> regression_tester_;

  Slice name() override;
  Status verify(Slice data) override;
};

template <class T>
class RegisterTest {
 public:
  RegisterTest(string name, TestsRunner &runner = TestsRunner::get_default()) {
    runner.add_test(name, std::make_unique<T>());
  }
};

class Stage {
 public:
  void wait(uint64 need) {
    value_.fetch_add(1, std::memory_order_release);
    while (value_.load(std::memory_order_acquire) < need) {
      td::this_thread::yield();
    }
  };

 private:
  std::atomic<uint64> value_{0};
};

inline string rand_string(char from, char to, int len) {
  string res(len, 0);
  for (auto &c : res) {
    c = static_cast<char>(Random::fast(from, to));
  }
  return res;
}

inline std::vector<string> rand_split(string str) {
  std::vector<string> res;
  size_t pos = 0;
  while (pos < str.size()) {
    size_t len;
    if (Random::fast(0, 1) == 1) {
      len = Random::fast(1, 10);
    } else {
      len = Random::fast(100, 200);
    }
    res.push_back(str.substr(pos, len));
    pos += len;
  }
  return res;
}

template <class T1, class T2>
void assert_eq_impl(const T1 &expected, const T2 &got, const char *file, int line) {
  CHECK(expected == got) << tag("expected", expected) << tag("got", got) << " in " << file << " at line " << line;
}

template <class T>
void assert_true_impl(const T &got, const char *file, int line) {
  CHECK(got) << "Expected true in " << file << " at line " << line;
}

}  // namespace td

#define ASSERT_EQ(expected, got) ::td::assert_eq_impl((expected), (got), __FILE__, __LINE__)

#define ASSERT_TRUE(got) ::td::assert_true_impl((got), __FILE__, __LINE__)

#define ASSERT_STREQ(expected, got) \
  ::td::assert_eq_impl(::td::Slice((expected)), ::td::Slice((got)), __FILE__, __LINE__)

#define REGRESSION_VERIFY(data) ::td::TestContext::get()->verify(data).ensure()

#define TEST_NAME(test_case_name, test_name) \
  TD_CONCAT(Test, TD_CONCAT(_, TD_CONCAT(test_case_name, TD_CONCAT(_, test_name))))

#define TEST(test_case_name, test_name) TEST_IMPL(TEST_NAME(test_case_name, test_name))

#define TEST_IMPL(test_name)                                                                                         \
  class test_name : public ::td::Test {                                                                              \
   public:                                                                                                           \
    using Test::Test;                                                                                                \
    void run() final;                                                                                                \
  };                                                                                                                 \
  ::td::RegisterTest<test_name> TD_CONCAT(test_instance_, TD_CONCAT(test_name, __LINE__))(TD_DEFINE_STR(test_name)); \
  void test_name::run()
