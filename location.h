#pragma once
#include <string>

namespace helper {

// Location provides basic info where of an object was constructed, or was
// significantly brought to life.
class Location {
 public:
  // Constructor should be called with a long-lived char*, such as __FILE__.
  // It assumes the provided value will persist as a global constant, and it
  // will not make a copy of it.
  Location(const char* function_name, const char* file_name, int line_number)
      : function_name_(function_name),
        file_name_(file_name),
        line_number_(line_number) {}
  Location() = default;

  const char* function_name() const { return function_name_; }
  const char* file_name() const { return file_name_; }
  int line_number() const { return line_number_; }
  // TODO(steveanton): Remove once all downstream users have been updated to use
  // `file_name()` and/or `line_number()`.
  const char* file_and_line() const { return file_name_; }

  std::string ToString() {
      char buf[256];
      snprintf(buf, sizeof(buf), "%s@%s:%d", function_name_, file_name_,
          line_number_);
      return buf;
  };

 private:
  //文件名和函数名都是常量字符串，所以可以使用裸指针
  const char* function_name_ = "Unknown";
  const char* file_name_ = "Unknown";
  int line_number_ = -1;
};

// Define a macro to record the current source location.
#define HELPER_FROM_HERE HELPER_FROM_HERE_WITH_FUNCTION(__FUNCTION__)

#define HELPER_FROM_HERE_WITH_FUNCTION(function_name) \
  ::helper::Location(function_name, __FILE__, __LINE__)

}  // namespace helper

