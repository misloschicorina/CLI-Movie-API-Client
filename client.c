#include <stdio.h>
#include <string.h>
#include "state.h"
#include "commands/commands.h"
#include "helpers.h"

int main() {
    char line[LINELEN];

    // Initialize client session state (cookies, token)
    init_state();

    // Main command processing loop
    while (1) {
        // Read a line (command) from stdin
        if (fgets(line, LINELEN, stdin) == NULL)
            break;

        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n')
            line[len - 1] = '\0'; // remove trailing newline

        // Exit command ends the session
        if (strcmp(line, "exit") == 0)
            break;

        // Dispatch the command
        process_command(line);
    }

    // Cleanup any allocated state before exiting
    clear_state();
    fflush(stdout);
    return 0;
}
