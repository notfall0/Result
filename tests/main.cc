#include <iostream>
#include <string>

#include "notfall/result.h"

namespace {

struct LifeTracker {
  int* tracker;

  LifeTracker(int* ptr) : tracker(ptr) {
    if (tracker) {
      (*tracker)++;
    }
  }

  LifeTracker(const LifeTracker& other) : tracker(other.tracker) {
    if (tracker) {
      (*tracker)++;
    }
  }

  LifeTracker(LifeTracker&& other) : tracker(other.tracker) {
    other.tracker = nullptr;
  }

  ~LifeTracker() {
    if (tracker) {
      (*tracker)--;
    }
  }
};

void ThrowIfFailed(bool condition) {
  if (!condition) {
    throw std::exception();
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
    ThrowIfFailed(false);  // Should not reach here.
  } catch (const std::runtime_error& ex) {
    std::cerr << "\tCaught an EXPECTED exception: " << ex.what() << '\n';
  }

  std::cout << "\tComplete!\n";
}

void TestCopySemantics() {
  std::cout << "[TEST] Copy Constructor & Copy Assignment...\n";

  notfall::Result<int, std::string> original(notfall::Some(100));

  notfall::Result<int, std::string> copy_constructor(original);
  ThrowIfFailed(copy_constructor.HasValue());
  ThrowIfFailed(copy_constructor.Value() == 100);

  // Check whether the original was left untouched.
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

  // Verify that the source transitioned to an empty state safely.
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
  std::cout << "[TEST] Destructor Cleanup & Leak Prevention (LifeTracker)...\n";

  int active_instances = 0;

  {
    notfall::Result<LifeTracker, int> result{
        notfall::Some(LifeTracker(&active_instances))};
    ThrowIfFailed(active_instances == 1);

    {
      notfall::Result<LifeTracker, int> moved(std::move(result));
      ThrowIfFailed(!result.HasValue());
      ThrowIfFailed(!result.HasError());

      // The active instance count should remain 1 because ownership moved.
      ThrowIfFailed(active_instances == 1);
    }

    ThrowIfFailed(active_instances == 0);
  }

  ThrowIfFailed(active_instances == 0);

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
    std::cout << "All tests successfully passed!\n";
  } catch (const std::exception& ex) {
    std::cerr << ex.what() << '\n';
  }
}
