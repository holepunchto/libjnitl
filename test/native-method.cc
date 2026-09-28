#include <assert.h>
#include <jnitl.h>
#include <stdlib.h>

auto
hello(java_env_t env, java_object_t<"Hello"> receiver, std::string argument) {
  return argument.size();
}

int
main() {
  auto class_path = getenv("TEST_CLASS_PATH");

  assert(class_path != nullptr);

  auto [vm, env] = java_vm_t::create(std::vector<std::string>{std::string("-Djava.class.path=") + class_path});

  auto hello_class = java_class_t<"Hello">(env);

  hello_class.register_natives(java_native_method_t<hello>("hello"));

  auto call = hello_class.get_method<long(std::string)>("hello");

  assert(call(hello_class(), "hello") == 5);
}
