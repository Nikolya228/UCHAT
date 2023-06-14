#include <errno.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <resolv.h>
#include "openssl/ssl.h"
#include "openssl/err.h"
#include <pthread.h>
#include "../libs/sqlite/sqlite3.h"
#include <stdbool.h>
#include <stdlib.h>
#define FAIL    -1

SSL *clients[100];
sqlite3 *db;

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// Create the SSL socket and intialize the socket address structure
int OpenListener(int port)
{
    int sd;
    struct sockaddr_in addr;
    sd = socket(PF_INET, SOCK_STREAM, 0);
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sd, (struct sockaddr*)&addr, sizeof(addr)) != 0 )
    {
        perror("can't bind port");
        abort();
    }
    if ( listen(sd, 100) != 0 )
    {
        perror("Can't configure listening port");
        abort();
    }
    return sd;
}
int isRoot()
{
    if (getuid() != 0)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}
SSL_CTX* InitServerCTX(void)
{
    SSL_METHOD *method;
    SSL_CTX *ctx;
    OpenSSL_add_all_algorithms();  /* load & register all cryptos, etc. */
    SSL_load_error_strings();   /* load all error messages */
    method = TLSv1_2_server_method();  /* create new server-method instance */
    ctx = SSL_CTX_new(method);   /* create new context from method */
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}
void LoadCertificates(SSL_CTX* ctx, char* CertFile, char* KeyFile)
{
    /* set the local certificate from CertFile */
    if ( SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* set the private key from KeyFile (may be the same as CertFile) */
    if ( SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* verify private key */
    if ( !SSL_CTX_check_private_key(ctx) )
    {
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
}
void ShowCerts(SSL* ssl)
{
    X509 *cert;
    char *line;
    cert = SSL_get_peer_certificate(ssl); /* Get certificates (if available) */
    if ( cert != NULL )
    {
        printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Subject: %s\n", line);
        free(line);
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);
        free(line);
        X509_free(cert);
    }
    else
        printf("No certificates.\n");
}

int registrationUser(char *str){
    char login[512];
    char pass[512];
    str += 2;
    for (int i = 0; i < strlen(str); i++){
        if (str[i] != '@') login[i] = str[i];
        else{
            login[i] = '\0';
            i++;
            for (int j = i; j < strlen(str); j++){
                pass[j - i] = str[j];
            }
            break;
        }
    }
    // printf("%s\n", str);
    // printf("%s\n", login);
    // printf("%s\n", pass);

    pthread_mutex_lock(&clients_mutex);
    
    char *err_msg = 0;

    sqlite3_stmt *res;

    int rc = sqlite3_prepare_v2(db, "INSERT INTO Users(name, password, online, image) VALUES(?, ?, true, 1)", -1, &res, 0);

    if (rc == SQLITE_OK){
        sqlite3_bind_text(res, 1, login, -1, SQLITE_STATIC);
        sqlite3_bind_text(res, 2, pass, -1, SQLITE_STATIC);
    }
    else {
        fprintf(stderr, "Failed to insert statement: %s\n", sqlite3_errmsg(db));
    }

    pthread_mutex_unlock(&clients_mutex);

    if (sqlite3_step(res) == SQLITE_DONE){
        sqlite3_finalize(res);
        pthread_mutex_lock(&clients_mutex);

        rc = sqlite3_prepare_v2(db, "SELECT * FROM Users WHERE name = ?", -1, &res, 0);
        sqlite3_bind_text(res, 1, login, -1, SQLITE_STATIC);
        sqlite3_step(res);
        pthread_mutex_unlock(&clients_mutex);
        return sqlite3_column_int(res, 0);
    }
    else {
        return 0;
    }
}

void notOnline(int id){
    sqlite3_stmt *res;
    
    pthread_mutex_lock(&clients_mutex);

    sqlite3_prepare_v2(db, "UPDATE Users SET online = false WHERE id = ?", -1, &res, 0);
    sqlite3_bind_int(res, 1, id);
    sqlite3_step(res);

    pthread_mutex_unlock(&clients_mutex);
}

int loginUser(char *str){
    char login[512];
    char pass[512];
    str += 2;
    for (int i = 0; i < strlen(str); i++){
        if (str[i] != '@') login[i] = str[i];
        else{
            login[i] = '\0';
            i++;
            for (int j = i; j < strlen(str); j++){
                pass[j - i] = str[j];
            }
            break;
        }
    }
    // printf("%s\n", str);
    // printf("%s\n", login);
    // printf("%s\n", pass);

    pthread_mutex_lock(&clients_mutex);

    sqlite3_stmt *res;

    int rc = sqlite3_prepare_v2(db, "SELECT * FROM Users WHERE name = ? AND password = ?", -1, &res, 0);

    if (rc == SQLITE_OK){
        sqlite3_bind_text(res, 1, login, -1, SQLITE_STATIC);
        sqlite3_bind_text(res, 2, pass, -1, SQLITE_STATIC);
    }
    else {
        fprintf(stderr, "Failed to insert statement: %s\n", sqlite3_errmsg(db));
    }
    

    if (sqlite3_step(res) == SQLITE_ROW){
        int id = sqlite3_column_int(res, 0);
        // sqlite3_prepare_v2(db, "SELECT * FROM Users WHERE id = ? AND online != true", -1, &res, 0);
        // sqlite3_bind_int(res, 1, id);
        // if (sqlite3_step(res) != SQLITE_ROW){
        //     pthread_mutex_unlock(&clients_mutex);
        //     return -2;
        // }
        sqlite3_prepare_v2(db, "UPDATE Users SET online = true WHERE id = ?", -1, &res, 0);
        sqlite3_bind_int(res, 1, id);
        sqlite3_step(res);
        pthread_mutex_unlock(&clients_mutex);
        return id;
    }
    else {
        pthread_mutex_unlock(&clients_mutex);
        return -1;
    }
}

int deleteChat(char *str){
    char temp_id[16];
    char temp_chater[16];
    bool flag = false;
    int idx = 0;
    for (int i = 0; i < strlen(str); i++){
        if (str[i] == '@' && !flag){
            temp_id[i] = '\0';
            flag = true;
        }
        else if (str[i] == '@' && flag){
            temp_chater[idx] = '\0';
            break;
        }
        else if (!flag) temp_id[i] = str[i];
        else if (flag) {
            temp_chater[idx] = str[i];
            idx++;
        }
    }

    pthread_mutex_lock(&clients_mutex);

    sqlite3_stmt *res;

    // write(STDOUT_FILENO, temp_id, strlen(temp_id));
    // write(STDOUT_FILENO, "\n", strlen("\n"));
    // write(STDOUT_FILENO, temp_chater, strlen(temp_chater));
    // write(STDOUT_FILENO, "\n", strlen("\n"));
    int id = atoi(temp_id);
    int chater = atoi(temp_chater);
    sqlite3_prepare_v2(db, "DELETE FROM Messeges JOIN Chats ON Messeges.chat_id = Chats.id WHERE (Chats.user_id = ? AND Chats.chater_id = ?) OR (Chats.user_id = ? AND Chats.chater_id = ?)", -1, &res, 0);
    sqlite3_bind_int(res, 1, id);
    sqlite3_bind_int(res, 2, chater);
    sqlite3_bind_int(res, 3, chater);
    sqlite3_bind_int(res, 4, id);
    sqlite3_step(res);
    int rc = sqlite3_prepare_v2(db, "DELETE FROM Chats WHERE (user_id = ? AND chater_id = ?) OR (user_id = ? AND chater_id = ?)", -1, &res, 0);
    if (rc != SQLITE_OK){
        fprintf(stderr, "Failed to insert statement: %s\n", sqlite3_errmsg(db));
    }
    sqlite3_bind_int(res, 1, id);
    sqlite3_bind_int(res, 2, chater);
    sqlite3_bind_int(res, 3, chater);
    sqlite3_bind_int(res, 4, id);
    sqlite3_step(res);

    pthread_mutex_unlock(&clients_mutex);

    return id;
}

int countChats(int id){
    char *err_msg = 0;
    

    sqlite3_stmt *res;
    // printf("%d", id);
    pthread_mutex_lock(&clients_mutex);

    int rc = sqlite3_prepare_v2(db, "SELECT Users.id, Users.name FROM Users JOIN Chats ON Users.id = Chats.chater_id WHERE Chats.user_id = ? AND Chats.new = true", -1, &res, 0);

    if (rc != SQLITE_OK){
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(res);
        pthread_mutex_unlock(&clients_mutex);
        return 0;
    }

    sqlite3_bind_int(res, 1, id);

    int count = 0;

    while(sqlite3_step(res) == SQLITE_ROW){
        count++;
    }

    if(count > 0){
        sqlite3_prepare_v2(db, "UPDATE Chats SET new = 0 WHERE user_id = ? AND new = true", -1, &res, 0);
        sqlite3_bind_int(res, 1, id);
        sqlite3_step(res);
    }
    pthread_mutex_unlock(&clients_mutex);
    
    return count;
}

int countChatsOld(int id){
    char *err_msg = 0;
    

    sqlite3_stmt *res;
    // printf("%d", id);
    pthread_mutex_lock(&clients_mutex);

    int rc = sqlite3_prepare_v2(db, "SELECT Users.id, Users.name FROM Users JOIN Chats ON Users.id = Chats.chater_id WHERE Chats.user_id = ?", -1, &res, 0);

    if (rc != SQLITE_OK){
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(res);
        pthread_mutex_unlock(&clients_mutex);
        return 0;
    }

    sqlite3_bind_int(res, 1, id);

    int count = 0;

    while(sqlite3_step(res) == SQLITE_ROW){
        count++;
    }
    pthread_mutex_unlock(&clients_mutex);
    
    return count;
}

void sendChats(int id, SSL *ssl){
    char *err_msg = 0;
    

    sqlite3_stmt *res;
    // printf("%d", id);
    pthread_mutex_lock(&clients_mutex);

    int rc = sqlite3_prepare_v2(db, "SELECT Users.id, Users.name, Users.image FROM Users JOIN Chats ON Users.id = Chats.chater_id WHERE Chats.user_id = ?", -1, &res, 0);

    if (rc != SQLITE_OK){
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(res);
        pthread_mutex_unlock(&clients_mutex);
        return;
    }

    sqlite3_bind_int(res, 1, id);

    bool flag = false;

    while(sqlite3_step(res) == SQLITE_ROW){
        int id_user = sqlite3_column_int(res, 0);
        char *name = sqlite3_column_text(res, 1);
        int image = sqlite3_column_int(res, 2);
        char result[1024];
        sprintf(result, "%d@%s@%d", id_user, name, image);
        SSL_write(ssl, result, strlen(result));
    }
    pthread_mutex_unlock(&clients_mutex);
}

int searchFriend(char *str){
    str += 2;
    char name[512];
    int id;

    for (int i = 0; str[0] != '@'; i++, str++) name[i] = str[0];
    str++;
    id = atoi(str);

    pthread_mutex_lock(&clients_mutex);
    
    char *err_msg = 0;

    sqlite3_stmt *res;

    int rc = sqlite3_prepare_v2(db, "SELECT * FROM Users WHERE name = ? AND id != ?", -1, &res, 0);

    if (rc == SQLITE_OK){
        sqlite3_bind_text(res, 1, name, -1, SQLITE_STATIC);
        sqlite3_bind_int(res, 2, id);
    }
    else {
        fprintf(stderr, "Failed to insert statement: %s\n", sqlite3_errmsg(db));
    }
    if (sqlite3_step(res) == SQLITE_ROW){
        char *answer = sqlite3_column_text(res, 1);
        int chater = sqlite3_column_int(res, 0);
        rc = sqlite3_prepare_v2(db, "INSERT INTO Chats(user_id, chater_id, new, new_mes) VALUES (?, ?, 0, 0), (?, ?, 1, 0)", -1, &res, 0);
        if (rc == SQLITE_OK){
            sqlite3_bind_int(res, 1, id);
            sqlite3_bind_int(res, 2, chater);
            sqlite3_bind_int(res, 3, chater);
            sqlite3_bind_int(res, 4, id);
        }
        else {
            fprintf(stderr, "Failed to insert statement: %s\n", sqlite3_errmsg(db));
        }
        if (sqlite3_step(res) == SQLITE_DONE){
            pthread_mutex_unlock(&clients_mutex);
            
            return countChatsOld(id);
        }
        else {
            pthread_mutex_unlock(&clients_mutex);
            return 0;
        }  
    }
    else {
        pthread_mutex_unlock(&clients_mutex);
        return 0;
    }
}

char *delete_ykazatel(char *str){
    char *temp = malloc(512 * sizeof(char));
    bool flag = false;
    int c = 0;
    for (int i = 0; i < strlen(str); i++){
        if (str[i] != '@' && !flag){

        }
        else if (str[i] == '@' && !flag){
            flag = true;
        }
        else if (flag){
            temp[c] = str[i];
            c++;
        }
    }
    temp[c] = '\0';
    return temp;
}

char *cutDate(char *str){
    char *temp = malloc(16 * sizeof(char));


    temp[0] = str[8];
    temp[1] = str[9];
    temp[2] = '.';
    temp[3] = str[5];
    temp[4] = str[6];
    temp[5] = ' ';

    for (int i = 6; i < 11; i++) temp[i] = str[i + 5];
    temp[11] = '\0';
    return temp;
}

void showChatMes(char *str, int n, SSL *ssl){
    int id, id_chater;
    bool flag = false;
    char temp_id[128];
    char temp_chater[128];
    char mes[2048];
    int index = 0;
    for (int i = 0; i < strlen(str); i++){
        if (str[i] == '@' && !flag){
            flag = true;
        }
        else if (!flag){
            temp_id[i] = str[i];
        }
        else if (flag){
            temp_chater[index] = str[i];
            index++;
        }
    }
    // write(STDOUT_FILENO, temp_id, strlen(temp_id));
    // write(STDOUT_FILENO, temp_chater, strlen(temp_chater));
    id = atoi(temp_id);
    id_chater = atoi(temp_chater);

    pthread_mutex_lock(&clients_mutex);

    char *err_msg = 0;

    sqlite3_stmt *res;


    if (n > 0){
        int rc = sqlite3_prepare_v2(db, "SELECT Messeges.user_id, mes, insert_date, Messeges.id FROM Messeges JOIN Chats ON Messeges.chat_id = Chats.id WHERE (Chats.user_id = ? AND Chats.chater_id = ?) OR (Chats.user_id = ? AND Chats.chater_id = ?) ORDER BY insert_date", -1, &res, 0);
        

        // SELECT Messeges.user_id, mes, insert_date, id FROM Messeges JOIN Chats ON Messeges.chat_id = Chats.id WHERE (Chats.user_id = 34 AND Chats.chater_id = 35) OR (Chats.user_id = 35 AND Chats.chater_id = 34) ORDER BY insert_date
        sqlite3_bind_int(res, 1, id);
        sqlite3_bind_int(res, 2, id_chater);
        sqlite3_bind_int(res, 3, id_chater);
        sqlite3_bind_int(res, 4, id);

        int count = 0;
        flag = false;

        while(sqlite3_step(res) == SQLITE_ROW){
            count++;
            int id_user = sqlite3_column_int(res, 0);
            char *name = sqlite3_column_text(res, 1);
            char *data = sqlite3_column_text(res, 2);
            int id_mes = sqlite3_column_int(res, 3);
            char *dataObrez = cutDate(data);
            sprintf(mes, "%d@%d@%s@%s", id_mes, id_user, name, dataObrez);
            SSL_write(ssl, mes, strlen(mes));
            // write(STDOUT_FILENO, mes, strlen(mes));
        }
        sqlite3_prepare_v2(db, "UPDATE Chats SET new_mes = false WHERE user_id = ? AND chater_id = ?", -1, &res, 0);
        sqlite3_bind_int(res, 1, id);
        sqlite3_bind_int(res, 2, id_chater);
    

        sqlite3_step(res);
    }

    pthread_mutex_unlock(&clients_mutex);



    // SELECT mes FROM Messeges JOIN Chats ON Messeges.chat_id = Chats.id WHERE (Chats.user_id = 5 AND Chats.chater_id = 6) OR (Chats.user_id = 6 AND Chats.chater_id = 5);

}

void deleteMes(char *str){
    bool flag = false;
    char id_mes[128];
    char temp_user_id[128];
    char *chater;
    int index = 0;
    for (int i = 0; i < strlen(str); i++){
        if (str[i] == '@' && !flag){
            flag = true;
        }
        else if (str[i] == '@' && flag){
            temp_user_id[index] = '\0';
            chater = &str[i + 1];
            break;
        }
        else if (!flag){
            id_mes[i] = str[i];
        }
        else if (flag){
            temp_user_id[index] = str[i];
            index++;
        }
    }
    // write(STDOUT_FILENO, id_mes, strlen(id_mes));
    // write(STDOUT_FILENO, "\n", 1);
    // write(STDOUT_FILENO, temp_user_id, strlen(temp_user_id));
    // write(STDOUT_FILENO, "\n", 1);
    // write(STDOUT_FILENO, chater, strlen(chater));
    // write(STDOUT_FILENO, "\n", 1);
    sqlite3_stmt *res;

    pthread_mutex_lock(&clients_mutex);

    sqlite3_prepare_v2(db, "DELETE FROM Messeges WHERE id = ?", -1, &res, 0);
    sqlite3_bind_int(res, 1, atoi(id_mes));
    sqlite3_step(res);
    sqlite3_prepare_v2(db, "UPDATE Chats SET new_mes = true WHERE (user_id = ? AND chater_id = ?) OR (user_id = ? AND chater_id = ?)", -1, &res, 0);
    sqlite3_bind_int(res, 1, atoi(temp_user_id));
    sqlite3_bind_int(res, 2, atoi(chater));
    sqlite3_bind_int(res, 3, atoi(chater));
    sqlite3_bind_int(res, 4, atoi(temp_user_id));
    sqlite3_step(res);
    
    pthread_mutex_unlock(&clients_mutex);
}

void changeMes(char *str){
    bool flag = false;
    char id_mes[128];
    char mes[128];
    char *chater;
    char temp_id[128];
    char *temp;
    
    int index = 0;
    for (int i = 0; i < strlen(str); i++){
        if (str[i] == '@' && !flag){
            flag = true;
        }
        else if (str[i] == '@' && flag){
            mes[index] = '\0';
            temp = &str[i + 1];
            index = 0;
            flag = false;
            break;
        }
        else if (!flag){
            id_mes[i] = str[i];
        }
        else if (flag){
            mes[index] = str[i];
            index++;
        }
    }
    for (int i = 0; i < strlen(temp); i++){
        if (temp[i] == '@' && !flag){
            temp_id[i] = '\0';
            chater = &temp[i + 1];
            break;
        }
        else temp_id[i] = temp[i];
    }
    sqlite3_stmt *res;

    pthread_mutex_lock(&clients_mutex);

    sqlite3_prepare_v2(db, "UPDATE Messeges SET mes = ? WHERE id = ?", -1, &res, 0);
    sqlite3_bind_text(res, 1, mes, -1, SQLITE_STATIC);
    sqlite3_bind_int(res, 2, atoi(id_mes));
    sqlite3_step(res);
    sqlite3_prepare_v2(db, "UPDATE Chats SET new_mes = true WHERE (user_id = ? AND chater_id = ?) OR (user_id = ? AND chater_id = ?)", -1, &res, 0);
    sqlite3_bind_int(res, 1, atoi(temp_id));
    sqlite3_bind_int(res, 2, atoi(chater));
    sqlite3_bind_int(res, 3, atoi(chater));
    sqlite3_bind_int(res, 4, atoi(temp_id));
    sqlite3_step(res);
    
    pthread_mutex_unlock(&clients_mutex);

}

int countChatMes(char *str){
    int id, id_chater;
    bool flag = false;
    char temp_id[128];
    char temp_chater[128];
    char mes[2048];
    int index = 0;
    for (int i = 0; i < strlen(str); i++){
        if (str[i] == '@' && !flag){
            flag = true;
        }
        else if (!flag){
            temp_id[i] = str[i];
        }
        else if (flag){
            temp_chater[index] = str[i];
            index++;
        }
    }
    id = atoi(temp_id);
    id_chater = atoi(temp_chater);

    pthread_mutex_lock(&clients_mutex);

    char *err_msg = 0;

    sqlite3_stmt *res;

    int rc = sqlite3_prepare_v2(db, "SELECT * FROM Chats WHERE user_id = ? AND chater_id = ? AND new_mes = true", -1, &res, 0);
    sqlite3_bind_int(res, 1, id);
    sqlite3_bind_int(res, 2, id_chater);
        
    
    int count = 0;
    if (sqlite3_step(res) == SQLITE_ROW){
        rc = sqlite3_prepare_v2(db, "SELECT Messeges.user_id, mes FROM Messeges JOIN Chats ON Messeges.chat_id = Chats.id WHERE (Chats.user_id = ? AND Chats.chater_id = ?) OR (Chats.user_id = ? AND Chats.chater_id = ?)", -1, &res, 0);
        sqlite3_bind_int(res, 1, id);
        sqlite3_bind_int(res, 2, id_chater);
        sqlite3_bind_int(res, 3, id_chater);
        sqlite3_bind_int(res, 4, id);

        // write(STDOUT_FILENO, "123", strlen("123"));
        // write(STDOUT_FILENO, "\n", 1);
        

        // while (sqlite3_step(res) == SQLITE_ROW){
        //     int id_user = sqlite3_column_int(res, 0);
        //     char *name = sqlite3_column_text(res, 1);
        //     sprintf(mes, "m@%d@%s", id_user, name);
        //     count++;
        //     // write(STDOUT_FILENO, "123", strlen("123"));
        //     // write(STDOUT_FILENO, mes, strlen(mes));
        //     SSL_write(ssl, mes, strlen(mes));
        // }

        while(sqlite3_step(res) == SQLITE_ROW){
            count++;
            // int id_user = sqlite3_column_int(res, 0);
            // char *name = sqlite3_column_text(res, 1);
            // if (!flag){
            //     sprintf(mes, "m@%d@%s@", id_user, name);
            //     // write(STDOUT_FILENO, result, strlen(result));
                // write(STDOUT_FILENO, "\n", 1);
            //     flag = true;
            // }
            // else{
            //     sprintf(mes, "%s%d@%s@", mes, id_user, name);
            //     //write(STDOUT_FILENO, result, strlen(result));
            //     //write(STDOUT_FILENO, "\n", 1);
            // }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    


    // SELECT mes FROM Messeges JOIN Chats ON Messeges.chat_id = Chats.id WHERE (Chats.user_id = 5 AND Chats.chater_id = 6) OR (Chats.user_id = 6 AND Chats.chater_id = 5);
    return count;
}

int countChatMesOld(char *str){
    // write(STDOUT_FILENO, str, strlen(str));
    // write(STDOUT_FILENO, "\n", strlen("\n"));
    int id, id_chater;
    bool flag = false;
    char temp_id[128];
    char temp_chater[128];
    char mes[2048];
    int index = 0;
    for (int i = 0; i < strlen(str); i++){
        if (str[i] == '@' && !flag){
            flag = true;
        }
        else if (!flag){
            temp_id[i] = str[i];
        }
        else if (flag){
            temp_chater[index] = str[i];
            index++;
        }
    }
    // write(STDOUT_FILENO, temp_id, strlen(temp_id));
    // write(STDOUT_FILENO, "\n", strlen("\n"));
    // write(STDOUT_FILENO, temp_chater, strlen(temp_chater));
    // write(STDOUT_FILENO, "\n", strlen("\n"));
    id = atoi(temp_id);
    id_chater = atoi(temp_chater);

    pthread_mutex_lock(&clients_mutex);

    char *err_msg = 0;

    sqlite3_stmt *res;
        
    int count = 0;
        int rc = sqlite3_prepare_v2(db, "SELECT Messeges.user_id, mes FROM Messeges JOIN Chats ON Messeges.chat_id = Chats.id WHERE (Chats.user_id = ? AND Chats.chater_id = ?) OR (Chats.user_id = ? AND Chats.chater_id = ?)", -1, &res, 0);
        sqlite3_bind_int(res, 1, id);
        sqlite3_bind_int(res, 2, id_chater);
        sqlite3_bind_int(res, 3, id_chater);
        sqlite3_bind_int(res, 4, id);

        // write(STDOUT_FILENO, "123", strlen("123"));
        

        // while (sqlite3_step(res) == SQLITE_ROW){
        //     int id_user = sqlite3_column_int(res, 0);
        //     char *name = sqlite3_column_text(res, 1);
        //     sprintf(mes, "m@%d@%s", id_user, name);
        //     count++;
        //     // write(STDOUT_FILENO, "123", strlen("123"));
        //     // write(STDOUT_FILENO, mes, strlen(mes));
        //     SSL_write(ssl, mes, strlen(mes));
        // }

        while(sqlite3_step(res) == SQLITE_ROW){
            count++;
            // int id_user = sqlite3_column_int(res, 0);
            // char *name = sqlite3_column_text(res, 1);
            // if (!flag){
            //     sprintf(mes, "m@%d@%s@", id_user, name);
            //     // write(STDOUT_FILENO, result, strlen(result));
            //     // write(STDOUT_FILENO, "\n", 1);
            //     flag = true;
            // }
            // else{
            //     sprintf(mes, "%s%d@%s@", mes, id_user, name);
            //     //write(STDOUT_FILENO, result, strlen(result));
            //     //write(STDOUT_FILENO, "\n", 1);
            // }
        }

        pthread_mutex_unlock(&clients_mutex);
    // SELECT mes FROM Messeges JOIN Chats ON Messeges.chat_id = Chats.id WHERE (Chats.user_id = 5 AND Chats.chater_id = 6) OR (Chats.user_id = 6 AND Chats.chater_id = 5);
    return count;
}

void insertMes(char *str, SSL *ssl){
    // write(STDOUT_FILENO, str, strlen(str));
    // write(STDOUT_FILENO, "\n", strlen("\n"));
    str = delete_ykazatel(str);
    char temp_id[128];
    char temp_chater[128];
    char *mes;
    bool flag = false;
    int index = 0;

    for (int i = 0; i < strlen(str); i++){
        if (str[i] == '@' && !flag){
            temp_id[i] = '\0';
            flag = true;
        }
        else if (!flag) temp_id[i] = str[i]; 
        else if (str[i] == '@' && flag) {
            temp_chater[index] = '\0';
            mes = &str[i + 1];
            break;
        }
        else if (flag){
            temp_chater[index] = str[i];
            index++;
        }
    }
    // write(STDOUT_FILENO, temp_id, strlen(temp_id));
    // write(STDOUT_FILENO, "\n", strlen("\n"));
    // write(STDOUT_FILENO, temp_chater, strlen(temp_chater));
    // write(STDOUT_FILENO, "\n", strlen("\n"));
    int id = atoi(temp_id);
    int chater = atoi(temp_chater);

    pthread_mutex_lock(&clients_mutex);

    char *err_msg = 0;

    sqlite3_stmt *res;

    int rc = sqlite3_prepare_v2(db, "INSERT INTO Messeges (chat_id, user_id, mes, insert_date) SELECT id, ?, ?, DATETIME('now', 'localtime') FROM Chats WHERE (user_id = ? AND chater_id = ?)", -1, &res, 0);
    sqlite3_bind_int(res, 1, id);
    sqlite3_bind_text(res, 2, mes, -1, SQLITE_STATIC);
    sqlite3_bind_int(res, 3, id);
    sqlite3_bind_int(res, 4, chater);
    if (sqlite3_step(res) == SQLITE_DONE){
        // SSL_write(ssl, "yes", strlen("yes"));
    }
    // else SSL_write(ssl, "be", strlen("be"));

    sqlite3_prepare_v2(db, "UPDATE Chats SET new_mes = true WHERE (user_id = ? AND chater_id = ?) OR (user_id = ? AND chater_id = ?)", -1, &res, 0);
    sqlite3_bind_int(res, 1, chater);
    sqlite3_bind_int(res, 2, id);
    sqlite3_bind_int(res, 3, id);
    sqlite3_bind_int(res, 4, chater);
    sqlite3_step(res);
    
    pthread_mutex_unlock(&clients_mutex);

    //INSERT INTO Messeges (chat_id, user_id, mes) SELECT id, 5, 'hi' FROM Chats WHERE (user_id = 5 AND chater_id = 16);

}

int idBeginGame(){
    pthread_mutex_lock(&clients_mutex);

    char *err_msg = 0;

    sqlite3_stmt *res;

    int rc = sqlite3_prepare_v2(db, "INSERT INTO game DEFAULT VALUES", -1, &res, 0);
    if (sqlite3_step(res) == SQLITE_DONE){
        rc = sqlite3_prepare_v2(db, "SELECT * FROM game ORDER BY id DESC LIMIT 1", -1, &res, 0);
        if (sqlite3_step(res) == SQLITE_ROW){
            pthread_mutex_unlock(&clients_mutex);
            return sqlite3_column_int(res, 0);
        }
        else {
            pthread_mutex_unlock(&clients_mutex);
            return 0;
        }
    }
    else {
        pthread_mutex_unlock(&clients_mutex);
        return 0;
    }
}
void insertIntoLobby(char *str, int id){
    pthread_mutex_lock(&clients_mutex);

    sqlite3_stmt *res;

    int rc = sqlite3_prepare_v2(db, "INSERT INTO lobby (id, name) SELECT ?, u.name FROM Users u WHERE u.id = ?", -1, &res, 0);

    sqlite3_bind_int(res, 1, id);
    sqlite3_bind_int(res, 2, atoi(str));

    sqlite3_step(res); 
    free(str);
    pthread_mutex_unlock(&clients_mutex);
}

void deleteFromLobby(char *str){
    int count = 0;

    pthread_mutex_lock(&clients_mutex);

    sqlite3_stmt *res;

    sqlite3_prepare_v2(db, "SELECT * FROM lobby WHERE name = (SELECT name FROM Users WHERE id = ?)", -1, &res, 0);

    // SELECT lobby.* FROM lobby JOIN Users ON lobby.name = Users.name WHERE Users.id = ?;
    sqlite3_bind_int(res, 1, atoi(str));
    sqlite3_step(res);
    int id = sqlite3_column_int(res, 0);
    sqlite3_finalize(res);

    sqlite3_prepare_v2(db, "SELECT * FROM lobby WHERE id = ?", -1, &res, 0);

    // SELECT lobby.* FROM lobby JOIN Users ON lobby.name = Users.name WHERE Users.id = ?;
    sqlite3_bind_int(res, 1, id);

    while (sqlite3_step(res) == SQLITE_ROW) count++;

    // fg[15];
    // sprintf(fg, "|%d|", count);
    // write(STDOUT_FILENO, fg, strlen(fg));

    if (count == 1){
        sqlite3_prepare_v2(db, "DELETE FROM invited WHERE id_game = ?", -1, &res, 0);
        // SELECT lobby.* FROM lobby JOIN Users ON lobby.name = Users.name WHERE Users.id = ?;
        sqlite3_bind_int(res, 1, id);
        sqlite3_step(res);
        
    }
    sqlite3_finalize(res);

    sqlite3_prepare_v2(db, "DELETE FROM lobby WHERE name = (SELECT name FROM Users WHERE id = ?)", -1, &res, 0);

    sqlite3_bind_int(res, 1, atoi(str));

    sqlite3_step(res); 
    free(str);
    pthread_mutex_unlock(&clients_mutex);
}

void invitePlayer(char *str){
    char *game = delete_ykazatel(str);
    char name[64];
    for (int i = 0; i < strlen(str); i++){
        if (str[i] == '@'){
            name[i] = '\0';
            break;
        }
        else name[i] = str[i];
    }
    pthread_mutex_lock(&clients_mutex);

    sqlite3_stmt *res;

    int count = 0;

    sqlite3_prepare_v2(db, "SELECT * FROM lobby WHERE id = ?", -1, &res, 0);
    sqlite3_bind_int(res, 1, atoi(game));
    while(sqlite3_step(res) == SQLITE_ROW) count++;


    if (count >= 4){

    }
    else{
        int rc = sqlite3_prepare_v2(db, "INSERT INTO invited (id_game, name) SELECT ?, ? WHERE NOT EXISTS (SELECT * FROM lobby WHERE name = ? AND id = ?) AND NOT EXISTS (SELECT * FROM invited WHERE id_game = ? AND name = ?);", -1, &res, 0);

        sqlite3_bind_int(res, 1, atoi(game));
        sqlite3_bind_text(res, 2, name,  -1, SQLITE_STATIC);
        sqlite3_bind_text(res, 3, name,  -1, SQLITE_STATIC);
        sqlite3_bind_int(res, 4, atoi(game));
        sqlite3_bind_text(res, 6, name,  -1, SQLITE_STATIC);
        sqlite3_bind_int(res, 5, atoi(game));


        sqlite3_step(res); 
    }
    free(str);
    pthread_mutex_unlock(&clients_mutex);

}

int countPlayers(int id){
    int count = 0;
    pthread_mutex_lock(&clients_mutex);

    char *err_msg = 0;

    sqlite3_stmt *res;

    int rc = sqlite3_prepare_v2(db, "SELECT * FROM lobby WHERE id  = ?", -1, &res, 0);
    sqlite3_bind_int(res, 1, id);
    while (sqlite3_step(res) == SQLITE_ROW) count++;
    pthread_mutex_unlock(&clients_mutex);
    return count;
        
}

void showPlayers(int id, SSL *ssl){
    pthread_mutex_lock(&clients_mutex);

    char *err_msg = 0;

    sqlite3_stmt *res;
    sqlite3_stmt *res_1;
    // char temp[50];
    // sprintf(temp, "|%d|\n", id);
    // write(STDOUT_FILENO, temp, strlen(temp));


    int rc = sqlite3_prepare_v2(db, "SELECT * FROM lobby WHERE id  = ?", -1, &res, 0);
    sqlite3_bind_int(res, 1, id);
    while (sqlite3_step(res) == SQLITE_ROW){
        char *result = sqlite3_column_text(res, 1);
        sqlite3_prepare_v2(db, "SELECT image FROM Users WHERE name  = ?", -1, &res_1, 0);
        sqlite3_bind_text(res_1, 1, result, -1, SQLITE_STATIC);
        sqlite3_step(res_1);
        int image = sqlite3_column_int(res_1, 0);

        char temp[128];
        sprintf(temp, "%s@%d", result, image);
        // write(STDOUT_FILENO, result, strlen(result));
        SSL_write(ssl, temp, strlen(temp));
    }
    pthread_mutex_unlock(&clients_mutex);

}

int countInvites(int id){
    int count = 0;
    pthread_mutex_lock(&clients_mutex);

    char *err_msg = 0;

    sqlite3_stmt *res;

    // char temp[50];
    // sprintf(temp, "|%d|", id);
    // write(STDOUT_FILENO, temp, strlen(temp));

    int rc = sqlite3_prepare_v2(db, "SELECT * FROM invited JOIN Users ON invited.name = Users.name WHERE Users.id = ?", -1, &res, 0);
    sqlite3_bind_int(res, 1, id);
    while (sqlite3_step(res) == SQLITE_ROW) count++;
    pthread_mutex_unlock(&clients_mutex);
    return count;
}
// SELECT * FROM invited JOIN Users ON invited.name = Users.name WHERE Users.id = 6;



void showLobby(int id, SSL *ssl){
    pthread_mutex_lock(&clients_mutex);

    char *err_msg = 0;

    sqlite3_stmt *res;

    sqlite3_stmt *res_1;

    int rc = sqlite3_prepare_v2(db, "SELECT * FROM invited JOIN Users ON invited.name = Users.name WHERE Users.id = ?", -1, &res, 0);
    sqlite3_bind_int(res, 1, id);
    while (sqlite3_step(res) == SQLITE_ROW){
        int n = sqlite3_column_int(res, 0);
        sqlite3_prepare_v2(db, "SELECT * FROM lobby WHERE id = ?", -1, &res_1, 0);
        sqlite3_bind_int(res_1, 1, n);
        char str[1024];
        bool flag = false;
        while(sqlite3_step(res_1) == SQLITE_ROW){
            char *name = sqlite3_column_text(res_1, 1);
            if (!flag){
                sprintf(str, "%d@%s", n, name);
                flag = true;
            }
            else sprintf(str, "%s@%s", str, name);
        }
        SSL_write(ssl, str, strlen(str));
    }
    pthread_mutex_unlock(&clients_mutex);
}
void insertInLobby(char *str){
    char id_game[32];
    for (int i = 0; i < strlen(str); i++){
        if (str[i] == '@'){
            id_game[i] = '\0';
            break;
        }
        else id_game[i] = str[i];
    }
    char *id = delete_ykazatel(str);
    pthread_mutex_lock(&clients_mutex);

    char *err_msg = 0;

    sqlite3_stmt *res;


    int rc = sqlite3_prepare_v2(db, "INSERT INTO lobby (id, name) VALUES (?, (SELECT name FROM Users WHERE id = ?));", -1, &res, 0);
    sqlite3_bind_int(res, 1, atoi(id_game));
    sqlite3_bind_int(res, 2, atoi(id));
    sqlite3_step(res);

    sqlite3_prepare_v2(db, "DELETE FROM invited WHERE id_game = ? AND name = (SELECT name FROM Users WHERE id = ?);", -1, &res, 0);
    sqlite3_bind_int(res, 1, atoi(id_game));
    sqlite3_bind_int(res, 2, atoi(id));
    sqlite3_step(res);

    sqlite3_prepare_v2(db, "SELECT * FROM lobby WHERE id = ?", -1, &res, 0);
    sqlite3_bind_int(res, 1, atoi(id_game));
    
    int count = 0;
    while (sqlite3_step(res) == SQLITE_ROW) count++;

    if (count >= 3){
        sqlite3_prepare_v2(db, "DELETE FROM invited WHERE id_game = ?", -1, &res, 0);
        sqlite3_bind_int(res, 1, atoi(id_game));
        sqlite3_step(res);
    }

    pthread_mutex_unlock(&clients_mutex);

}

bool loadMap(char *str){
    int game = atoi(str);
    free(str);

    pthread_mutex_lock(&clients_mutex);

    char *err_msg = 0;

    sqlite3_stmt *res;
    sqlite3_stmt *res_1;
    sqlite3_stmt *res_2;
    sqlite3_stmt *res_3;
    
    int rc = sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM lobby WHERE id = ?", -1, &res, 0);
    sqlite3_bind_int(res, 1, game);
    sqlite3_step(res);
    int count = sqlite3_column_int(res, 0);

    

    if (count == 1){
        pthread_mutex_unlock(&clients_mutex);
        return false;
    }
    int count_1 = 2;
    bool flag = false;

    sqlite3_prepare_v2(db, "SELECT name FROM lobby WHERE id = ?", -1, &res, 0);
    sqlite3_bind_int(res, 1, game);
    while (sqlite3_step(res) == SQLITE_ROW){
        if (!flag){
            sqlite3_prepare_v2(db, "INSERT INTO player_info (id, pl_1, count_pl, turn, money_player_1) VALUES (?, ?, ?, ?, ?)", -1, &res_1, 0);
            sqlite3_bind_int(res_1, 1, game);
            sqlite3_bind_text(res_1, 2, sqlite3_column_text(res, 0),  -1, SQLITE_STATIC);
            sqlite3_bind_int(res_1, 3, count);
            sqlite3_prepare_v2(db, "SELECT id FROM Users WHERE name = ?", -1, &res_2, 0);
            sqlite3_bind_text(res_2, 1, sqlite3_column_text(res, 0), -1, SQLITE_STATIC);
            sqlite3_step(res_2);
            sqlite3_bind_int(res_1, 4, sqlite3_column_int(res_2, 0));
            sqlite3_bind_int(res_1, 5, 1500);
            sqlite3_step(res_1);
            flag = true;
        }
        else{
            char temp[1024];
            sprintf(temp, "UPDATE player_info SET pl_%d = '%s', money_player_%d = %d WHERE id = %d", count_1, sqlite3_column_text(res, 0), count_1, 1500, game);
            int rc = sqlite3_prepare_v2(db, temp, -1, &res_1, 0);
            if (rc == SQLITE_OK){
                sqlite3_step(res_1);
            }
            else fprintf(stderr, "Failed to insert statement: %s\n", sqlite3_errmsg(db));
            sqlite3_step(res_1);
            count_1++;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    return true;
}

bool checkMap(int game){
    pthread_mutex_lock(&clients_mutex);

    char *err_msg = 0;

    sqlite3_stmt *res;

    int rc = sqlite3_prepare_v2(db, "SELECT * FROM player_info WHERE id = ?", -1, &res, 0);
    sqlite3_bind_int(res, 1, game);
    if (sqlite3_step(res) == SQLITE_ROW){
        pthread_mutex_unlock(&clients_mutex);
        return true;
    }

    pthread_mutex_unlock(&clients_mutex);
    return false;
}

void beginGame(int game, SSL *ssl){
    pthread_mutex_lock(&clients_mutex);

    char *err_msg = 0;

    sqlite3_stmt *res;

    int rc = sqlite3_prepare_v2(db, "SELECT count_pl, pl_1, pl_2, pl_3, pl_4 FROM player_info WHERE id = ?", -1, &res, 0);
    sqlite3_bind_int(res, 1, game);
    if (sqlite3_step(res) == SQLITE_ROW){
        char temp[4096];
        
        sprintf(temp, "g@h@%d@%s@%s@%s@%s", sqlite3_column_int(res, 0), sqlite3_column_text(res, 1), sqlite3_column_text(res, 2), sqlite3_column_text(res, 3), sqlite3_column_text(res, 4));
        SSL_write(ssl, temp, strlen(temp));
    }

    // char temp[50];
    // sprintf(temp, "|%s|",);
    // write(STDOUT_FILENO, temp, strlen(temp));
    pthread_mutex_unlock(&clients_mutex);
}

bool checkTurn(char *str, int id){

    char temp[16];
    for (int i = 0; i < strlen(str); i++){
        if (str[i] == '@'){
            temp[i] = '\0';
            break;
        }
        else temp[i] = str[i];
    }
    int game = atoi(temp);
    

    pthread_mutex_lock(&clients_mutex);

    char *err_msg = 0;

    sqlite3_stmt *res;



    char temp_1[216];
    sprintf(temp_1, "SELECT * FROM player_info WHERE id = ? and turn = ?", id);

    sqlite3_prepare_v2(db, temp_1, -1, &res, 0);
    sqlite3_bind_int(res, 1, game);
    sqlite3_bind_int(res, 2, id);
    if (sqlite3_step(res) == SQLITE_ROW){
        pthread_mutex_unlock(&clients_mutex);
        return true;
    } 


    // char temp[50];
    // sprintf(temp, "|%s|%d|\n", str, count);
    // write(STDOUT_FILENO, temp, strlen(temp));
    pthread_mutex_unlock(&clients_mutex);
    return false;
}
int rollDice(){
    srand(time(0));
    int i = 1 + rand() % 6;
    return i;
}

int randPos(){
    srand(time(0));
    int i = 1 + rand() % 4;
    return i;
}

int rand_num()
{
    int num[] = {50, 100, 150, 200, 250, 300, 350, 400, 450, 500};
    srand(time(0));
    int i = rand() % 10;
    return num[i];
}



void makeTurn(char *str, int id){
    char temp[16];
    for (int i = 0; i < strlen(str); i++){
        if (str[i] == '@'){
            temp[i] = '\0';
            break;
        }
        else temp[i] = str[i];
    }
    int game = atoi(temp);
    
    int next_pos;

    pthread_mutex_lock(&clients_mutex);

    char *err_msg = 0;

    sqlite3_stmt *res;

    char pos_players[256];
    // SELECT CASE WHEN pl_1 = u.name THEN pos_pl_1 WHEN pl_2 = u.name THEN pos_pl_2 WHEN pl_3 = u.name THEN pos_pl_3 WHEN pl_4 = u.name THEN pos_pl_4 END AS pos_pl FROM player_info JOIN Users u ON player_info.turn = u.id WHERE u.id = 5 AND player_info.id = 311

    sqlite3_prepare_v2(db, "SELECT CASE WHEN pl_1 = u.name THEN pos_pl_1 WHEN pl_2 = u.name THEN pos_pl_2 WHEN pl_3 = u.name THEN pos_pl_3 WHEN pl_4 = u.name THEN pos_pl_4 END AS pos_pl FROM player_info JOIN Users u ON player_info.turn = u.id WHERE u.id = ? AND player_info.id = ?", -1, &res, 0);
    sqlite3_bind_int(res, 1, id);
    sqlite3_bind_int(res, 2, game);

    if (sqlite3_step(res) == SQLITE_ROW){
        int dice = rollDice();
        int pos = sqlite3_column_int(res, 0);
        next_pos = pos + dice;
        bool flag_200 = false;
        if (next_pos >= 36){
            next_pos = next_pos - 36;
            flag_200 = true;
        }
        else if (next_pos == 9) next_pos = 27;
        else if (next_pos == 22){ //DELETE FROM map_info WHERE id = 549 AND player = 6 AND key != 1
            sqlite3_prepare_v2(db, "DELETE FROM map_info WHERE id = ? AND player = ? AND key != 1", -1, &res, 0);
            sqlite3_bind_int(res, 1, game);
            sqlite3_bind_int(res, 2, id);
            sqlite3_step(res);
        }
        // switch (randPos()){
        //     case 1:
        //         x = 335;
        //         y = 20;
        //         break;
        //     case 2:
        //         x = 335;
        //         y = -10;
        //         break;
        //     case 3:
        //         x = 375;
        //         y = -10;
        //         break;
        //     case 4:
        //         x = 375;
        //         y = 20;
        //         break;
        // }
        // for (int i = 0; i <= next_pos; i++){
        //     if (i == 1 || i == 9) x += 81;
        //     else if (i > 1 && i < 9) x += 54;
        //     else if (i == 10 || i == 18) y += 81;
        //     else if (i > 10 && i < 18) y += 54;         
        //     else if (i == 19 || i == 27) x -= 81;
        //     else if (i > 19 && i < 27) x -=54;      
        //     else if (i == 28) y -= 81;
        //     else if (i > 28 && i < 36) y -= 54;
        // }UPDATE player_info SET pos_pl_1 = CASE WHEN pl_1 = (SELECT name FROM Users WHERE id = 5) THEN 6 ELSE pos_pl_2 END, pos_pl_3 = CASE WHEN pl_3 = (SELECT name FROM Users WHERE id = 5) THEN 6 ELSE pos_pl_3 END, pos_pl_4 = CASE WHEN pl_4 = (SELECT name FROM Users WHERE id = 5) THEN 6 ELSE pos_pl_4 END WHERE id = 482
        sqlite3_prepare_v2(db, "UPDATE player_info SET pos_pl_1 = CASE WHEN pl_1 = (SELECT name FROM Users WHERE id = ?) THEN ? ELSE pos_pl_1 END, pos_pl_2 = CASE WHEN pl_2 = (SELECT name FROM Users WHERE id = ?) THEN ? ELSE pos_pl_2 END, pos_pl_3 = CASE WHEN pl_3 = (SELECT name FROM Users WHERE id = ?) THEN ? ELSE pos_pl_3 END, pos_pl_4 = CASE WHEN pl_4 = (SELECT name FROM Users WHERE id = ?) THEN ? ELSE pos_pl_4 END WHERE id = ?", -1, &res, 0);
        sqlite3_bind_int(res, 1, id);
        sqlite3_bind_int(res, 2, next_pos);
        sqlite3_bind_int(res, 3, id);
        sqlite3_bind_int(res, 4, next_pos);
        sqlite3_bind_int(res, 5, id);
        sqlite3_bind_int(res, 6, next_pos);
        sqlite3_bind_int(res, 7, id);
        sqlite3_bind_int(res, 8, next_pos);
        sqlite3_bind_int(res, 9, game);
        sqlite3_step(res);

        sqlite3_prepare_v2(db, "SELECT CASE WHEN pl_1 = u.name THEN money_player_1 WHEN pl_2 = u.name THEN money_player_2 WHEN pl_3 = u.name THEN money_player_3 WHEN pl_4 = u.name THEN money_player_4 END AS money_player FROM player_info JOIN Users u ON player_info.turn = u.id WHERE u.id = ? AND player_info.id = ?", -1, &res, 0);
        sqlite3_bind_int(res, 1, id);
        sqlite3_bind_int(res, 2, game);
        sqlite3_step(res);

        char bonus[1028];
        sprintf(bonus, "n@");

        int money = sqlite3_column_int(res, 0);
        if (flag_200) money += 200;
        
        if (next_pos == 3 || next_pos == 29){
            if (money > 150) money -= 150;
            else money = 0;
        }
        else if (next_pos == 12 || next_pos == 20 || next_pos == 33){
            money += rand_num();
        }
        else if(next_pos == 14){
            if (money  <= 400) money = 0;
            else money - 400;
        }
        else if (next_pos == 18){
            money += 150;
            
        }
        else if (next_pos == 24 || next_pos == 7 || next_pos == 16){
            int n = rollDice();
            switch (n){
                case 1:
                    if (money < 50) money = 0;
                    else money -= 50;
                    sprintf(bonus, "s@1@");
                    break;
                case 2:
                    if (money < 100) money = 0;
                    else money -= 100;
                    sprintf(bonus, "s@2@");
                    break;
                case 3:
                    if (money < 150) money = 0;
                    else money -= 150;
                    sprintf(bonus, "s@3@");
                    break;
                case 4:
                    money += 50;
                    sprintf(bonus, "s@4@");
                    break;
                case 5:
                    money -= 100;
                    sprintf(bonus, "s@5@");
                    break;
                case 6:
                    money += 150;
                    sprintf(bonus, "s@6@");
                    break;
            }
        }
        else if (next_pos == 1 || next_pos == 2 || (next_pos >= 4 && next_pos <= 6) || next_pos == 8 || next_pos == 10
            || next_pos == 11 || next_pos == 13 || next_pos == 15 || next_pos == 17 || next_pos == 19 || next_pos == 21
            || next_pos == 23 || next_pos == 25 || next_pos == 26 || next_pos == 28 || (next_pos >= 30 && next_pos <= 32)
            || next_pos == 34 || next_pos == 35){
            // SELECT * FROM map_info WHERE id = ? AND pole = ? AND player != ?
            sqlite3_prepare_v2(db, "SELECT * FROM map_info WHERE id = ? AND pole = ? AND player == ?", -1, &res, 0);
            sqlite3_bind_int(res, 1, game);
            sqlite3_bind_int(res, 2, next_pos);
            sqlite3_bind_int(res, 3, id);
            if(sqlite3_step(res) == SQLITE_ROW){
            }
            else{
                sqlite3_prepare_v2(db, "SELECT * FROM map_info WHERE id = ? AND pole = ? AND player != ?", -1, &res, 0);
                sqlite3_bind_int(res, 1, game);
                sqlite3_bind_int(res, 2, next_pos);
                sqlite3_bind_int(res, 3, id);
                if (sqlite3_step(res) == SQLITE_ROW){
                    int m = 0;
                    if (next_pos >= 1 && next_pos <= 4){
                        money -= 120;
                        m = 120;
                    }
                    else if (next_pos >= 5 && next_pos <= 8){
                        money -= 140;
                        m = 140;
                    }
                    else if (next_pos >= 10 && next_pos <= 13){
                        money -= 160;
                        m = 160;
                    }
                    else if (next_pos >= 15 && next_pos <= 17){
                        money -= 200;
                        m = 200;
                    }
                    else if (next_pos >= 19 && next_pos <= 21){
                        money -= 40;
                        m = 40;
                    }
                    else if (next_pos >= 23 && next_pos <= 26){
                        money -= 60;
                        m = 60;
                    }
                    else if (next_pos >= 28 && next_pos <= 31){
                        money -= 80;
                        m = 80;
                    }
                    else if (next_pos >= 32 && next_pos <= 35){
                        money -= 100;
                        m = 100;
                    }
                    if (money <= 0){
                        money = 0;
                    }
                    int p = sqlite3_column_int(res, 2);
                    sqlite3_prepare_v2(db, "UPDATE player_info SET money_player_1 = CASE WHEN pl_1 = (SELECT name FROM Users WHERE id = ?) THEN money_player_1 + ? ELSE money_player_1 END, money_player_2 = CASE WHEN pl_2 = (SELECT name FROM Users WHERE id = ?) THEN money_player_2 + ? ELSE money_player_2 END, money_player_3 = CASE WHEN pl_3 = (SELECT name FROM Users WHERE id = ?) THEN money_player_3 + ? ELSE money_player_3 END, money_player_4 = CASE WHEN pl_4 = (SELECT name FROM Users WHERE id = ?) THEN money_player_4 + ? ELSE money_player_4 END WHERE id = ?", -1, &res, 0);
                    sqlite3_bind_int(res, 1, p);
                    sqlite3_bind_int(res, 2, m);
                    sqlite3_bind_int(res, 3, p);
                    sqlite3_bind_int(res, 4, m);
                    sqlite3_bind_int(res, 5, p);
                    sqlite3_bind_int(res, 6, m);
                    sqlite3_bind_int(res, 7, p);
                    sqlite3_bind_int(res, 8, m);
                    sqlite3_bind_int(res, 9, game);
                    sqlite3_step(res);
                }
                else {
                    sprintf(bonus, "b@%d@", next_pos);
                }
            }
        } 


        sqlite3_prepare_v2(db, "UPDATE player_info SET money_player_1 = CASE WHEN pl_1 = (SELECT name FROM Users WHERE id = ?) THEN ? ELSE money_player_1 END, money_player_2 = CASE WHEN pl_2 = (SELECT name FROM Users WHERE id = ?) THEN ? ELSE money_player_2 END, money_player_3 = CASE WHEN pl_3 = (SELECT name FROM Users WHERE id = ?) THEN ? ELSE money_player_3 END, money_player_4 = CASE WHEN pl_4 = (SELECT name FROM Users WHERE id = ?) THEN ? ELSE money_player_4 END WHERE id = ?", -1, &res, 0);
        sqlite3_bind_int(res, 1, id);
        sqlite3_bind_int(res, 2, money);
        sqlite3_bind_int(res, 3, id);
        sqlite3_bind_int(res, 4, money);
        sqlite3_bind_int(res, 5, id);
        sqlite3_bind_int(res, 6, money);
        sqlite3_bind_int(res, 7, id);
        sqlite3_bind_int(res, 8, money);
        sqlite3_bind_int(res, 9, game);
        sqlite3_step(res);


        sqlite3_prepare_v2(db, "SELECT count_pl, pos_pl_1, pos_pl_2, pos_pl_3, pos_pl_4, money_player_1, money_player_2, money_player_3, money_player_4 FROM player_info where id = ?", -1, &res, 0);
        sqlite3_bind_int(res, 1, game);
        sqlite3_step(res);

        sprintf(pos_players, "g@m@u@%d@%d@%d@%d@%d@", sqlite3_column_int(res, 0), sqlite3_column_int(res, 1), sqlite3_column_int(res, 2), sqlite3_column_int(res, 3), sqlite3_column_int(res, 4));
        char dicebuff[256];
        sprintf(dicebuff, "d@%d@", dice);
        char moneybuff[512];
        sprintf(moneybuff, "m@%d@%d@%d@%d@", sqlite3_column_int(res, 5), sqlite3_column_int(res, 6), sqlite3_column_int(res, 7), sqlite3_column_int(res, 8));
        sqlite3_prepare_v2(db, "INSERT INTO game_log(id_game, pos_players, cube, money, bonus) VALUES(?, ?, ?, ?, ?)", -1, &res, 0);
        sqlite3_bind_int(res, 1, game);
        sqlite3_bind_text(res, 2, pos_players, -1, SQLITE_STATIC);
        sqlite3_bind_text(res, 3, dicebuff, -1, SQLITE_STATIC);
        sqlite3_bind_text(res, 4, moneybuff, -1, SQLITE_STATIC);
        sqlite3_bind_text(res, 5, bonus, -1, SQLITE_STATIC);
        sqlite3_step(res);
    }
    sqlite3_prepare_v2(db, "SELECT count_pl FROM player_info WHERE id = ?", -1, &res, 0); 
    sqlite3_bind_int(res, 1, game);
    sqlite3_step(res);
    int count_pl = sqlite3_column_int(res, 0);
    char *new_turner;
    int new_turn; 


    if (next_pos == 1 || next_pos == 2 || (next_pos >= 4 && next_pos <= 6) || next_pos == 8 || next_pos == 10
        || next_pos == 11 || next_pos == 13 || next_pos == 15 || next_pos == 17 || next_pos == 19 || next_pos == 21
        || next_pos == 23 || next_pos == 25 || next_pos == 26 || next_pos == 28 || (next_pos >= 30 && next_pos <= 32)
        || next_pos == 34 || next_pos == 35){
            sqlite3_prepare_v2(db, "SELECT * FROM map_info WHERE id = ? AND pole = ?", -1, &res, 0);
            sqlite3_bind_int(res, 1, game);
            sqlite3_bind_int(res, 2, next_pos);
            sqlite3_bind_int(res, 3, id);
            if (sqlite3_step(res) == SQLITE_ROW){
                switch(count_pl){
                    case 2:
                        sqlite3_prepare_v2(db, "SELECT CASE WHEN pl_1 = u.name THEN pl_2 WHEN pl_2 = u.name THEN pl_1 WHEN pl_3 = u.name THEN pos_pl_3 WHEN pl_4 = u.name THEN pos_pl_4 END AS pos_pl FROM player_info JOIN Users u ON player_info.turn = u.id WHERE u.id = ? AND player_info.id = ?", -1, &res, 0);
                        sqlite3_bind_int(res, 1, id);
                        sqlite3_bind_int(res, 2, game);
                        sqlite3_step(res);
                        new_turner = sqlite3_column_text(res, 0);
                        sqlite3_prepare_v2(db, "SELECT id FROM Users WHERE name = ?", -1, &res, 0);
                        sqlite3_bind_text(res, 1, new_turner, -1, SQLITE_STATIC);
                        sqlite3_step(res);
                        new_turn = sqlite3_column_int(res, 0);
                        sqlite3_prepare_v2(db, "UPDATE player_info SET turn = ? WHERE id = ?;", -1, &res, 0);
                        sqlite3_bind_int(res, 1, new_turn);
                        sqlite3_bind_int(res, 2, game);
                        break;
                    case 3:
                        sqlite3_prepare_v2(db, "SELECT CASE WHEN pl_1 = u.name THEN pl_2 WHEN pl_2 = u.name THEN pl_3 WHEN pl_3 = u.name THEN pl_1 WHEN pl_4 = u.name THEN pos_pl_4 END AS pos_pl FROM player_info JOIN Users u ON player_info.turn = u.id WHERE u.id = ? AND player_info.id = ?", -1, &res, 0);
                        sqlite3_bind_int(res, 1, id);
                        sqlite3_bind_int(res, 2, game);
                        sqlite3_step(res);
                        new_turner = sqlite3_column_text(res, 0);
                        sqlite3_prepare_v2(db, "SELECT id FROM Users WHERE name = ?", -1, &res, 0);
                        sqlite3_bind_text(res, 1, new_turner, -1, SQLITE_STATIC);
                        sqlite3_step(res);
                        new_turn = sqlite3_column_int(res, 0);
                        sqlite3_prepare_v2(db, "UPDATE player_info SET turn = ? WHERE id = ?;", -1, &res, 0);
                        sqlite3_bind_int(res, 1, new_turn);
                        sqlite3_bind_int(res, 2, game);
                        break;
                    case 4:
                        sqlite3_prepare_v2(db, "SELECT CASE WHEN pl_1 = u.name THEN pl_2 WHEN pl_2 = u.name THEN pl_3 WHEN pl_3 = u.name THEN pl_4 WHEN pl_4 = u.name THEN pl_1 END AS pos_pl FROM player_info JOIN Users u ON player_info.turn = u.id WHERE u.id = ? AND player_info.id = ?", -1, &res, 0);
                        sqlite3_bind_int(res, 1, id);
                        sqlite3_bind_int(res, 2, game);
                        sqlite3_step(res);
                        new_turner = sqlite3_column_text(res, 0);
                        sqlite3_prepare_v2(db, "SELECT id FROM Users WHERE name = ?", -1, &res, 0);
                        sqlite3_bind_text(res, 1, new_turner, -1, SQLITE_STATIC);
                        sqlite3_step(res);
                        new_turn = sqlite3_column_int(res, 0);
                        sqlite3_prepare_v2(db, "UPDATE player_info SET turn = ? WHERE id = ?;", -1, &res, 0);
                        sqlite3_bind_int(res, 1, new_turn);
                        sqlite3_bind_int(res, 2, game);
                        break;
                }
            }
            else{

            }
        }
        else{
            switch(count_pl){
                case 2:
                    sqlite3_prepare_v2(db, "SELECT CASE WHEN pl_1 = u.name THEN pl_2 WHEN pl_2 = u.name THEN pl_1 WHEN pl_3 = u.name THEN pos_pl_3 WHEN pl_4 = u.name THEN pos_pl_4 END AS pos_pl FROM player_info JOIN Users u ON player_info.turn = u.id WHERE u.id = ? AND player_info.id = ?", -1, &res, 0);
                    sqlite3_bind_int(res, 1, id);
                    sqlite3_bind_int(res, 2, game);
                    sqlite3_step(res);
                    new_turner = sqlite3_column_text(res, 0);
                    sqlite3_prepare_v2(db, "SELECT id FROM Users WHERE name = ?", -1, &res, 0);
                    sqlite3_bind_text(res, 1, new_turner, -1, SQLITE_STATIC);
                    sqlite3_step(res);
                    new_turn = sqlite3_column_int(res, 0);
                    sqlite3_prepare_v2(db, "UPDATE player_info SET turn = ? WHERE id = ?;", -1, &res, 0);
                    sqlite3_bind_int(res, 1, new_turn);
                    sqlite3_bind_int(res, 2, game);
                    sqlite3_step(res);
                    break;
                case 3:
                    sqlite3_prepare_v2(db, "SELECT CASE WHEN pl_1 = u.name THEN pl_2 WHEN pl_2 = u.name THEN pl_3 WHEN pl_3 = u.name THEN pl_1 WHEN pl_4 = u.name THEN pos_pl_4 END AS pos_pl FROM player_info JOIN Users u ON player_info.turn = u.id WHERE u.id = ? AND player_info.id = ?", -1, &res, 0);
                    sqlite3_bind_int(res, 1, id);
                    sqlite3_bind_int(res, 2, game);
                    sqlite3_step(res);
                    new_turner = sqlite3_column_text(res, 0);
                    sqlite3_prepare_v2(db, "SELECT id FROM Users WHERE name = ?", -1, &res, 0);
                    sqlite3_bind_text(res, 1, new_turner, -1, SQLITE_STATIC);
                    sqlite3_step(res);
                    new_turn = sqlite3_column_int(res, 0);
                    sqlite3_prepare_v2(db, "UPDATE player_info SET turn = ? WHERE id = ?;", -1, &res, 0);
                    sqlite3_bind_int(res, 1, new_turn);
                    sqlite3_bind_int(res, 2, game);
                    sqlite3_step(res);
                    break;
                case 4:
                    sqlite3_prepare_v2(db, "SELECT CASE WHEN pl_1 = u.name THEN pl_2 WHEN pl_2 = u.name THEN pl_3 WHEN pl_3 = u.name THEN pl_4 WHEN pl_4 = u.name THEN pl_1 END AS pos_pl FROM player_info JOIN Users u ON player_info.turn = u.id WHERE u.id = ? AND player_info.id = ?", -1, &res, 0);
                    sqlite3_bind_int(res, 1, id);
                    sqlite3_bind_int(res, 2, game);
                    sqlite3_step(res);
                    new_turner = sqlite3_column_text(res, 0);
                    sqlite3_prepare_v2(db, "SELECT id FROM Users WHERE name = ?", -1, &res, 0);
                    sqlite3_bind_text(res, 1, new_turner, -1, SQLITE_STATIC);
                    sqlite3_step(res);
                    new_turn = sqlite3_column_int(res, 0);
                    sqlite3_prepare_v2(db, "UPDATE player_info SET turn = ? WHERE id = ?;", -1, &res, 0);
                    sqlite3_bind_int(res, 1, new_turn);
                    sqlite3_bind_int(res, 2, game);
                    sqlite3_step(res);
                    break;
            }
        }


    // char temp[50];
    // sprintf(temp, "|%s|%d|\n", str, count);
    // write(STDOUT_FILENO, temp, strlen(temp));
    pthread_mutex_unlock(&clients_mutex);
}

void goObnovaToClient(SSL *ssl, char *str){
    char temp[16];
    for (int i = 0; i < strlen(str); i++){
        if (str[i] == '@'){
            temp[i] = '\0';
            break;
        }
        else temp[i] = str[i];
    }
    // write(STDOUT_FILENO, "Ya TUUUUUUT\n", strlen("Ya TUUUUUUT\n"));
    int game = atoi(temp);

    // write(STDOUT_FILENO, str, strlen(str));

    sqlite3_stmt *res;

    pthread_mutex_lock(&clients_mutex);

    sqlite3_prepare_v2(db, "SELECT * FROM game_log where id_game = ? ORDER BY id DESC LIMIT 1", -1, &res, 0);
    sqlite3_bind_int(res, 1, game);
    if (sqlite3_step(res) == SQLITE_ROW){
        
        char result[1024];
        sprintf(result, "%s%s%s%s", sqlite3_column_text(res, 2), sqlite3_column_text(res, 3), sqlite3_column_text(res, 4), sqlite3_column_text(res, 7));
        SSL_write(ssl, result, strlen(result));
    }

    pthread_mutex_unlock(&clients_mutex);
}

void agree(char *str){
    char temp[8];
    char temp_1[8];
    bool flag = false;
    int count = 0;

    for (int i = 0; i < strlen(str); i++){
        if (str[i] == '@' && !flag){
            temp[i] = '\0';
            flag = true;
        }
        else if (!flag) temp[i] = str[i];
        else if(flag){
            temp_1[count] = str[i];
            count++;
        }
        
    }
    temp_1[count] = '\0';
    int game = atoi(temp);
    int id = atoi(temp_1);
    // write(STDOUT_FILENO, str, strlen(str));

    sqlite3_stmt *res;

    pthread_mutex_lock(&clients_mutex);

    sqlite3_prepare_v2(db, "SELECT * FROM player_info WHERE id = ? AND turn = ?", -1, &res, 0);
    sqlite3_bind_int(res, 1, game);
    sqlite3_bind_int(res, 2, id);
    if (sqlite3_step(res) == SQLITE_ROW){
        
        sqlite3_prepare_v2(db, "SELECT bonus FROM game_log WHERE id_game = ?", -1, &res, 0);
        sqlite3_bind_int(res, 1, game);
        if (sqlite3_step(res) == SQLITE_ROW){
            char *mes = sqlite3_column_text(res, 0);
            if (mes[0] == 'b'){
                sqlite3_prepare_v2(db, "SELECT CASE WHEN pl_1 = u.name THEN pos_pl_1 WHEN pl_2 = u.name THEN pos_pl_2 WHEN pl_3 = u.name THEN pos_pl_3 WHEN pl_4 = u.name THEN pos_pl_4 END AS pos_pl FROM player_info JOIN Users u ON player_info.turn = u.id WHERE u.id = ? AND player_info.id = ?", -1, &res, 0);
                //SELECT CASE WHEN pl_1 = u.name THEN 1 WHEN pl_2 = u.name THEN 2 WHEN pl_3 = u.name THEN 3 WHEN pl_4 = u.name THEN 4 END AS pos_pl FROM player_info JOIN Users u ON player_info.turn = u.id WHERE u.id = 5 AND player_info.id = 557
                sqlite3_bind_int(res, 1, id);
                sqlite3_bind_int(res, 2, game);
                sqlite3_step(res);
                int pos = sqlite3_column_int(res, 0);
                sqlite3_prepare_v2(db, "SELECT CASE WHEN pl_1 = u.name THEN money_player_1 WHEN pl_2 = u.name THEN money_player_2 WHEN pl_3 = u.name THEN money_player_3 WHEN pl_4 = u.name THEN money_player_4 END AS money_player FROM player_info JOIN Users u ON player_info.turn = u.id WHERE u.id = ? AND player_info.id = ?", -1, &res, 0);
                sqlite3_bind_int(res, 1, id);
                sqlite3_bind_int(res, 2, game);
                sqlite3_step(res);
                int money = sqlite3_column_int(res, 0);
                int m = 0; 
                if (pos >= 1 && pos <= 4){
                    m = 240;
                }
                else if (pos >= 5 && pos <= 8){
                    m = 280;
                }
                else if (pos >= 10 && pos <= 13){
                    m = 320;
                }
                else if (pos >= 15 && pos <= 17){
                    m = 400;
                }
                else if (pos >= 19 && pos <= 21){
                    m = 80;
                }
                else if (pos >= 23 && pos <= 26){
                    m = 120;
                }
                else if (pos >= 28 && pos <= 31){
                    m = 160;
                }
                else if (pos >= 32 && pos <= 35){
                    m = 200;
                }
                money -= m;
                if (money <= 0){
                    money = 0;
                }
                sqlite3_prepare_v2(db, "UPDATE player_info SET money_player_1 = CASE WHEN pl_1 = (SELECT name FROM Users WHERE id = ?) THEN money_player_1 - ? ELSE money_player_1 END, money_player_2 = CASE WHEN pl_2 = (SELECT name FROM Users WHERE id = ?) THEN money_player_2 - ? ELSE money_player_2 END, money_player_3 = CASE WHEN pl_3 = (SELECT name FROM Users WHERE id = ?) THEN money_player_3 - ? ELSE money_player_3 END, money_player_4 = CASE WHEN pl_4 = (SELECT name FROM Users WHERE id = ?) THEN money_player_4 - ? ELSE money_player_4 END WHERE id = ?", -1, &res, 0);
                sqlite3_bind_int(res, 1, id);
                sqlite3_bind_int(res, 2, m);
                sqlite3_bind_int(res, 3, id);
                sqlite3_bind_int(res, 4, m);
                sqlite3_bind_int(res, 5, id);
                sqlite3_bind_int(res, 6, m);
                sqlite3_bind_int(res, 7, id);
                sqlite3_bind_int(res, 8, m);
                sqlite3_bind_int(res, 9, game);
                sqlite3_step(res);
                sqlite3_prepare_v2(db, "SELECT CASE WHEN pl_1 = u.name THEN 1 WHEN pl_2 = u.name THEN 2 WHEN pl_3 = u.name THEN 3 WHEN pl_4 = u.name THEN 4 END AS pos_pl FROM player_info JOIN Users u ON player_info.turn = u.id WHERE u.id = ? AND player_info.id = ?", -1, &res, 0);
                // //SELECT CASE WHEN pl_1 = u.name THEN 1 WHEN pl_2 = u.name THEN 2 WHEN pl_3 = u.name THEN 3 WHEN pl_4 = u.name THEN 4 END AS money_player FROM player_info JOIN Users u ON player_info.turn = u.id WHERE u.id = 6 AND player_info.id = 555
                sqlite3_bind_int(res, 1, id);
                sqlite3_bind_int(res, 2, game);
                sqlite3_step(res);
                int pl = sqlite3_column_int(res, 0);
                sqlite3_prepare_v2(db, "INSERT INTO map_info VALUES (?, ?, ?, ?, ?)", -1, &res, 0);
                sqlite3_bind_int(res, 1, game);
                sqlite3_bind_int(res, 2, pos);
                sqlite3_bind_int(res, 3, id);
                sqlite3_bind_int(res, 4, 0);
                sqlite3_bind_int(res, 5, pl);
                sqlite3_step(res);
                char bonus[256];

                sprintf(bonus, "n@");

                sqlite3_prepare_v2(db, "SELECT count_pl, pos_pl_1, pos_pl_2, pos_pl_3, pos_pl_4, money_player_1, money_player_2, money_player_3, money_player_4 FROM player_info where id = ?", -1, &res, 0);
                sqlite3_bind_int(res, 1, game);
                sqlite3_step(res); //ALTER TABLE map_info ADD COLUMN pl INTEGER;

                char pos_players[256];
                
                sprintf(pos_players, "g@m@u@%d@%d@%d@%d@%d@", sqlite3_column_int(res, 0), sqlite3_column_int(res, 1), sqlite3_column_int(res, 2), sqlite3_column_int(res, 3), sqlite3_column_int(res, 4));
                char dicebuff[256];
                sprintf(dicebuff, "d@%d@", 0);
                char moneybuff[512];
                sprintf(moneybuff, "m@%d@%d@%d@%d@", sqlite3_column_int(res, 5), sqlite3_column_int(res, 6), sqlite3_column_int(res, 7), sqlite3_column_int(res, 8));
                sqlite3_prepare_v2(db, "INSERT INTO game_log(id_game, pos_players, cube, money, bonus) VALUES(?, ?, ?, ?, ?)", -1, &res, 0);
                sqlite3_bind_int(res, 1, game);
                sqlite3_bind_text(res, 2, pos_players, -1, SQLITE_STATIC);
                sqlite3_bind_text(res, 3, dicebuff, -1, SQLITE_STATIC);
                sqlite3_bind_text(res, 4, moneybuff, -1, SQLITE_STATIC);
                sqlite3_bind_text(res, 5, bonus, -1, SQLITE_STATIC);
                sqlite3_step(res);
            }
            sqlite3_prepare_v2(db, "SELECT count_pl FROM player_info WHERE id = ?", -1, &res, 0); 
            sqlite3_bind_int(res, 1, game);
            sqlite3_step(res);
            int count_pl = sqlite3_column_int(res, 0);
            char *new_turner;
            int new_turn;

            switch(count_pl){
                case 2:
                    sqlite3_prepare_v2(db, "SELECT CASE WHEN pl_1 = u.name THEN pl_2 WHEN pl_2 = u.name THEN pl_1 WHEN pl_3 = u.name THEN pos_pl_3 WHEN pl_4 = u.name THEN pos_pl_4 END AS pos_pl FROM player_info JOIN Users u ON player_info.turn = u.id WHERE u.id = ? AND player_info.id = ?", -1, &res, 0);
                    sqlite3_bind_int(res, 1, id);
                    sqlite3_bind_int(res, 2, game);
                    sqlite3_step(res);
                    new_turner = sqlite3_column_text(res, 0);
                    sqlite3_prepare_v2(db, "SELECT id FROM Users WHERE name = ?", -1, &res, 0);
                    sqlite3_bind_text(res, 1, new_turner, -1, SQLITE_STATIC);
                    sqlite3_step(res);
                    new_turn = sqlite3_column_int(res, 0);
                    sqlite3_prepare_v2(db, "UPDATE player_info SET turn = ? WHERE id = ?;", -1, &res, 0);
                    sqlite3_bind_int(res, 1, new_turn);
                    sqlite3_bind_int(res, 2, game);
                    sqlite3_step(res);
                    break;
                case 3:
                    sqlite3_prepare_v2(db, "SELECT CASE WHEN pl_1 = u.name THEN pl_2 WHEN pl_2 = u.name THEN pl_3 WHEN pl_3 = u.name THEN pl_1 WHEN pl_4 = u.name THEN pos_pl_4 END AS pos_pl FROM player_info JOIN Users u ON player_info.turn = u.id WHERE u.id = ? AND player_info.id = ?", -1, &res, 0);
                    sqlite3_bind_int(res, 1, id);
                    sqlite3_bind_int(res, 2, game);
                    sqlite3_step(res);
                    new_turner = sqlite3_column_text(res, 0);
                    sqlite3_prepare_v2(db, "SELECT id FROM Users WHERE name = ?", -1, &res, 0);
                    sqlite3_bind_text(res, 1, new_turner, -1, SQLITE_STATIC);
                    sqlite3_step(res);
                    new_turn = sqlite3_column_int(res, 0);
                    sqlite3_prepare_v2(db, "UPDATE player_info SET turn = ? WHERE id = ?;", -1, &res, 0);
                    sqlite3_bind_int(res, 1, new_turn);
                    sqlite3_bind_int(res, 2, game);
                    sqlite3_step(res);
                    break;
                case 4:
                    sqlite3_prepare_v2(db, "SELECT CASE WHEN pl_1 = u.name THEN pl_2 WHEN pl_2 = u.name THEN pl_3 WHEN pl_3 = u.name THEN pl_4 WHEN pl_4 = u.name THEN pl_1 END AS pos_pl FROM player_info JOIN Users u ON player_info.turn = u.id WHERE u.id = ? AND player_info.id = ?", -1, &res, 0);
                    sqlite3_bind_int(res, 1, id);
                    sqlite3_bind_int(res, 2, game);
                    sqlite3_step(res);
                    new_turner = sqlite3_column_text(res, 0);
                    sqlite3_prepare_v2(db, "SELECT id FROM Users WHERE name = ?", -1, &res, 0);
                    sqlite3_bind_text(res, 1, new_turner, -1, SQLITE_STATIC);
                    sqlite3_step(res);
                    new_turn = sqlite3_column_int(res, 0);
                    sqlite3_prepare_v2(db, "UPDATE player_info SET turn = ? WHERE id = ?;", -1, &res, 0);
                    sqlite3_bind_int(res, 1, new_turn);
                    sqlite3_bind_int(res, 2, game);
                    sqlite3_step(res);
                    break;
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}
void disagree(char *str){
    char temp[8];
    char temp_1[8];
    bool flag = false;
    int count = 0;

    for (int i = 0; i < strlen(str); i++){
        if (str[i] == '@' && !flag){
            temp[i] = '\0';
            flag = true;
        }
        else if (!flag) temp[i] = str[i];
        else if(flag){
            temp_1[count] = str[i];
            count++;
        }
        
    }
    temp_1[count] = '\0';
    int game = atoi(temp);
    int id = atoi(temp_1);

    // write(STDOUT_FILENO, str, strlen(str));

    sqlite3_stmt *res;

    pthread_mutex_lock(&clients_mutex);

    sqlite3_prepare_v2(db, "SELECT count_pl FROM player_info WHERE id = ?", -1, &res, 0); 
            sqlite3_bind_int(res, 1, game);
            sqlite3_step(res);
            int count_pl = sqlite3_column_int(res, 0);
            int new_turn;
            char *new_turner;

            switch(count_pl){
                case 2:
                    sqlite3_prepare_v2(db, "SELECT CASE WHEN pl_1 = u.name THEN pl_2 WHEN pl_2 = u.name THEN pl_1 WHEN pl_3 = u.name THEN pos_pl_3 WHEN pl_4 = u.name THEN pos_pl_4 END AS pos_pl FROM player_info JOIN Users u ON player_info.turn = u.id WHERE u.id = ? AND player_info.id = ?", -1, &res, 0);
                    sqlite3_bind_int(res, 1, id);
                    sqlite3_bind_int(res, 2, game);
                    sqlite3_step(res);
                    new_turner = sqlite3_column_text(res, 0);
                    sqlite3_prepare_v2(db, "SELECT id FROM Users WHERE name = ?", -1, &res, 0);
                    sqlite3_bind_text(res, 1, new_turner, -1, SQLITE_STATIC);
                    sqlite3_step(res);
                    new_turn = sqlite3_column_int(res, 0);
                    sqlite3_prepare_v2(db, "UPDATE player_info SET turn = ? WHERE id = ?;", -1, &res, 0);
                    sqlite3_bind_int(res, 1, new_turn);
                    sqlite3_bind_int(res, 2, game);
                    sqlite3_step(res);
                    break;
                case 3:
                    sqlite3_prepare_v2(db, "SELECT CASE WHEN pl_1 = u.name THEN pl_2 WHEN pl_2 = u.name THEN pl_3 WHEN pl_3 = u.name THEN pl_1 WHEN pl_4 = u.name THEN pos_pl_4 END AS pos_pl FROM player_info JOIN Users u ON player_info.turn = u.id WHERE u.id = ? AND player_info.id = ?", -1, &res, 0);
                    sqlite3_bind_int(res, 1, id);
                    sqlite3_bind_int(res, 2, game);
                    sqlite3_step(res);
                    new_turner = sqlite3_column_text(res, 0);
                    sqlite3_prepare_v2(db, "SELECT id FROM Users WHERE name = ?", -1, &res, 0);
                    sqlite3_bind_text(res, 1, new_turner, -1, SQLITE_STATIC);
                    sqlite3_step(res);
                    new_turn = sqlite3_column_int(res, 0);
                    sqlite3_prepare_v2(db, "UPDATE player_info SET turn = ? WHERE id = ?;", -1, &res, 0);
                    sqlite3_bind_int(res, 1, new_turn);
                    sqlite3_bind_int(res, 2, game);
                    sqlite3_step(res);
                    break;
                case 4:
                    sqlite3_prepare_v2(db, "SELECT CASE WHEN pl_1 = u.name THEN pl_2 WHEN pl_2 = u.name THEN pl_3 WHEN pl_3 = u.name THEN pl_4 WHEN pl_4 = u.name THEN pl_1 END AS pos_pl FROM player_info JOIN Users u ON player_info.turn = u.id WHERE u.id = ? AND player_info.id = ?", -1, &res, 0);
                    sqlite3_bind_int(res, 1, id);
                    sqlite3_bind_int(res, 2, game);
                    sqlite3_step(res);
                    new_turner = sqlite3_column_text(res, 0);
                    sqlite3_prepare_v2(db, "SELECT id FROM Users WHERE name = ?", -1, &res, 0);
                    sqlite3_bind_text(res, 1, new_turner, -1, SQLITE_STATIC);
                    sqlite3_step(res);
                    new_turn = sqlite3_column_int(res, 0);
                    sqlite3_prepare_v2(db, "UPDATE player_info SET turn = ? WHERE id = ?;", -1, &res, 0);
                    sqlite3_bind_int(res, 1, new_turn);
                    sqlite3_bind_int(res, 2, game);
                    sqlite3_step(res);
                    break;
            }

    char bonus[256];

    sprintf(bonus, "n@");

    sqlite3_prepare_v2(db, "SELECT count_pl, pos_pl_1, pos_pl_2, pos_pl_3, pos_pl_4, money_player_1, money_player_2, money_player_3, money_player_4 FROM player_info where id = ?", -1, &res, 0);
    sqlite3_bind_int(res, 1, game);
    sqlite3_step(res);

    char pos_players[1024];
    sprintf(pos_players, "g@m@u@%d@%d@%d@%d@%d@", sqlite3_column_int(res, 0), sqlite3_column_int(res, 1), sqlite3_column_int(res, 2), sqlite3_column_int(res, 3), sqlite3_column_int(res, 4));
    char dicebuff[256];
    sprintf(dicebuff, "d@%d@", 0);
    char moneybuff[512];
    sprintf(moneybuff, "m@%d@%d@%d@%d@", sqlite3_column_int(res, 5), sqlite3_column_int(res, 6), sqlite3_column_int(res, 7), sqlite3_column_int(res, 8));
    sqlite3_prepare_v2(db, "INSERT INTO game_log(id_game, pos_players, cube, money, bonus) VALUES(?, ?, ?, ?, ?)", -1, &res, 0);
    sqlite3_bind_int(res, 1, game);
    sqlite3_bind_text(res, 2, pos_players, -1, SQLITE_STATIC);
    sqlite3_bind_text(res, 3, dicebuff, -1, SQLITE_STATIC);
    sqlite3_bind_text(res, 4, moneybuff, -1, SQLITE_STATIC);
    sqlite3_bind_text(res, 5, bonus, -1, SQLITE_STATIC);
    sqlite3_step(res);

    pthread_mutex_unlock(&clients_mutex);
}

void sendSetting(int id, SSL *ssl){
    sqlite3_stmt *res;

    pthread_mutex_lock(&clients_mutex);

    sqlite3_prepare_v2(db, "SELECT name, image FROM Users WHERE id = ?", -1, &res, 0);
    sqlite3_bind_int(res, 1, id);
    sqlite3_step(res);
    char *name = sqlite3_column_text(res, 0);
    int image = sqlite3_column_int(res, 1);
    char temp[216];
    sprintf(temp, "s@%d@%s@", image, name);
    SSL_write(ssl, temp, strlen(temp));

    pthread_mutex_unlock(&clients_mutex);
}
void updateUserWithName(char *str){
    char temp_img[8];
    char name[36];
    char *temp_id;
    bool flag = false;
    int idx = 0;
    for (int i = 0; i < strlen(str); i++){
        if (str[i] == '@' && !flag){
            temp_img[i] = '\0';
            flag = true;
        }
        else if (str[i] == '@' && flag){
            name[idx] = '\0';
            temp_id = &str[i + 1];
            break;
        }
        else if (!flag){
            temp_img[i] = str[i];
        }
        else if (flag){
            name[idx] = str[i];
            idx++;
        }
    }

    sqlite3_stmt *res;

    pthread_mutex_lock(&clients_mutex);

    sqlite3_prepare_v2(db, "UPDATE Users SET name = ?, image = ? WHERE id = ?", -1, &res, 0);
    sqlite3_bind_text(res, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_int(res, 2, atoi(temp_img));
    sqlite3_bind_int(res, 3, atoi(temp_id));
    sqlite3_step(res);

    sqlite3_prepare_v2(db, "UPDATE Chats SET new = true WHERE chater_id = ?", -1, &res, 0);
    sqlite3_bind_int(res, 1, atoi(temp_id));
    sqlite3_step(res);


    pthread_mutex_unlock(&clients_mutex);
}
void updateUserWithoutName(char *str){
    char temp_img[8];
    char id[8];
    bool flag = false;
    int idx = 0;
    for (int i = 0; i < strlen(str); i++){
        if (str[i] == '@' && !flag){
            temp_img[i] = '\0';
            flag = true;
        }
        else if (str[i] == '@' && flag){
            id[idx] = '\0';
            break;
        }
        else if (!flag){
            temp_img[i] = str[i];
        }
        else if (flag){
            id[idx] = str[i];
            idx++;
        }
    }

    sqlite3_stmt *res;

    pthread_mutex_lock(&clients_mutex);

    sqlite3_prepare_v2(db, "UPDATE Users SET image = ? WHERE id = ?", -1, &res, 0);
    sqlite3_bind_int(res, 1, atoi(temp_img));
    sqlite3_bind_int(res, 2, atoi(id));
    sqlite3_step(res);

    pthread_mutex_unlock(&clients_mutex);

    sqlite3_prepare_v2(db, "UPDATE Chats SET new = true WHERE chater_id = ?", -1, &res, 0);
    sqlite3_bind_int(res, 1, atoi(id));
    sqlite3_step(res);

    free(str);
}

void sendMessGame(char *str){
    char temp_mess[1024];
    char game[8];
    bool flag = false;
    int idx = 0;
    int i = 0;
    for (; i < strlen(str); i++){
        if (str[i] == '@' && !flag){
            temp_mess[i] = '\0';
            flag = true;
        }
        else if (str[i] == '@' && flag){
            game[idx] = '\0';
            break;
        }
        else if (!flag){
            temp_mess[i] = str[i];
        }
        else if (flag){
            game[idx] = str[i];
            idx++;
        }
    }
    i++;
    idx = 0;
    char temp_id[8];
    for (; i < strlen(str); i++){
        if (str[i] == '@'){
            temp_id[idx] = '\0';
            break;
        }
        else {
        temp_id[idx] = str[i];
        idx++;
        }
    }
    sqlite3_stmt *res;

    pthread_mutex_lock(&clients_mutex);

    sqlite3_prepare_v2(db, "SELECT name FROM Users WHERE id = ?", -1, &res, 0);
    sqlite3_bind_int(res, 1, atoi(temp_id));
    sqlite3_step(res);
    // write(STDOUT_FILENO, temp_mess, strlen(temp_mess));
    // write(STDOUT_FILENO, "\n", 1);
    char mess[1024];
    sprintf(mess, "%s: %s", sqlite3_column_text(res, 0), temp_mess);

    sqlite3_prepare_v2(db, "SELECT count_pl FROM player_info WHERE id = ?", -1, &res, 0);
    sqlite3_bind_int(res, 1, atoi(game));
    sqlite3_step(res);
    int count = sqlite3_column_int(res, 0);
    for (int i = 1; i < count + 1; i++){
        char temp[512];
        sprintf(temp, "SELECT pl_%d FROM player_info WHERE id = ?", i);
        sqlite3_prepare_v2(db, temp, -1, &res, 0);
        sqlite3_bind_int(res, 1, atoi(game));
        sqlite3_step(res);
        char *name = sqlite3_column_text(res, 0);
        sprintf(temp, "SELECT id FROM Users WHERE name = ?");
        sqlite3_prepare_v2(db, temp, -1, &res, 0);
        sqlite3_bind_text(res, 1, name, -1, SQLITE_STATIC);
        sqlite3_step(res);
        int id = sqlite3_column_int(res, 0);
        sprintf(temp, "INSERT INTO mes_game (mes, new, id_game, id_user) VALUES (?, true, ?, ?)");
        sqlite3_prepare_v2(db, temp, -1, &res, 0);
        sqlite3_bind_text(res, 1, mess, -1, SQLITE_STATIC);
        sqlite3_bind_int(res, 2, atoi(game));
        sqlite3_bind_int(res, 3, id);
        sqlite3_step(res);
    }

    pthread_mutex_unlock(&clients_mutex);

}

void nameAndImage(char *str, SSL *ssl){
    char friend[36];
    bool flag = false;
    int idx = 0;
    for (int i = 0; i < strlen(str); i++){
        if (str[i] == '@' && !flag){
            flag = true;
        }
        else if (flag){
            friend[idx] = str[i];
            idx++;
        }
    }
    friend[idx] = '\0';
    
    sqlite3_stmt *res;

    pthread_mutex_lock(&clients_mutex);

    sqlite3_prepare_v2(db, "SELECT name, image FROM Users WHERE id = ?", -1, &res, 0);
    sqlite3_bind_int(res, 1, atoi(friend));
    sqlite3_step(res);
    char *result = sqlite3_column_text(res, 0);
    int image = sqlite3_column_int(res, 1);
    char temp[32];
    sprintf(temp, "S@%s@%d", result, image);
    SSL_write(ssl, temp, strlen(temp));

    pthread_mutex_unlock(&clients_mutex);
}

void sMess(char *str, SSL *ssl){
    char id[8];
    char game[8];
    bool flag = false;
    int idx = 0;
    for (int i = 0; i < strlen(str); i++){
        if (str[i] == '@' && !flag){
            game[i] = '\0';
            flag = true;
        }
        else if (str[i] == '@' && flag){
            id[idx] = '\0';
            break;
        }
        else if (!flag){
            game[i] = str[i];
        }
        else if (flag){
            id[idx] = str[i];
            idx++;
        }
    }

    sqlite3_stmt *res;

    pthread_mutex_lock(&clients_mutex);

    sqlite3_prepare_v2(db, "SELECT mes FROM mes_game WHERE id_game = ? AND id_user = ? AND new = true", -1, &res, 0);
    sqlite3_bind_int(res, 1, atoi(game));
    sqlite3_bind_int(res, 2, atoi(id));
    sqlite3_step(res);
    if (sqlite3_column_text(res, 0) != NULL){
    char mes[256];
    sprintf(mes, "g@M@%s", sqlite3_column_text(res, 0));
    SSL_write(ssl, mes, strlen(mes));

    sqlite3_prepare_v2(db, "UPDATE mes_game SET new = false WHERE id_game = ? and id_user = ?", -1, &res, 0);
    sqlite3_bind_int(res, 1, atoi(game));
    sqlite3_bind_int(res, 2, atoi(id));
    sqlite3_step(res);
    }
    else {}
    // SSL_write(ssl, "ok", strlen("ok"));
    pthread_mutex_unlock(&clients_mutex);

}

void Servlet(SSL* ssl) /* Serve the connection -- threadable */
{
    char buf[1024] = {0};
    int sd, bytes;
    if ( SSL_accept(ssl) == FAIL )     /* do SSL-protocol accept */
        ERR_print_errors_fp(stderr);
    else
    {
        while (1){
            
            bytes = SSL_read(ssl, buf, sizeof(buf)); /* get request */
            if ( bytes > 0 )
            {
                buf[bytes] = '\0';
                // write(STDOUT_FILENO, buf, bytes);
                // write(STDOUT_FILENO, "\n", strlen("\n"));
                if (buf[0] == 'r'){
                    int id = registrationUser(buf);
                    if ( id != 0){
                        char str[512];
                        sprintf(str, "Y@%i", id);
                        SSL_write(ssl, str, strlen(str));
                    }
                    else SSL_write(ssl, "N@r", strlen("N@r"));
                }
                else if (buf[0] == 'l'){
                    int id = loginUser(buf);
                    if( id > 0){
                        char *str = malloc(128 * sizeof(char));
                        sprintf(str, "Y@%i", id);
                        SSL_write(ssl, str, strlen(str));
                        
                    }
                    else if (id == -1) SSL_write(ssl, "N@l@c", strlen("N@l@c"));
                    else if (id == -2) SSL_write(ssl, "N@l@b", strlen("N@l@b"));
                }
                else if (buf[0] == 'x'){
                    char *temp = delete_ykazatel(buf);
                    sendSetting(atoi(temp), ssl);
                    free(temp);
                }
                else if (buf[0] == 'C'){
                    char *temp = delete_ykazatel(buf);
                    if (temp[0] == 'n'){
                        updateUserWithoutName(delete_ykazatel(temp));
                        free(temp);
                    }
                    else {
                        updateUserWithName(temp);
                        free(temp);
                    }
                }
                else if (buf[0] == 'c'){
                    char *temp = delete_ykazatel(buf);
                    //write(STDOUT_FILENO, "\n", strlen("\n"));
                    //write(STDOUT_FILENO, temp, strlen(temp));
                    //char *str = returnChats(atoi(temp));
                    char str[16];
                    int n = countChatsOld(atoi(temp));
                    sprintf(str, "n@%d", n);
                    SSL_write(ssl, str, strlen(str));
                    if (n != 0) sendChats(atoi(temp), ssl);
                    free(temp);
                }
                else if (buf[0] == 'e'){
                    char *temp = delete_ykazatel(buf);
                    notOnline(atoi(temp));
                    free(temp);
                }
                else if (buf[0] == 's'){
                    char *temp = delete_ykazatel(buf);
                    char *str = delete_ykazatel(temp);
                    free(temp);
                    //write(STDOUT_FILENO, str, strlen(str));
                    int n = searchFriend(buf);
                    char str_i[16];
                    sprintf(str_i, "n@%d", n);
                    SSL_write(ssl, str_i, strlen(str_i));
                    
                    sendChats(atoi(str), ssl);
                    free(str);
                    
                    // SSL_write(ssl, str, strlen(str));
                    // else SSL_write(ssl, "Be", strlen("Be"));
                }
                else if (buf[0] == 'u'){
                    char *temp = delete_ykazatel(buf);
                    char str[16];
                    int n = countChats(atoi(temp));
                    if (n > 0){
                        n = countChatsOld(atoi(temp));
                    }
                    sprintf(str, "n@%d", n);
                    SSL_write(ssl, str, strlen(str));
                    if (n != 0) sendChats(atoi(temp), ssl);
                    free(temp);
                    //write(STDOUT_FILENO, str, strlen(str));
                    
                }
                else if (buf[0] == 'w'){
                    char *temp = delete_ykazatel(buf);
                    if (temp[0] == 'r'){
                        char *temp_1 = delete_ykazatel(temp);
                        insertMes(delete_ykazatel(temp_1), ssl);
                        // write(STDOUT_FILENO, "KJDKJ", strlen("KJDKJ"));
                        int n = countChatMesOld(temp_1);
                        char str[16];
                        sprintf(str, "c@%d", n);
                        SSL_write(ssl, str, strlen(str));
                        showChatMes(temp_1, n, ssl);
                    }
                    else if (temp[0] == 'p'){
                        char *temp_1 = delete_ykazatel(temp);
                        int n = countChatMesOld(temp_1);
                        char str[16];
                        sprintf(str, "c@%d", n);
                        SSL_write(ssl, str, strlen(str));
                        
                        showChatMes(temp_1, n, ssl);
                    }
                    else {
                        // write(STDOUT_FILENO, temp, strlen(temp));
                        int n = countChatMes(temp);
                        char str[16];
                        sprintf(str, "c@%d", n);
                        SSL_write(ssl, str, strlen(str));
                        showChatMes(temp, n, ssl);
                    }
                    //SSL_write(ssl, "yes", strlen("yes"));
                }
                else if (buf[0] == 'W')
                {
                    char *temp_1 = delete_ykazatel(buf);
                    nameAndImage(temp_1, ssl);
                }
                else if (buf[0] == 'm'){
                    char *temp = delete_ykazatel(buf);
                    insertMes(temp, ssl);
                }
                else if (buf[0] == 'M'){
                    char *temp = delete_ykazatel(buf);
                    changeMes(temp);
                    free(temp);
                }
                else if (buf[0] == 'd'){
                    if (buf[2] == 'm'){
                        char *temp = delete_ykazatel(buf);
                        char *temp_1 = delete_ykazatel(temp);
                        deleteMes(temp_1);
                        free(temp);
                        free(temp_1);
                    }
                    else {
                        char *temp = delete_ykazatel(buf);
                        int id = deleteChat(temp);
                        char str[16];
                        int n = countChatsOld(id);
                        sprintf(str, "n@%d", n);
                        SSL_write(ssl, str, strlen(str));
                        if (n != 0) sendChats(id, ssl);
                    }
                }
                else if (buf[0] == 'g'){  //game if's
                    char *temp = delete_ykazatel(buf);
                    if (temp[0] == 'i'){
                        invitePlayer(delete_ykazatel(temp));
                    }
                    else if (temp[0] == 'b'){
                        char *temp_1 = delete_ykazatel(temp);
                        if (temp_1[0] == 'l'){
                            free(temp);
                            char str[64];
                            int n = idBeginGame();
                            sprintf(str, "g@i@%d", n);
                            SSL_write(ssl, str, strlen(str));
                            insertIntoLobby(delete_ykazatel(temp_1), n);
                            free(temp_1);
                        }
                        else if(temp_1[0] == 'm'){
                            free(temp);
                            if (loadMap(delete_ykazatel(temp_1))){
                                char str[32];
                                sprintf(str, "g@m@");
                                SSL_write(ssl, str, strlen(str));
                            }
                        }
                        else if (temp_1[0] == 'y'){
                            free(temp);
                            agree(delete_ykazatel(temp_1));
                            free(temp_1);
                        }
                        else if (temp_1[0] == 'n'){
                            free(temp);
                            disagree(delete_ykazatel(temp_1));
                            free(temp_1);
                        }
                    }
                    else if (temp[0] == 'e'){
                        char *temp_1 = delete_ykazatel(temp);
                        if (temp_1[0] == 'l'){
                            deleteFromLobby(delete_ykazatel(temp_1));
                            free(temp_1);
                        }
                    }
                    else if (temp[0] == 'u'){
                        char *temp_1 = delete_ykazatel(temp);
                        if (checkMap(atoi(temp_1))){
                            char str[32];
                            sprintf(str, "g@m@");
                            SSL_write(ssl, str, strlen(str));
                        }
                        else {
                            free(temp);
                            int count = countPlayers(atoi(temp_1));
                            char str[16];
                            sprintf(str, "g@c@%d", count);
                            SSL_write(ssl, str, strlen(str));
                            showPlayers(atoi(temp_1), ssl);
                        }
                    }
                    else if (temp[0] == 'c'){
                        char *temp_1 = delete_ykazatel(temp);
                        free(temp);
                        int count = countInvites(atoi(temp_1));
                        char str[16];
                        sprintf(str, "g@l@%d", count);
                        SSL_write(ssl, str, strlen(str));
                        showLobby(atoi(temp_1), ssl);
                    }
                    else if (temp[0] == 'a'){
                        char *game = delete_ykazatel(temp);
                        free(temp);
                        insertInLobby(game);
                    }
                    else if (temp[0] == 'm'){
                        char *temp_1 = delete_ykazatel(temp);
                        beginGame(atoi(temp_1), ssl);
                        free(temp_1);
                    }
                    else if (temp[0] == 'M'){
                        if (temp[2] == 'U'){
                            char *temp_1 = delete_ykazatel(temp);
                            char *temp_2 = delete_ykazatel(temp_1);
                            sMess(temp_2, ssl);
                            free(temp_1);
                            free(temp_2);
                        }
                        else {
                            char *temp_1 = delete_ykazatel(temp);
                            sendMessGame(temp_1);
                            free(temp_1);
                        }
                    }
                    else if (temp[0] == 't'){
                        char *temp_1 = delete_ykazatel(temp);
                        free(temp);
                        char *temp_2 = delete_ykazatel(temp_1);
                        if(checkTurn(temp_1, atoi(temp_2))){
                            makeTurn(temp_1, atoi(temp_2));
                        }
                        // goObnovaToClient(ssl, temp_1);
                        free(temp_1);
                        free(temp_2);
                    }
                    else if (temp[0] == 'r'){
                        goObnovaToClient(ssl, delete_ykazatel(temp));
                    }
                }
                
                // printf("Client msg: \"%s\"\n", buf);
                // SSL_write(ssl, buf, strlen(buf));
            }
            else
            {
                break;
                ERR_print_errors_fp(stderr);
            }
        }
    }
    sd = SSL_get_fd(ssl);       /* get socket connection */
    SSL_free(ssl);         /* release SSL state */
    close(sd);          /* close connection */
    pthread_detach(pthread_self());
}
int main(int argc, char *argv[])
{
    SSL_CTX *ctx;
    int server;
    char *portnum;
    
//Only root user have the permsion to run the server
    if ( argc != 2 )
    {
        printf("Usage: ./uchat_server <portnum>\n");
        exit(0);
    }



    if (sqlite3_open("db/ex.db", &db) != SQLITE_OK){
        printf("db no such file");
    }
    // Initialize the SSL library
    SSL_library_init();
    portnum = argv[1];
    ctx = InitServerCTX();        /* initialize SSL */
    LoadCertificates(ctx, "sertificate/mycert.pem", "sertificate/mycert.pem"); /* load certs */
    server = OpenListener(atoi(portnum));    /* create server socket */

    pid_t pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        printf("PID: %d\n", pid);
        exit(EXIT_SUCCESS);
    }
    umask(0);
    if (setsid() < 0) {
        exit(EXIT_FAILURE);
    }
    if (chdir("/") < 0) {
        exit(EXIT_FAILURE);
    }
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    while (1)
    {
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);
        SSL *ssl;
        int client = accept(server, (struct sockaddr*)&addr, &len);  /* accept connection as usual */
        // printf("Connection: %s:%d\n",inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
        ssl = SSL_new(ctx);              /* get new SSL state with context */
        SSL_set_fd(ssl, client);      /* set connection socket to SSL state */
        //Servlet(ssl);         /* service connection */
        pthread_t tid;
        pthread_create(&tid, NULL, &Servlet, (void*)ssl);
    }
    close(server);          /* close server socket */
    SSL_CTX_free(ctx);         /* release context */
}
