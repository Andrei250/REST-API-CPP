#include "handler.hpp"
#include "client.hpp"

string command;
int sockfd;
char *message, *response;
char *HOST = (char*) "34.118.48.238";
int PORT = 8080;
char *REGISTER_ROUTE = (char*) "/api/v1/tema/auth/register";
char *AUTH_ROUTE = (char*) "/api/v1/tema/auth/login";
char *LOGOUT_ROUTE = (char*) "/api/v1/tema/auth/logout";
char *LIBRARY_ACCESS_ROUTE = (char*) "/api/v1/tema/library/access";
char *BOOKS_ROUTE = (char*) "/api/v1/tema/library/books";
char *CONTENT_TYPE = (char*) "application/json";
char *POST_REQ = (char*) "POST";
char *GET_REQ = (char*) "GET";
char *DELETE_REQ = (char*) "DELETE";
Client client;

// creeaza conexiunea prin socket
void createConnection() {
    sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        cerr << "EROARE LA DESCHDIEREA SOCKET-ULUI\n";
        exit(1);
    }
}

Client::Client() {
}

// extragerea codului din raspuns (ex 200, 404, etc)
int Client::getCode(char *response) {
    char code[strlen(response) + 1];
    strcpy(code, (response + 9));

    if (strlen(code) >= 4) {
        code[4] = '\0';
    }

    return atoi(code);
}

// extragerea erorii din payload
char* Client::getErrorMessage(char *response) {
    char *parsed = strstr(response, "error");

    if (parsed != NULL && strlen(parsed) > 8) {
        parsed = parsed + 8;

        if (strlen(parsed) >= 2) {
            parsed[strlen(parsed) - 2] = '\0';
        }
    }
    
    return parsed == NULL || strlen(parsed) < 8 ? (char*)"" : parsed;
}

// extragerea erorii 404 din raspunsurile de tip html
char* Client::parse404Error(char *response) {
    char *parsed = strstr(response, "<pre>");

    if (parsed != NULL && strlen(parsed) > 5) {
        parsed = parsed + 5;

        int pos = strstr(parsed, "</pre>") - parsed;

        if (pos <= strlen(parsed)) {
            parsed[pos] = '\0';
        }
    }
    
    return parsed == NULL || strlen(parsed) < 5 ? (char*)"" : parsed;
}

// Handle Bad Request
char* Client::getyBadRequest(char *respone) {
    char *parsed = strstr(response, "Bad Request");

    if (parsed != NULL && strlen(parsed) > 11) {
        parsed[11] = '\0';
    }

    return parsed == NULL || strlen(parsed) < 11 ? (char*) "" : parsed;
}

// extragerea cookie-ului de login
char* Client::extractConnectionCookie(char *response) {
    char *parsed = strstr(response, "Set-Cookie");

    if (parsed != NULL && strlen(parsed) > 12) {
        parsed = parsed + 12;
        int pos = strstr(parsed, ";") - parsed;

        parsed[pos] = '\0'; 
    }

    
    return parsed == NULL || strlen(parsed) < 12 ? (char*) "" : parsed;
}

// extragerea token-ului de acces
char* Client::extractAccessToken(char* response) {
    char *parsed = strstr(response, "token");

    if (parsed != NULL && strlen(parsed) > 8) {
        parsed = parsed + 8;

        parsed[strlen(parsed) - 2] = '\0';
    }

    return parsed == NULL || strlen(parsed) < 8 ? (char*) "" : parsed;
}

// extragerea vectorului de carti
char* Client::getPayload(char *response) {
    char *parsed = strstr(response, "[");

    if (parsed != NULL && strlen(parsed) > 1) {
        parsed = parsed + 1;
        int pos = strstr(parsed, "]") - parsed;

        parsed[pos] = '\0';
    }

    return parsed == NULL || strlen(parsed) < 1 ? (char*) "" : parsed;
}

// inregistrarea unui user
// creearea unui Post request cu informatiile necesare
// trimiterea requestului la server
// extragerea informatiilor din raspuns
void Client::registerUser(string username, string password) {
    vector<pair<string,string>> payload = {
        {"username", username},
        {"password", password}
    };

    createConnection();
    message = buildPOSTDELETERequest(HOST, REGISTER_ROUTE, CONTENT_TYPE,
                                payload, payload.size(), NULL, 0, POST_REQ,
                                false);

    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    if (strlen(response)) {
        int respCode = client.getCode(response);

        if (respCode >= 200 && respCode < 300) {
            cout << "SUCCES: User inregistrat cu succes.\n";
        } else if (respCode == 404) {
            cout << "EROARE: " << client.parse404Error(response) << '\n';
        } else if(respCode == 400 &&
                    strlen(client.getyBadRequest(response)) > 1) {
            cout << "EROARE: " << client.getyBadRequest(response) << '\n';
        } else if (respCode == 429) {
            cout << "EROARE: Too many requests, please try again later.\n";
        } else {
            cout << "EROARE: " << client.getErrorMessage(response) << '\n';
        }
    } else {
        cout << "SUCCES: User inregistrat cu succes.\n";
    }

    close(sockfd);
}

// logarea unui user
// creearea unui Post request cu informatiile necesare
// trimiterea requestului la server
// extragerea informatiilor din raspuns
void Client::loginUser(string username, string password) {
    vector<pair<string,string>> payload = {
        {"username", username},
        {"password", password}
    };

    createConnection();
    message = buildPOSTDELETERequest(HOST, AUTH_ROUTE, CONTENT_TYPE,
                                payload, payload.size(), NULL, 0, POST_REQ,
                                false);

    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    if (strlen(response)) {
        int respCode = client.getCode(response);

        if (respCode == 200) {
            client.cookie = client.extractConnectionCookie(response);
            client.libraryAcces = "";
            cout << "SUCCES: User conectat cu succes.\n";
        } else if (respCode == 404) {
            cout << "EROARE: " << client.parse404Error(response) << '\n';
        } else if(respCode == 400 &&
                    strlen(client.getyBadRequest(response)) > 1) {
            cout << "EROARE: " << client.getyBadRequest(response) << '\n';
        } else if (respCode == 429) {
            cout << "EROARE: Too many requests, please try again later.\n";
        } else {
            cout << "EROARE: " << client.getErrorMessage(response) << '\n';
        }
    }

    close(sockfd);
}

// intrarea in biblioteca
// creearea unui Get request cu informatiile necesare (+ token autentificare)
// trimiterea requestului la server
// extragerea informatiilor din raspuns
void Client::enterLibrary() {
    char **cookies = (char**) malloc(sizeof(char*));

    cookies[0] = strdup(client.cookie.c_str());
    createConnection();

    message = buildGETRequest(HOST, LIBRARY_ACCESS_ROUTE,
                                NULL, cookies, 1, false);

    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    if (strlen(response)) {
        int respCode = client.getCode(response);

        if (respCode == 200) {
            client.libraryAcces = client.extractAccessToken(response);
            cout << "SUCCES: Acces garantat.\n";
        } else if (respCode == 404) {
            cout << "EROARE: " << client.parse404Error(response) << '\n';
        } else if(respCode == 400 &&
                    strlen(client.getyBadRequest(response)) > 1) {
            cout << "EROARE: " << client.getyBadRequest(response) << '\n';
        } else if (respCode == 429) {
            cout << "EROARE: Too many requests, please try again later.\n";
        } else {
            cout << "EROARE: " << client.getErrorMessage(response) << '\n';
        }
    }

    close(sockfd);
}

// extragerea tuturor cartilor
// creearea unui Get request cu informatiile necesare (+ token JWT)
// trimiterea requestului la server
// extragerea informatiilor din raspuns
void Client::getBooks() {
    char **cookies = (char**) malloc(sizeof(char*));

    cookies[0] = strdup(client.libraryAcces.c_str());
    createConnection();

    message = buildGETRequest(HOST, BOOKS_ROUTE,
                                NULL, cookies, 1, true);

    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    if (strlen(response)) {
        int respCode = client.getCode(response);

        if (respCode == 200) {
            response = client.getPayload(response);

            if (strlen(response)) {
                cout << "SUCCES: Carti extrase cu succes.\n";
                cout << "[" << response << "]\n";
            } else {
                cout << "SUCCES: Nu se afla inca nicio carte adaugata.\n";
            }
        } else if (respCode == 404 &&
                    strlen(client.parse404Error(response)) > 1) {
            cout << "EROARE: " << client.parse404Error(response) << '\n';
        } else if(respCode == 400 &&
                    strlen(client.getyBadRequest(response)) > 1) {
            cout << "EROARE: " << client.getyBadRequest(response) << '\n';
        } else if (respCode == 429) {
            cout << "EROARE: Too many requests, please try again later.\n";
        } else {
            cout << "EROARE: " << client.getErrorMessage(response) << '\n';
        }
    }

    close(sockfd);
}

// adaugarea unei carti
// creearea unui Post request cu informatiile necesare (+ token JWT)
// trimiterea requestului la server
// extragerea informatiilor din raspuns
void Client::addBook() {
    string title, author, genre, publisher, pageCount;
    char **cookies = (char**) malloc(sizeof(char*));

    cookies[0] = strdup(client.libraryAcces.c_str());
    cout << "title=";
    getline(cin, title);
    cout << "author=";
    getline(cin, author);
    cout << "genre=";
    getline(cin, genre);
    cout << "publisher=";
    getline(cin, publisher);
    cout << "page_count=";
    getline(cin, pageCount);

    vector<pair<string,string>> payload = {
        {"title", title.size() < 1 ? "" : title},
        {"author", author.size() < 1 ? "" : author},
        {"genre", genre.size() < 1 ? "" : genre},
        {"page_count", pageCount.size() < 1 ? "" : pageCount},
        {"publisher", publisher.size() < 1 ? "" : publisher}
    };

    createConnection();
    message = buildPOSTDELETERequest(HOST, BOOKS_ROUTE, CONTENT_TYPE,
                                payload, payload.size(), cookies, 1, POST_REQ,
                                true);

    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    if (strlen(response)) {
        int respCode = client.getCode(response);

        if (respCode == 200) {
            cout << "SUCCES: Carte adaugata cu succes\n";
        } else if (respCode == 404 &&
                    strlen(client.parse404Error(response)) > 1) {
            cout << "EROARE: " << client.parse404Error(response) << '\n';
        } else if(respCode == 400 &&
                    strlen(client.getyBadRequest(response)) > 1) {
            cout << "EROARE: " << client.getyBadRequest(response) << '\n';
        } else if (respCode == 429) {
            cout << "EROARE: Too many requests, please try again later.\n";
        } else {
            cout << "EROARE: " << client.getErrorMessage(response) << '\n';
        }
    }

    close(sockfd);   
}

// extragerea unei carti
// creearea unui Post request cu informatiile necesare (+ token JWT)
// trimiterea requestului la server
// extragerea informatiilor din raspuns
void Client::getBook(string id) {
    char **cookies = (char**) malloc(sizeof(char*));

    cookies[0] = strdup(client.libraryAcces.c_str());

    createConnection();

    message = buildGETRequest(HOST, BOOKS_ROUTE,
                                (char *)id.c_str(), cookies, 1, true);

    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    if (strlen(response)) {
        int respCode = client.getCode(response);

        if (respCode == 200) {
            response = client.getPayload(response);
            cout << "SUCCES: Carte extrasa cu succes.\n";
            cout << response << '\n';
        } else if (respCode == 429) {
            cout << "EROARE: Too many requests, please try again later.\n";
        } else if(respCode == 400 &&
                    strlen(client.getyBadRequest(response)) > 1) {
            cout << "EROARE: " << client.getyBadRequest(response) << '\n';
        } else if (respCode == 400 &&
                    strlen(client.getyBadRequest(response)) > 1) {
            cout << "EROARE: " << client.getyBadRequest(response) << '\n';
        } else {
            cout << "EROARE: " << client.getErrorMessage(response) << '\n';
        }
    }

    close(sockfd);
}

// stergerea unei carti
// creearea unui Post request cu informatiile necesare (+ token JWT)
// trimiterea requestului la server
// extragerea informatiilor din raspuns
void Client::deleteBook(string id) {
    char **cookies = (char**) malloc(sizeof(char*));
    string route = string(BOOKS_ROUTE) + "/" +id;

    cookies[0] = strdup(client.libraryAcces.c_str());

    createConnection();

    message = buildPOSTDELETERequest(HOST, route.c_str(), CONTENT_TYPE,
                                {}, 0, cookies, 1, DELETE_REQ,
                                true);

    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    if (strlen(response)) {
        int respCode = client.getCode(response);

        if (respCode == 200) {
            cout << "SUCCES: Carte stearsa cu succes.\n";
        } else if (respCode == 404 &&
                    strlen(client.parse404Error(response)) > 1) {
            cout << "EROARE: " << client.parse404Error(response) << '\n';
        } else if (respCode == 429) {
            cout << "EROARE: Too many requests, please try again later.\n";
        } else if(respCode == 400 &&
                    strlen(client.getyBadRequest(response)) > 1) {
            cout << "EROARE: " << client.getyBadRequest(response) << '\n';
        } else {
            cout << "EROARE: " << client.getErrorMessage(response) << '\n';
        }  
    }

    close(sockfd);
}

// delogarea unui user
// creearea unui Post request cu informatiile necesare (+ token JWT)
// trimiterea requestului la server
// extragerea informatiilor din raspuns
void Client::logoutUser() {
    char **cookies = (char**) malloc(sizeof(char*));

    cookies[0] = strdup(client.cookie.c_str());
    createConnection();

    message = buildGETRequest(HOST, LOGOUT_ROUTE,
                                NULL, cookies, 1, false);

    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    if (strlen(response)) {
        int respCode = client.getCode(response);

        if (respCode == 200) {
            client.libraryAcces = "";
            client.cookie = "";
            cout << "SUCCES: User deconectat cu succes.\n";
        } else if (respCode == 404 &&
                    strlen(client.parse404Error(response)) > 1) {
            cout << "EROARE: " << client.parse404Error(response) << '\n';
        } else if(respCode == 400 &&
                    strlen(client.getyBadRequest(response)) > 1) {
            cout << "EROARE: " << client.getyBadRequest(response) << '\n';
        } else if (respCode == 429) {
            cout << "EROARE: Too many requests, please try again later.\n";
        } else {
            cout << "EROARE: " << client.getErrorMessage(response) << '\n';
        }
    } else {
        client.libraryAcces = "";
        client.cookie = "";
    }

    close(sockfd);
}

int main() {
    while (1) {
        command.clear();
        getline(cin, command);

        // verificarea fiecarui caz de comanda
        if (command == "exit") {
            close(sockfd);
            break;
        } else if (command == "register") {
            string username, password;

            cout << "username=";
            getline(cin, username);
            cout << "password=";
            getline(cin, password);

            client.registerUser(username, password);
        } else if (command == "login") {
            string username, password;

            cout << "username=";
            getline(cin, username);
            cout << "password=";
            getline(cin, password);

            client.loginUser(username, password);
        } else if (command == "enter_library") {
            client.enterLibrary();
        } else if (command == "get_books") {
            client.getBooks();
        } else if (command == "add_book") {
            client.addBook();
        } else if (command == "get_book") {
            string id;
            cout << "id=";
            getline(cin, id);
            client.getBook(id);
        } else if (command == "delete_book") {
            string id;
            cout << "id=";
            getline(cin, id);
            client.deleteBook(id);
        } else if (command == "logout") {
            client.logoutUser();
        }
    }

    return 0;
}