
#include "swig_export.h"
#include "xspcomm/xcomm.h"

Difftest * GetDifftest(int index){
    if(index < NUM_CORES)return difftest[index];
    return NULL;
}

void InitRam(std::string image, uint64_t n_bytes){
    init_ram(image.c_str(), n_bytes);
}

void InitFlash(std::string flash_bin){
    if(flash_bin.empty()){
        init_flash(0);
        return;
    }
    init_flash(flash_bin.c_str());
}

uint64_t buff_picker_uart[2];
void     InitPikerUart(uint64_t p1, uint64_t p2){
    buff_picker_uart[0] = p1;
    buff_picker_uart[1] = p2;
}

uint64_t GetPickerUartArgPtr(){
    return (uint64_t)buff_picker_uart;
}

void picker_uart(uint64_t c, void *p){
    xspcomm::XData** pins = (xspcomm::XData **)p;
    if ((*pins[0]) != 0){
        fprintf(stderr,"%c", (char)(*pins[1]));
    }
}

uint64_t GetPickerUartFucPtr(){
    return (uint64_t)picker_uart;
}
