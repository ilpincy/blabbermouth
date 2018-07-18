#ifndef BM_DEBUG_H
#define BM_DEBUG_H

/*
 * Prints a debug message, if the stream verbosity is 1.
 * This function works exactly like printf().
 * @param ds The datastream.
 * @param fmt The format of the string to print.
 */
extern void bm_debug(void* ds,
                     const char* fmt, ...);

#endif

