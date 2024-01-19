// The memory used to load the checkpoint
#ifndef __XS_MEMRAM_H__
#define __XS_MEMRAM_H__
#include <common.h>
class xs_MemRam
{
  private:
    char *ram = NULL;
    uint64_t ram_size = 0;
    bool init_ok;
  public:
    xs_MemRam(uint64_t size);
    ~xs_MemRam();

    uint64_t read_data(uint64_t addr);
    void write_data(uint64_t addr, uint64_t w_mask, uint64_t data);

    void check_ram_addr(uint64_t addr);

    void load_bin(const char * bin_file);
};

#endif
