#ifndef _HTTP_REQUESTS_H_
#define _HTTP_REQUESTS_H_

// Builds a GET request string
char *compute_get_request(char *host, char *url, char *query_params,
                          char **cookies, int cookies_count, char *TWJ);

// Builds a DELETE request string
char *compute_delete_request(char *host, char *url, char *query_params,
                             char **cookies, int cookies_count, char *TWJ);

// Builds a POST request string
char *compute_post_request(char *host, char *url, char *content_type, char **body_data,
                           int body_data_fields_count, char **cookies, int cookies_count, char *TWJ);

// Builds a PUT request string
char *compute_put_request(char *host, char *url, char *content_type, char **body_data,
                          int body_data_fields_count, char **cookies, int cookies_count, char *TWJ);

#endif
