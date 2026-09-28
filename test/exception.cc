#include <assert.h>
#include <jnitl.h>
#include <string.h>

int
main() {
  auto [vm, env] = java_vm_t::create();

  JNIEnv *jenv = env;

  auto integer_class = java_class_t<"java/lang/Integer">(env);

  auto parse_int = integer_class.get_static_method<int(std::string)>("parseInt");

  bool caught = false;

  try {
    parse_int("not a number");
  } catch (const java_exception_t &error) {
    caught = true;

    assert(strstr(error.what(), "NumberFormatException") != nullptr);
  }

  assert(caught);
  assert(jenv->ExceptionCheck() == JNI_FALSE);

  assert(parse_int("42") == 42);
}
