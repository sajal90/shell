#ifndef MYSH_H
#define MYSH_H

#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE   700
#ifndef PATH_MAX
#  define PATH_MAX 4096
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <termios.h>
#include <limits.h>
#include <dirent.h>
#include <pwd.h>
#include <ctype.h>

/* ── Constants ─────────────────────────────────────────── */
#define MYSH_VERSION      "1.0.0"
#define MYSH_NAME         "mysh"
#define MAX_INPUT         4096
#define MAX_ARGS          256
#define MAX_PIPES         32
#define HIST_MAX          500
#define MAX_JOBS          64

/* ── Colours ────────────────────────────────────────────── */
#define CLR_RESET   "\033[0m"
#define CLR_BOLD    "\033[1m"
#define CLR_RED     "\033[31m"
#define CLR_GREEN   "\033[32m"
#define CLR_YELLOW  "\033[33m"
#define CLR_BLUE    "\033[34m"
#define CLR_CYAN    "\033[36m"

/* ── Job states ─────────────────────────────────────────── */
typedef enum {
    JOB_RUNNING,
    JOB_STOPPED,
    JOB_DONE
} job_state_t;

/* ── Job entry ──────────────────────────────────────────── */
typedef struct {
    int       id;
    pid_t     pgid;
    job_state_t state;
    char      cmdline[MAX_INPUT];
    int       active;
} job_t;

/* ── Redirect info ──────────────────────────────────────── */
typedef struct {
    char *infile;
    char *outfile;
    char *errfile;
    int   append;         /* 1 if >> */
} redirect_t;

/* ── Single command in a pipeline ───────────────────────── */
typedef struct {
    char      *argv[MAX_ARGS];
    int        argc;
    redirect_t redir;
} cmd_t;

/* ── Pipeline ───────────────────────────────────────────── */
typedef struct {
    cmd_t  cmds[MAX_PIPES];
    int    ncmds;
    int    background;
} pipeline_t;

/* ── History ─────────────────────────────────────────────── */
typedef struct {
    char  *entries[HIST_MAX];
    int    count;
    int    head;          /* ring-buffer head */
} history_t;

/* ── Globals (declared in main.c) ────────────────────────── */
extern job_t     jobs[MAX_JOBS];
extern int       njobs;
extern history_t history;
extern int       last_exit;
extern char      cwd[PATH_MAX];

/* ── Function prototypes ─────────────────────────────────── */

/* parser.c */
int  parse_line(char *line, pipeline_t *pl);

/* executor.c */
int  execute_pipeline(pipeline_t *pl);

/* builtins.c */
int  is_builtin(const char *cmd);
int  run_builtin(cmd_t *cmd);

/* jobs.c */
int   add_job(pid_t pgid, const char *cmdline, job_state_t st);
void  remove_job(int id);
job_t *find_job_by_pgid(pid_t pgid);
void  print_jobs(void);
void  reap_jobs(void);

/* history.c */
void  history_add(const char *line);
void  history_print(void);
char *history_get(int n);

/* signals.c */
void  setup_signals(void);
void  sigchld_handler(int sig);

/* utils.c */
char *mysh_strdup(const char *s);
void  mysh_free_pipeline(pipeline_t *pl);
char *expand_vars(const char *s);
void  print_prompt(void);

#endif /* MYSH_H */
