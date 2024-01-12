debug .
host bjos_emu
xc xt0 zt0 on -tbrun
database -open xs
probe -create -all -depth all tb_top -database xs
#xeset mtHost
xeset traceMemSize 20000
xeset triggerPos 2
run -swap
run
database -upload
xc off
# exit
