#include <assert.h>
#include <exception>
#include <jnitl.h>
#include <new>
#include <stdexcept>
#include <stdlib.h>

static std::exception_ptr next;

static void
thrower(java_env_t env, java_object_t<"Thrower"> receiver) {
  std::rethrow_exception(next);
}

int
main() {
  auto class_path = getenv("TEST_CLASS_PATH");

  assert(class_path != nullptr);

  auto [vm, env] = java_vm_t::create(std::vector<std::string>{std::string("-Djava.class.path=") + class_path});

  auto thrower_class = java_class_t<"Thrower">(env);

  thrower_class.register_natives(java_native_method_t<thrower>("thrower"));

  auto caught = thrower_class.get_static_method<std::string()>("caught");

  next = std::make_exception_ptr(std::invalid_argument("invalid"));

  assert(caught() == "java.lang.IllegalArgumentException: invalid");

  next = std::make_exception_ptr(std::out_of_range("out of range"));

  assert(caught() == "java.lang.IndexOutOfBoundsException: out of range");

  next = std::make_exception_ptr(std::bad_alloc());

  assert(caught() == std::string("java.lang.OutOfMemoryError: ") + std::bad_alloc().what());

  next = std::make_exception_ptr(std::runtime_error("runtime"));

  assert(caught() == "java.lang.RuntimeException: runtime");

  next = std::make_exception_ptr(42);

  assert(caught() == "java.lang.RuntimeException: Unknown native exception");
}
