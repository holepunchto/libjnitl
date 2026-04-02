#pragma once

#include <array>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include <jni.h>

template <typename A, typename B>
constexpr bool java_is_same = false;

template <typename A>
constexpr bool java_is_same<A, A> = true;

template <size_t N>
struct java_string_literal_t {
  constexpr java_string_literal_t(const char (&data)[N]) {
    std::copy(data, data + N, data_);
  }

  constexpr operator const char *() const {
    return data_;
  }

  constexpr auto
  c_str() const {
    return data_;
  }

  char data_[N];
  static constexpr size_t size_ = N - 1;

  template <size_t P, size_t Q>
  friend constexpr auto
  operator+(const java_string_literal_t<P> &a, const java_string_literal_t<Q> &b);
};

template <size_t N, size_t M>
constexpr auto
operator+(const java_string_literal_t<N> &a, const java_string_literal_t<M> &b) {
  char result[N + M - 1];

  for (size_t i = 0; i < N - 1; i++) {
    result[i] = a.data_[i];
  }

  for (size_t i = 0; i < M - 1; i++) {
    result[N - 1 + i] = b.data_[i];
  }

  result[N + M - 2] = '\0';

  return java_string_literal_t<N + M - 1>(result);
}

template <size_t N, size_t M>
constexpr auto
operator+(const char (&a)[N], const java_string_literal_t<M> &b) {
  return java_string_literal_t<N>(a) + b;
}

template <size_t N, size_t M>
constexpr auto
operator+(const java_string_literal_t<N> &a, const char (&b)[M]) {
  return a + java_string_literal_t<M>(b);
}

struct java_value_t {
  java_value_t() : env_(nullptr) {}

  java_value_t(JNIEnv *env) : env_(env) {}

  java_value_t(java_value_t &&that) {
    this->swap(that);
  }

  java_value_t(const java_value_t &that) {
    env_ = that.env_;
  }

  virtual ~java_value_t() = default;

  java_value_t &
  operator=(java_value_t that) {
    this->swap(that);

    return *this;
  }

  void
  swap(java_value_t &that) {
    std::swap(env_, that.env_);
  }

protected:
  JNIEnv *env_;
};

template <size_t N>
using java_class_name_t = java_string_literal_t<N>;

template <java_class_name_t, typename>
struct java_field_t;

template <java_class_name_t, typename>
struct java_method_t;

template <java_class_name_t, typename>
struct java_class_t;

template <java_class_name_t N>
struct java_object_t : java_value_t {
  static constexpr java_class_name_t name = N;

  java_object_t() : java_value_t(), handle_(nullptr) {}

  java_object_t(JNIEnv *env, jobject handle) : java_value_t(env), handle_(handle) {}

  java_object_t(std::nullptr_t) : java_object_t() {}

  java_object_t(java_object_t &&that) {
    this->swap(that);
  }

  java_object_t(const java_object_t &that) : java_object_t(that.env_, that.handle_) {}

  java_object_t &
  operator=(java_object_t that) {
    this->swap(that);

    return *this;
  }

  operator jobject() const {
    return handle_;
  }

  void
  swap(java_object_t &that) {
    java_value_t::swap(that);

    std::swap(handle_, that.handle_);
  }

  template <typename T>
  T
  get(const java_field_t<N, T> &field) const;

  template <typename T>
  void
  set(const java_field_t<N, T> &field, T value) const;

  template <typename... A>
  void
  apply(const java_method_t<N, void(A...)> &method, A... args) const;

  template <typename R, typename... A>
  R
  apply(const java_method_t<N, R(A...)> &method, A... args) const;

  template <typename T = java_object_t<N>>
  auto
  get_class() {
    return java_class_t<N, java_object_t<N>>(env_, env_->GetObjectClass(handle_));
  }

  template <typename T = java_object_t<N>>
  static auto
  get_class(JNIEnv *env) {
    return java_class_t<N, java_object_t<N>>(env);
  }

private:
  template <typename>
  friend struct java_local_ref_t;

  template <typename>
  friend struct java_global_ref_t;

  template <typename>
  friend struct java_weak_local_ref_t;

protected:
  jobject handle_;
};

template <typename T>
struct java_local_ref_t : T {
  java_local_ref_t() : T() {}

  java_local_ref_t(JNIEnv *env, jobject handle) : T(env, env->NewLocalRef(handle)) {}

  java_local_ref_t(const T &object) : java_local_ref_t(object.env_, object.handle_) {}

  java_local_ref_t(java_local_ref_t &&that) {
    this->swap(that);
  }

  java_local_ref_t(const java_local_ref_t &that) : java_local_ref_t(that.env_, that.handle_) {}

  ~java_local_ref_t() {
    if (this->handle_) this->env_->DeleteLocalRef(this->handle_);
  }

  java_local_ref_t &
  operator=(java_local_ref_t that) {
    this->swap(that);

    return *this;
  }

  jobject release() {
    jobject h = this->handle_;
    this->handle_ = nullptr;
    return h;
  }
};

template <typename T>
struct java_global_ref_t : T {
  java_global_ref_t() : T() {}

  java_global_ref_t(JNIEnv *env, jobject handle) : T(env, env->NewGlobalRef(handle)) {}

  java_global_ref_t(const T &object) : java_global_ref_t(object.env_, object.handle_) {}

  java_global_ref_t(java_global_ref_t &&that) {
    this->swap(that);
  }

  java_global_ref_t(const java_global_ref_t &that) : java_global_ref_t(that.env_, that.handle_) {}

  ~java_global_ref_t() {
    if (this->handle_) this->env_->DeleteGlobalRef(this->handle_);
  }

  java_global_ref_t &
  operator=(java_global_ref_t that) {
    this->swap(that);

    return *this;
  }

  jobject release() {
    jobject h = this->handle_;
    this->handle_ = nullptr;
    return h;
  }
};

template <typename T>
struct java_weak_global_ref_t : T {
  java_weak_global_ref_t() : T() {}

  java_weak_global_ref_t(JNIEnv *env, jobject handle) : T(env, env->NewWeakGlobalRef(handle)) {}

  java_weak_global_ref_t(const T &object) : java_weak_global_ref_t(object.env_, object.handle_) {}

  java_weak_global_ref_t(java_weak_global_ref_t &&that) {
    this->swap(that);
  }

  java_weak_global_ref_t(const java_weak_global_ref_t &that) : java_weak_global_ref_t(that.env_, that.handle_) {}

  ~java_weak_global_ref_t() {
    if (this->handle_) this->env_->DeleteWeakGlobalRef(this->handle_);
  }

  java_weak_global_ref_t &
  operator=(java_weak_global_ref_t that) {
    this->swap(that);

    return *this;
  }

  jobject release() {
    jobject h = this->handle_;
    this->handle_ = nullptr;
    return h;
  }
};

struct java_string_t : java_object_t<"java/lang/String"> {
  java_string_t() : java_object_t(), utf8_(nullptr) {}

  java_string_t(JNIEnv *env, jobject handle) : java_object_t(env, handle), utf8_(nullptr) {}

  java_string_t(JNIEnv *env, const char *value) : java_string_t(env, env->NewStringUTF(value)) {}

  java_string_t(JNIEnv *env, const std::string &value) : java_string_t(env, value.c_str()) {}

  java_string_t(java_string_t &&that) {
    this->swap(that);
  }

  java_string_t(const java_string_t &that) : java_object_t(that), utf8_(nullptr) {}

  ~java_string_t() {
    if (utf8_) env_->ReleaseStringUTFChars(jstring(handle_), utf8_);
  }

  java_string_t &
  operator=(java_string_t that) {
    this->swap(that);

    return *this;
  }

  operator jstring() const {
    return jstring(handle_);
  }

  operator const char *() const {
    if (utf8_ == nullptr) utf8_ = env_->GetStringUTFChars(jstring(handle_), nullptr);

    return utf8_;
  }

  operator std::string() const {
    return static_cast<const char *>(*this);
  }

  void
  swap(java_string_t &that) {
    java_object_t::swap(that);

    std::swap(utf8_, that.utf8_);
  }

  auto
  c_str() const {
    return static_cast<const char *>(*this);
  }

private:
  mutable const char *utf8_;
};

template <typename T, typename U>
struct java_primitive_array_t : java_object_t<"java/lang/Object"> {
  static constexpr size_t npos = -1;

  java_primitive_array_t() : java_object_t(), elements_(nullptr) {}

  java_primitive_array_t(JNIEnv *env, jobject handle) : java_object_t(env, handle), elements_(nullptr) {}

  java_primitive_array_t(java_primitive_array_t &&that) {
    this->swap(that);
  }

  java_primitive_array_t(const java_primitive_array_t &that) : java_object_t(that), elements_(nullptr) {}

  java_primitive_array_t &
  operator=(java_primitive_array_t &&that) {
    this->swap(that);

    return *this;
  }

  java_primitive_array_t &
  operator=(const java_primitive_array_t &) = delete;

  operator T *() const {
    if (elements_ == nullptr) elements_ = get_elements();

    return reinterpret_cast<T *>(elements_);
  }

  T &operator[](std::size_t idx) {
    return static_cast<T *>(*this)[idx];
  }

  const T &operator[](std::size_t idx) const {
    return static_cast<T *>(*this)[idx];
  }

  void
  swap(java_primitive_array_t &that) {
    java_object_t::swap(that);

    std::swap(elements_, that.elements_);
  }

  auto
  size() const {
    return env_->GetArrayLength(jarray(handle_));
  }

  void
  commit() const {
    if (elements_) release_elements(JNI_COMMIT);
  }

  void
  abort() const {
    if (elements_ == nullptr) return;

    release_elements(JNI_ABORT);

    elements_ = nullptr;
  }

  void
  copy_to(std::span<T> dest, size_t start = 0) const {
    get_region(start, dest.size(), dest.data());
  }

  void
  copy_from(std::span<const T> src, size_t start = 0) {
    set_region(start, src.size(), src.data());
  }

  auto
  slice(size_t start = 0, size_t count = npos) const {
    if (count == npos) count = size() - start;

    std::vector<T> result(count);

    copy_to(result, start);

    return result;
  }

protected:
  virtual U *
  get_elements() const = 0;

  virtual void
  release_elements(int mode) const = 0;

  virtual void
  get_region(size_t start, size_t len, U *dest) const = 0;

  virtual void
  set_region(size_t start, size_t len, const U *src) = 0;

  mutable U *elements_;
};

template <typename T>
struct java_array_t;

template <>
struct java_array_t<bool> : java_primitive_array_t<bool, jboolean> {
  java_array_t() : java_primitive_array_t() {}

  java_array_t(JNIEnv *env, jobject handle) : java_primitive_array_t(env, handle) {}

  java_array_t(JNIEnv *env, int len) : java_array_t(env, env->NewBooleanArray(len)) {}

  java_array_t(java_array_t &&that) {
    this->swap(that);
  }

  java_array_t(const java_array_t &that) : java_primitive_array_t(that) {}

  ~java_array_t() override {
    if (elements_) release_elements(0);
  }

  java_array_t &
  operator=(java_array_t that) {
    this->swap(that);

    return *this;
  }

  operator jbooleanArray() const {
    return jbooleanArray(handle_);
  }

protected:
  jboolean *
  get_elements() const override {
    return env_->GetBooleanArrayElements(*this, nullptr);
  }

  void
  release_elements(int mode) const override {
    env_->ReleaseBooleanArrayElements(*this, elements_, mode);
  }

  void
  get_region(size_t start, size_t len, jboolean *dest) const override {
    env_->GetBooleanArrayRegion(*this, start, len, dest);
  }

  void
  set_region(size_t start, size_t len, const jboolean *src) override {
    env_->SetBooleanArrayRegion(*this, start, len, src);
  }
};

template <>
struct java_array_t<unsigned char> : java_primitive_array_t<unsigned char, jbyte> {
  java_array_t() : java_primitive_array_t() {}

  java_array_t(JNIEnv *env, jobject handle) : java_primitive_array_t(env, handle) {}

  java_array_t(JNIEnv *env, int len) : java_array_t(env, env->NewByteArray(len)) {}

  java_array_t(java_array_t &&that) {
    this->swap(that);
  }

  java_array_t(const java_array_t &that) : java_primitive_array_t(that) {}

  ~java_array_t() override {
    if (elements_) release_elements(0);
  }

  java_array_t &
  operator=(java_array_t that) {
    this->swap(that);

    return *this;
  }

  operator jbyteArray() const {
    return jbyteArray(handle_);
  }

protected:
  jbyte *
  get_elements() const override {
    return env_->GetByteArrayElements(*this, nullptr);
  }

  void
  release_elements(int mode) const override {
    env_->ReleaseByteArrayElements(*this, elements_, mode);
  }

  void
  get_region(size_t start, size_t len, jbyte *dest) const override {
    env_->GetByteArrayRegion(*this, start, len, dest);
  }

  void
  set_region(size_t start, size_t len, const jbyte *src) override {
    env_->SetByteArrayRegion(*this, start, len, src);
  }
};

template <>
struct java_array_t<char> : java_primitive_array_t<char, jchar> {
  java_array_t() : java_primitive_array_t() {}

  java_array_t(JNIEnv *env, jobject handle) : java_primitive_array_t(env, handle) {}

  java_array_t(JNIEnv *env, int len) : java_array_t(env, env->NewCharArray(len)) {}

  java_array_t(java_array_t &&that) {
    this->swap(that);
  }

  java_array_t(const java_array_t &that) : java_primitive_array_t(that) {}

  ~java_array_t() override {
    if (elements_) release_elements(0);
  }

  java_array_t &
  operator=(java_array_t that) {
    this->swap(that);

    return *this;
  }

  operator jcharArray() const {
    return jcharArray(handle_);
  }

protected:
  jchar *
  get_elements() const override {
    return env_->GetCharArrayElements(*this, nullptr);
  }

  void
  release_elements(int mode) const override {
    env_->ReleaseCharArrayElements(*this, elements_, mode);
  }

  void
  get_region(size_t start, size_t len, jchar *dest) const override {
    env_->GetCharArrayRegion(*this, start, len, dest);
  }

  void
  set_region(size_t start, size_t len, const jchar *src) override {
    env_->SetCharArrayRegion(*this, start, len, src);
  }
};

template <>
struct java_array_t<short> : java_primitive_array_t<short, jshort> {
  java_array_t() : java_primitive_array_t() {}

  java_array_t(JNIEnv *env, jobject handle) : java_primitive_array_t(env, handle) {}

  java_array_t(JNIEnv *env, int len) : java_array_t(env, env->NewShortArray(len)) {}

  java_array_t(java_array_t &&that) {
    this->swap(that);
  }

  java_array_t(const java_array_t &that) : java_primitive_array_t(that) {}

  ~java_array_t() override {
    if (elements_) release_elements(0);
  }

  java_array_t &
  operator=(java_array_t that) {
    this->swap(that);

    return *this;
  }

  operator jshortArray() const {
    return jshortArray(handle_);
  }

protected:
  jshort *
  get_elements() const override {
    return env_->GetShortArrayElements(*this, nullptr);
  }

  void
  release_elements(int mode) const override {
    env_->ReleaseShortArrayElements(*this, elements_, mode);
  }

  void
  get_region(size_t start, size_t len, jshort *dest) const override {
    env_->GetShortArrayRegion(*this, start, len, dest);
  }

  void
  set_region(size_t start, size_t len, const jshort *src) override {
    env_->SetShortArrayRegion(*this, start, len, src);
  }
};

template <>
struct java_array_t<int> : java_primitive_array_t<int, jint> {
  java_array_t() : java_primitive_array_t() {}

  java_array_t(JNIEnv *env, jobject handle) : java_primitive_array_t(env, handle) {}

  java_array_t(JNIEnv *env, int len) : java_array_t(env, env->NewIntArray(len)) {}

  java_array_t(java_array_t &&that) {
    this->swap(that);
  }

  java_array_t(const java_array_t &that) : java_primitive_array_t(that) {}

  ~java_array_t() override {
    if (elements_) release_elements(0);
  }

  java_array_t &
  operator=(java_array_t that) {
    this->swap(that);

    return *this;
  }

  operator jintArray() const {
    return jintArray(handle_);
  }

protected:
  jint *
  get_elements() const override {
    return env_->GetIntArrayElements(*this, nullptr);
  }

  void
  release_elements(int mode) const override {
    env_->ReleaseIntArrayElements(*this, elements_, mode);
  }

  void
  get_region(size_t start, size_t len, jint *dest) const override {
    env_->GetIntArrayRegion(*this, start, len, dest);
  }

  void
  set_region(size_t start, size_t len, const jint *src) override {
    env_->SetIntArrayRegion(*this, start, len, src);
  }
};

template <>
struct java_array_t<long> : java_primitive_array_t<long, jlong> {
  java_array_t() : java_primitive_array_t() {}

  java_array_t(JNIEnv *env, jobject handle) : java_primitive_array_t(env, handle) {}

  java_array_t(JNIEnv *env, int len) : java_array_t(env, env->NewLongArray(len)) {}

  java_array_t(java_array_t &&that) {
    this->swap(that);
  }

  java_array_t(const java_array_t &that) : java_primitive_array_t(that) {}

  ~java_array_t() override {
    if (elements_) release_elements(0);
  }

  java_array_t &
  operator=(java_array_t that) {
    this->swap(that);

    return *this;
  }

  operator jlongArray() const {
    return jlongArray(handle_);
  }

protected:
  jlong *
  get_elements() const override {
    return env_->GetLongArrayElements(*this, nullptr);
  }

  void
  release_elements(int mode) const override {
    env_->ReleaseLongArrayElements(*this, elements_, mode);
  }

  void
  get_region(size_t start, size_t len, jlong *dest) const override {
    env_->GetLongArrayRegion(*this, start, len, dest);
  }

  void
  set_region(size_t start, size_t len, const jlong *src) override {
    env_->SetLongArrayRegion(*this, start, len, src);
  }
};

template <>
struct java_array_t<float> : java_primitive_array_t<float, jfloat> {
  java_array_t() : java_primitive_array_t() {}

  java_array_t(JNIEnv *env, jobject handle) : java_primitive_array_t(env, handle) {}

  java_array_t(JNIEnv *env, int len) : java_array_t(env, env->NewFloatArray(len)) {}

  java_array_t(java_array_t &&that) {
    this->swap(that);
  }

  java_array_t(const java_array_t &that) : java_primitive_array_t(that) {}

  ~java_array_t() override {
    if (elements_) release_elements(0);
  }

  java_array_t &
  operator=(java_array_t that) {
    this->swap(that);

    return *this;
  }

  operator jfloatArray() const {
    return jfloatArray(handle_);
  }

protected:
  jfloat *
  get_elements() const override {
    return env_->GetFloatArrayElements(*this, nullptr);
  }

  void
  release_elements(int mode) const override {
    env_->ReleaseFloatArrayElements(*this, elements_, mode);
  }

  void
  get_region(size_t start, size_t len, jfloat *dest) const override {
    env_->GetFloatArrayRegion(*this, start, len, dest);
  }

  void
  set_region(size_t start, size_t len, const jfloat *src) override {
    env_->SetFloatArrayRegion(*this, start, len, src);
  }
};

template <>
struct java_array_t<double> : java_primitive_array_t<double, jdouble> {
  java_array_t() : java_primitive_array_t() {}

  java_array_t(JNIEnv *env, jobject handle) : java_primitive_array_t(env, handle) {}

  java_array_t(JNIEnv *env, int len) : java_array_t(env, env->NewDoubleArray(len)) {}

  java_array_t(java_array_t &&that) {
    this->swap(that);
  }

  java_array_t(const java_array_t &that) : java_primitive_array_t(that) {}

  ~java_array_t() override {
    if (elements_) release_elements(0);
  }

  java_array_t &
  operator=(java_array_t that) {
    this->swap(that);

    return *this;
  }

  operator jdoubleArray() const {
    return jdoubleArray(handle_);
  }

protected:
  jdouble *
  get_elements() const override {
    return env_->GetDoubleArrayElements(*this, nullptr);
  }

  void
  release_elements(int mode) const override {
    env_->ReleaseDoubleArrayElements(*this, elements_, mode);
  }

  void
  get_region(size_t start, size_t len, jdouble *dest) const override {
    env_->GetDoubleArrayRegion(*this, start, len, dest);
  }

  void
  set_region(size_t start, size_t len, const jdouble *src) override {
    env_->SetDoubleArrayRegion(*this, start, len, src);
  }
};

template <typename T>
struct java_array_t : java_object_t<"java/lang/Object"> {
  java_array_t() : java_object_t() {}

  java_array_t(JNIEnv *env, jobjectArray handle) : java_object_t(env, handle) {}

  java_array_t(JNIEnv *env, int len, jclass type, jobject initial)
      : java_array_t(env, env->NewObjectArray(len, type, initial)) {}

  java_array_t(JNIEnv *env, int len, jclass type)
      : java_array_t(env, len, type, nullptr) {}

  java_array_t(java_array_t &&that) {
    this->swap(that);
  }

  java_array_t(const java_array_t &that) : java_object_t(that) {}

  java_array_t &
  operator=(java_array_t that) {
    this->swap(that);

    return *this;
  }

  operator jobjectArray() const {
    return jobjectArray(handle_);
  }

  auto
  size() const {
    return env_->GetArrayLength(jarray(handle_));
  }

  auto
  get(int i) const {
    return java_unmarshall_value<T>(env_, env_->GetObjectArrayElement(*this, i));
  }

  void
  set(int i, T value) const {
    env_->SetObjectArrayElement(*this, i, java_marshall_value(env_, value));
  }
};

struct java_byte_buffer_t : java_object_t<"java/nio/ByteBuffer"> {
  java_byte_buffer_t() : java_object_t(), data_(nullptr), size_(0) {}

  java_byte_buffer_t(JNIEnv *env, jobject handle)
      : java_object_t(env, handle),
        data_(env->GetDirectBufferAddress(handle)),
        size_(env->GetDirectBufferCapacity(handle)) {}

  java_byte_buffer_t(JNIEnv *env, uint8_t *data, size_t len)
      : java_byte_buffer_t(env, env->NewDirectByteBuffer(data, len)) {}

  java_byte_buffer_t(java_byte_buffer_t &&that) {
    this->swap(that);
  }

  java_byte_buffer_t(const java_byte_buffer_t &that) : java_object_t(that), data_(that.data_), size_(that.size_) {}

  java_byte_buffer_t &
  operator=(java_byte_buffer_t that) {
    this->swap(that);

    return *this;
  }

  operator jobjectArray() const {
    return jobjectArray(handle_);
  }

  uint8_t &
  operator[](size_t i) {
    return reinterpret_cast<uint8_t *>(data_)[i];
  }

  const uint8_t
  operator[](size_t i) const {
    return reinterpret_cast<uint8_t *>(data_)[i];
  }

  void
  swap(java_byte_buffer_t &that) {
    java_object_t::swap(that);

    std::swap(data_, that.data_);
    std::swap(size_, that.size_);
  }

  auto
  data() const {
    return reinterpret_cast<uint8_t *>(data_);
  }

  auto
  size() const {
    return size_;
  }

  auto
  empty() const {
    return size_ == 0;
  }

  auto
  begin() const {
    return reinterpret_cast<uint8_t *>(data_);
  }

  auto
  end() const {
    return reinterpret_cast<uint8_t *>(data_) + size_;
  }

private:
  void *data_;
  size_t size_;
};

template <typename T>
struct java_type_info_t;

template <>
struct java_type_info_t<void> {
  using type = void;

  static constexpr java_string_literal_t signature = "V";
};

template <>
struct java_type_info_t<bool> {
  using type = jboolean;

  static constexpr java_string_literal_t signature = "Z";

  static auto
  marshall(JNIEnv *env, bool value) {
    return jboolean(value);
  }

  static auto
  unmarshall(JNIEnv *env, jboolean value) {
    return bool(value);
  }
};

template <>
struct java_type_info_t<unsigned char> {
  using type = jbyte;

  static constexpr java_string_literal_t signature = "B";

  static auto
  marshall(JNIEnv *env, unsigned char value) {
    return jbyte(value);
  }

  static auto
  unmarshall(JNIEnv *env, jbyte value) {
    return (unsigned char) (value);
  }
};

template <>
struct java_type_info_t<char> {
  using type = jchar;

  static constexpr java_string_literal_t signature = "C";

  static auto
  marshall(JNIEnv *env, char value) {
    return jchar(value);
  }

  static auto
  unmarshall(JNIEnv *env, jchar value) {
    return char(value);
  }
};

template <>
struct java_type_info_t<short> {
  using type = jshort;

  static constexpr java_string_literal_t signature = "S";

  static auto
  marshall(JNIEnv *env, short value) {
    return jshort(value);
  }

  static auto
  unmarshall(JNIEnv *env, jshort value) {
    return short(value);
  }
};

template <>
struct java_type_info_t<int> {
  using type = jint;

  static constexpr java_string_literal_t signature = "I";

  static auto
  marshall(JNIEnv *env, int value) {
    return jint(value);
  }

  static auto
  unmarshall(JNIEnv *env, jint value) {
    return int(value);
  }
};

template <>
struct java_type_info_t<long> {
  using type = jlong;

  static constexpr java_string_literal_t signature = "J";

  static auto
  marshall(JNIEnv *env, long value) {
    return jlong(value);
  }

  static auto
  unmarshall(JNIEnv *env, jlong value) {
    return long(value);
  }
};

template <>
struct java_type_info_t<long long> {
  using type = jlong;

  static constexpr java_string_literal_t signature = "J";

  static auto
  marshall(JNIEnv *env, long long value) {
    return jlong(value);
  }

  static auto
  unmarshall(JNIEnv *env, jlong value) {
    return (long long) (value);
  }
};

template <>
struct java_type_info_t<unsigned long> {
  using type = jlong;

  static constexpr java_string_literal_t signature = "J";

  static auto
  marshall(JNIEnv *env, unsigned long value) {
    return jlong(value);
  }

  static auto
  unmarshall(JNIEnv *env, jlong value) {
    return (unsigned long) (value);
  }
};

template <>
struct java_type_info_t<unsigned long long> {
  using type = jlong;

  static constexpr java_string_literal_t signature = "J";

  static auto
  marshall(JNIEnv *env, unsigned long long value) {
    return jlong(value);
  }

  static auto
  unmarshall(JNIEnv *env, jlong value) {
    return (unsigned long long) (value);
  }
};

template <>
struct java_type_info_t<float> {
  using type = jfloat;

  static constexpr java_string_literal_t signature = "F";

  static auto
  marshall(JNIEnv *env, float value) {
    return jfloat(value);
  }

  static auto
  unmarshall(JNIEnv *env, jfloat value) {
    return float(value);
  }
};

template <>
struct java_type_info_t<double> {
  using type = jdouble;

  static constexpr java_string_literal_t signature = "D";

  static auto
  marshall(JNIEnv *env, double value) {
    return jdouble(value);
  }

  static auto
  unmarshall(JNIEnv *env, jdouble value) {
    return double(value);
  }
};

template <java_class_name_t N>
struct java_type_info_t<java_object_t<N>> {
  using type = jobject;

  static constexpr java_string_literal_t signature = "L" + N + ";";

  static auto
  marshall(JNIEnv *env, const java_object_t<N> &value) {
    return static_cast<jobject>(value);
  }

  static auto
  unmarshall(JNIEnv *env, const jobject &value) {
    return java_object_t<N>(env, value);
  }
};

template <java_class_name_t N, typename T>
struct java_type_info_t<java_class_t<N, T>> {
  using type = jobject;

  static constexpr java_string_literal_t signature = "Ljava/lang/Class;";

  static auto
  marshall(JNIEnv *env, const java_class_t<N, T> &value) {
    return static_cast<jobject>(value);
  }

  static auto
  unmarshall(JNIEnv *env, const jobject &value) {
    return java_class_t<N, T>(env, value);
  }
};

template <typename R>
struct java_type_info_t<R(void)> {
  static constexpr java_string_literal_t signature = "()" + java_type_info_t<R>::signature;
};

template <typename R, typename... A>
struct java_type_info_t<R(A...)> {
  static constexpr java_string_literal_t signature = "(" + (java_type_info_t<A>::signature + ...) + ")" + java_type_info_t<R>::signature;
};

template <typename T>
struct java_type_info_t<java_array_t<T>> {
  using type = jobject;

  static constexpr java_string_literal_t signature = "[" + java_type_info_t<T>::signature;

  static auto
  marshall(JNIEnv *env, const java_array_t<T> &value) {
    return static_cast<jobject>(value);
  }

  static auto
  unmarshall(JNIEnv *env, const jobjectArray &value) {
    return java_array_t<T>(env, value);
  }
};

template <typename T>
struct java_type_info_t<std::vector<T>> {
  using type = jobject;

  static constexpr java_string_literal_t signature = "[" + java_type_info_t<T>::signature;
};

template <typename T, size_t N>
struct java_type_info_t<std::array<T, N>> {
  using type = jobject;

  static constexpr java_string_literal_t signature = "[" + java_type_info_t<T>::signature;
};

template <>
struct java_type_info_t<const char *> {
  using type = jobject;

  static constexpr java_string_literal_t signature = "Ljava/lang/String;";

  static auto
  marshall(JNIEnv *env, const char *value) {
    return env->NewStringUTF(value);
  }
};

template <>
struct java_type_info_t<std::string> {
  using type = jobject;

  static constexpr java_string_literal_t signature = "Ljava/lang/String;";

  static auto
  marshall(JNIEnv *env, const std::string &value) {
    return env->NewStringUTF(value.c_str());
  }

  static auto
  unmarshall(JNIEnv *env, const jobject &value) {
    auto chars = env->GetStringUTFChars(jstring(value), nullptr);

    std::string result(chars);

    env->ReleaseStringUTFChars(jstring(value), chars);

    return result;
  }
};

template <typename T>
static auto
java_marshall_value(JNIEnv *env, T value) {
  return java_type_info_t<T>::marshall(env, value);
}

template <typename T>
static jvalue
java_marshall_argument_value(JNIEnv *env, T value) {
  return jvalue{.l = reinterpret_cast<jobject>(java_marshall_value(env, value))};
}

template <typename T>
static auto
java_unmarshall_value(JNIEnv *env, typename java_type_info_t<T>::type value) {
  return java_type_info_t<T>::unmarshall(env, value);
}

struct java_env_t {
  java_env_t() : vm_(nullptr), env_(nullptr), detach_(false) {}

  java_env_t(JavaVM *vm, JNIEnv *env, bool detach) : vm_(vm), env_(env), detach_(detach) {}

  java_env_t(JNIEnv *env) : vm_(nullptr), env_(env), detach_(false) {
    env_->GetJavaVM(&vm_);
  }

  java_env_t(java_env_t &&that) : java_env_t() {
    this->swap(that);
  }

  java_env_t(const java_env_t &) = delete;

  ~java_env_t() {
    if (detach_) vm_->DetachCurrentThread();
  }

  java_env_t &
  operator=(java_env_t &&that) {
    this->swap(that);

    return *this;
  }

  java_env_t &
  operator=(const java_env_t &) = delete;

  operator JNIEnv *() const {
    return env_;
  }

  void
  swap(java_env_t &that) {
    std::swap(vm_, that.vm_);
    std::swap(env_, that.env_);
    std::swap(detach_, that.detach_);
  }

private:
  JavaVM *vm_;
  JNIEnv *env_;
  bool detach_;
};

struct java_vm_t {
  java_vm_t() : vm_(nullptr), destroy_(false) {}

  java_vm_t(JavaVM *vm, bool destroy) : vm_(vm), destroy_(destroy) {}

  java_vm_t(JavaVM *vm) : vm_(vm), destroy_(false) {}

  java_vm_t(java_vm_t &&that) : java_vm_t() {
    this->swap(that);
  }

  java_vm_t(const java_vm_t &) = delete;

  ~java_vm_t() {
    if (destroy_) vm_->DestroyJavaVM();
  }

  java_vm_t &
  operator=(java_vm_t &&that) {
    this->swap(that);

    return *this;
  }

  java_vm_t &
  operator=(const java_vm_t &) = delete;

  operator JavaVM *() const {
    return vm_;
  }

  void
  swap(java_vm_t &that) {
    std::swap(vm_, that.vm_);
    std::swap(destroy_, that.destroy_);
  }

  static std::optional<java_vm_t>
  get_created() {
    int err;

    JavaVM *vm;
    jsize len;
    err = JNI_GetCreatedJavaVMs(&vm, 1, &len);

    if (err != JNI_OK) return std::nullopt;

    return java_vm_t(vm, false);
  }

  static std::pair<java_vm_t, java_env_t>
  create(std::vector<std::string> options) {
    int err;

    std::vector<JavaVMOption> vm_options;

    vm_options.reserve(options.size());

    for (const auto &option : options) {
      vm_options.push_back({const_cast<char *>(option.c_str())});
    }

    JavaVMInitArgs vm_args;

    vm_args.version = JNI_VERSION_1_6;
    vm_args.options = vm_options.data();
    vm_args.nOptions = vm_options.size();
    vm_args.ignoreUnrecognized = true;

    JavaVM *vm;
    JNIEnv *env;

#if defined(__ANDROID__)
    err = JNI_CreateJavaVM(&vm, &env, &vm_args);
#else
    err = JNI_CreateJavaVM(&vm, reinterpret_cast<void **>(&env), reinterpret_cast<void *>(&vm_args));
#endif

    if (err != JNI_OK) throw std::invalid_argument("Could not create VM");

    return std::pair(java_vm_t(vm, true), java_env_t(vm, env, false));
  }

  static auto
  create(std::string options...) {
    return create(std::vector({options}));
  }

  static auto
  create() {
    return create(std::vector<std::string>());
  }

  std::optional<java_env_t>
  get_env() const {
    int err;

    JNIEnv *env;
    err = vm_->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);

    if (err != JNI_OK) return std::nullopt;

    return java_env_t(vm_, env, false);
  }

  auto
  attach_current_thread() const {
    int err;

    JNIEnv *env;

#if defined(__ANDROID__)
    err = vm_->AttachCurrentThread(&env, nullptr);
#else
    err = vm_->AttachCurrentThread(reinterpret_cast<void **>(&env), nullptr);
#endif

    if (err != JNI_OK) throw std::invalid_argument("Could not attach current thread");

    return java_env_t(vm_, env, true);
  }

private:
  JavaVM *vm_;
  bool destroy_;
};

template <auto fn>
struct java_callback_t;

template <typename T, typename R, typename... A, R fn(java_env_t, T, A...)>
struct java_callback_t<fn> {
  static constexpr java_string_literal_t signature = "(" + (java_type_info_t<A>::signature + ...) + ")" + java_type_info_t<R>::signature;

  static constexpr auto
  create() {
    return +[](JNIEnv *env, typename java_type_info_t<T>::type receiver, typename java_type_info_t<A>::type... args) -> typename java_type_info_t<R>::type {
      return apply(env, receiver, std::move(args)...);
    };
  }

  static constexpr auto
  apply(JNIEnv *env, typename java_type_info_t<T>::type receiver, typename java_type_info_t<A>::type... args) {
    return java_marshall_value<R>(env, fn(java_env_t(env), java_unmarshall_value<T>(env, std::move(receiver)), java_unmarshall_value<A>(env, std::move(args))...));
  }
};

template <auto fn>
struct java_type_info_t<java_callback_t<fn>> {
  static constexpr java_string_literal_t signature = java_callback_t<fn>::signature;
};

template <typename T>
struct java_field_accessor_t;

template <>
struct java_field_accessor_t<bool> {
  static auto
  get(JNIEnv *env, jobject receiver, jfieldID field) {
    return env->GetBooleanField(receiver, field);
  }

  static auto
  get(JNIEnv *env, jclass receiver, jfieldID field) {
    return env->GetStaticBooleanField(receiver, field);
  }

  static void
  set(JNIEnv *env, jobject receiver, jfieldID field, jboolean value) {
    env->SetBooleanField(receiver, field, value);
  }

  static void
  set(JNIEnv *env, jclass receiver, jfieldID field, jboolean value) {
    env->SetStaticBooleanField(receiver, field, value);
  }
};

template <>
struct java_field_accessor_t<unsigned char> {
  static auto
  get(JNIEnv *env, jobject receiver, jfieldID field) {
    return env->GetByteField(receiver, field);
  }

  static auto
  get(JNIEnv *env, jclass receiver, jfieldID field) {
    return env->GetStaticByteField(receiver, field);
  }

  static void
  set(JNIEnv *env, jobject receiver, jfieldID field, jbyte value) {
    env->SetByteField(receiver, field, value);
  }

  static void
  set(JNIEnv *env, jclass receiver, jfieldID field, jbyte value) {
    env->SetStaticByteField(receiver, field, value);
  }
};

template <>
struct java_field_accessor_t<char> {
  static auto
  get(JNIEnv *env, jobject receiver, jfieldID field) {
    return env->GetCharField(receiver, field);
  }

  static auto
  get(JNIEnv *env, jclass receiver, jfieldID field) {
    return env->GetStaticCharField(receiver, field);
  }

  static void
  set(JNIEnv *env, jobject receiver, jfieldID field, jchar value) {
    env->SetCharField(receiver, field, value);
  }

  static void
  set(JNIEnv *env, jclass receiver, jfieldID field, jchar value) {
    env->SetStaticCharField(receiver, field, value);
  }
};

template <>
struct java_field_accessor_t<short> {
  static auto
  get(JNIEnv *env, jobject receiver, jfieldID field) {
    return env->GetShortField(receiver, field);
  }

  static auto
  get(JNIEnv *env, jclass receiver, jfieldID field) {
    return env->GetStaticShortField(receiver, field);
  }

  static void
  set(JNIEnv *env, jobject receiver, jfieldID field, jshort value) {
    env->SetShortField(receiver, field, value);
  }

  static void
  set(JNIEnv *env, jclass receiver, jfieldID field, jshort value) {
    env->SetStaticShortField(receiver, field, value);
  }
};

template <>
struct java_field_accessor_t<int> {
  static auto
  get(JNIEnv *env, jobject receiver, jfieldID field) {
    return env->GetIntField(receiver, field);
  }

  static auto
  get(JNIEnv *env, jclass receiver, jfieldID field) {
    return env->GetStaticIntField(receiver, field);
  }

  static void
  set(JNIEnv *env, jobject receiver, jfieldID field, jint value) {
    env->SetIntField(receiver, field, value);
  }

  static void
  set(JNIEnv *env, jclass receiver, jfieldID field, jint value) {
    env->SetStaticIntField(receiver, field, value);
  }
};

template <>
struct java_field_accessor_t<long> {
  static auto
  get(JNIEnv *env, jobject receiver, jfieldID field) {
    return env->GetLongField(receiver, field);
  }

  static auto
  get(JNIEnv *env, jclass receiver, jfieldID field) {
    return env->GetStaticLongField(receiver, field);
  }

  static void
  set(JNIEnv *env, jobject receiver, jfieldID field, jlong value) {
    env->SetLongField(receiver, field, value);
  }

  static void
  set(JNIEnv *env, jclass receiver, jfieldID field, jlong value) {
    env->SetStaticLongField(receiver, field, value);
  }
};

template <>
struct java_field_accessor_t<float> {
  static auto
  get(JNIEnv *env, jobject receiver, jfieldID field) {
    return env->GetFloatField(receiver, field);
  }

  static auto
  get(JNIEnv *env, jclass receiver, jfieldID field) {
    return env->GetStaticFloatField(receiver, field);
  }

  static void
  set(JNIEnv *env, jobject receiver, jfieldID field, jfloat value) {
    env->SetFloatField(receiver, field, value);
  }

  static void
  set(JNIEnv *env, jclass receiver, jfieldID field, jfloat value) {
    env->SetStaticFloatField(receiver, field, value);
  }
};

template <>
struct java_field_accessor_t<double> {
  static auto
  get(JNIEnv *env, jobject receiver, jfieldID field) {
    return env->GetDoubleField(receiver, field);
  }

  static auto
  get(JNIEnv *env, jclass receiver, jfieldID field) {
    return env->GetStaticDoubleField(receiver, field);
  }

  static void
  set(JNIEnv *env, jobject receiver, jfieldID field, jdouble value) {
    env->SetDoubleField(receiver, field, value);
  }

  static void
  set(JNIEnv *env, jclass receiver, jfieldID field, jdouble value) {
    env->SetStaticDoubleField(receiver, field, value);
  }
};

template <typename T>
struct java_field_accessor_t {
  static auto
  get(JNIEnv *env, jobject receiver, jfieldID field) {
    return java_unmarshall_value<T>(env, env->GetObjectField(receiver, field));
  }

  static auto
  get(JNIEnv *env, jclass receiver, jfieldID field) {
    return java_unmarshall_value<T>(env, env->GetStaticObjectField(receiver, field));
  }

  static void
  set(JNIEnv *env, jobject receiver, jfieldID field, T value) {
    env->SetObjectField(receiver, field, java_marshall_value(env, value));
  }

  static void
  set(JNIEnv *env, jclass receiver, jfieldID field, T value) {
    env->SetStaticObjectField(receiver, field, java_marshall_value(value));
  }
};

struct java_field_base_t {
  java_field_base_t() : env_(nullptr), class_(nullptr), id_(nullptr) {}

  java_field_base_t(JNIEnv *env, jclass clazz, jfieldID id) : env_(env), class_(clazz), id_(id) {}

  java_field_base_t(java_field_base_t &&) = default;

  java_field_base_t(const java_field_base_t &) = delete;

  java_field_base_t &
  operator=(java_field_base_t &&) = default;

  java_field_base_t &
  operator=(const java_field_base_t &) = delete;

  operator jfieldID() const {
    return id_;
  }

protected:
  JNIEnv *env_;
  jclass class_;
  jfieldID id_;
};

template <java_class_name_t N, typename T>
struct java_field_t : java_field_base_t {
  auto
  get(const java_object_t<N> &receiver) const {
    return java_field_accessor_t<T>::get(env_, receiver, id_);
  }

  void
  set(const java_object_t<N> &receiver, T value) const {
    java_field_accessor_t<T>::set(env_, receiver, id_, value);
  }
};

template <java_class_name_t N, typename T>
struct java_static_field_t : java_field_base_t {
  auto
  get() const {
    return java_field_accessor_t<T>::get(env_, class_, id_);
  }

  void
  set(T value) const {
    java_field_accessor_t<T>::set(env_, class_, id_, value);
  }
};

template <java_class_name_t N>
template <typename T>
T
java_object_t<N>::get(const java_field_t<N, T> &field) const {
  return field.get(*this);
}

template <java_class_name_t N>
template <typename T>
void
java_object_t<N>::set(const java_field_t<N, T> &field, T value) const {
  return field.set(*this, value);
}

template <typename T>
struct java_method_invoker_t;

template <typename... A>
struct java_method_invoker_t<void(A...)> {
  static void
  call(JNIEnv *env, jobject receiver, jmethodID method, A... args) {
    jvalue argv[] = {
      java_marshall_argument_value(env, std::move(args))...
    };

    env->CallVoidMethodA(receiver, method, argv);
  }

  static void
  call(JNIEnv *env, jclass receiver, jmethodID method, A... args) {
    jvalue argv[] = {
      java_marshall_argument_value(env, std::move(args))...
    };

    env->CallStaticVoidMethodA(receiver, method, argv);
  }
};

template <typename... A>
struct java_method_invoker_t<bool(A...)> {
  static bool
  call(JNIEnv *env, jobject receiver, jmethodID method, A... args) {
    jvalue argv[] = {
      java_marshall_argument_value(env, std::move(args))...
    };

    return env->CallBooleanMethodA(receiver, method, argv);
  }

  static bool
  call(JNIEnv *env, jclass receiver, jmethodID method, A... args) {
    jvalue argv[] = {
      java_marshall_argument_value(env, std::move(args))...
    };

    return env->CallStaticBooleanMethodA(receiver, method, argv);
  }
};

template <typename... A>
struct java_method_invoker_t<unsigned char(A...)> {
  static unsigned char
  call(JNIEnv *env, jobject receiver, jmethodID method, A... args) {
    jvalue argv[] = {
      java_marshall_argument_value(env, std::move(args))...
    };

    return env->CallByteMethodA(receiver, method, argv);
  }

  static unsigned char
  call(JNIEnv *env, jclass receiver, jmethodID method, A... args) {
    jvalue argv[] = {
      java_marshall_argument_value(env, std::move(args))...
    };

    return env->CallStaticByteMethodA(receiver, method, argv);
  }
};

template <typename... A>
struct java_method_invoker_t<char(A...)> {
  static char
  call(JNIEnv *env, jobject receiver, jmethodID method, A... args) {
    jvalue argv[] = {
      java_marshall_argument_value(env, std::move(args))...
    };

    return env->CallCharMethodA(receiver, method, argv);
  }

  static char
  call(JNIEnv *env, jclass receiver, jmethodID method, A... args) {
    jvalue argv[] = {
      java_marshall_argument_value(env, std::move(args))...
    };

    return env->CallStaticCharMethodA(receiver, method, argv);
  }
};

template <typename... A>
struct java_method_invoker_t<short(A...)> {
  static short
  call(JNIEnv *env, jobject receiver, jmethodID method, A... args) {
    jvalue argv[] = {
      java_marshall_argument_value(env, std::move(args))...
    };

    return env->CallShortMethodA(receiver, method, argv);
  }

  static char
  call(JNIEnv *env, jclass receiver, jmethodID method, A... args) {
    jvalue argv[] = {
      java_marshall_argument_value(env, std::move(args))...
    };

    return env->CallStaticShortMethodA(receiver, method, argv);
  }
};

template <typename... A>
struct java_method_invoker_t<int(A...)> {
  static int
  call(JNIEnv *env, jobject receiver, jmethodID method, A... args) {
    jvalue argv[] = {
      java_marshall_argument_value(env, std::move(args))...
    };

    return env->CallIntMethodA(receiver, method, argv);
  }

  static int
  call(JNIEnv *env, jclass receiver, jmethodID method, A... args) {
    jvalue argv[] = {
      java_marshall_argument_value(env, std::move(args))...
    };

    return env->CallStaticIntMethodA(receiver, method, argv);
  }
};

template <typename... A>
struct java_method_invoker_t<long(A...)> {
  static long
  call(JNIEnv *env, jobject receiver, jmethodID method, A... args) {
    jvalue argv[] = {
      java_marshall_argument_value(env, std::move(args))...
    };

    return env->CallLongMethodA(receiver, method, argv);
  }

  static long
  call(JNIEnv *env, jclass receiver, jmethodID method, A... args) {
    jvalue argv[] = {
      java_marshall_argument_value(env, std::move(args))...
    };

    return env->CallStaticLongMethodA(receiver, method, argv);
  }
};

template <typename... A>
struct java_method_invoker_t<float(A...)> {
  static float
  call(JNIEnv *env, jobject receiver, jmethodID method, A... args) {
    jvalue argv[] = {
      java_marshall_argument_value(env, std::move(args))...
    };

    return env->CallFloatMethodA(receiver, method, argv);
  }

  static float
  call(JNIEnv *env, jclass receiver, jmethodID method, A... args) {
    jvalue argv[] = {
      java_marshall_argument_value(env, std::move(args))...
    };

    return env->CallStaticFloatMethodA(receiver, method, argv);
  }
};

template <typename... A>
struct java_method_invoker_t<double(A...)> {
  static double
  call(JNIEnv *env, jobject receiver, jmethodID method, A... args) {
    jvalue argv[] = {
      java_marshall_argument_value(env, std::move(args))...
    };

    return env->CallDoubleMethodA(receiver, method, argv);
  }

  static double
  call(JNIEnv *env, jclass receiver, jmethodID method, A... args) {
    jvalue argv[] = {
      java_marshall_argument_value(env, std::move(args))...
    };

    return env->CallStaticDoubleMethodA(receiver, method, argv);
  }
};

template <typename R, typename... A>
struct java_method_invoker_t<R(A...)> {
  static R
  call(JNIEnv *env, jobject receiver, jmethodID method, A... args) {
    jvalue argv[] = {
      java_marshall_argument_value(env, std::move(args))...
    };

    return java_unmarshall_value<R>(env, env->CallObjectMethodA(receiver, method, argv));
  }

  static R
  call(JNIEnv *env, jclass receiver, jmethodID method, A... args) {
    jvalue argv[] = {
      java_marshall_argument_value(env, std::move(args))...
    };

    return java_unmarshall_value<R>(env, env->CallStaticObjectMethodA(receiver, method, argv));
  }
};

struct java_method_base_t {
  java_method_base_t() : env_(nullptr), class_(nullptr), id_(nullptr) {}

  java_method_base_t(JNIEnv *env, jclass clazz, jmethodID id) : env_(env), class_(clazz), id_(id) {}

  java_method_base_t(java_method_base_t &&) = default;

  java_method_base_t(const java_method_base_t &) = delete;

  java_method_base_t &
  operator=(java_method_base_t &&) = default;

  java_method_base_t &
  operator=(const java_method_base_t &) = delete;

  operator jmethodID() const {
    return id_;
  }

protected:
  JNIEnv *env_;
  jclass class_;
  jmethodID id_;
};

template <java_class_name_t, typename>
struct java_method_t;

template <java_class_name_t N, typename... A>
struct java_method_t<N, void(A...)> : java_method_base_t {
  void call(const java_object_t<N> &receiver, A... args) const {
    java_method_invoker_t<void(A...)>::call(env_, receiver, id_, std::move(args)...);
  }

  void operator()(const java_object_t<N> &receiver, A... args) const {
    call(receiver, std::move(args)...);
  }
};

template <java_class_name_t N, typename R, typename... A>
struct java_method_t<N, R(A...)> : java_method_base_t {
  R call(const java_object_t<N> &receiver, A... args) const {
    return java_method_invoker_t<R(A...)>::call(env_, receiver, id_, std::move(args)...);
  }

  R operator()(const java_object_t<N> &receiver, A... args) const {
    return call(receiver, std::move(args)...);
  }
};

template <java_class_name_t N>
template <typename... A>
void
java_object_t<N>::apply(const java_method_t<N, void(A...)> &method, A... args) const {
  method(*this, std::move(args)...);
}

template <java_class_name_t N>
template <typename R, typename... A>
R
java_object_t<N>::apply(const java_method_t<N, R(A...)> &method, A... args) const {
  return method(*this, std::move(args)...);
}

template <typename T>
struct java_static_method_t;

template <typename... A>
struct java_static_method_t<void(A...)> : java_method_base_t {
  void call(A... args) const {
    java_method_invoker_t<void(A...)>::call(env_, class_, id_, std::move(args)...);
  }

  void operator()(A... args) const {
    call(std::move(args)...);
  }
};

template <typename R, typename... A>
struct java_static_method_t<R(A...)> : java_method_base_t {
  R call(A... args) const {
    return java_method_invoker_t<R(A...)>::call(env_, class_, id_, std::move(args)...);
  }

  R operator()(A... args) const {
    return call(std::move(args)...);
  }
};

template <auto fn>
struct java_native_method_t {
  java_native_method_t(const char *name) : name_(name) {}

  java_native_method_t(const std::string &name) : name_(name) {}

  java_native_method_t(java_native_method_t &&) = default;

  java_native_method_t(const java_native_method_t &) = delete;

  java_native_method_t &
  operator=(java_native_method_t &&) = default;

  java_native_method_t &
  operator=(const java_native_method_t &) = delete;

private:
  operator JNINativeMethod() const {
    return {
      .name = const_cast<char *>(name_.c_str()),
      .signature = const_cast<char *>(java_callback_t<fn>::signature.c_str()),
      .fnPtr = reinterpret_cast<void *>(java_callback_t<fn>::create()),
    };
  }

  template <java_class_name_t, typename>
  friend struct java_class_t;

private:
  std::string name_;
};

template <typename T>
constexpr bool java_is_native_method = false;

template <auto fn>
constexpr bool java_is_native_method<java_native_method_t<fn>> = true;

template <typename T>
concept java_native_method = java_is_native_method<T>;

template <java_class_name_t N, typename T = java_object_t<N>>
struct java_class_t : java_object_t<"java/lang/Class"> {
  java_class_t() : java_object_t() {}

  java_class_t(JNIEnv *env, jobject clazz) : java_object_t(env, clazz) {}

  java_class_t(JNIEnv *env) : java_object_t(env, nullptr) {
    handle_ = env_->FindClass(N);

    if (handle_ == nullptr) {
      env_->ExceptionClear();

      throw std::invalid_argument("Could not find class with name '" + std::string(name) + "'");
    }
  }

  java_class_t(java_class_t &&that) {
    this->swap(that);
  }

  java_class_t(const java_class_t &that) : java_object_t(that) {}

  java_class_t &
  operator=(java_class_t that) {
    this->swap(that);

    return *this;
  }

  operator jclass() const {
    return handle_;
  }

  template <typename... A>
  T operator()(A... args) const {
    auto init = get_method<void(A...)>("<init>");

    jvalue argv[] = {
      java_marshall_argument_value(env_, std::move(args))...
    };

    return T(env_, env_->NewObjectA(jclass(handle_), init, argv));
  }

  template <typename U>
  auto
  get_field(const char *name) const {
    auto signature = java_type_info_t<U>::signature.c_str();

    auto id = env_->GetFieldID(jclass(handle_), name, signature);

    if (id == nullptr) {
      throw std::invalid_argument(
        "Could not find field '" + std::string(name) + "' with signature '" + std::string(signature) + "'"
      );
    }

    return java_field_t<N, U>(java_field_base_t(env_, jclass(handle_), id));
  }

  template <typename U>
  auto
  get_field(std::string name) const {
    return get_field<U>(name.c_str());
  }

  template <typename U>
  auto
  get_static_field(const char *name) const {
    auto signature = java_type_info_t<U>::signature.c_str();

    auto id = env_->GetStaticFieldID(jclass(handle_), name, signature);

    if (id == nullptr) {
      throw std::invalid_argument(
        "Could not find field '" + std::string(name) + "' with signature '" + std::string(signature) + "'"
      );
    }

    return java_static_field_t<N, U>(java_field_base_t(env_, jclass(handle_), id));
  }

  template <typename U>
  auto
  get_static_field(std::string name) const {
    return get_static_field<U>(name.c_str());
  }

  template <typename U>
  auto
  get_method(const char *name) const {
    auto signature = java_type_info_t<U>::signature.c_str();

    auto id = env_->GetMethodID(jclass(handle_), name, signature);

    if (id == nullptr) {
      throw std::invalid_argument(
        "Could not find method '" + std::string(name) + "' with signature '" + std::string(signature) + "'"
      );
    }

    return java_method_t<N, U>(java_method_base_t(env_, jclass(handle_), id));
  }

  template <typename U>
  auto
  get_method(std::string name) const {
    return get_method<U>(name.c_str());
  }

  template <typename U>
  auto
  get_static_method(const char *name) const {
    auto signature = java_type_info_t<U>::signature.c_str();

    auto id = env_->GetStaticMethodID(jclass(handle_), name, signature);

    if (id == nullptr) {
      throw std::invalid_argument(
        "Could not find method '" + std::string(name) + "' with signature '" + std::string(signature) + "'"
      );
    }

    return java_static_method_t<U>(java_method_base_t(env_, jclass(handle_), id));
  }

  template <typename U>
  auto
  get_static_method(std::string name) const {
    return get_static_method<U>(name.c_str());
  }

  template <typename U>
  U
  get(const java_static_field_t<N, U> &field) const {
    return java_field_accessor_t<U>::get(env_, jclass(handle_), field);
  }

  template <typename U>
  void
  set(const java_static_field_t<N, U> &field, const U &value) const {
    java_field_accessor_t<U>::set(env_, jclass(handle_), field, value);
  }

  template <typename... A>
  void
  apply(const java_static_method_t<void(A...)> &method, A... args) const {
    method(*this, std::move(args)...);
  }

  template <typename R, typename... A>
  R
  apply(const java_static_method_t<R(A...)> &method, A... args) const {
    return method(*this, std::move(args)...);
  }

  template <java_native_method... M>
  void
  register_natives(M... methods) {
    env_->RegisterNatives(jclass(handle_), (JNINativeMethod[]) {methods...}, sizeof...(M));
  }

  void
  unregister_natives() {
    env_->UnregisterNatives(jclass(handle_));
  }
};

struct java_class_loader_t : java_object_t<"java/lang/ClassLoader"> {
  java_class_loader_t() : java_object_t() {}

  java_class_loader_t(JNIEnv *env, jobject handle) : java_object_t(env, handle) {}

  java_class_loader_t(java_class_loader_t &&that) {
    this->swap(that);
  }

  java_class_loader_t(const java_class_loader_t &that) : java_object_t(that) {}

  java_class_loader_t &
  operator=(java_class_loader_t that) {
    this->swap(that);

    return *this;
  }

  template <java_class_name_t N, typename T = java_object_t<N>>
  auto
  load_class(const char *class_name) {
    auto load_class = get_class().get_method<java_class_t<N, T>(const char *)>("loadClass");

    return java_class_t<N, T>(env_, load_class(*this, class_name));
  }

  template <java_class_name_t N, typename T = java_object_t<N>>
  auto
  load_class(const std::string &class_name) {
    return load_class<N, T>(class_name.c_str());
  }

  template <java_class_name_t N, typename T = java_object_t<N>>
  auto
  load_class() {
    auto class_name = std::string(N);

    for (auto &c : class_name) {
      if (c == '/') c = '.';
    }

    return load_class<N, T>(class_name);
  }
};

struct java_thread_t : java_object_t<"java/lang/Thread"> {
  java_thread_t() : java_object_t() {}

  java_thread_t(JNIEnv *env, jobject handle) : java_object_t(env, handle) {}

  java_thread_t(java_thread_t &&that) {
    this->swap(that);
  }

  java_thread_t(const java_thread_t &that) : java_object_t(that) {}

  java_thread_t &
  operator=(java_thread_t that) {
    this->swap(that);

    return *this;
  }

  auto
  get_context_class_loader() {
    auto get_context_class_loader = get_class().get_method<java_object_t<"java/lang/ClassLoader">()>("getContextClassLoader");

    return java_class_loader_t(env_, get_context_class_loader(*this));
  }

  static auto
  current_thread(JNIEnv *env) {
    auto current_thread = get_class(env).get_static_method<java_object_t<"java/lang/Thread">()>("currentThread");

    return java_thread_t(env, current_thread());
  }
};
