#ifndef _STATE_H_
#define _STATE_H_

#include <stdbool.h>

/**
 * Struct that holds the current client state.
 * It tracks both admin and user session cookies, and the JWT token for library access.
 */
typedef struct {
    char *admin_cookie;    // session cookie for admin
    char *user_cookie;     // session cookie for normal user
    char *jwt_token;       // JWT token for library access
} client_state_t;

/* Initialization and reset of client state */
void init_state(void);
void clear_state(void);

/* Setters for session values */
void set_admin_cookie(const char *cookie);
void set_user_cookie(const char *cookie);
void set_jwt_token(const char *token);

/* Getters for cookies and JWT header */
char **get_cookies(int *count);      // returns array of active cookies (caller must free array)
char *get_jwt_header(void);          // returns "Authorization: Bearer <token>" (caller must free)

/* State check helpers */
bool is_admin_logged(void);          // returns true if admin is logged in
bool is_user_logged(void);           // returns true if user is logged in
bool has_library_access(void);       // returns true if JWT token is available

/* Debug print helpers */
void debug_print_cookies_separat(void);
void debug_print_jwt_token(void);

#endif // _STATE_H_
