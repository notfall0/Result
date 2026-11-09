#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include "notfall/result.h"

namespace {

struct LifeTracker {
  int* tracker;

  explicit LifeTracker(int* ptr) : tracker(ptr) {
    if (tracker) {
      (*tracker)++;
    }
  }

  LifeTracker(const LifeTracker& other) : tracker(other.tracker) {
    if (tracker) {
      (*tracker)++;
    }
  }

  LifeTracker(LifeTracker&& other) noexcept : tracker(other.tracker) {
    other.tracker = nullptr;
  }

  ~LifeTracker() {
    if (tracker) {
      (*tracker)--;
    }
  }
};

struct ThrowingType {
  ThrowingType() {
    throw std::runtime_error("Construction failed");
  }

  ThrowingType(const ThrowingType&) {
    throw std::runtime_error("Copy failed");
  }

  ThrowingType(ThrowingType&&) {
    throw std::runtime_error("Move failed");
  }
};

void ThrowIfFailed(bool condition) {
  if (!condition) {
    throw std::runtime_error("Test assertion failed");
  }
}

void TestFactoriesAndStates() {
  std::cout << "[TEST] Factories & Initial States...\n";

  notfall::Result<int, std::string> default_result;
  ThrowIfFailed(default_result.HasValue());
  ThrowIfFailed(!default_result.HasError());
  ThrowIfFailed(default_result.Value() == 0);

  notfall::Result<int, std::string> some_result(notfall::Some(43));
  ThrowIfFailed(some_result.HasValue());
  ThrowIfFailed(some_result.Value() == 43);
  ThrowIfFailed(static_cast<bool>(some_result));

  notfall::Result<int, std::string> error_result(
      notfall::Error<std::string>("Something went wrong"));
  ThrowIfFailed(error_result.HasError());
  ThrowIfFailed(!error_result.HasValue());
  ThrowIfFailed(error_result.Error() == "Something went wrong");
  ThrowIfFailed(!static_cast<bool>(error_result));

  std::cout << "\tComplete!\n";
}

void TestAccessorsAndOperators() {
  std::cout << "[TEST] Accessors, Dereference & Arrow Operators...\n";

  notfall::Result<std::string, int> result(
      notfall::Some(std::string("Minecraft")));

  ThrowIfFailed(*result == "Minecraft");
  ThrowIfFailed(result->length() == 9);

  const auto& cref = result;
  ThrowIfFailed(*cref == "Minecraft");
  ThrowIfFailed(cref->length() == 9);

  notfall::Result<std::string, int> error_only(notfall::Error(404));

  try {
    (void)error_only.Value();
    ThrowIfFailed(false);
  } catch (const std::runtime_error& ex) {
    std::cerr << "\tCaught expected exception: "
              << ex.what() << '\n';
  }

  std::cout << "\tComplete!\n";
}

void TestCopySemantics() {
  std::cout << "[TEST] Copy Constructor & Copy Assignment...\n";

  notfall::Result<int, std::string> original(notfall::Some(100));

  notfall::Result<int, std::string> copy_constructor(original);
  ThrowIfFailed(copy_constructor.HasValue());
  ThrowIfFailed(copy_constructor.Value() == 100);
  ThrowIfFailed(original.HasValue());

  notfall::Result<int, std::string> target(notfall::Some(4));
  target = original;
  ThrowIfFailed(target.Value() == 100);

  notfall::Result<int, std::string> error_target(
      notfall::Error(std::string("Some error")));

  error_target = original;
  ThrowIfFailed(error_target.HasValue());
  ThrowIfFailed(error_target.Value() == 100);

  std::cout << "\tComplete!\n";
}

void TestMoveSemantics() {
  std::cout << "[TEST] Move Constructor, Move Assignment & Empty State...\n";

  notfall::Result<std::string, int> source(
      notfall::Some(std::string("ChunkData")));

  notfall::Result<std::string, int> moved_constructor(std::move(source));
  ThrowIfFailed(moved_constructor.HasValue());
  ThrowIfFailed(moved_constructor.Value() == "ChunkData");

  ThrowIfFailed(!source.HasValue());
  ThrowIfFailed(!source.HasError());

  notfall::Result<std::string, int> destination(notfall::Error(500));
  destination = std::move(moved_constructor);

  ThrowIfFailed(destination.HasValue());
  ThrowIfFailed(destination.Value() == "ChunkData");

  ThrowIfFailed(!moved_constructor.HasValue());
  ThrowIfFailed(!moved_constructor.HasError());

  std::cout << "\tComplete!\n";
}

void TestLifecycleTracking() {
  std::cout << "[TEST] Destructor Cleanup & Leak Prevention...\n";

  int active_instances = 0;

  {
    notfall::Result<LifeTracker, int> result{
        notfall::Some(LifeTracker(&active_instances))};

    ThrowIfFailed(active_instances == 1);

    {
      notfall::Result<LifeTracker, int> moved(std::move(result));

      ThrowIfFailed(!result.HasValue());
      ThrowIfFailed(!result.HasError());
      ThrowIfFailed(active_instances == 1);
    }

    ThrowIfFailed(active_instances == 0);
  }

  ThrowIfFailed(active_instances == 0);

  std::cout << "\tComplete!\n";
}

void TestErrorLifecycle() {
  std::cout << "[TEST] Error State Lifecycle...\n";

  int active_instances = 0;

  {
    notfall::Result<LifeTracker, LifeTracker> result(
        notfall::Error<LifeTracker>{ LifeTracker(&active_instances) });

    ThrowIfFailed(result.HasError());
    ThrowIfFailed(active_instances == 1);

    {
      auto copy(result);
      ThrowIfFailed(copy.HasError());
      ThrowIfFailed(active_instances == 2);

      auto moved(std::move(copy));

      ThrowIfFailed(moved.HasError());
      ThrowIfFailed(!copy.HasValue());
      ThrowIfFailed(!copy.HasError());
      ThrowIfFailed(active_instances == 2);
    }

    ThrowIfFailed(active_instances == 1);
  }

  ThrowIfFailed(active_instances == 0);

  std::cout << "\tComplete!\n";
}

void TestCrossStateAssignment() {
  std::cout << "[TEST] Cross-State Assignment...\n";

  notfall::Result<int, std::string> result(notfall::Some(42));

  result = notfall::Error(std::string("Failure"));
  ThrowIfFailed(result.HasError());
  ThrowIfFailed(result.Error() == "Failure");

  result = notfall::Some(100);
  ThrowIfFailed(result.HasValue());
  ThrowIfFailed(result.Value() == 100);

  std::cout << "\tComplete!\n";
}

void TestSelfAssignment() {
  std::cout << "[TEST] Self-Assignment...\n";

  notfall::Result<int, std::string> result(notfall::Some(42));

  result = result;
  ThrowIfFailed(result.HasValue());
  ThrowIfFailed(result.Value() == 42);

  result = std::move(result);
  ThrowIfFailed(result.HasValue());
  ThrowIfFailed(result.Value() == 42);

  notfall::Result<int, std::string> error(
      notfall::Error(std::string("Failure")));

  error = error;
  ThrowIfFailed(error.HasError());

  error = std::move(error);
  ThrowIfFailed(error.HasError());

  std::cout << "\tComplete!\n";
}

void TestMoveOnlyTypes() {
  std::cout << "[TEST] Move-Only Types...\n";

  notfall::Result<std::unique_ptr<int>, int> result(
      notfall::Some(std::make_unique<int>(42)));

  auto moved(std::move(result));

  ThrowIfFailed(moved.HasValue());
  ThrowIfFailed(*moved.Value() == 42);
  ThrowIfFailed(!result.HasValue());
  ThrowIfFailed(!result.HasError());

  std::cout << "\tComplete!\n";
}

void TestExceptionSafety() {
  std::cout << "[TEST] Exception Safety...\n";

  bool caught = false;

  try {
    notfall::Result<ThrowingType, int> result(
        notfall::Some(ThrowingType{}));
  } catch (const std::runtime_error&) {
    caught = true;
  }

  ThrowIfFailed(caught);

  std::cout << "\tComplete!\n";
}

void TestConstCorrectness() {
  std::cout << "[TEST] Const Correctness...\n";

  const notfall::Result<std::string, int> result(
      notfall::Some(std::string("Minecraft")));

  static_assert(std::is_same_v<
      decltype(result.Value()),
      const std::string&>);

  ThrowIfFailed(result.Value() == "Minecraft");

  std::cout << "\tComplete!\n";
}

void TestSameType() {
  std::cout << "[TEST] Same Value & Error Types...\n";

  notfall::Result<int, int> value(notfall::Some(42));
  notfall::Result<int, int> error(notfall::Error(404));

  ThrowIfFailed(value.HasValue());
  ThrowIfFailed(value.Value() == 42);

  ThrowIfFailed(error.HasError());
  ThrowIfFailed(error.Error() == 404);

  std::cout << "\tComplete!\n";
}

}  // namespace

int main() {
  try {
    std::cout << "Starting tests...\n";

    TestFactoriesAndStates();
    TestAccessorsAndOperators();
    TestCopySemantics();
    TestMoveSemantics();
    TestLifecycleTracking();
    TestErrorLifecycle();
    TestCrossStateAssignment();
    TestSelfAssignment();
    TestMoveOnlyTypes();
    TestExceptionSafety();
    TestConstCorrectness();
    TestSameType();

    std::cout << "\nAll tests successfully passed!\n";
  } catch (const std::exception& ex) {
    std::cerr << "\nTEST FAILURE: " << ex.what() << '\n';
    return 1;
  }

  return 0;
}
