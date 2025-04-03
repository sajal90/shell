#include "mysh.h"

static int  tokenise(char *line, char **toks, int max);
static void parse_cmd(char **toks, int *pos, int ntoks, cmd_t *cmd);
static int  is_redirect(const char *tok);

int parse_line(char *line, pipeline_t *pl)
{
    char *expanded = expand_vars(line);
    if (!expanded)
        return -1;

    char  *toks[MAX_ARGS * MAX_PIPES];
    int    ntoks = tokenise(expanded, toks, MAX_ARGS * MAX_PIPES);
    free(expanded);

    if (ntoks <= 0)
        return 0;

    if (strcmp(toks[ntoks - 1], "&") == 0) {
        pl->background = 1;
        toks[--ntoks]  = NULL;
    }

    int pos   = 0;
    int ncmds = 0;

    while (pos < ntoks) {
        if (ncmds >= MAX_PIPES) {
            fprintf(stderr, "%s: too many pipes\n", MYSH_NAME);
            return -1;
        }
        parse_cmd(toks, &pos, ntoks, &pl->cmds[ncmds]);
        ncmds++;

        if (pos < ntoks && strcmp(toks[pos], "|") == 0)
            pos++;
    }

    pl->ncmds = ncmds;
    return 0;
}

static int tokenise(char *line, char **toks, int max)
{
    static char buf[MAX_INPUT * 2];
    char *b   = buf;
    int   n   = 0;
    char *p   = line;

    while (*p) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;

        if (n >= max - 1) {
            fprintf(stderr, "%s: too many tokens\n", MYSH_NAME);
            return -1;
        }

        if (*p == '|' || *p == '&') {
            char op[2] = { *p++, '\0' };
            toks[n++] = mysh_strdup(op);
            continue;
        }

        if ((p[0]=='2' && p[1]=='>') ||
            (p[0]=='>' && p[1]=='>') ||
             p[0]=='>' || p[0]=='<') {
            char op[3] = {0};
            if (p[0]=='2' && p[1]=='>') { op[0]='2'; op[1]='>'; p+=2; }
            else if (p[0]=='>' && p[1]=='>') { op[0]=op[1]='>'; p+=2; }
            else { op[0]=*p++; }
            toks[n++] = mysh_strdup(op);
            continue;
        }

        char *start = b;

        while (*p && *p!=' ' && *p!='\t' && *p!='|' && *p!='&') {
            if (*p == '\'') {
                p++;
                while (*p && *p!='\'') *b++ = *p++;
                if (*p) p++;    /* consume closing ' */
            } else if (*p == '"') {
                p++;
                while (*p && *p!='"') *b++ = *p++;
                if (*p) p++;
            } else {
                *b++ = *p++;
            }
        }
        *b++ = '\0';
        toks[n++] = start;
    }

    toks[n] = NULL;
    return n;
}

static void parse_cmd(char **toks, int *pos, int ntoks, cmd_t *cmd)
{
    int argc = 0;

    while (*pos < ntoks) {
        const char *t = toks[*pos];

        if (strcmp(t, "|") == 0)
            break;

        if (strcmp(t, "<") == 0) {
            (*pos)++;
            if (*pos < ntoks)
                cmd->redir.infile = mysh_strdup(toks[(*pos)++]);
        } else if (strcmp(t, ">") == 0) {
            (*pos)++;
            if (*pos < ntoks) {
                cmd->redir.outfile = mysh_strdup(toks[(*pos)++]);
                cmd->redir.append  = 0;
            }
        } else if (strcmp(t, ">>") == 0) {
            (*pos)++;
            if (*pos < ntoks) {
                cmd->redir.outfile = mysh_strdup(toks[(*pos)++]);
                cmd->redir.append  = 1;
            }
        } else if (strcmp(t, "2>") == 0) {
            (*pos)++;
            if (*pos < ntoks)
                cmd->redir.errfile = mysh_strdup(toks[(*pos)++]);
        } else {
            if (argc < MAX_ARGS - 1)
                cmd->argv[argc++] = mysh_strdup(t);
            (*pos)++;
        }
    }

    cmd->argv[argc] = NULL;
    cmd->argc       = argc;
}

static int is_redirect(const char *tok)
{
    return (strcmp(tok,"<")==0 || strcmp(tok,">")==0 ||
            strcmp(tok,">>")==0 || strcmp(tok,"2>")==0);
}
