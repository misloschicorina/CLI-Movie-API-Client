#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "admin_commands.h"
#include "commands.h"
#include "helpers.h"
#include "http_requests.h"
#include "parson.h"
#include "state.h"

#define HOST "63.32.125.183"
#define PORT 8081

// Function to add a movie to a collection by its ID (helper function)
static bool add_movie_to_collection_by_id(int coll_id, int movie_id)
{
    char json[64];
    snprintf(json, sizeof(json), "{\"id\":%d}", movie_id);
    char *body[1] = { json };

    char url[128];
    snprintf(url, sizeof(url),
             "/api/v1/tema/library/collections/%d/movies", coll_id);

    int nc; 
    char **cookies = get_cookies(&nc);
    char *jwt = get_jwt_header();

    int sock = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    char *req = compute_post_request(
        HOST, url, "application/json",
        body, 1, cookies, nc, jwt);
    free(cookies); 
    free(jwt);

    send_to_server(sock, req); free(req);
    char *resp = receive_from_server(sock); close_connection(sock);

    bool ok = resp && (strstr(resp, "200 OK") || strstr(resp, "201"));
    free(resp);
    return ok;
}

void handle_get_collections(void) {
    // Check if the user has access to the library
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
    if (!json)
        return;

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
        handle_error_response(json, "Failed to fetch collections.");
    }
    free(response);
}

void handle_add_collection(void) {
    // Check if the user has access to the library
    if (!has_library_access()) {
        printf("ERROR: You do not have access to the library.\n");
        return;
    }

    char title[1024];
    printf("title="); fflush(stdout);
    if (!fgets(title, sizeof(title), stdin)) return;
    title[strcspn(title, "\r\n")] = '\0';
    if (strlen(title) == 0 || strlen(title) > 1000) {
        printf("ERROR: Invalid title length.\n"); 
        return;
    }

    char buf[32];
    printf("num_movies="); fflush(stdout);
    if (!fgets(buf, sizeof(buf), stdin)) return;
    int num_movies = atoi(buf);
    if (num_movies <= 0) {
        printf("ERROR: Invalid number of movies.\n"); 
        return;
    }

    char body[2048];
    snprintf(body, sizeof(body), "{\"title\":\"%s\"}", title);
    char *data[1] = { body };

    int nc; char **cookies = get_cookies(&nc);
    char *jwt = get_jwt_header();

    int sock = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    char *req = compute_post_request(
        HOST, "/api/v1/tema/library/collections",
        "application/json", data, 1, cookies, nc, jwt);
    free(cookies); free(jwt);

    send_to_server(sock, req); free(req);
    char *resp = receive_from_server(sock); close_connection(sock);

    if (!resp || !(strstr(resp, "200 OK") || strstr(resp, "201"))) {
        printf("ERROR: Failed to create collection.\n");
        free(resp); 
        return;
    }

    // Extract collection ID from response
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

    int ok_cnt = 0;
    for (int i = 0; i < num_movies; i++) {
        printf("movie_id[%d]=", i); fflush(stdout);
        if (!fgets(buf, sizeof(buf), stdin)) break;
        int movie_id = atoi(buf);
        if (add_movie_to_collection_by_id(coll_id, movie_id)) ok_cnt++;
    }
    
    if (ok_cnt == num_movies)
        printf("SUCCESS: All movies added in the new collection\n");
    else
       printf("ERROR: Only %d of %d movies were added to collection\n", ok_cnt, num_movies);
}

void handle_get_collection(void) {
    // Check if the user has access to the library
    if (!has_library_access()) {
        printf("ERROR: You do not have access to the library.\n");
        return;
    }

    // Read collection id from user
    char buf[32];
    printf("id="); fflush(stdout);
    if (!fgets(buf, sizeof(buf), stdin)) return;
    int collection_id = atoi(buf);
    if (collection_id <= 0) {
        printf("ERROR: Invalid collection ID.\n");
        return;
    }

    // Prepare request
    int nc;
    char **cookies = get_cookies(&nc);
    char *jwt = get_jwt_header();

    // Build URL with the given collection ID
    char url[128];
    snprintf(url, sizeof(url), "/api/v1/tema/library/collections/%d", collection_id);

    // Create and send request
    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    char *request = compute_get_request(HOST, url, NULL, cookies, nc, jwt);
    free(cookies);
    free(jwt);

    send_to_server(sockfd, request);
    free(request);

    // Receive response
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);

    // Process response
    char *json = basic_extract_json_response(response);
    if (!json) {
        printf("ERROR: Invalid server response.\n");
        free(response);
        return;
    }

    if (strstr(response, "200 OK")) {
        JSON_Value *root = json_parse_string(json);
        JSON_Object *collection = json_value_get_object(root);
        
        // Print collection details
        printf("SUCCESS: Collection details\n");
        printf("title: %s\n", json_object_get_string(collection, "title"));
        printf("owner: %s\n", json_object_get_string(collection, "owner"));
        
        // Print movies in the collection
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
        handle_error_response(json, "Failed to retrieve collection details.");
    }

    free(response);
}

void handle_delete_collection(void) {
    // Check if the user has access to the library
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

    if (strstr(response, "200 OK"))
        printf("SUCCESS: Collection deleted successfully.\n");
    else
        handle_error_response(json, "Failed to delete collection.");

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

    // Use helper
    if (add_movie_to_collection_by_id(collection_id, movie_id))
        printf("SUCCESS: Movie successfully added to collection.\n");
    else
        printf("ERROR: Failed to add movie to collection.\n");
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

    char *json = basic_extract_json_response(response);

    if (strstr(response, "200 OK") || strstr(response, "204"))
        printf("SUCCESS: Movie succesfully deleted from collection.\n");
    else
        handle_error_response(json, "Failed to remove movie from collection");

    free(response);
}