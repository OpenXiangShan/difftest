/* Versioned, fixed-width UAPI for opt-in persistent XDMA C2H reception. */
#ifndef XDMA_C2H_POOL_UAPI_H
#define XDMA_C2H_POOL_UAPI_H
#include <linux/ioctl.h>
#include <linux/types.h>

#define XDMA_C2H_POOL_ABI      1U
#define XDMA_C2H_POOL_ADJACENT (1U << 1)
struct xdma_c2h_pool_config {
  __u32 version;
  __u32 packet_bytes;
  __u32 slots;
  __u32 flags;
};
struct xdma_c2h_pool_stats {
  __u64 launches;
  __u64 reads;
  __u64 packets;
  __u64 bytes;
  __u64 errors;
  __u64 partial_reads;
  __u32 packet_bytes;
  __u32 slots;
};
#define XDMA_IOCTL_C2H_POOL_SETUP _IOW('x', 16, struct xdma_c2h_pool_config)
#define XDMA_IOCTL_C2H_POOL_STATS _IOR('x', 17, struct xdma_c2h_pool_stats)
#define XDMA_IOCTL_C2H_POOL_STOP  _IO('x', 18)
#endif
