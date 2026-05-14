#include <assert.h>
#include <jnitl.h>
#include <thread>

int
main() {
  auto [vm, env] = java_vm_t::create("-Xcheck:jni");

  java_global_ref_t<java_object_t<"java/lang/String">> ref;

  std::thread([&] {
    auto worker = vm.attach_current_thread();
    auto string = java_string_t(worker, "jnitl");
    ref = java_global_ref_t<java_object_t<"java/lang/String">>(worker, string);
  }).join();

  assert(static_cast<jobject>(ref) != nullptr);

  auto moved = std::move(ref);

  assert(static_cast<jobject>(moved) != nullptr);
  assert(static_cast<jobject>(ref) == nullptr);
}
