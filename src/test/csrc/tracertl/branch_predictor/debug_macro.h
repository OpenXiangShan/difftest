#ifndef DEBUG_MACRO_H
#define DEBUG_MACRO_H

// #define TAGE_DEBUG
// #define BIM_DEBUG
// #define BTB_DEBUG
// #define RAS_DEBUG
// #define BPU_TOP_DEBUG
// #define ITTAGE_DEBUG
#if defined(TAGE_DEBUG) || defined(BIM_DEBUG) || defined(BTB_DEBUG) || defined(RAS_DEBUG) || defined(BPU_TOP_DEBUG) || defined(ITTAGE_DEBUG)
#define MAIN_DEBUG
#endif

// #define MAIN_VERBOSE

#endif // DEBUG_MACRO_H