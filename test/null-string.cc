#include <assert.h>
#include <jnitl.h>

int
main() {
  auto [vm, env] = java_vm_t::create();

  auto system_class = java_class_t<"java/lang/System">(env);

  auto get_property = system_class.get_static_method<std::string(std::string)>("getProperty");

  assert(get_property("java.version").size() > 0);

  // An absent property is null, which is not a string.
  bool caught = false;

  try {
    get_property("no.such.property");
  } catch (const std::invalid_argument &error) {
    caught = true;
  }

  assert(caught);
}
