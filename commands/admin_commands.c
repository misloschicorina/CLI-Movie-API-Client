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

#define HOST "63.32.125.183"
#define PORT 8081

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
        handle_error_response(json_start, "Other error from server.");
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
        handle_error_response(json_start, "Logout failed.");
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
        handle_error_response(json_start, "Failed to create user.");
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
       handle_error_response(json_start, "Failed to retrieve users.");
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

            if (err && strstr(err, "not found"))
                printf("ERROR: The user '%s' does not exist.\n", username);
            else
                handle_error_response(json_start, "Failed to delete user.");

            json_value_free(val);
        } else {
            printf("ERROR: Invalid response from server.\n");
        }
    }

    free(response);
}