#include <assert.h>
#include <jnitl.h>

int
main() {
  auto [vm, env] = java_vm_t::create();

  auto thread = java_thread_t::current_thread(env);

  auto class_loader = thread.get_context_class_loader();

  auto ref = java_local_ref_t<java_class_loader_t>(class_loader);

  auto string_class = ref.load_class<"java/lang/String">();
}
