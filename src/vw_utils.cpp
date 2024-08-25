#include "vw_utils.h"
ArgsVW parse_arg(int argc, char *argv[]) {
  auto args_vw = ArgsVW();
  for (int i = 0; i < argc; i++) {
    char *arg = argv[i];
    if (arg[0] == '-' && strlen(arg) > 1 && i < argc - 1) {
      std::string name = std::string(arg).replace(0, arg[1] == '-' ? 2 : 1, "");
      args_vw.named_argv[name] = argv[i + 1];
      i++;
    } else {
      args_vw.positional_argv.push_back(argv[i]);
    }
  }
  return args_vw;
}
