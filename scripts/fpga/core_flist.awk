BEGIN{printf "set cpu_files [list \\\n"}
{
    printf " [file normalize \""
    printf $0
    printf "\" ]\\\n"
}
END{printf "]\n"}