#include <stdio.h>
#include "common.h"
#include "jtag_utils.h"


jtag_testcase_driver::jtag_testcase_driver() {
    char * noop_home = getenv("NOOP_HOME");
    assert(noop_home != NULL);
    char * filename = (char *) malloc(strlen(noop_home) + strlen("/jtag-input.txt"));
    strcpy(filename, noop_home);
    strcat(filename, "/jtag-input.txt");
    printf("JTAG Reading from%s\n", filename);
    test_case = fopen(filename, "r");
    assert(test_case);
    ended = false;
    tck_old == 0;
}

jtag_testcase_driver::~jtag_testcase_driver() {
  fclose(test_case);
}

void jtag_testcase_driver::tick(
    unsigned char * jtag_tck,
    unsigned char * jtag_tms,
    unsigned char * jtag_tdi,
    unsigned char * jtag_trstn,
    unsigned char jtag_tdo)
{
  if (ended) return;
  if (tck_old == 0) {
    char *test;
    unsigned int tms, tdi, tdo;

    // int ret = fscanf(test_case, "%s", &test);
    int ret = fscanf(test_case, "%d %d %d\n", &tms, &tdi, &tdo);
    // printf("Line %s\n", test);
    if(ret == 3) {
      *jtag_tck = 1;
      *jtag_tms = tms;
      *jtag_tdi = tdi;
      tck_old = 1;
    } else if (ret == EOF) {
      printf("End of JTAG Test-case!\n");
      ended = true;
      tck_old = 1;
    } else {
      assert(0);
    }
    printf("%d %d %d\n", * jtag_tms, *jtag_tdi, jtag_tdo);
  } else {
    *jtag_tck = 0;
    tck_old = 0;
  }

}



jtag_dump_helper::jtag_dump_helper() {
    char * noop_home = getenv("NOOP_HOME");
    assert(noop_home != NULL);
    char * filename = (char *) malloc(strlen(noop_home) + strlen("/jtag-dump.txt"));
    strcpy(filename, noop_home);
    strcat(filename, "/jtag-dump.txt");    
    printf("JTAG dumping to %s\n", filename);
    dump_file = fopen(filename, "w");
    assert(dump_file);
}

jtag_dump_helper::~jtag_dump_helper() {
  fclose(dump_file);
}

void jtag_dump_helper::dump(unsigned char tms, unsigned char tdi, unsigned char tdo) {
  fprintf(dump_file, "%d %d %d\n", tms, tdi, tdo);
}