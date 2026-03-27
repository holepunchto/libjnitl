#include <assert.h>
#include <jnitl.h>

auto
hello(java_env_t env, java_object_t<"java/lang/String"> receiver, std::string argument) {
  return argument.size();
}

int
main() {
  auto [vm, env] = java_vm_t::create();

  auto string_class = java_class_t<"java/lang/String">(env);

  string_class.register_natives(java_native_method_t<hello>("hello"));
}
