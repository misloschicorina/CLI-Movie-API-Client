#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "helpers.h"
#include "http_requests.h"

char *compute_get_request(char *host, char *url, char *query_params,
                          char **cookies, int cookies_count, char *TWJ) {
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));
    char *cookie_header = calloc(LINELEN, sizeof(char));

    // First line
    if (query_params)
        sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
    else
        sprintf(line, "GET %s HTTP/1.1", url);
    compute_message(message, line);

    // Host header
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // Cookies header
    if (cookies) {
        strcpy(cookie_header, "Cookie: ");
        for (int i = 0; i < cookies_count; i++) {
            strcat(cookie_header, cookies[i]);
            if (i < cookies_count - 1)
                strcat(cookie_header, "; ");
        }
        compute_message(message, cookie_header);
    }

    // Optional JWT header
    if (TWJ) 
        compute_message(message, TWJ);

    // End of headers
    compute_message(message, "");

    free(line);
    free(cookie_header);
    return message;
}

char *compute_delete_request(char *host, char *url, char *query_params,
                             char **cookies, int cookies_count, char *TWJ) {
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));
    char *cookie_header = calloc(LINELEN, sizeof(char));

    // First line
    if (query_params)
        sprintf(line, "DELETE %s?%s HTTP/1.1", url, query_params);
    else
        sprintf(line, "DELETE %s HTTP/1.1", url);
    compute_message(message, line);

    // Host header
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // Cookies header
    if (cookies) {
        strcpy(cookie_header, "Cookie: ");
        for (int i = 0; i < cookies_count; i++) {
            strcat(cookie_header, cookies[i]);
            if (i < cookies_count - 1)
                strcat(cookie_header, "; ");
        }
        compute_message(message, cookie_header);
    }

    // Optional JWT header
    if (TWJ) 
        compute_message(message, TWJ);

    // End of headers
    compute_message(message, "");

    free(line);
    free(cookie_header);
    return message;
}

char *compute_post_request(char *host, char *url, char *content_type, char **body_data,
                           int body_data_fields_count, char **cookies, int cookies_count, char *TWJ) {
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));
    char *body = calloc(LINELEN, sizeof(char));
    char *cookie_header = calloc(LINELEN, sizeof(char));

    // First line
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);

    // Host header
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // Content-Type
    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);

    // Body data
    for (int i = 0; i < body_data_fields_count; i++) {
        strcat(body, body_data[i]);
        if (i < body_data_fields_count - 1)
            strcat(body, "&");
    }

    // Content-Length
    sprintf(line, "Content-Length: %ld", strlen(body));
    compute_message(message, line);

    // Cookies
    if (cookies) {
        strcpy(cookie_header, "Cookie: ");
        for (int i = 0; i < cookies_count; i++) {
            strcat(cookie_header, cookies[i]);
            if (i < cookies_count - 1)
                strcat(cookie_header, "; ");
        }
        compute_message(message, cookie_header);
    }

    // Optional JWT header
    if (TWJ)
        compute_message(message, TWJ);

    // End headers and add body
    compute_message(message, "");
    compute_message(message, body);

    free(line);
    free(body);
    free(cookie_header);
    return message;
}

char *compute_put_request(char *host, char *url, char *content_type, char **body_data,
                          int body_data_fields_count, char **cookies, int cookies_count, char *TWJ) {
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));
    char *body = calloc(LINELEN, sizeof(char));
    char *cookie_header = calloc(LINELEN, sizeof(char));

    // First line
    sprintf(line, "PUT %s HTTP/1.1", url);
    compute_message(message, line);

    // Host header
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // Content-Type
    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);

    // Body data
    for (int i = 0; i < body_data_fields_count; i++) {
        strcat(body, body_data[i]);
        if (i < body_data_fields_count - 1)
            strcat(body, "&");
    }

    // Content-Length
    sprintf(line, "Content-Length: %ld", strlen(body));
    compute_message(message, line);

    // Cookies
    if (cookies) {
        strcpy(cookie_header, "Cookie: ");
        for (int i = 0; i < cookies_count; i++) {
            strcat(cookie_header, cookies[i]);
            if (i < cookies_count - 1)
                strcat(cookie_header, "; ");
        }
        compute_message(message, cookie_header);
    }

    // Optional JWT header
    if (TWJ)
        compute_message(message, TWJ);

    // End of headers and add body
    compute_message(message, "");
    compute_message(message, body);

    free(line);
    free(body);
    free(cookie_header);
    return message;
}

