#include <assert.h>
#include <jnitl.h>
#include <stdexcept>
#include <stdlib.h>
#include <string.h>

static void
thrower(java_env_t env, java_object_t<"Thrower"> receiver) {
  throw std::invalid_argument("thrown from native");
}

int
main() {
  auto class_path = getenv("TEST_CLASS_PATH");

  assert(class_path != nullptr);

  auto [vm, env] = java_vm_t::create(std::vector<std::string>{std::string("-Djava.class.path=") + class_path});

  auto thrower_class = java_class_t<"Thrower">(env);

  thrower_class.register_natives(java_native_method_t<thrower>("thrower"));

  auto call = thrower_class.get_static_method<void()>("thrower");

  bool caught = false;

  try {
    call();
  } catch (const java_exception_t &error) {
    caught = true;

    assert(strstr(error.what(), "thrown from native") != nullptr);
  }

  assert(caught);
}
