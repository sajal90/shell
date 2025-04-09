#include "mysh.h"

char *mysh_strdup(const char *s)
{
    if (!s) return NULL;
    char *p = strdup(s);
    if (!p) { perror("strdup"); exit(1); }
    return p;
}

void mysh_free_pipeline(pipeline_t *pl)
{
    for (int i = 0; i < pl->ncmds; i++) {
        cmd_t *cmd = &pl->cmds[i];
        for (int j = 0; j < cmd->argc; j++) {
            free(cmd->argv[j]);
            cmd->argv[j] = NULL;
        }
        free(cmd->redir.infile);
        free(cmd->redir.outfile);
        free(cmd->redir.errfile);
        cmd->redir.infile = cmd->redir.outfile = cmd->redir.errfile = NULL;
    }
}

char *expand_vars(const char *s)
{
    char  out[MAX_INPUT * 4];
    char *o   = out;
    const char *p = s;

    while (*p && (size_t)(o - out) < sizeof(out) - 1) {
        if (*p == '$' && *(p+1)) {
            p++;
            if (*p == '?') {
                o += snprintf(o, sizeof(out) - (o - out), "%d", last_exit);
                p++;
            } else if (*p == '$') {
                o += snprintf(o, sizeof(out) - (o - out), "%d", (int)getpid());
                p++;
            } else if (*p == '{') {
                p++;
                const char *end = strchr(p, '}');
                if (!end) { *o++ = '$'; *o++ = '{'; continue; }
                char var[256];
                size_t len = (size_t)(end - p);
                if (len >= sizeof(var)) len = sizeof(var) - 1;
                strncpy(var, p, len);
                var[len] = '\0';
                const char *val = getenv(var);
                if (val)
                    o += snprintf(o, sizeof(out) - (o - out), "%s", val);
                p = end + 1;
            } else if (isalpha((unsigned char)*p) || *p == '_') {
                char var[256];
                int  vi = 0;
                while ((isalnum((unsigned char)*p) || *p == '_') && vi < 255)
                    var[vi++] = *p++;
                var[vi] = '\0';
                const char *val = getenv(var);
                if (val)
                    o += snprintf(o, sizeof(out) - (o - out), "%s", val);
            } else {
                *o++ = '$';
            }
        } else {
            *o++ = *p++;
        }
    }
    *o = '\0';
    return mysh_strdup(out);
}

void print_prompt(void)
{
    const char *user = getenv("USER");
    if (!user) user = "user";

    const char *home = getenv("HOME");
    char  short_cwd[PATH_MAX];
    if (home && strncmp(cwd, home, strlen(home)) == 0) {
        snprintf(short_cwd, sizeof(short_cwd), "~%s", cwd + strlen(home));
    } else {
        snprintf(short_cwd, sizeof(short_cwd), "%s", cwd);
    }

    printf("%s%s%s:%s%s%s %s$%s ",
           CLR_GREEN, user, CLR_RESET,
           CLR_BLUE,  short_cwd, CLR_RESET,
           CLR_BOLD,  CLR_RESET);
    fflush(stdout);
}
