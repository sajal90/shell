#include "mysh.h"

static const char *builtin_names[] = {
    "cd", "pwd", "echo", "export", "unset", "env",
    "exit", "help", "history", "jobs", "fg", "bg", "kill",
    NULL
};

int is_builtin(const char *cmd)
{
    if (!cmd) return 0;
    for (int i = 0; builtin_names[i]; i++)
        if (strcmp(cmd, builtin_names[i]) == 0)
            return 1;
    return 0;
}

static int builtin_cd(cmd_t *cmd)
{
    const char *dir;
    if (cmd->argc < 2) {
        dir = getenv("HOME");
        if (!dir) { fprintf(stderr, "cd: HOME not set\n"); return 1; }
    } else {
        dir = cmd->argv[1];
    }

    if (chdir(dir) < 0) {
        perror(dir);
        return 1;
    }
    if (!getcwd(cwd, sizeof(cwd)))
        strncpy(cwd, "?", sizeof(cwd));
    setenv("PWD", cwd, 1);
    return 0;
}

static int builtin_pwd(cmd_t *cmd)
{
    (void)cmd;
    printf("%s\n", cwd);
    return 0;
}

static int builtin_echo(cmd_t *cmd)
{
    int newline = 1;
    int start   = 1;

    if (cmd->argc > 1 && strcmp(cmd->argv[1], "-n") == 0) {
        newline = 0;
        start   = 2;
    }

    for (int i = start; i < cmd->argc; i++) {
        if (i > start) putchar(' ');
        fputs(cmd->argv[i], stdout);
    }
    if (newline) putchar('\n');
    return 0;
}

static int builtin_export(cmd_t *cmd)
{
    if (cmd->argc < 2) {
        /* print all exported vars */
        extern char **environ;
        for (char **e = environ; *e; e++)
            printf("export %s\n", *e);
        return 0;
    }
    for (int i = 1; i < cmd->argc; i++) {
        char *eq = strchr(cmd->argv[i], '=');
        if (eq) {
            *eq = '\0';
            setenv(cmd->argv[i], eq + 1, 1);
            *eq = '=';
        } else {
            /* mark existing variable for export (no-op in our env model) */
        }
    }
    return 0;
}

static int builtin_unset(cmd_t *cmd)
{
    for (int i = 1; i < cmd->argc; i++)
        unsetenv(cmd->argv[i]);
    return 0;
}

static int builtin_env(cmd_t *cmd)
{
    (void)cmd;
    extern char **environ;
    for (char **e = environ; *e; e++)
        printf("%s\n", *e);
    return 0;
}

static int builtin_exit(cmd_t *cmd)
{
    int code = (cmd->argc > 1) ? atoi(cmd->argv[1]) : last_exit;
    exit(code);
}

static int builtin_help(cmd_t *cmd)
{
    (void)cmd;
    printf("\n%s%s v%s%s built-in commands:\n\n", CLR_BOLD, MYSH_NAME, MYSH_VERSION, CLR_RESET);
    printf("  %-12s  %s\n", "cd [dir]",      "Change directory (default: $HOME)");
    printf("  %-12s  %s\n", "pwd",            "Print working directory");
    printf("  %-12s  %s\n", "echo [-n]",      "Print arguments to stdout");
    printf("  %-12s  %s\n", "export",         "Set/print environment variables");
    printf("  %-12s  %s\n", "unset",          "Remove environment variables");
    printf("  %-12s  %s\n", "env",            "Print environment");
    printf("  %-12s  %s\n", "history",        "Show command history");
    printf("  %-12s  %s\n", "jobs",           "List background/stopped jobs");
    printf("  %-12s  %s\n", "fg [%n]",        "Bring job to foreground");
    printf("  %-12s  %s\n", "bg [%n]",        "Resume stopped job in background");
    printf("  %-12s  %s\n", "kill",           "Send signal to process/job");
    printf("  %-12s  %s\n", "exit [code]",    "Exit the shell");
    printf("  %-12s  %s\n", "help",           "Show this message");
    printf("\n  Operators: |  <  >  >>  2>  &\n\n");
    return 0;
}

static int builtin_history(cmd_t *cmd)
{
    (void)cmd;
    history_print();
    return 0;
}

static int builtin_jobs(cmd_t *cmd)
{
    (void)cmd;
    print_jobs();
    return 0;
}

static int builtin_fg(cmd_t *cmd)
{
    int jid = -1;
    if (cmd->argc > 1 && cmd->argv[1][0] == '%')
        jid = atoi(cmd->argv[1] + 1);

    job_t *j = NULL;
    if (jid < 0) {
        for (int i = njobs - 1; i >= 0; i--)
            if (jobs[i].active) { j = &jobs[i]; break; }
    } else {
        for (int i = 0; i < njobs; i++)
            if (jobs[i].active && jobs[i].id == jid) { j = &jobs[i]; break; }
    }

    if (!j) { fprintf(stderr, "fg: no such job\n"); return 1; }

    printf("%s\n", j->cmdline);
    tcsetpgrp(STDIN_FILENO, j->pgid);
    kill(-j->pgid, SIGCONT);
    j->state = JOB_RUNNING;

    int status = 0;
    waitpid(-j->pgid, &status, WUNTRACED);
    tcsetpgrp(STDIN_FILENO, getpgrp());

    if (WIFEXITED(status) || WIFSIGNALED(status))
        remove_job(j->id);
    else if (WIFSTOPPED(status))
        j->state = JOB_STOPPED;

    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}

static int builtin_bg(cmd_t *cmd)
{
    int jid = -1;
    if (cmd->argc > 1 && cmd->argv[1][0] == '%')
        jid = atoi(cmd->argv[1] + 1);

    job_t *j = NULL;
    if (jid < 0) {
        for (int i = njobs - 1; i >= 0; i--)
            if (jobs[i].active && jobs[i].state == JOB_STOPPED)
                { j = &jobs[i]; break; }
    } else {
        for (int i = 0; i < njobs; i++)
            if (jobs[i].active && jobs[i].id == jid) { j = &jobs[i]; break; }
    }

    if (!j) { fprintf(stderr, "bg: no such job\n"); return 1; }

    j->state = JOB_RUNNING;
    kill(-j->pgid, SIGCONT);
    printf("[%d] %s &\n", j->id, j->cmdline);
    return 0;
}

static int builtin_kill(cmd_t *cmd)
{
    if (cmd->argc < 2) {
        fprintf(stderr, "kill: usage: kill [-SIG] pid|%%job\n");
        return 1;
    }

    int sig = SIGTERM;
    int start = 1;

    if (cmd->argv[1][0] == '-') {
        sig   = atoi(cmd->argv[1] + 1);
        start = 2;
    }

    for (int i = start; i < cmd->argc; i++) {
        if (cmd->argv[i][0] == '%') {
            int jid = atoi(cmd->argv[i] + 1);
            for (int k = 0; k < njobs; k++)
                if (jobs[k].active && jobs[k].id == jid)
                    kill(-jobs[k].pgid, sig);
        } else {
            kill((pid_t)atoi(cmd->argv[i]), sig);
        }
    }
    return 0;
}

int run_builtin(cmd_t *cmd)
{
    if (!cmd || !cmd->argv[0]) return 1;

    if (strcmp(cmd->argv[0], "cd")      == 0) return builtin_cd(cmd);
    if (strcmp(cmd->argv[0], "pwd")     == 0) return builtin_pwd(cmd);
    if (strcmp(cmd->argv[0], "echo")    == 0) return builtin_echo(cmd);
    if (strcmp(cmd->argv[0], "export")  == 0) return builtin_export(cmd);
    if (strcmp(cmd->argv[0], "unset")   == 0) return builtin_unset(cmd);
    if (strcmp(cmd->argv[0], "env")     == 0) return builtin_env(cmd);
    if (strcmp(cmd->argv[0], "exit")    == 0) return builtin_exit(cmd);
    if (strcmp(cmd->argv[0], "help")    == 0) return builtin_help(cmd);
    if (strcmp(cmd->argv[0], "history") == 0) return builtin_history(cmd);
    if (strcmp(cmd->argv[0], "jobs")    == 0) return builtin_jobs(cmd);
    if (strcmp(cmd->argv[0], "fg")      == 0) return builtin_fg(cmd);
    if (strcmp(cmd->argv[0], "bg")      == 0) return builtin_bg(cmd);
    if (strcmp(cmd->argv[0], "kill")    == 0) return builtin_kill(cmd);

    return 1;
}
