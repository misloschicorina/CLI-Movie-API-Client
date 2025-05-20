#ifndef _COMMANDS_H_
#define _COMMANDS_H_

#include "admin_commands.h"
#include "user_commands.h"
#include "movie_commands.h"
#include "collection_commands.h"

void process_command(const char *cmd);
void clear_input_buffer(void);
void handle_error_response(const char *json_body, const char *fallback_msg);

#endif
