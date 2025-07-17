set file "${rtl_dir}/syscfg_define.v"
set file [file normalize $file]
set file_obj [get_files -of_objects [get_filesets sources_1] [list "*$file"]]
set_property -name "is_global_include" -value "1" -objects $file_obj

