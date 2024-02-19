#!bin/bash
CHECKPOINT_PATH="your checkpoint-gz path"
RESULT_PATH="your log path"
CHECKPOINT_LIST="$CHECKPOINT_PATH/list.txt"

if [ ! -f "$CHECKPOINT_LIST" ]; then
    touch "$CHECKPOINT_LIST"
fi
for file in $CHECKPOINT_PATH/*/*/*.gz
do
    printf "%s\n" $file $ >> $CHECKPOINT_LIST
done

LOG_FILE="$RESULT_PATH/log.txt"

mkdir -p "$LOG_DIR"
if [ ! -f "$LOG_FILE" ]; then
    touch "$LOG_FILE"
fi

if make pldm-run PLDM_EXTRA_ARGS="+workload=$file +max-instrs=$suffix +=ckpt_list=$CHECKPOINT_LIST +diff=../../ready-to-run/riscv64-nemu-interpreter-so" >> $LOG_FILE;then
    printf "\033[1;32mPASS!\033[0m\n" >> RESULT
else
    printf "\033[1;31mFAIL!\033[0m\n" >> RESULT
fi

cat RESULT
rm -f RESULT
