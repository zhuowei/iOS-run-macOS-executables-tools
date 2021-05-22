#include <mach-o/loader.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

int spawn_child(int argc, char** argv) { return -1; }

int map_dyld(int child_pid, const char* dyld_path) { return -1; }

int main(int argc, char** argv) {
  if (argc < 3) {
    fprintf(stderr,
            "usage: dyldloader <path_to_dyld> <executable> <args...>\n");
    return 1;
  }
  int child_pid = spawn_child(argc - 2, argv + 2);
  if (child_pid == -1) {
    return 1;
  }
  int err = map_dyld(child_pid, argv[1]);
  if (err) {
    kill(child_pid, SIGKILL);
    return 1;
  }
  kill(child_pid, SIGCONT);
  return 0;
}
