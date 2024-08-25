#ifndef vw_utilsvw_utils_h_
#define vw_utilsvw_utils_h_

#include "logging.h"
#include <format>
#include <map>
#include <string>
#include <vector>

struct ArgsVW {
  std::map<std::string, const char *> named_argv;
  std::vector<const char *> positional_argv;
  const char *find_named(const char *name) {
    auto val = named_argv.find(name);
    if (val == named_argv.end()) {
      LOG_ERROR(std::format("Missing arg -{} or --{}", name, name).c_str());
      return nullptr;
    }
    return val->second;
  }
};

ArgsVW parse_arg(int argc, char *argv[]);
#endif // vw_utilsvw_utils_h_
