#include <assert.h>
#include <jnitl.h>

int
main() {
  auto [vm, env] = java_vm_t::create();

  auto short_class = java_class_t<"java/lang/Short">(env);

  auto parse_short = short_class.get_static_method<short(std::string)>("parseShort");

  assert(parse_short("1000") == 1000);
}
