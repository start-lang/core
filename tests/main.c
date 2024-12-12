#include <stdlib.h>
#include <microcuts.h>
#include <lang_assertions.h>

int main(int argc, char* argv[]){
  print_info();
  set_cleanup(clean);
  set_target(validate);
  return run_target();
}

#ifdef EXPORT
int main_init(int argc, char* argv[]) {
  return main(argc, argv);
}

int main_step(void) {
  return 1;
}

void main_free(void) {}
#endif
