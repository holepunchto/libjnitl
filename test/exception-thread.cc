#include <assert.h>
#include <exception>
#include <jnitl.h>
#include <thread>

int
main() {
  auto [vm, env] = java_vm_t::create(std::vector<std::string>{"-Xcheck:jni"});

  auto integer_class = java_class_t<"java/lang/Integer">(env);

  auto parse_int = integer_class.get_static_method<int(std::string)>("parseInt");

  std::exception_ptr error;

  try {
    parse_int("not a number");
  } catch (const java_exception_t &) {
    error = std::current_exception();
  }

  assert(error);

  std::thread([&error]() {
    error = nullptr;
  }).join();
}
