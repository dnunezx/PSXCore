/* Startup progress is available only in the separate local diagnostic build. */
#ifndef PSXCORE_STARTUP_DIAG_H
#define PSXCORE_STARTUP_DIAG_H
#ifndef NP_STARTUP_DIAGNOSTICS
#define NP_STARTUP_DIAGNOSTICS 0
#endif
void np_startup_error(const char *code);
void np_startup_stop(const char *fallback);
#if NP_STARTUP_DIAGNOSTICS
void np_startup_init(void);
void np_startup_begin(unsigned stage,const char *operation);
void np_startup_operation(const char *operation);
void np_startup_operationf(const char *format,...) __attribute__((format(printf,1,2)));
void np_startup_done(void);
void np_startup_selection(const char *backend,unsigned device,const char *save_backend,
    unsigned save_device,unsigned environment,const char *path);
void np_startup_context(const char *format,...) __attribute__((format(printf,1,2)));
#else
/* Do not evaluate diagnostic arguments or retain their text in production. */
#define np_startup_init(...) ((void)0)
#define np_startup_begin(...) ((void)0)
#define np_startup_operation(...) ((void)0)
#define np_startup_operationf(...) ((void)0)
#define np_startup_done(...) ((void)0)
#define np_startup_selection(...) ((void)0)
#define np_startup_context(...) ((void)0)
#endif
#endif
