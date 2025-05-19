#include "state.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Singleton instance that stores the current state
static client_state_t state;

/**
 * Initializes the client state by setting all fields to NULL.
 */
void init_state(void) {
    state.admin_cookie = NULL;
    state.user_cookie = NULL;
    state.jwt_token = NULL;
}

/**
 * Frees all dynamically allocated strings in the state and resets it.
 */
void clear_state(void) {
    free(state.admin_cookie);
    free(state.user_cookie);
    free(state.jwt_token);
    init_state();
    fflush(stdout);  
}

/**
 * Sets the admin cookie (replaces previous one if exists).
 */
void set_admin_cookie(const char *cookie) {
    free(state.admin_cookie);
    state.admin_cookie = cookie ? strdup(cookie) : NULL;
}

/**
 * Sets the user cookie (replaces previous one if exists).
 */
void set_user_cookie(const char *cookie) {
    free(state.user_cookie);
    state.user_cookie = cookie ? strdup(cookie) : NULL;
}

/**
 * Sets the JWT token (replaces previous one if exists).
 */
void set_jwt_token(const char *token) {
    free(state.jwt_token);
    state.jwt_token = token ? strdup(token) : NULL;
}

/**
 * Returns an array of all active cookies (admin/user).
 * Caller must free only the returned array (not the internal cookie strings).
 */
char **get_cookies(int *count) {
    char **arr = malloc(sizeof(char*) * 2);
    int idx = 0;
    if (state.admin_cookie) {
        arr[idx++] = state.admin_cookie;
    }
    if (state.user_cookie) {
        arr[idx++] = state.user_cookie;
    }
    *count = idx;
    return arr;
}

/**
 * Returns a formatted Authorization header for the JWT token.
 * Caller must free the returned string.
 */
char *get_jwt_header(void) {
    if (!state.jwt_token) return NULL;

    const char *prefix = "Authorization: Bearer ";
    size_t len = strlen(prefix) + strlen(state.jwt_token) + 1;
    char *hdr = malloc(len);
    if (!hdr) return NULL;

    strcpy(hdr, prefix);
    strcat(hdr, state.jwt_token);
    return hdr;
}

/* State query functions */
bool is_admin_logged(void)    { return state.admin_cookie != NULL; }
bool is_user_logged(void)     { return state.user_cookie != NULL; }
bool has_library_access(void) { return state.jwt_token != NULL; }

/**
 * Prints the current admin and user cookies for debugging purposes.
 */
void debug_print_cookies_separat(void) {
    printf("[COOKIE] Admin cookie: %s\n", state.admin_cookie ? state.admin_cookie : "NULL");
    printf("[COOKIE] User cookie:  %s\n", state.user_cookie ? state.user_cookie : "NULL");
}

/**
 * Prints the current JWT token for debugging purposes.
 */
void debug_print_jwt_token(void) {
    printf("[JWT] Token: %s\n", state.jwt_token ? state.jwt_token : "NULL");
}
