#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "admin_commands.h"
#include "helpers.h"
#include "http_requests.h"
#include "state.h"
#include "commands.h"
#include "parson.h"

#define HOST "63.32.125.183"
#define PORT 8081

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

            if (err && (strstr(err, "invalid") || strstr(err, "incorrect")))
                printf("ERROR: Invalid username or password.\n");
            else
                handle_error_response(json_start, "Login failed.");
            
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
        handle_error_response(json_start, "Logout failed.");
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
        }
    } else {
        handle_error_response(json_start, "Access request failed.");
    }

    free(response);
}