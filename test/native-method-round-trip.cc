#include <assert.h>
#include <jnitl.h>
#include <stdlib.h>

static void
rethrower(java_env_t env, java_object_t<"Thrower"> receiver) {
  java_class_t<"Thrower">(env).get_static_method<void()>("fail")();
}

int
main() {
  auto class_path = getenv("TEST_CLASS_PATH");

  assert(class_path != nullptr);

  auto [vm, env] = java_vm_t::create(std::vector<std::string>{std::string("-Djava.class.path=") + class_path});

  auto thrower_class = java_class_t<"Thrower">(env);

  thrower_class.register_natives(java_native_method_t<rethrower>("rethrower"));

  auto round_trip = thrower_class.get_static_method<bool()>("roundTrip");

  assert(round_trip());
}
