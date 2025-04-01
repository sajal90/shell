#include "mysh.h"

job_t     jobs[MAX_JOBS];
int       njobs      = 0;
history_t history    = { .count = 0, .head = 0 };
int       last_exit  = 0;
char      cwd[PATH_MAX];

static char *read_line(void)
{
    static char buf[MAX_INPUT];
    if (!fgets(buf, sizeof(buf), stdin))
        return NULL;
    buf[strcspn(buf, "\n")] = '\0';
    return buf;
}

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    setup_signals();

    if (!getcwd(cwd, sizeof(cwd)))
        strncpy(cwd, "/", sizeof(cwd));

    printf("%s%s v%s%s  –  type 'help' for built-in commands\n",
           CLR_BOLD, MYSH_NAME, MYSH_VERSION, CLR_RESET);

    while (1) {
        reap_jobs();        /* collect any finished background jobs */
        print_prompt();

        char *line = read_line();
        if (!line) {        /* EOF (Ctrl-D) */
            printf("\nexit\n");
            break;
        }

        while (*line == ' ' || *line == '\t') line++;
        if (*line == '\0' || *line == '#')
            continue;

        history_add(line);

        pipeline_t pl;
        memset(&pl, 0, sizeof(pl));

        if (parse_line(line, &pl) < 0) {
            fprintf(stderr, "%s: parse error\n", MYSH_NAME);
            continue;
        }

        last_exit = execute_pipeline(&pl);
        mysh_free_pipeline(&pl);
    }

    history_print();   /* optional: show session history on exit */
    return last_exit;
}
