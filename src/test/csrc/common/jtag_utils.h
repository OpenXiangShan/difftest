#include<stdio.h>

class jtag_testcase_driver
{
  public:
    jtag_testcase_driver();
    ~jtag_testcase_driver();
    void tick(unsigned char * jtag_tck,
      unsigned char * jtag_tms,
      unsigned char * jtag_tdi,
      unsigned char * jtag_trstn,
      unsigned char jtag_tdo);

  private:
    char tck;
    char tck_old;
    bool ended;
    FILE* test_case;
};



class jtag_dump_helper{
  public:
    jtag_dump_helper();
    ~jtag_dump_helper();
    void dump(unsigned char tms, unsigned char tdi, unsigned char tdo);

  private:
    FILE* dump_file;
};


