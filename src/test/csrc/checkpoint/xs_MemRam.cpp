// The memory used to load the checkpoint
#include <common.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <zlib.h>
#include "xs_MemRam.h"

using namespace std;
#define GCPT_MAX_SIZE 0x700
//#define ADDR_CHECK
static bool isGzFile(const char *filename);
static long readFromGz(void* ptr, const char *file_name, long buf_size);

xs_MemRam::xs_MemRam(uint64_t size)
{
  ram_size = size;
  ram = (char *)malloc(size);
}

xs_MemRam::~xs_MemRam()
{
  free(ram);
}

inline uint64_t
xs_MemRam::read_data(uint64_t addr)
{
#ifdef ADDR_CHECK
  check_ram_addr(addr);
#endif
  return (*((uint64_t *)ram + addr));
}

inline void
xs_MemRam::write_data(uint64_t addr, uint64_t w_mask, uint64_t data)
{
#ifdef ADDR_CHECK
  check_ram_addr(addr);
#endif
  uint64_t* mem = (uint64_t *)ram + addr;
  *mem = (data & w_mask) | (*mem & ~w_mask);
}

inline void
xs_MemRam::check_ram_addr(uint64_t addr)
{
  if (ram_size < addr) {
    printf("xs-ram addr out of bounds\n");
    assert(0);
  }
}

void
xs_MemRam::load_bin(const char * bin_file)
{
  if (isGzFile(bin_file)) {
    readFromGz(ram,bin_file,ram_size);
  }
  else {
    ifstream fp(bin_file,ios::binary);
    if (!fp) {
      printf("can not open %s \n",bin_file);
      free(ram);
      assert(0);
    }
    // get size
    fp.seekg(0, fp.end);
    uint64_t length = fp.tellg();
    fp.seekg(0, fp.beg);
    length = (length > ram_size) ? ram_size : length;
    // load bin
    if (fp.is_open()) {
      fp.read(ram, length);
    }

    fp.close();
  }
}


static char *bin_file = NULL;
static char *gcpt_bin_file = NULL;
static xs_MemRam *MemRam = NULL;
static bool init_ok = false;
extern "C" void ram_set_bin_file(char *s) {
  bin_file = (char *)malloc(256);
  strcpy(bin_file, s);
}

extern "C" void ram_set_gcpt_bin(char *s) {
  gcpt_bin_file = (char *)malloc(256);
  strcpy(gcpt_bin_file, s);
}

extern "C" void ram_read_data(uint64_t addr, uint64_t *r_data) {
  *r_data = MemRam->read_data(addr);
}

extern "C" void ram_write_data(uint64_t addr, uint64_t w_mask, uint64_t data) {
  MemRam->write_data(addr, w_mask, data);
}

extern "C" void init_ram(uint64_t size) {
  if (init_ok) return;
  assert(size > 0);

  MemRam = new xs_MemRam(size);
  printf("ram init size %d MB\t",size/1024/1024);

  if (bin_file != NULL) {
  MemRam->load_bin(bin_file);
  printf("load bin_file %s\n",bin_file);
    if (gcpt_bin_file != NULL) {
      // override gcpt-restore
      MemRam->load_bin(gcpt_bin_file);
      printf("ram override gcpt_bin_file %s\n",gcpt_bin_file);
    }
  } 
  else {
    eprintf("ram cannot be uninitialized");
    assert(0);
  }

  init_ok = 1;
}

extern "C" void ram_free() {
  MemRam->~xs_MemRam();
  init_ok = 0;
  delete MemRam;
}

// Return whether the file is a gz file
bool isGzFile(const char *filename) {
  if (filename == NULL || strlen(filename) < 4) {
    return false;
  }
  return !strcmp(filename + (strlen(filename) - 3), ".gz");
}

long readFromGz(void* ptr, const char *file_name, long buf_size) {
  assert(buf_size > 0);
  gzFile gz_file = gzopen(file_name, "rb");

  if (gz_file == NULL) {
    printf("Can't open compressed binary file '%s'", file_name);
    return -1;
  }

  uint64_t curr_size = 0;
  const uint32_t chunk_size = 16384;

  long *temp_page = new long[chunk_size];

  while (curr_size < buf_size) {
    uint32_t bytes_read = gzread(gz_file, temp_page, chunk_size * sizeof(long));
    if (bytes_read == 0) {
      break;
    }
    uint32_t copy_size = (bytes_read < (buf_size - curr_size)) ? bytes_read : (buf_size - curr_size);

    memcpy((uint8_t*)ptr + curr_size, temp_page, copy_size);
    curr_size += bytes_read;
  }

  if(gzread(gz_file, temp_page, chunk_size) > 0) {
    printf("File size is larger than buf_size!\n");
    assert(0);
  }

  delete [] temp_page;

  if(gzclose(gz_file)) {
    printf("Error closing '%s'\n", file_name);
    return -1;
  }
  return curr_size;
}