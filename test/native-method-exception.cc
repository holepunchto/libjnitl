#include <assert.h>
#include <jnitl.h>
#include <stdexcept>
#include <stdlib.h>
#include <string.h>

struct test_error : std::runtime_error {
  test_error() : std::runtime_error("thrown from native") {}
};

static void
thrower(java_env_t env, java_object_t<"Thrower"> receiver) {
  throw test_error();
}

int
main() {
  auto class_path = getenv("TEST_CLASS_PATH");

  assert(class_path != nullptr);

  auto [vm, env] = java_vm_t::create(std::vector<std::string>{std::string("-Djava.class.path=") + class_path});

  JNIEnv *jenv = env;

  auto thrower_class = java_class_t<"Thrower">(env);

  thrower_class.register_natives(java_native_method_t<thrower>("thrower"));

  auto propagate = thrower_class.get_static_method<void()>("propagate");

  bool caught = false;

  try {
    propagate();
  } catch (const test_error &error) {
    caught = true;

    assert(strcmp(error.what(), "thrown from native") == 0);
  }

  assert(caught);
  assert(jenv->ExceptionCheck() == JNI_FALSE);

  auto caught_by_java = thrower_class.get_static_method<std::string()>("caught");

  assert(caught_by_java() == "java.lang.RuntimeException: thrown from native");
}
