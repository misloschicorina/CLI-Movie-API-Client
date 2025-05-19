// commands.h
#ifndef _COMMANDS_H_
#define _COMMANDS_H_

/**
 * Receives a command line and processes it.
 */
void process_command(const char *cmd);

// Admin-related commands
void handle_login_admin(void);
void handle_admin_logout(void);
void handle_add_user(void);
void handle_get_users(void);
void handle_delete_user(void);

// User-related commands
void handle_login(void);
void handle_logout(void);
void handle_get_access(void);

// Movie commands
void handle_get_movies(void);
void handle_get_movie(void);
void handle_add_movie(void);
void handle_delete_movie(void);
void handle_update_movie(void);

// Collection commands
void handle_get_collections(void);
void handle_add_collection(void);
void handle_delete_collection(void);
void handle_get_collection(void);
void handle_add_movie_to_collection(void);
void handle_delete_movie_from_collection(void);

// Helpers
void clear_input_buffer(void);
void update_movie_id_map_silent(void);

#endif // _COMMANDS_H_