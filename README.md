# pkexec-lpe-poc
POC for CVE-2021-4034

[Original Writeup](https://seclists.org/oss-sec/2022/q1/80)

For ease of use, it accepts a C file payload instead of a hardcoded shell.

usage:
```
make
./poc payload.c
```

tested on `Ubuntu 20.04.3 LTS - Linux target 5.4.0-81-generic`
