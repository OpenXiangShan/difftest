debug .
host $env(PLDM_HOST)
xc on -zt0 -xt0 -tbrun
date
run -swap
if {[info exists env(PLDM_MEM_LOAD)]} {
    source $env(PLDM_MEM_LOAD)
}
run
exit
