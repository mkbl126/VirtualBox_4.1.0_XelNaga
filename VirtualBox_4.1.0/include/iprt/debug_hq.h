
/* debug facilities for customization. */

#ifndef ___iprt_debug_hq_h
#define ___iprt_debug_hq_h


#include <stdio.h>
#include <stdarg.h>

/* declarations for the static functions. */ 
static void dbg_out(const char* file, const char *fmt, const char* arg1, const char* arg2);

/* print debug info into log files with var args. */
static void dbg_log(const char* file, const char * fmt, ...);


/* declarations for the static functions. */

/* print debug info into log files. */

static void dbg_out(const char* file, const char *fmt, const char* arg1, const char* arg2)
{
    FILE *fp = fopen(file, "a");
    fprintf(fp, fmt, arg1, arg2);
    fclose(fp);

    return;
}


/* print debug info into log files with no args limitations. */
static void dbg_log(const char* file, const char * fmt, ...)
{
    va_list arg_ptr;
    va_start(arg_ptr, fmt);

    FILE *fp = fopen(file, "a");
    vfprintf(fp, fmt, arg_ptr);
    fclose(fp);
	
    va_end(arg_ptr);
    return ;
}


#endif
