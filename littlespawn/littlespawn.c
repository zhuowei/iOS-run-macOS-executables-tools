#include <errno.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
extern char** environ;
int main(int argc, char** argv) {
  printf("%d\n", getpid());
  // https://github.com/coolstar/electra/issues/53#issuecomment-359287851
  if (getenv("xDYLD_SHARED_CACHE_DIR")) {
    setenv("DYLD_SHARED_CACHE_DIR", getenv("xDYLD_SHARED_CACHE_DIR"), 1);
  }
  if (getenv("xDYLD_SHARED_REGION")) {
    setenv("DYLD_SHARED_REGION", getenv("xDYLD_SHARED_REGION"), 1);
  }
  if (getenv("xDYLD_ROOT_PATH")) {
    setenv("DYLD_ROOT_PATH", getenv("xDYLD_ROOT_PATH"), 1);
  }
  posix_spawnattr_t attr;
  posix_spawnattr_init(&attr);
  posix_spawnattr_setflags(&attr,
                           POSIX_SPAWN_START_SUSPENDED | POSIX_SPAWN_SETEXEC);

  int ret = posix_spawnp(NULL, argv[1], NULL, &attr, &argv[1], environ);
  if (ret) {
    fprintf(stderr, "failed to exec %s: %s\n", argv[1], strerror(errno));
  }
  return 1;
}
