#!bin/bash
CHECKPOINT_PATH="your checkpoint-gz path"
RESULT_PATH="your log path"
for file in $CHECKPOINT_PATH/*/*/*.gz
do
    echo $file
    var=$file
    work_load_name=${var#$CHECKPOINT_PATH/}
    work_load_name=${work_load_name%/*}
    var=${var##*/}
    var=${var%.gz}

    prefix=${var%%.*}  # Get the first dot (.) The previous section
    suffix=${var#*.}   # Get the first dot (.) The next part
    suffix=$(echo $suffix | sed 's/[^0-9]//g')
    LOG_FILE="$RESULT_PATH/$work_load_name/$prefix.txt"
    LOG_DIR="$RESULT_PATH/$work_load_name"

    mkdir -p "$LOG_DIR"
    if [ ! -f "$LOG_FILE" ]; then
        touch "$LOG_FILE"
    fi

    if make pldm-run PLDM_EXTRA_ARGS="+workload=$file +max-instrs=$suffix +diff=../../ready-to-run/riscv64-nemu-interpreter-so" >> $LOG_FILE;then
        printf "[%14s]\t \033[1;32mPASS!\033[0m\n" $work_load_name/$prefix >> RESULT
    else
        printf "[%14s]\t \033[1;31mFAIL!\033[0m\n" $work_load_name/$prefix >> RESULT
    fi
done
cat RESULT
rm -f RESULT
