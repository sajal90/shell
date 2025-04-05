#include "mysh.h"

static int apply_redirections(const redirect_t *r)
{
    if (r->infile) {
        int fd = open(r->infile, O_RDONLY);
        if (fd < 0) {
            perror(r->infile);
            return -1;
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }

    if (r->outfile) {
        int flags = O_WRONLY | O_CREAT | (r->append ? O_APPEND : O_TRUNC);
        int fd    = open(r->outfile, flags, 0644);
        if (fd < 0) {
            perror(r->outfile);
            return -1;
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }

    if (r->errfile) {
        int fd = open(r->errfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror(r->errfile);
            return -1;
        }
        dup2(fd, STDERR_FILENO);
        close(fd);
    }

    return 0;
}

int execute_pipeline(pipeline_t *pl)
{
    if (pl->ncmds == 0)
        return 0;

    if (pl->ncmds == 1 && is_builtin(pl->cmds[0].argv[0]))
        return run_builtin(&pl->cmds[0]);

    int    n      = pl->ncmds;
    pid_t  pids[MAX_PIPES];
    int    pipes[MAX_PIPES - 1][2];   /* [i][0]=read end [i][1]=write end */

    for (int i = 0; i < n - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            return 1;
        }
    }

    pid_t pgid = 0;   /* process group for this pipeline */

    for (int i = 0; i < n; i++) {
        cmd_t *cmd = &pl->cmds[i];

        if (cmd->argc == 0)
            continue;

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return 1;
        }

        if (pid == 0) {
            if (pgid == 0) pgid = getpid();
            setpgid(0, pgid);

            if (i > 0) {
                dup2(pipes[i-1][0], STDIN_FILENO);
            }
            if (i < n - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            for (int j = 0; j < n - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            if (apply_redirections(&cmd->redir) < 0)
                _exit(1);

            signal(SIGINT,  SIG_DFL);
            signal(SIGQUIT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            signal(SIGTTIN, SIG_DFL);
            signal(SIGTTOU, SIG_DFL);
            signal(SIGCHLD, SIG_DFL);

            if (is_builtin(cmd->argv[0])) {
                int r = run_builtin(cmd);
                _exit(r);
            }

            execvp(cmd->argv[0], cmd->argv);
            fprintf(stderr, "%s: %s: command not found\n",
                    MYSH_NAME, cmd->argv[0]);
            _exit(127);
        }

        if (pgid == 0) pgid = pid;
        setpgid(pid, pgid);
        pids[i] = pid;
    }

    for (int i = 0; i < n - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    char cmdline[MAX_INPUT] = {0};
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < pl->cmds[i].argc; j++) {
            strncat(cmdline, pl->cmds[i].argv[j],
                    sizeof(cmdline) - strlen(cmdline) - 2);
            strncat(cmdline, " ",
                    sizeof(cmdline) - strlen(cmdline) - 1);
        }
        if (i < n - 1)
            strncat(cmdline, "| ", sizeof(cmdline) - strlen(cmdline) - 1);
    }

    if (pl->background) {
        int jid = add_job(pgid, cmdline, JOB_RUNNING);
        printf("[%d] %d\n", jid, (int)pgid);
        return 0;
    }

    tcsetpgrp(STDIN_FILENO, pgid);

    int status = 0;
    for (int i = 0; i < n; i++) {
        int st;
        waitpid(pids[i], &st, WUNTRACED);
        if (WIFSTOPPED(st)) {
            add_job(pgid, cmdline, JOB_STOPPED);
            printf("\n[stopped] %s\n", cmdline);
            status = 0;
        } else if (WIFEXITED(st)) {
            status = WEXITSTATUS(st);
        } else if (WIFSIGNALED(st)) {
            status = 128 + WTERMSIG(st);
        }
    }

    tcsetpgrp(STDIN_FILENO, getpgrp());
    return status;
}
