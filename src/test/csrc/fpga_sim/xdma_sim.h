
#ifndef __XDMA_SIM_H__
#define __XDMA_SIM_H__
#include <stddef.h>
#include <stdint.h>

void xdma_sim_open(int channel, bool is_host);
void xdma_sim_close(int channel);
int xdma_sim_read(int channel, char *buf, size_t size);
int xdma_sim_write(int channel, const char *buf, uint8_t tlast, size_t size);

#endif // __XDMA_SIM_H__
