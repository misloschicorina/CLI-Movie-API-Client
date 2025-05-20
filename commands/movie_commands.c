#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
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

void handle_get_movies(void) {
    // Check if the user has access to the library
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
        
        printf("SUCCESS: Movie list\n");
        for (size_t i = 0; i < count; i++) {
            JSON_Object *movie = json_array_get_object(movies, i);
            int id = (int)json_object_get_number(movie, "id");
            const char *title = json_object_get_string(movie, "title");
            printf("#%d %s\n", id, title ? title : "(null)");
        }
        
        json_value_free(val);
    } else {
        handle_error_response(json_start, "Failed to get movies.");
    }
   
    free(response);
}

void handle_get_movie(void) {
    // Check if the user has access to the library
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

        printf("SUCCESS: Movie details\n");
        printf("title: %s\n", title ? title : "(null)");
        printf("year: %d\n", year);
        printf("description: %s\n", description ? description : "(null)");
        printf("rating: %.1f\n", rating);

        json_value_free(val);
    } else {
        handle_error_response(json, "Movie not found.");
    }
    
    free(response);
}

void handle_add_movie(void) {
    // Check if the user has access to the library
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
    char *request = compute_post_request(HOST, "/api/v1/tema/library/movies",
        "application/json", body_data, 1, cookies, nc, jwt_header);

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
        handle_error_response(json_start, "Failed to add movie.");
    }

    free(response);
}

void handle_delete_movie(void) {
    // Check if the user has access to the library
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
    if (strstr(resp, "200 OK")) 
        printf("SUCCESS: Movie deleted successfully.\n");
    else
        handle_error_response(json, "Failed to delete movie.");

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
    if (strstr(response, "200 OK"))
        printf("SUCCESS: Movie updated successfully.\n");
    else
        handle_error_response(json, "Failed to update movie.");

    free(response);
}