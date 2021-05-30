#ifndef CLIENT_HPP
#define CLIENT_HPP

class Client {
 public:
    string cookie;
    string libraryAcces;
    Client();

    void registerUser(string username, string password);

    void loginUser(string username, string password);

    void enterLibrary();

    void getBooks();

    void addBook();

    void getBook(string id);

    void deleteBook(string id);

    int getCode(char *resposne);

    void logoutUser();

    char* getErrorMessage(char *response);

    char* parse404Error(char *response);

    char* extractConnectionCookie(char *response);

    char* extractAccessToken(char *response);

    char* getPayload(char *response);

    char* getyBadRequest(char *response);

};

#endif