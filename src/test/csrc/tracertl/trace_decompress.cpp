/***************************************************************************************
* Copyright (c) 2020-2023 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2020-2021 Peng Cheng Laboratory
*
* DiffTest is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/
#include <iostream>
#include "trace_decompress.h"

size_t traceDecompressSizeZSTD(const char *src, uint64_t src_len) {
  size_t const sizeAfterDC = ZSTD_getFrameContentSize(src, src_len);
  if (sizeAfterDC == ZSTD_CONTENTSIZE_ERROR || sizeAfterDC == ZSTD_CONTENTSIZE_UNKNOWN) {
    std::cerr << "Decompress Error. Wrong SizeAfterDC " << sizeAfterDC << std::endl;
    exit(1);
    return sizeAfterDC;
  }
  return sizeAfterDC;
}

uint64_t traceDecompressZSTD(char *dst, uint64_t dst_len, const char *src, uint64_t src_len) {
  ZSTD_DCtx *dctx = ZSTD_createDCtx();
  size_t decompressedSize = ZSTD_decompressDCtx(dctx, dst, dst_len, src, src_len);
  if (ZSTD_isError(decompressedSize)) {
    std::cerr << "Decompress Error " << ZSTD_getErrorName(decompressedSize) << std::endl;
    exit(1);
    return 0;
  }
  return decompressedSize;
}