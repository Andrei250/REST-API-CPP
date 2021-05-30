#include "handler.hpp"
#include "config.hpp"

// funcite care construeiste GET request-ul
// preluata din laborator si modificata pentru taskul curent
char *buildGETRequest(const char *host, const char *url, char *params,
                            char **cookies, int count, bool libraryAccess)
{
    char *message = (char*)calloc(BUFLEN, sizeof(char));
    char *line = (char*)calloc(LINELEN, sizeof(char));

    if (params != NULL) {
        sprintf(line, "GET %s/%s HTTP/1.1", url, params);
    } else {
        sprintf(line, "GET %s HTTP/1.1", url);
    }

    compute_message(message, line);

    memset(line, 0, LINELEN);
    sprintf(line, "Host: %s", host);

    compute_message(message, line);

    if (cookies != NULL) {
        // daca este vorba de autorizarea JWT pentru requesturile
        // din librarie
        if (libraryAccess) {
            memset(line, 0, LINELEN);
            sprintf(line, "Authorization: Bearer %s", cookies[0]);
            strcat(message, line);
        } else {
            memset(line, 0, LINELEN);
            sprintf(line, "Cookie: ");
            strcat(message, line);

            for (int i = 0; i < count; ++i) {
                memset(line, 0, LINELEN);
                sprintf(line, "%s; ", cookies[i]);

                strcat(message, line);
            }
        }

        compute_message(message, "");
    }

    compute_message(message, "");
    return message;
}

char *buildPOSTDELETERequest(const char *host, const char *url,
                const char* contentType, vector<pair<string, string>> data,
                int dataCount,  char **cookies, int cookiesCount,
                const char *type, bool libraryAccess)
{
    char *message = (char*)calloc(BUFLEN, sizeof(char));
    char *line = (char*)calloc(LINELEN, sizeof(char));
    char *body_data_buffer = (char*)calloc(LINELEN, sizeof(char));

    sprintf(line, "%s %s HTTP/1.1", type, url);
    compute_message(message, line);

    memset(line, 0, LINELEN);
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    strcpy(body_data_buffer, "");

    // adaugarea payload-ului
    if (dataCount > 0) {
        strcat(body_data_buffer, "{");

        for (int i = 0; i < dataCount; i++) {
            string entry = "\"" + data[i].first + "\": " +
                            "\"" + data[i].second + "\"";
            strcat(body_data_buffer, entry.c_str());

            if (i != dataCount - 1) {
                strcat(body_data_buffer, ",");
            }
        }

        strcat(body_data_buffer, "}\r\n");
    }

    sprintf(line, "Content-Type: %s", contentType);
    compute_message(message, line);
    sprintf(line, "Content-Length: %ld", strlen(body_data_buffer));
    compute_message(message, line);

    // adaugarea cookie-urilor
    if (cookies != NULL) {
        if (libraryAccess) {
            memset(line, 0, LINELEN);
            sprintf(line, "Authorization: Bearer %s", cookies[0]);
            strcat(message, line);
        } else {
            memset(line, 0, LINELEN);
            sprintf(line, "Cookie: ");
            strcat(message, line);

            for (int i = 0; i < cookiesCount; ++i) {
                memset(line, 0, LINELEN);
                sprintf(line, "%s; ", cookies[i]);

                strcat(message, line);
            }
        }

        compute_message(message, "");       
    }

    compute_message(message, "");
    memset(line, 0, LINELEN);
    compute_message(message, body_data_buffer);
    free(line);
    return message;
}
