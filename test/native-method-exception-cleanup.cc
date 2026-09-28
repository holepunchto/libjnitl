#include <assert.h>
#include <jnitl.h>
#include <stdexcept>
#include <stdlib.h>

static int alive = 0;

struct counted_error : std::exception {
  counted_error() {
    alive++;
  }

  counted_error(const counted_error &) {
    alive++;
  }

  ~counted_error() override {
    alive--;
  }
};

static bool counted = true;

static void
thrower(java_env_t env, java_object_t<"Thrower"> receiver) {
  if (counted) throw counted_error();

  throw std::invalid_argument("uncounted");
}

int
main() {
  auto class_path = getenv("TEST_CLASS_PATH");

  assert(class_path != nullptr);

  auto [vm, env] = java_vm_t::create(std::vector<std::string>{std::string("-Djava.class.path=") + class_path});

  auto thrower_class = java_class_t<"Thrower">(env);

  thrower_class.register_natives(java_native_method_t<thrower>("thrower"));

  auto caught = thrower_class.get_static_method<std::string()>("caught");

  auto gc = java_class_t<"java/lang/System">(env).get_static_method<void()>("gc");

  caught();

  assert(alive == 1);

  counted = false;

  for (int i = 0; i < 10 && alive > 0; i++) {
    gc();

    caught();
  }

  assert(alive == 0);
}
