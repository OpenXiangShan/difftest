#!bin/bash
CHECKPOINT_PATH="/nfs/home/fengkehan/zstd-test"
CHECKPOINT_LIST="$CHECKPOINT_PATH/list-test.txt"

if [ ! -f "$CHECKPOINT_LIST" ]; then
    touch "$CHECKPOINT_LIST"
fi

for file in $CHECKPOINT_PATH/*/*/*.zstd
do
    if [ -e "$file" ]; then
        var=$file
        work_load_name=${var#$CHECKPOINT_PATH/}
        work_load_name=${work_load_name%/*}
        IFS='/' read -ra ADDR <<< "$work_load_name"  
        work_load_name=${ADDR[0]}
        prefix=${ADDR[1]}

        var=${var##*/}
        var=${var%.zstd}

        #prefix=${var%%.*}  # Get the first dot (.) The previous section
        suffix=${var#*.}   # Get the first dot (.) The next part
        suffix=$(echo $suffix | sed 's/[^0-9]//g')
        printf "%s %s %s %s %s\n" $file $suffix $work_load_name $prefix >> "$CHECKPOINT_LIST"
    fi
done