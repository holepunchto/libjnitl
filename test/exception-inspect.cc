#include <assert.h>
#include <jnitl.h>
#include <stdlib.h>

int
main() {
  auto class_path = getenv("TEST_CLASS_PATH");

  assert(class_path != nullptr);

  auto [vm, env] = java_vm_t::create(std::vector<std::string>{std::string("-Djava.class.path=") + class_path});

  JNIEnv *jenv = env;

  auto integer_class = java_class_t<"java/lang/Integer">(env);

  auto parse_int = integer_class.get_static_method<int(std::string)>("parseInt");

  bool caught = false;

  try {
    parse_int("not a number");
  } catch (const java_exception_t &error) {
    caught = true;

    assert(error.is_instance_of<"java/lang/NumberFormatException">(env));
    assert(error.is_instance_of<"java/lang/IllegalArgumentException">(env));
    assert(!error.is_instance_of<"java/lang/IllegalStateException">(env));

    assert(error.message(env) == "For input string: \"not a number\"");

    auto throwable = error.get(env);

    assert(jenv->IsSameObject(throwable, error));
  }

  assert(caught);
  assert(jenv->ExceptionCheck() == JNI_FALSE);

  auto fail_without_message = java_class_t<"Thrower">(env).get_static_method<void()>("failWithoutMessage");

  caught = false;

  try {
    fail_without_message();
  } catch (const java_exception_t &error) {
    caught = true;

    assert(error.message(env) == std::nullopt);
  }

  assert(caught);
  assert(jenv->ExceptionCheck() == JNI_FALSE);
}
