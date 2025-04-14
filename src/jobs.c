#include "mysh.h"

int add_job(pid_t pgid, const char *cmdline, job_state_t st)
{
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!jobs[i].active) {
            jobs[i].active = 1;
            jobs[i].pgid   = pgid;
            jobs[i].state  = st;
            jobs[i].id     = i + 1;
            strncpy(jobs[i].cmdline, cmdline, MAX_INPUT - 1);
            if (njobs <= i) njobs = i + 1;
            return i + 1;
        }
    }
    fprintf(stderr, "%s: job table full\n", MYSH_NAME);
    return -1;
}

void remove_job(int id)
{
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].active && jobs[i].id == id) {
            jobs[i].active = 0;
            return;
        }
    }
}

job_t *find_job_by_pgid(pid_t pgid)
{
    for (int i = 0; i < MAX_JOBS; i++)
        if (jobs[i].active && jobs[i].pgid == pgid)
            return &jobs[i];
    return NULL;
}

void print_jobs(void)
{
    int any = 0;
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!jobs[i].active) continue;
        any = 1;
        const char *st = (jobs[i].state == JOB_RUNNING)  ? "running"  :
                         (jobs[i].state == JOB_STOPPED)   ? "stopped"  : "done";
        printf("[%d] %-10s %s\n", jobs[i].id, st, jobs[i].cmdline);
    }
    if (!any)
        printf("No active jobs.\n");
}

void reap_jobs(void)
{
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        job_t *j = find_job_by_pgid(pid);   /* pgid == first pid */
        if (!j) continue;

        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            printf("\n[%d] Done\t%s\n", j->id, j->cmdline);
            remove_job(j->id);
        } else if (WIFSTOPPED(status)) {
            j->state = JOB_STOPPED;
        }
    }
}
