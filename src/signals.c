#include "mysh.h"

void sigchld_handler(int sig)
{
    (void)sig;
}

void setup_signals(void)
{
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);

    sa.sa_handler = SIG_IGN;
    sa.sa_flags   = 0;
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGTSTP, &sa, NULL);
    sigaction(SIGTTIN, &sa, NULL);
    sigaction(SIGTTOU, &sa, NULL);

    sa.sa_handler = sigchld_handler;
    sa.sa_flags   = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);
}
