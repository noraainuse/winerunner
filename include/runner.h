#ifndef WINERUNNER_RUNNER_H
#define WINERUNNER_RUNNER_H

#include "config.h"
#include <stdbool.h>
#include <sys/types.h>

typedef struct {
    bool foreground;                /* Run in foreground instead of detached */
    bool debug_wine;                /* Enable WINEDEBUG */
    char override_res[MAX_RES_LEN]; /* Temporary resolution override */
    bool force_no_desktop;          /* Force disable virtual desktop */
} RunOptions;

int runner_launch(const GameConfig *game, const RunOptions *opts);
int runner_kill(const GameConfig *game);
bool runner_is_running(const GameConfig *game, pid_t *out_pid);
int runner_show_logs(const GameConfig *game, int num_lines);

#endif /* WINERUNNER_RUNNER_H */
