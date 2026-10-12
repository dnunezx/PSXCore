/* Bootstrap-only display; never call after np_runtime_transfer. */
#ifndef PSXCORE_STARTUP_DIAG_H
#define PSXCORE_STARTUP_DIAG_H
void np_startup_init(void);
void np_startup_begin(unsigned stage,const char *operation);
void np_startup_operation(const char *operation);
void np_startup_done(void);
void np_startup_selection(const char *backend,unsigned device,const char *save_backend,
    unsigned save_device,unsigned environment,const char *path);
void np_startup_context(const char *format,...) __attribute__((format(printf,1,2)));
void np_startup_error(const char *code);
void np_startup_stop(const char *fallback);
#endif
