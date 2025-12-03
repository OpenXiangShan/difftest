#ifndef __TRACERTL_FPGA_SIM_H__
#include <stdio.h>
#include <stdlib.h>

bool fpga_sim_write(char *buf, size_t size);
bool fpga_sim_read(char *buf, size_t size);

#endif // __TRACERTL_FPGA_SIM_H__