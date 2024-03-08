debug .
host $env(PLDM_HOST)
xc on -zt0 -xt0 -tbrun
date
run -swap
run
exit
