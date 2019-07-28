
#include "rtl.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>


extern void SpikeTrap(void);
extern void SpikePrintStackTrace(struct Context *activeContext, FILE *out);


static void trapHandler(int sig, siginfo_t *info, void *vuc) {
    ucontext_t *uc = (ucontext_t *)vuc;
    greg_t spikeTrap;
    struct Context *activeContext;
    struct Symbol **sp;
    
    spikeTrap = (greg_t)&SpikeTrap;
    
    if (uc->uc_mcontext.gregs[REG_EIP] == spikeTrap) {
        
        /* Print the stack trace. */
        activeContext = (struct Context *)uc->uc_mcontext.gregs[REG_EBP];
        SpikePrintStackTrace(activeContext, stderr);
        
        /* Print the trap symbol. */
        sp = (struct Symbol **)uc->uc_mcontext.gregs[REG_ESP];
        fprintf(stderr, "$%s\n", sp[1]->str);
    }
    
    abort();
}


void SpikeInstallTrapHandler(void) {
    struct sigaction sa;
    
    sa.sa_sigaction = &trapHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO | SA_NODEFER;
    
    sigaction(SIGTRAP, &sa, NULL);
}

