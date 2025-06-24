%module difftest

%{
#include "difftest.h"
#include "export.h"
#include "ram.h"
#include "flash.h"
#include "device.h"
#include "diffstate.h"
#include "difftrace.h"
#include "refproxy.h"
%}

#ifdef ENABLE_CHISEL_DB
%{
#include "perfCCT.h"
#include "chisel_db.h"
%}
#endif

%apply unsigned long long {u_int64_t}
%apply unsigned int {u_uint32_t}
%apply unsigned short {u_uint16_t}
%apply unsigned char {u_uint8_t}
%apply unsigned long long {uint64_t}
%apply unsigned int {uint32_t}
%apply unsigned short {uint16_t}
%apply unsigned char {uint8_t}
%apply long long {i_int64_t}
%apply int {i_int32_t}
%apply short {i_int16_t}
%apply char {i_int8_t}
%apply long long {int64_t}
%apply int {int32_t}
%apply short {int16_t}
%apply char {int8_t}

%include stdint.i
%include std_string.i
%include std_map.i
%include std_vector.i

#ifdef ENABLE_CHISEL_DB
%ignore std::mutex::mutex(const std::mutex&);
%ignore std::mutex::operator=;
#endif

%include "difftest.h"
%include "export.h"
%include "ram.h"
%include "flash.h"
%include "device.h"
%include "diffstate.h"
%include "difftrace.h"
%include "refproxy.h"

#ifdef ENABLE_CHISEL_DB
%include "perfCCT.h"
%include "chisel_db.h"
#endif

%define GAL_METHODS(STRUCT_TYPE, MEMBER)
%extend STRUCT_TYPE {
    uint64_t get_##MEMBER##_address() {
        return (uint64_t)((void*)&(self->MEMBER));
    }
    uint64_t get_##MEMBER##_length() {
        return (uint64_t)(sizeof(self->MEMBER));
    }
}
%enddef

GAL_METHODS(DifftestInstrCommit, pc)
GAL_METHODS(DifftestInstrCommit, valid)
GAL_METHODS(DifftestInstrCommit, instr)
GAL_METHODS(DifftestTrapEvent, pc)
GAL_METHODS(DifftestTrapEvent, code)
GAL_METHODS(DifftestTrapEvent, hasTrap)

