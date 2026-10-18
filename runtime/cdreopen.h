/* Native POPS disc transport used by the main swap controller. */
#ifndef PSXCORE_CD_REOPEN_H
#define PSXCORE_CD_REOPEN_H
#include <stdint.h>
typedef struct {uint32_t active,attempts,successes;int32_t result;} NpCdReopen;
extern NpCdReopen np_cd_reopen;
/* Caller must first validate the pinned POPS revision and native call windows,
 * then own the main thread outside PS1/event dispatch, with normal interrupts,
 * POPS GP and a valid stack. This is a synchronous native RPC experiment and
 * cannot be called from the interrupt-masked gate. The caller supplies the lid
 * open transition; the status lid bit must remain set, while native commands
 * may update other mode/status bits. Successful completion closes it.
 * No new thread/semaphore,
 * bind, reset, save-path change or arbitrary image path is allowed.
 *
 * Only pfs0:/disc/disc0 is reopened. Exact header/TOC agreement is required with
 * the currently installed disc. The sector semaphore covers reset/open/TOC;
 * busy ownership or a pending worker request defers without media changes.
 * 1=success, 0=busy, -1=failed; failure retains lid-open status. Native IOPCD
 * does not acknowledge underlying close errors. Synchronous RPC has no timeout;
 * no transport/audio/DMA/save drain or production replacement claim follows. */
int np_cd_reopen_same_disc(void);
/* Validated replacement transport primitive. Caller must have retained and
 * validated the target image's exact 1024-byte TOC and sector count at startup,
 * registered the matching slot in the IOP bridge, and own the same main-thread
 * context as above. Geometry is committed only on success; failure leaves the
 * old TOC/last-LBA and the lid open. The main swap controller owns insertion and lid transitions. */
int np_cd_reopen_target(unsigned slot,const unsigned char expected[1024],uint32_t sectors);
#endif
