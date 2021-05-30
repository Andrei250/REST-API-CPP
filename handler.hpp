#ifndef HANDLER_HPP
#define HANDLER_HPP
#include "config.hpp"

char* buildGETRequest(const char* host, const char* url, char* params,
                    char** cookies, int count, bool libraryAccess);

char* buildPOSTDELETERequest(const char *host, const char *url,
                    const char* contentType, vector<pair<string, string>> data,
                    int dataCount,  char **cookies, int cookiesCount,
                    const char *type, bool libraryAccess);
#endif
