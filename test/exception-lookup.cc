#include <assert.h>
#include <jnitl.h>
#include <string.h>

int
main() {
  auto [vm, env] = java_vm_t::create();

  JNIEnv *jenv = env;

  bool caught = false;

  try {
    java_class_t<"does/not/Exist"> missing(env);
  } catch (const java_exception_t &error) {
    caught = true;

    assert(strstr(error.what(), "NoClassDefFoundError") != nullptr);
  }

  assert(caught);
  assert(jenv->ExceptionCheck() == JNI_FALSE);

  auto integer_class = java_class_t<"java/lang/Integer">(env);

  caught = false;

  try {
    integer_class.get_static_method<void()>("doesNotExist");
  } catch (const java_exception_t &error) {
    caught = true;

    assert(strstr(error.what(), "NoSuchMethodError") != nullptr);
  }

  assert(caught);
  assert(jenv->ExceptionCheck() == JNI_FALSE);

  caught = false;

  try {
    integer_class.get_field<int>("doesNotExist");
  } catch (const java_exception_t &error) {
    caught = true;

    assert(strstr(error.what(), "NoSuchFieldError") != nullptr);
  }

  assert(caught);
  assert(jenv->ExceptionCheck() == JNI_FALSE);
}
