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

// Main dispatcher function that executes a command based on the input string
void process_command(const char *cmd) {
    if (!cmd)
        return;

    if (strcmp(cmd, "login_admin") == 0) {
        handle_login_admin();
    } else if (strcmp(cmd, "logout_admin") == 0) {
        handle_admin_logout();
    } else if (strcmp(cmd, "add_user") == 0) {
        handle_add_user();
    } else if (strcmp(cmd, "get_users") == 0) {
        handle_get_users();
    } else if (strcmp(cmd, "delete_user") == 0) {
        handle_delete_user();
    } else if (strcmp(cmd, "login") == 0) {
        handle_login();
    } else if (strcmp(cmd, "logout") == 0) {
        handle_logout();
    } else if (strcmp(cmd, "get_access") == 0) {
        handle_get_access();
    } else if (strcmp(cmd, "get_movies") == 0) {
        handle_get_movies();
    } else if (strcmp(cmd, "get_movie") == 0) {
        handle_get_movie();
    } else if (strcmp(cmd, "add_movie") == 0) {
        handle_add_movie();
    } else if (strcmp(cmd, "delete_movie") == 0) {
        handle_delete_movie();
    } else if (strcmp(cmd, "update_movie") == 0) {
        handle_update_movie();
    } else if (strcmp(cmd, "get_collections") == 0) {
        handle_get_collections();
    } else if (strcmp(cmd, "add_collection") == 0) {
        handle_add_collection();
    } else if (strcmp(cmd, "delete_collection") == 0) {
        handle_delete_collection();
    }    else if (strcmp(cmd, "get_collection") == 0) {
        handle_get_collection();
    } else if (strcmp(cmd, "add_movie_to_collection") == 0) {
        handle_add_movie_to_collection();
    } else if (strcmp(cmd, "delete_movie_from_collection") == 0) {
        handle_delete_movie_from_collection();
    } else {
        printf("ERROR: Unknown command: %s\n", cmd);
    }
}

// Helper functions

// Clears the remaining characters from the input buffer (stdin),
// usually after a scanf to prevent issues when using fgets after.
void clear_input_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// Command handlers

void handle_login_admin(void) {
    // Prevent login if already logged in
    if (is_user_logged()) {
        printf("ERROR: You are already logged in as user.\n");
        return;
    }

    if (is_admin_logged()) {
        printf("ERROR: You are already logged in as admin.\n");
        return;
    }

    // Read username and password from input
    char username[LINELEN], password[LINELEN];

    printf("username=");
    if (!fgets(username, LINELEN, stdin)) return;
    username[strcspn(username, "\r\n")] = '\0';

    printf("password=");
    if (!fgets(password, LINELEN, stdin)) return;
    password[strcspn(password, "\r\n")] = '\0';

    // Prepare JSON payload
    char json_body[BUFLEN];
    snprintf(json_body, BUFLEN,
             "{\"username\":\"%s\",\"password\":\"%s\"}", username, password);
    char *body_data[1] = { json_body };

    // Send POST request to admin login endpoint
    int sockfd = open_connection((char *)HOST, PORT, AF_INET, SOCK_STREAM, 0);
    char *request = compute_post_request(HOST, "/api/v1/tema/admin/login",
                                         "application/json", body_data, 1, NULL, 0, NULL);
    send_to_server(sockfd, request);
    free(request);

    char *response = receive_from_server(sockfd);
    close_connection(sockfd);

    // Parse JSON response body
    char *json_start = basic_extract_json_response(response);

    if (strstr(response, "200 OK")) {
        // Extract and store session cookie
        char *cookie = strstr(response, "Set-Cookie:");
        if (cookie) {
            cookie += strlen("Set-Cookie:");
            while (*cookie == ' ') cookie++;
            char *end = strchr(cookie, ';');
            if (end) *end = '\0';
            set_admin_cookie(cookie);
        }

        printf("SUCCESS: Admin logged in.\n");

    } else {
        if (json_start) {
            JSON_Value *val = json_parse_string(json_start);
            JSON_Object *obj = json_value_get_object(val);
            const char *err = json_object_get_string(obj, "error");

            // Display specific or generic error based on server response
            if (err && (strstr(err, "Invalid") || strstr(err, "credentials")))
                printf("ERROR: Invalid username or password.\n");
            else
                printf("ERROR: Other error from server.\n");

            json_value_free(val);
        } else {
            printf("ERROR: Invalid response from server.\n");
        }
    }

    free(response);
}

void handle_admin_logout(void) {
    // Check if admin is logged in
    if (!is_admin_logged()) {
        printf("ERROR: You are not logged in as admin.\n");
        return;
    }

    // Prepare and send GET request to logout endpoint
    int nc;
    char **cookies = get_cookies(&nc);
    int sockfd = open_connection((char *)HOST, PORT, AF_INET, SOCK_STREAM, 0);
    char *request = compute_get_request(HOST, "/api/v1/tema/admin/logout", 
                                            NULL, cookies, nc, NULL);
    free(cookies);

    send_to_server(sockfd, request);
    free(request);

    char *response = receive_from_server(sockfd);
    close_connection(sockfd);

    // Extract JSON body
    char *json_start = basic_extract_json_response(response);

    if (strstr(response, "200 OK")) {
        clear_state();  // Reset session state
        printf("SUCCESS: Admin logged out.\n");
    } else {
        if (json_start) {
            JSON_Value *val = json_parse_string(json_start);
            JSON_Object *obj = json_value_get_object(val);
            const char *err = json_object_get_string(obj, "error");
            err ? printf("ERROR: %s\n", err) : printf("ERROR: Other error from server.\n");

            json_value_free(val);
        } else {
            printf("ERROR: Logout failed.\n");
        }
    }

    free(response);
}

void handle_add_user(void) {
    // Ensure the user is an admin before proceeding
    if (!is_admin_logged()) {
        printf("ERROR: You must be logged in as admin to add users.\n");
        return;
    }

    char username[LINELEN], password[LINELEN];

    printf("username=");
    if (!fgets(username, LINELEN, stdin)) return;
    username[strcspn(username, "\r\n")] = '\0';

    printf("password=");
    if (!fgets(password, LINELEN, stdin)) return;
    password[strcspn(password, "\r\n")] = '\0';

    // Create JSON body
    char json_body[BUFLEN];
    snprintf(json_body, BUFLEN,
             "{\"username\":\"%s\",\"password\":\"%s\"}", username, password);
    char *body_data[1] = { json_body };

    int nc;
    char **cookies = get_cookies(&nc);
    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);

    char *request = compute_post_request(
        HOST, "/api/v1/tema/admin/users",
        "application/json", body_data, 1, cookies, nc, NULL);
    free(cookies);

    send_to_server(sockfd, request);
    free(request);

    char *response = receive_from_server(sockfd);
    close_connection(sockfd);

    char *json_start = basic_extract_json_response(response);

    if (strstr(response, "201 CREATED")) {
        printf("SUCCESS: User created.\n");
    } else {
        if (json_start) {
            JSON_Value *val = json_parse_string(json_start);
            JSON_Object *obj = json_value_get_object(val);
            const char *err = json_object_get_string(obj, "error");

            if (err && (strcasecmp(err, "incomplete") || strcasecmp(err, "incorrect"))) {
                printf("ERROR: Incomplete or incorrect user information.\n");
            } else if (err) {
                printf("ERROR: %s\n", err);
            } else {
                printf("ERROR: Other error from server.\n");
            }

            json_value_free(val);
        } else {
            printf("ERROR: Invalid server response.\n");
        }
    }

    free(response);
}

void handle_get_users(void) {
    // Ensure the user is logged in as admin
    if (!is_admin_logged()) {
        printf("ERROR: You must be logged in as admin to view users.\n");
        return;
    }

    int nc;
    char **cookies = get_cookies(&nc);

    // Prepare and send GET request to retrieve all users
    int sockfd = open_connection((char *)HOST, PORT, AF_INET, SOCK_STREAM, 0);
    char *request = compute_get_request(HOST, "/api/v1/tema/admin/users",
                                        NULL, cookies, nc, NULL);
    free(cookies);

    send_to_server(sockfd, request);
    free(request);

    char *response = receive_from_server(sockfd);
    close_connection(sockfd);

    char *json_start = basic_extract_json_response(response);

    // Handle response
    if (strstr(response, "200 OK")) {
        if (json_start) {
            JSON_Value *val = json_parse_string(json_start);
            JSON_Object *root = json_value_get_object(val);
            JSON_Array *users = json_object_get_array(root, "users");

            size_t count = json_array_get_count(users);
            if (count == 0) {
                printf("SUCCESS: No users found.\n");
            } else {
                printf("SUCCESS: Users list\n");
                for (size_t i = 0; i < count; i++) {
                    JSON_Object *user = json_array_get_object(users, i);
                    const char *username = json_object_get_string(user, "username");
                    const char *password = json_object_get_string(user, "password");

                    printf("#%zu %s:%s\n", i + 1,
                           username ? username : "(null)",
                           password ? password : "(null)");
                }
            }

            json_value_free(val);
        } else {
            printf("SUCCESS: Users retrieved but response is not valid JSON.\n");
        }
    } else {
        // Handle server error messages
        if (json_start) {
            JSON_Value *val = json_parse_string(json_start);
            JSON_Object *obj = json_value_get_object(val);
            const char *err = json_object_get_string(obj, "error");
            printf("ERROR: %s\n", err ? err : "Other error from server.");

            json_value_free(val);
        } else {
            printf("ERROR: Invalid response from server.\n");
        }
    }

    free(response);
}

void handle_delete_user(void) {
    // Ensure the user is logged in as admin
    if (!is_admin_logged()) {
        printf("ERROR: You must be logged in as admin to delete users.\n");
        return;
    }

    char username[LINELEN];
    printf("username=");
    fflush(stdout);
    if (!fgets(username, LINELEN, stdin)) 
        return;
    
    username[strcspn(username, "\r\n")] = '\0';

    int nc;
    char **cookies = get_cookies(&nc);

    // Build the DELETE request URL
    char url[BUFLEN];
    snprintf(url, BUFLEN, "/api/v1/tema/admin/users/%s", username);

    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    char *request = compute_delete_request(HOST, url, NULL, cookies, nc, NULL);
    free(cookies);

    send_to_server(sockfd, request);
    free(request);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);

    char *json_start = basic_extract_json_response(response);

    if (strstr(response, "200 OK")) {
        printf("SUCCESS: User deleted successfully.\n");
    } else {
        // Try to extract a specific error message
        if (json_start) {
            JSON_Value *val = json_parse_string(json_start);
            JSON_Object *obj = json_value_get_object(val);
            const char *err = json_object_get_string(obj, "error");

            if (err && strstr(err, "not found")) {
                printf("ERROR: The user '%s' does not exist.\n", username);
            } else if (err) {
                printf("ERROR: %s\n", err);
            } else {
                printf("ERROR: Other error from server.\n");
            }
            json_value_free(val);
        } else {
            printf("ERROR: Invalid response from server.\n");
        }
    }

    free(response);
}

void handle_login(void) {
    // Prevent login if already logged in
    if (is_user_logged()) {
        printf("ERROR: You are already logged in as user.\n");
        return;
    }

    if (is_admin_logged()) {
        printf("ERROR: You are already logged in as admin.\n");
        return;
    }

    // Read credentials
    char admin_username[LINELEN], username[LINELEN], password[LINELEN];

    printf("admin_username=");
    if (!fgets(admin_username, LINELEN, stdin)) return;
    admin_username[strcspn(admin_username, "\r\n")] = '\0';

    printf("username=");
    if (!fgets(username, LINELEN, stdin)) return;
    username[strcspn(username, "\r\n")] = '\0';

    printf("password=");
    if (!fgets(password, LINELEN, stdin)) return;
    password[strcspn(password, "\r\n")] = '\0';

    // Build JSON request body
    char json_body[BUFLEN];
    snprintf(json_body, BUFLEN,
        "{\"admin_username\":\"%s\",\"username\":\"%s\",\"password\":\"%s\"}",
        admin_username, username, password);
    char *body_data[1] = { json_body };

    int nc;
    char **cookies = get_cookies(&nc);

    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    char *request = compute_post_request(HOST, "/api/v1/tema/user/login",
        "application/json", body_data, 1, cookies, nc, NULL);
    free(cookies);

    send_to_server(sockfd, request);
    free(request);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);

    // Store session cookie if available
    char *cookie = strstr(response, "Set-Cookie:");
    if (cookie) {
        cookie += strlen("Set-Cookie:");
        while (*cookie == ' ') cookie++;
        char *end = strchr(cookie, ';');
        if (end) *end = '\0';
        set_user_cookie(cookie);
    }

    char *json_start = basic_extract_json_response(response);

    if (strstr(response, "200 OK")) {
        printf("SUCCESS: User logged in successfully.\n");
    } else {
        if (json_start) {
            JSON_Value *val = json_parse_string(json_start);
            JSON_Object *obj = json_value_get_object(val);
            const char *err = json_object_get_string(obj, "error");

            if (err && (strcasecmp(err, "invalid") || strcasecmp(err, "incorrect")))
                printf("ERROR: Invalid username or password.\n");
            else
                printf("ERROR: %s\n", err ? err : "Other error from server.\n");

            json_value_free(val);
        } else {
            printf("ERROR: Invalid server response.\n");
        }
    }

    free(response);
}

void handle_logout(void) {
    if (!is_user_logged()) {
        printf("ERROR: You are not logged in as a user.\n");
        return;
    }

    int nc;
    char **cookies = get_cookies(&nc);

    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    char *request = compute_get_request(HOST, "/api/v1/tema/user/logout",
        NULL, cookies, nc, NULL);
    free(cookies);

    send_to_server(sockfd, request);
    free(request);

    char *response = receive_from_server(sockfd);
    close_connection(sockfd);

    char *json_start = basic_extract_json_response(response);

    if (strstr(response, "200 OK")) {
        set_user_cookie(NULL);
        set_jwt_token(NULL);
        printf("SUCCESS: User logged out successfully.\n");
    } else {
        if (json_start) {
            JSON_Value *val = json_parse_string(json_start);
            JSON_Object *obj = json_value_get_object(val);
            const char *err = json_object_get_string(obj, "error");
            printf("ERROR: %s\n", err ? err : "Other error from server.");
            json_value_free(val);
        } else {
            printf("ERROR: Logout failed.\n");
        }
    }

    free(response);
}

void handle_get_access(void) {
    // Make sure the user is logged in to get access
    if (!is_user_logged()) {
        printf("ERROR: You are not logged in as a user.\n");
        return;
    }

    int nc;
    char **cookies = get_cookies(&nc);

    int sockfd = open_connection((char *)HOST, PORT, AF_INET, SOCK_STREAM, 0);
    char *request = compute_get_request((char *)HOST, "/api/v1/tema/library/access",
                                                        NULL, cookies, nc, NULL);
    free(cookies);

    send_to_server(sockfd, request);
    free(request);

    char *response = receive_from_server(sockfd);
    close_connection(sockfd);

    char *json_start = basic_extract_json_response(response);

    if (strstr(response, "200 OK")) {
        if (json_start) {
            JSON_Value *val = json_parse_string(json_start);
            JSON_Object *obj = json_value_get_object(val);
            const char *token = json_object_get_string(obj, "token");

            if (token) {
                set_jwt_token(token);
                printf("SUCCESS: JWT token received.\n");
            } else {
                printf("SUCCESS: Access granted, but token missing.\n");
            }

            json_value_free(val);
        } else {
            printf("SUCCESS: Access granted.\n");
        }
    } else {
        if (json_start) {
            JSON_Value *val = json_parse_string(json_start);
            JSON_Object *obj = json_value_get_object(val);
            const char *err = json_object_get_string(obj, "error");
            printf("ERROR: %s\n", err ? err : "Other error from server.");
            json_value_free(val);
        } else {
            printf("ERROR: Invalid response from server.\n");
        }
    }
    free(response);
}

void handle_get_movies(void) {
    if (!has_library_access()) {
        printf("ERROR: You do not have access to the library.\n");
        return;
    }

    int nc;
    char **cookies = get_cookies(&nc);
    char *jwt_header = get_jwt_header();

    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    char *request = compute_get_request(HOST, "/api/v1/tema/library/movies", 
                                      NULL, cookies, nc, jwt_header);
    free(cookies);
    free(jwt_header);

    send_to_server(sockfd, request);
    free(request);

    char *response = receive_from_server(sockfd);
    close_connection(sockfd);

    char *json_start = basic_extract_json_response(response);
    if (strstr(response, "200 OK") && json_start) {
        JSON_Value *val = json_parse_string(json_start);
        JSON_Object *root = json_value_get_object(val);
        JSON_Array *movies = json_object_get_array(root, "movies");

        size_t count = json_array_get_count(movies);
        
        printf("SUCCESS: Lista filmelor\n");
        for (size_t i = 0; i < count; i++) {
            JSON_Object *movie = json_array_get_object(movies, i);
            int id = (int)json_object_get_number(movie, "id");
            const char *title = json_object_get_string(movie, "title");
            printf("#%d %s\n", id, title ? title : "(null)");
        }
        
        json_value_free(val);
    } else {
        printf("ERROR: Failed to get movies\n");
    }
    free(response);
}

void handle_get_movie(void) {
    if (!has_library_access()) {
        printf("ERROR: You do not have access to the library.\n");
        return;
    }

    printf("id=");
    char id_buf[LINELEN];
    if (!fgets(id_buf, LINELEN, stdin)) return;
    int id = atoi(id_buf);

    if (id <= 0) {
        printf("ERROR: ID invalid.\n");
        return;
    }

    char url[BUFLEN];
    snprintf(url, sizeof(url), "/api/v1/tema/library/movies/%d", id);

    int nc;
    char **cookies = get_cookies(&nc);
    char *jwt_header = get_jwt_header();

    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    char *request = compute_get_request(HOST, url, NULL, cookies, nc, jwt_header);
    free(cookies);
    free(jwt_header);

    send_to_server(sockfd, request);
    free(request);

    char *response = receive_from_server(sockfd);
    close_connection(sockfd);

    char *json = basic_extract_json_response(response);
    if (strstr(response, "200 OK") && json) {
        JSON_Value *val = json_parse_string(json);
        if (!val) {
            printf("ERROR: Failed to parse movie details\n");
            free(response);
            return;
        }

        JSON_Object *obj = json_value_get_object(val);
        
        // Get required fields
        const char *title = json_object_get_string(obj, "title");
        int year = (int)json_object_get_number(obj, "year");
        const char *description = json_object_get_string(obj, "description");
        
        // Handle rating which might be number or string
        double rating = 0.0;
        if (json_object_has_value_of_type(obj, "rating", JSONNumber)) {
            rating = json_object_get_number(obj, "rating");
        } 
        else if (json_object_has_value_of_type(obj, "rating", JSONString)) {
            const char *rating_str = json_object_get_string(obj, "rating");
            if (rating_str) {
                rating = strtod(rating_str, NULL);
            }
        }

        printf("SUCCESS: Detalii film\n");
        printf("title: %s\n", title ? title : "(null)");
        printf("year: %d\n", year);
        printf("description: %s\n", description ? description : "(null)");
        printf("rating: %.1f\n", rating);

        json_value_free(val);
    } else {
        printf("ERROR: Movie with id=%d doesn't exist!\n", id);
    }
    free(response);
}

 void handle_add_movie(void) {
    if (!has_library_access()) {
        printf("ERROR: You do not have access to the library.\n");
        return;
    }

    char title[1024], description[2048];
    int year;
    float rating;

    // Read user input
    printf("title=");
    if (!fgets(title, sizeof(title), stdin)) return;
    title[strcspn(title, "\r\n")] = '\0';

    printf("year=");
    if (scanf("%d", &year) != 1) {
        printf("ERROR: Invalid year.\n");
        clear_input_buffer();
        return;
    }
    clear_input_buffer();

    printf("description=");
    if (!fgets(description, sizeof(description), stdin)) return;
    description[strcspn(description, "\r\n")] = '\0';

    printf("rating=");
    if (scanf("%f", &rating) != 1) {
        printf("ERROR: Invalid rating.\n");
        clear_input_buffer();
        return;
    }
    clear_input_buffer();

    if (year <= 1800 || rating < 0 || rating > 10 || strlen(title) == 0 || strlen(description) == 0) {
        printf("ERROR: Invalid input values.\n");
        return;
    }

    // Build JSON payload
    char json_body[8192];
    int written = snprintf(json_body, sizeof(json_body),
        "{\"title\":\"%s\",\"year\":%d,\"description\":\"%s\",\"rating\":%.2f}",
        title, year, description, rating);

    if (written < 0 || written >= sizeof(json_body)) {
        printf("ERROR: Failed to construct JSON payload.\n");
        return;
    }

    char *body_data[1] = { json_body };
    int nc;
    char **cookies = get_cookies(&nc);
    char *jwt_header = get_jwt_header();

    int sockfd = open_connection((char *)HOST, PORT, AF_INET, SOCK_STREAM, 0);
    char *request = compute_post_request(
        HOST,
        "/api/v1/tema/library/movies",
        "application/json",
        body_data,
        1,
        cookies,
        nc,
        jwt_header
    );

    free(cookies);
    free(jwt_header);

    send_to_server(sockfd, request);
    free(request);

    char *response = receive_from_server(sockfd);
    close_connection(sockfd);

    char *json_start = basic_extract_json_response(response);

    if (strstr(response, "200 OK") || strstr(response, "201") || strstr(response, "201 CREATED")) {
        if (json_start) {
            JSON_Value *val = json_parse_string(json_start);
            JSON_Object *obj = json_value_get_object(val);
            const char *msg = json_object_get_string(obj, "message");
            printf("SUCCESS: %s\n", msg ? msg : "Movie added.\n");
            json_value_free(val);
        } else {
            printf("SUCCESS: Movie added.\n");
        }
    } else {
        if (json_start) {
            JSON_Value *val = json_parse_string(json_start);
            JSON_Object *obj = json_value_get_object(val);
            const char *err = json_object_get_string(obj, "error");
            printf("ERROR: %s\n", err ? err : "Unknown server error.");
            json_value_free(val);
        } else {
            printf("ERROR: Invalid server response.\n");
        }
    }

    free(response);
}

void handle_delete_movie(void) {
    if (!has_library_access()) {
        printf("ERROR: You do not have access to the library.\n");
        return;
    }

    char input[32];
    printf("id=");
    if (!fgets(input, sizeof(input), stdin)) return;
    int id = atoi(input);

    if (id <= 0) {
        printf("ERROR: Invalid movie ID!\n");
        return;
    }

    // Build URL with real ID
    char url[BUFLEN];
    snprintf(url, sizeof(url), "/api/v1/tema/library/movies/%d", id);

    int nc;
    char **cookies = get_cookies(&nc);
    char *jwt = get_jwt_header();

    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    char *req = compute_delete_request(HOST, url, NULL, cookies, nc, jwt);
    free(cookies);
    free(jwt);

    send_to_server(sockfd, req);
    free(req);

    char *resp = receive_from_server(sockfd);
    close_connection(sockfd);

    char *json = basic_extract_json_response(resp);
    if (strstr(resp, "200 OK")) {
        printf("SUCCESS: Movie deleted successfully.\n");
    } else {
        if (json) {
            JSON_Value *val = json_parse_string(json);
            JSON_Object *obj = json_value_get_object(val);
            const char *err = json_object_get_string(obj, "error");
            printf("ERROR: %s\n", err ? err : "Unknown error.");
            json_value_free(val);
        } else {
            printf("ERROR: Invalid server response.\n");
        }
    }

    free(resp);
}

void handle_update_movie(void) {
    if (!has_library_access()) {
        printf("ERROR: You do not have access to the library.\n");
        return;
    }

    printf("id=");
    char id_buf[LINELEN];
    if (!fgets(id_buf, LINELEN, stdin)) return;
    int id = atoi(id_buf);

    if (id <= 0) {
        printf("ERROR: Invalid movie ID!\n");
        return;
    }

    char title[256], description[1000], year_buf[32], rating_buf[32];
    printf("title=");
    if (!fgets(title, sizeof(title), stdin)) return;
    title[strcspn(title, "\r\n")] = '\0';

    printf("year=");
    if (!fgets(year_buf, sizeof(year_buf), stdin)) return;
    int year = atoi(year_buf);

    printf("description=");
    if (!fgets(description, sizeof(description), stdin)) return;
    description[strcspn(description, "\r\n")] = '\0';

    printf("rating=");
    if (!fgets(rating_buf, sizeof(rating_buf), stdin)) return;
    double rating = strtod(rating_buf, NULL);

    if (strlen(title) == 0 || strlen(description) == 0 || year <= 1800 || rating < 0 || rating > 10) {
        printf("ERROR: Invalid or incomplete data.\n");
        return;
    }

    char json_body[BUFLEN];
    snprintf(json_body, sizeof(json_body),
        "{\"title\":\"%s\",\"year\":%d,\"description\":\"%s\",\"rating\":%.2f}",
        title, year, description, rating);

    char *body_data[1] = { json_body };

    char url[BUFLEN];
    snprintf(url, sizeof(url), "/api/v1/tema/library/movies/%d", id);

    int nc;
    char **cookies = get_cookies(&nc);
    char *jwt_header = get_jwt_header();

    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    char *request = compute_put_request(HOST, url, "application/json",
                                        body_data, 1, cookies, nc, jwt_header);
    free(cookies);
    free(jwt_header);

    send_to_server(sockfd, request);
    free(request);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);

    char *json = basic_extract_json_response(response);
    if (strstr(response, "200 OK")) {
        printf("SUCCESS: Movie updated successfully.\n");
    } else if (json) {
        JSON_Value *val = json_parse_string(json);
        JSON_Object *obj = json_value_get_object(val);
        const char *err = json_object_get_string(obj, "error");
        printf("ERROR: %s\n", err ? err : "Unknown error.");
        json_value_free(val);
    } else {
        printf("ERROR: Invalid server response.\n");
    }

    free(response);
}

/* ========================================================== */
/*  helper: POST /collections/{coll_id}/movies (fără prompt)  */
/* ========================================================== */
static bool add_movie_to_collection_by_id(int coll_id, int movie_id)
{
    char json[64];
    snprintf(json, sizeof(json), "{\"id\":%d}", movie_id);
    char *body[1] = { json };

    /* URL */
    char url[128];
    snprintf(url, sizeof(url),
             "/api/v1/tema/library/collections/%d/movies", coll_id);

    /* cookies + JWT */
    int nc; char **cks = get_cookies(&nc);
    char *jwt = get_jwt_header();

    int sock = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    char *req = compute_post_request(
        HOST, url, "application/json",
        body, 1, cks, nc, jwt);
    free(cks); free(jwt);

    send_to_server(sock, req); free(req);
    char *resp = receive_from_server(sock); close_connection(sock);

    bool ok = resp && (strstr(resp, "200 OK") || strstr(resp, "201"));
    free(resp);
    return ok;
}

// FUNCTIILE DE COLLECTIONS
void handle_get_collections(void) {
    if (!has_library_access()) {
        printf("ERROR: You do not have access to the library.\n");
        return;
    }

    int nc;
    char **cookies = get_cookies(&nc);
    char *jwt = get_jwt_header();

    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    char *request = compute_get_request(HOST, "/api/v1/tema/library/collections",
                                      NULL, cookies, nc, jwt);
    free(cookies);
    free(jwt);

    send_to_server(sockfd, request);
    free(request);

    char *response = receive_from_server(sockfd);
    close_connection(sockfd);

    char *json = basic_extract_json_response(response);
    if (!json) {
        printf("ERROR: Invalid server response.\n");
        free(response);
        return;
    }

    if (strstr(response, "200 OK")) {
        JSON_Value *val = json_parse_string(json);
        JSON_Object *root = json_value_get_object(val);
        JSON_Array *collections = json_object_get_array(root, "collections");

        size_t count = json_array_get_count(collections);

        if (count == 0) {
            printf("SUCCESS: No collections found.\n");
        } else {
            printf("SUCCESS: Collections list\n");
            for (size_t i = 0; i < count; i++) {
                JSON_Object *coll = json_array_get_object(collections, i);
                int real_id = (int)json_object_get_number(coll, "id");
                const char *title = json_object_get_string(coll, "title");
                
                printf("#%d %s\n", real_id, title ? title : "(null)");
            }
        }

        json_value_free(val);
    } else {
        JSON_Value *val = json_parse_string(json);
        JSON_Object *obj = json_value_get_object(val);
        const char *err = json_object_get_string(obj, "error");
        printf("ERROR: %s\n", err ? err : "Unknown error.\n");
        json_value_free(val);
    }

    free(response);
}
/* ========================================================== */
/* 4.15  Creare colecţie + populare cu filme din prompt       */
/* ========================================================== */
void handle_add_collection(void)
{
    if (!has_library_access()) {
        printf("ERROR: You do not have access to the library.\n");
        return;
    }

    /* —— titlu —— */
    char title[1024];
    printf("title="); fflush(stdout);
    if (!fgets(title, sizeof(title), stdin)) return;
    title[strcspn(title, "\r\n")] = '\0';
    if (strlen(title) == 0 || strlen(title) > 1000) {
        printf("ERROR: Invalid title length.\n"); return;
    }

    /* —— număr filme —— */
    char buf[32];
    printf("num_movies="); fflush(stdout);
    if (!fgets(buf, sizeof(buf), stdin)) return;
    int num_movies = atoi(buf);
    if (num_movies <= 0) {
        printf("ERROR: Invalid number of movies.\n"); return;
    }

    /* —— 1️⃣  POST /collections cu doar title —— */
    char body[2048];
    snprintf(body, sizeof(body), "{\"title\":\"%s\"}", title);
    char *data[1] = { body };

    int nc; char **cks = get_cookies(&nc);
    char *jwt = get_jwt_header();

    int sock = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    char *req = compute_post_request(
        HOST, "/api/v1/tema/library/collections",
        "application/json", data, 1, cks, nc, jwt);
    free(cks); free(jwt);

    send_to_server(sock, req); free(req);
    char *resp = receive_from_server(sock); close_connection(sock);

    if (!resp || !(strstr(resp, "200 OK") || strstr(resp, "201"))) {
        printf("ERROR: Failed to create collection.\n");
        free(resp); return;
    }

    /* —— extragem id colecţie —— */
    char *json = basic_extract_json_response(resp);
    if (!json) {
        printf("ERROR: Server did not return collection id.\n");
        free(resp); return;
    }
    JSON_Value  *v = json_parse_string(json);
    JSON_Object *o = json_value_get_object(v);
    int coll_id    = (int)json_object_get_number(o, "id");
    json_value_free(v);
    free(resp);

    printf("SUCCESS: Collection \"%s\" created with id=%d\n", title, coll_id);

    /* —— 2️⃣  populăm cu filme —— */
    int ok_cnt = 0;
    for (int i = 0; i < num_movies; i++) {
        printf("movie_id[%d]=", i); fflush(stdout);
        if (!fgets(buf, sizeof(buf), stdin)) break;
        int movie_id = atoi(buf);
        if (add_movie_to_collection_by_id(coll_id, movie_id)) ok_cnt++;
    }
    
      if (ok_cnt == num_movies) {
        printf("SUCCESS: Toate filmele au fost adăugate în colecţie\n");
   } else {
       // În failure_cases testul aşteaptă să vadă un ERROR
       printf("ERROR: Doar %d din %d filme au fost adăugate în colecţie\n", ok_cnt, num_movies);
   }
    fflush(stdout);

}

void handle_get_collection(void) {
    if (!has_library_access()) {
        printf("ERROR: You do not have access to the library.\n");
        return;
    }

    /* Get collection ID from prompt */
    char buf[32];
    printf("id="); fflush(stdout);
    if (!fgets(buf, sizeof(buf), stdin)) return;
    int collection_id = atoi(buf);
    if (collection_id <= 0) {
        printf("ERROR: Invalid collection ID.\n");
        return;
    }

    /* Prepare request */
    int nc;
    char **cookies = get_cookies(&nc);
    char *jwt = get_jwt_header();

    /* Build URL with collection ID */
    char url[128];
    snprintf(url, sizeof(url), "/api/v1/tema/library/collections/%d", collection_id);

    /* Send request */
    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    char *request = compute_get_request(HOST, url, NULL, cookies, nc, jwt);
    free(cookies);
    free(jwt);

    send_to_server(sockfd, request);
    free(request);

    /* Get response */
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);

    /* Process response */
    char *json = basic_extract_json_response(response);
    if (!json) {
        printf("ERROR: Invalid server response.\n");
        free(response);
        return;
    }

    if (strstr(response, "200 OK")) {
        JSON_Value *root = json_parse_string(json);
        JSON_Object *collection = json_value_get_object(root);
        
        /* Print collection details */
        printf("SUCCESS: Detalii colecție\n");
        printf("title: %s\n", json_object_get_string(collection, "title"));
        printf("owner: %s\n", json_object_get_string(collection, "owner"));
        
        /* Print movies in collection */
        JSON_Array *movies = json_object_get_array(collection, "movies");
        size_t count = json_array_get_count(movies);
        for (size_t i = 0; i < count; i++) {
            JSON_Object *movie = json_array_get_object(movies, i);
            int movie_id = (int)json_object_get_number(movie, "id");
            const char *title = json_object_get_string(movie, "title");
            printf("#%d: %s\n", movie_id, title ? title : "(null)");
        }
        
        json_value_free(root);
    } else {
        JSON_Value *val = json_parse_string(json);
        JSON_Object *obj = json_value_get_object(val);
        const char *err = json_object_get_string(obj, "error");
        printf("ERROR: %s\n", err ? err : "Unknown error");
        json_value_free(val);
    }

    free(response);
}

void handle_delete_collection(void) {
    if (!has_library_access()) {
        printf("ERROR: You do not have access to the library.\n");
        return;
    }

    // Read collection id from user
    char input[32];
    printf("id=");
    if (!fgets(input, sizeof(input), stdin)) return;
    int id = atoi(input);

    if (id <= 0) {
        printf("ERROR: Invalid collection ID!\n");
        return;
    }

    // Build URL directly with the given id
    char url[BUFLEN];
    snprintf(url, sizeof(url), "/api/v1/tema/library/collections/%d", id);

    int nc;
    char **cookies = get_cookies(&nc);
    char *jwt = get_jwt_header();

    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    char *request = compute_delete_request(HOST, url, NULL, cookies, nc, jwt);

    free(cookies);
    free(jwt);

    send_to_server(sockfd, request);
    free(request);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);

    char *json = basic_extract_json_response(response);

    if (strstr(response, "200 OK")) {
        printf("SUCCESS: Collection deleted successfully.\n");
    } else if (json) {
        JSON_Value *val = json_parse_string(json);
        JSON_Object *obj = json_value_get_object(val);
        const char *err = json_object_get_string(obj, "error");
        printf("ERROR: %s\n", err ? err : "Unknown error.\n");
        json_value_free(val);
    } else {
        printf("ERROR: Invalid server response.\n");
    }

    free(response);
}

void handle_add_movie_to_collection(void) {
    if (!has_library_access()) {
        printf("ERROR: You do not have access to the library.\n");
        return;
    }

    // Get collection ID
    char buf[32];
    printf("collection_id=");
    fflush(stdout);
    if (!fgets(buf, sizeof(buf), stdin)) return;
    int collection_id = atoi(buf);
    if (collection_id <= 0) {
        printf("ERROR: Invalid collection ID.\n");
        return;
    }

    // Get movie ID
    printf("movie_id=");
    fflush(stdout);
    if (!fgets(buf, sizeof(buf), stdin)) return;
    int movie_id = atoi(buf);
    if (movie_id <= 0) {
        printf("ERROR: Invalid movie ID.\n");
        return;
    }

    // Prepare JSON payload
    char json_body[64];
    snprintf(json_body, sizeof(json_body), "{\"id\":%d}", movie_id);
    char *body_data[1] = { json_body };

    // Prepare URL
    char url[128];
    snprintf(url, sizeof(url), "/api/v1/tema/library/collections/%d/movies", collection_id);

    // Get auth tokens
    int nc;
    char **cookies = get_cookies(&nc);
    char *jwt = get_jwt_header();

    // Create and send request
    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    char *request = compute_post_request(
        HOST, url, "application/json", body_data, 1, cookies, nc, jwt);
    free(cookies);
    free(jwt);

    send_to_server(sockfd, request);
    free(request);

    // Get response
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);

    // Process response
    if (!response) {
        printf("ERROR: No response from server.\n");
        return;
    }

    if (strstr(response, "200 OK") || strstr(response, "201")) {
        printf("SUCCESS: Film adăugat în colecție\n");
    } else {
        char *json = basic_extract_json_response(response);
        if (json) {
            JSON_Value *val = json_parse_string(json);
            if (val) {
                JSON_Object *obj = json_value_get_object(val);
                const char *err = json_object_get_string(obj, "error");
                printf("ERROR: %s\n", err ? err : "Failed to add movie to collection");
                json_value_free(val);
            }
            free(json);
        } else {
            printf("ERROR: Failed to add movie to collection\n");
        }
    }

    free(response);
}

void handle_delete_movie_from_collection(void) {
    if (!has_library_access()) {
        printf("ERROR: You do not have access to the library.\n");
        return;
    }

    // Get collection ID
    char buf[32];
    printf("collection_id=");
    fflush(stdout);
    if (!fgets(buf, sizeof(buf), stdin)) return;
    int collection_id = atoi(buf);
    if (collection_id <= 0) {
        printf("ERROR: Invalid collection ID.\n");
        return;
    }

    // Get movie ID
    printf("movie_id=");
    fflush(stdout);
    if (!fgets(buf, sizeof(buf), stdin)) return;
    int movie_id = atoi(buf);
    if (movie_id <= 0) {
        printf("ERROR: Invalid movie ID.\n");
        return;
    }

    // Prepare URL
    char url[128];
    snprintf(url, sizeof(url), 
             "/api/v1/tema/library/collections/%d/movies/%d", 
             collection_id, movie_id);

    // Get auth tokens
    int nc;
    char **cookies = get_cookies(&nc);
    char *jwt = get_jwt_header();

    // Create and send request
    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    char *request = compute_delete_request(HOST, url, NULL, cookies, nc, jwt);
    free(cookies);
    free(jwt);

    send_to_server(sockfd, request);
    free(request);

    // Get response
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);

    // Process response
    if (!response) {
        printf("ERROR: No response from server.\n");
        return;
    }

    if (strstr(response, "200 OK") || strstr(response, "204")) {
        printf("SUCCESS: Film șters din colecție\n");
    } else {
        char *json = basic_extract_json_response(response);
        if (json) {
            JSON_Value *val = json_parse_string(json);
            if (val) {
                JSON_Object *obj = json_value_get_object(val);
                const char *err = json_object_get_string(obj, "error");
                printf("ERROR: %s\n", err ? err : "Failed to remove movie from collection");
                json_value_free(val);
            }
            free(json);
        } else {
            printf("ERROR: Failed to remove movie from collection\n");
        }
    }

    free(response);
}