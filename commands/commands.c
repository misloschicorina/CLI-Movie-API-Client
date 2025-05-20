#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "state.h"
#include "helpers.h"
#include "http_requests.h"
#include "parson.h"
#include "commands.h"

// Server configuration
#define HOST "63.32.125.183"
#define PORT 8081

// Command descriptor
typedef struct {
    const char *name;
    void (*handler)(void);
} Command;

// Table of available commands and their handlers
static const Command commands[] = {
    {"login_admin", handle_login_admin},
    {"logout_admin", handle_admin_logout},
    {"add_user", handle_add_user},
    {"get_users", handle_get_users},
    {"delete_user", handle_delete_user},
    {"login", handle_login},
    {"logout", handle_logout},
    {"get_access", handle_get_access},
    {"get_movies", handle_get_movies},
    {"get_movie", handle_get_movie},
    {"add_movie", handle_add_movie},
    {"delete_movie", handle_delete_movie},
    {"update_movie", handle_update_movie},
    {"get_collections", handle_get_collections},
    {"add_collection", handle_add_collection},
    {"delete_collection", handle_delete_collection},
    {"get_collection", handle_get_collection},
    {"add_movie_to_collection", handle_add_movie_to_collection},
    {"delete_movie_from_collection", handle_delete_movie_from_collection},
    {NULL, NULL}
};

// Main dispatcher function that executes a command based on the input string
void process_command(const char *cmd) {
    if (!cmd)
        return;

    for (const Command *c = commands; c->name != NULL; c++) {
        if (strcmp(cmd, c->name) == 0) {
            c->handler();
            return;
        }
    }

    printf("ERROR: Unknown command: %s\n", cmd);
}

// Clears the remaining characters from the input buffer (stdin),
// usually after a scanf to prevent issues when using fgets after.
void clear_input_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// Generic error handler: use pre-extracted JSON body or fallback
void handle_error_response(const char *json_body, const char *fallback_msg) {
    if (json_body) {
        JSON_Value *val = json_parse_string(json_body);
        if (val) {
            JSON_Object *obj = json_value_get_object(val);
            const char *err = json_object_get_string(obj, "error");
            if (err && err[0] != '\0')
                printf("ERROR: %s\n", err);
            else
                printf("ERROR: %s\n", fallback_msg);
            json_value_free(val);
            return;
        }
    }
    // No JSON or parse failure
    printf("ERROR: %s\n", fallback_msg);
}




