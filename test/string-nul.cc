#include <assert.h>
#include <jnitl.h>

int
main() {
  auto [vm, env] = java_vm_t::create();

  auto integer_class = java_class_t<"java/lang/Integer">(env);

  auto parse_int = integer_class.get_static_method<int(std::string)>("parseInt");

  bool caught = false;

  try {
    parse_int(std::string("42\0junk", 7));
  } catch (const std::invalid_argument &) {
    caught = true;
  }

  assert(caught);

  assert(parse_int("42") == 42);
}
