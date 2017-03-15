#ifndef GUARDED_VALUE_H
#define GUARDED_VALUE_H

#include <mutex>

template <typename T>
class GuardedValue {
 public:
  GuardedValue() : set_(false), value_{} {};
  void hold() { mu_.lock(); }
  void drop() { mu_.unlock(); }

  // CALL HOLD BEFORE THIS.
  bool set() { return set_; }

  // CALL HOLD BEFORE THIS.
  void set(const T& v) {
    set_ = true;
    value_ = v;
  }

  // CALL HOLD BEFORE THIS.
  void unset() { set_ = false; }

  // CALL HOLD BEFORE THIS.
  const T& get() { return value_; }

 private:
  bool set_;
  T value_;
  std::mutex mu_;
};

#endif /* GUARDED_VALUE_H */
