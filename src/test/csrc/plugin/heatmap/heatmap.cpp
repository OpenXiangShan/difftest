#include "coverage.h"

#ifndef COVER_BUCKET_CYCLES
#define COVER_BUCKET_CYCLES 1000ULL
#endif

#ifndef COVER_OUTPUT_FILE
#define COVER_OUTPUT_FILE "cover_branch_counts.csv"
#endif

// ===== 内部状态 =====
static uint64_t g_cycle = 0;                                   // 当前周期（由 tick 推进）
static uint64_t g_bucket_begin = 0;                            // 当前 bucket 的起始周期
static std::unordered_map<uint64_t, uint32_t> g_bucket_counts; // 稀疏计数：coverpoint_id -> 次数
static FILE *g_out = nullptr;

static inline const char *mod_name() {
  return firrtl_cover[0].cover.name ? firrtl_cover[0].cover.name : "branch";
}

static inline const char *cp_name(uint64_t id) {
  if (firrtl_cover[0].cover.point_names && id < firrtl_cover[0].cover.total) {
    const char *n = firrtl_cover[0].cover.point_names[id];
    return n ? n : "";
  }
  return "";
}

static void ensure_file_open() {
  if (!g_out) {
    g_out = std::fopen(COVER_OUTPUT_FILE, "w");
    if (!g_out) {
      std::perror("fopen COVER_OUTPUT_FILE");
      std::abort();
    }
    std::fprintf(g_out, "bucket_begin,bucket_end,module,coverpoint_id,coverpoint_name,count\n");
    std::fflush(g_out);
  }
}

static void flush_bucket_len(uint64_t bucket_len) {
  if (!g_out)
    return;
  const uint64_t bucket_end = (bucket_len == 0) ? g_bucket_begin : (g_bucket_begin + bucket_len - 1);

  const char *module = mod_name();
  for (const auto &kv: g_bucket_counts) {
    const uint64_t cp = kv.first;
    const uint32_t cnt = kv.second;
    if (cnt == 0)
      continue;
    std::fprintf(g_out, "%llu,%llu,%s,%llu,%s,%u\n", (unsigned long long)g_bucket_begin, (unsigned long long)bucket_end,
                 module, (unsigned long long)cp, firrtl_cover[0].cover.point_names[cp], cnt);
  }
  std::fflush(g_out);
  g_bucket_counts.clear();
  g_bucket_begin += bucket_len;
}

static void flush_full_bucket() {
  flush_bucket_len(COVER_BUCKET_CYCLES);
}

// ========== 对外接口 ==========

// 覆盖点触发：保留置位 + 计数
extern "C" void cover_dpi_branch(uint64_t index) {
  // 保留原 bitmap 行为
  if (index < firrtl_cover[0].cover.total && firrtl_cover[0].cover.points) {
    firrtl_cover[0].cover.points[index] = 1;
  }

  // 统计
  ensure_file_open();
  g_bucket_counts[index] += 1;
}

// 每周期调用一次（建议在 posedge 或 eval 后调用）
void heatmap_tick() {
  ++g_cycle;
  // 跨越 bucket 边界：落盘上一 bucket
  if (g_cycle >= g_bucket_begin + COVER_BUCKET_CYCLES) {
    flush_full_bucket();
  }
}

// 仿真结束：刷掉最后未满的 bucket（如果有）
void heatmap_finish() {
  if (g_out) {
    // 最后一段 bucket 长度 = 实际运行长度 - g_bucket_begin
    const uint64_t actual_end = (g_cycle == 0) ? 0 : (g_cycle - 1);
    const uint64_t rem_len = (actual_end >= g_bucket_begin) ? (actual_end - g_bucket_begin + 1) : 0ULL;

    if (!g_bucket_counts.empty() || rem_len > 0) {
      // 把最后一段（不足 COVER_BUCKET_CYCLES）也落盘
      flush_bucket_len(rem_len > 0 ? rem_len : 0ULL);
    }

    std::fclose(g_out);
    g_out = nullptr;
  }
}
