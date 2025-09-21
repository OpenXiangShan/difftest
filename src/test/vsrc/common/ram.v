`ifdef SYNTHESIS
  `define DISABLE_DIFFTEST_RAM_DPIC
`endif

`ifndef DISABLE_DIFFTEST_RAM_DPIC

import "DPI-C" function longint difftest_ram_read
(
  input longint index
);


import "DPI-C" function void difftest_ram_write
(
  input longint index,
  input longint data,
  input longint mask
);

`endif // DISABLE_DIFFTEST_RAM_DPIC
