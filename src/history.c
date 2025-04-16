#include "mysh.h"

void history_add(const char *line)
{
    if (!line || !*line) return;

    int idx = history.head % HIST_MAX;
    free(history.entries[idx]);
    history.entries[idx] = mysh_strdup(line);
    history.head++;
    if (history.count < HIST_MAX) history.count++;
}

void history_print(void)
{
    int total = history.count;
    int start = (history.head - total + HIST_MAX) % HIST_MAX;
    for (int i = 0; i < total; i++) {
        int idx = (start + i) % HIST_MAX;
        if (history.entries[idx])
            printf("  %4d  %s\n", i + 1, history.entries[idx]);
    }
}

char *history_get(int n)
{
    if (n < 1 || n > history.count) return NULL;
    int total = history.count;
    int start = (history.head - total + HIST_MAX) % HIST_MAX;
    int idx   = (start + n - 1) % HIST_MAX;
    return history.entries[idx];
}
