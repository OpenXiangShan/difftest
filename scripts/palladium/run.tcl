debug .
host bjos_emu
#run 1ns
#date
xc on -zt0 -xt0 -tbrun
date
run -swap
run
#date
#xc off
#date
#exit
