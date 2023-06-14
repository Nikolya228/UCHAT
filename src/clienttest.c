#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <sys/socket.h>
#include <resolv.h>
#include <netdb.h>
#include <stdbool.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "../inc/header.h"

#define FAIL    -1

Widgets *ui;
int id;
int friend;
int game;
SSL *ssl;
guint timer_id = 0;
guint timer_chats = 0;
guint timer_mes = 0;
guint timer_podg = 0;
guint timer_podg_mes = 0;

int sockfd;
SSL_CTX *ctx;
struct sockaddr_in addr;



void sigint_handler(int sig)
{
    char temp[16];
    sprintf(temp, "g@e@l@%d", id);
    SSL_write(ssl, temp, strlen(temp));
    sprintf(temp, "e@%d", id);
    SSL_write(ssl, temp, strlen(temp));
    exit(0);
}

int OpenConnection(const char *hostname, int port)
{
    int sd;
    struct hostent *host;
    
    if ( (host = gethostbyname(hostname)) == NULL )
    {
        perror(hostname);
        abort();
    }
    
    sd = socket(PF_INET, SOCK_STREAM, 0);
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = *(long*)(host->h_addr);
    if ( connect(sd, (struct sockaddr*)&addr, sizeof(addr)) != 0 )
    {
        close(sd);
        perror(hostname);
        abort();
    }
    return sd;
}
SSL_CTX* InitCTX(void)
{
    SSL_METHOD *method;
    SSL_CTX *ctx;
    OpenSSL_add_all_algorithms();  /* Load cryptos, et.al. */
    SSL_load_error_strings();   /* Bring in and register error messages */
    method = TLSv1_2_client_method();  /* Create new client-method instance */
    ctx = SSL_CTX_new(method);   /* Create new context */
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}
void ShowCerts(SSL* ssl)
{
    X509 *cert;
    char *line;
    cert = SSL_get_peer_certificate(ssl); /* get the server's certificate */
    if ( cert != NULL )
    {
        printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Subject: %s\n", line);
        free(line);       /* free the malloc'ed string */
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);
        free(line);       /* free the malloc'ed string */
        X509_free(cert);     /* free the malloc'ed certificate copy */
    }
    else
        printf("Info: No client certificates configured.\n");
}

G_MODULE_EXPORT void onExit(GtkWidget * w) {
    gtk_main_quit();
}

G_MODULE_EXPORT void onExitChat(GtkWidget * w) {
    char temp[16];
    sprintf(temp, "e@%d", id);
    SSL_write(ssl, temp, strlen(temp));
    gtk_main_quit();
}

G_MODULE_EXPORT void destroyPop(GtkWidget * w) {
    gtk_widget_hide(ui->popover);
    GtkBuilder *ui_builder;
    GtkWidget *new_window;

    ui_builder = gtk_builder_new_from_file("glade_img/gui.glade");

    if (ui_builder == NULL) {
        g_print("Failed to load GtkBuilder from file\n");
        return;
    }
    free(ui);
    ui = load_widgets(ui_builder, ssl);
    load_signal(ui_builder, ui);
    
    gtk_widget_show_all(ui->window_chat);

    g_object_unref(ui_builder);

    char temp[16];
    sprintf(temp, "c@%d", id);
    SSL_write(ssl, temp, strlen(temp));
    sprintf(temp, "u@%d", id);
    SSL_write(ssl, temp, strlen(temp));
}



G_MODULE_EXPORT void onDestroyLobby(GtkWidget * w) {
    char temp[16];
    sprintf(temp, "g@e@l@%d", id);
    SSL_write(ssl, temp, strlen(temp));

    GtkBuilder *ui_builder;

    
    ui_builder = gtk_builder_new_from_file("glade_img/gui.glade");
    if (ui_builder == NULL) {
        g_print("Failed to load GtkBuilder from file\n");
        return;
    }
    
    g_source_remove(timer_id);
}



G_MODULE_EXPORT void onCloseInviteWindow(GtkWidget * w) {
    GtkBuilder *ui_builder;
    GtkWidget *new_window;

    ui_builder = gtk_builder_new_from_file("glade_img/gui.glade");
    if (ui_builder == NULL) {
        g_print("Failed to load GtkBuilder from file\n");
        return;
    }
    new_window = GTK_WIDGET(gtk_builder_get_object(ui_builder, "loby_window"));
    
    gtk_widget_hide(ui->window_invite);
    gtk_widget_show_all(new_window);

    free(ui);
    ui = load_widgets(ui_builder, ssl);
    load_signal(ui_builder, ui);
    
    g_object_unref(ui_builder);


    char temp[16];
    sprintf(temp, "g@u@%d", game);
    SSL_write(ssl, temp, strlen(temp));

    g_timeout_add(1000, updatePlayer, NULL);
}

G_MODULE_EXPORT void goStart(GtkButton *btn, gpointer data){
    GtkBuilder *ui_builder;
    GtkWidget *new_window;

    ui_builder = gtk_builder_new_from_file("glade_img/gui.glade");

    if (ui_builder == NULL) {
        g_print("Failed to load GtkBuilder from file\n");
        return;
    }

    new_window = GTK_WIDGET(gtk_builder_get_object(ui_builder, "main_map"));
    GdkScreen *Screen = gtk_widget_get_screen(new_window);
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(provider, "style/gtkchat.css", NULL);
    gtk_style_context_add_provider_for_screen(Screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);

    gtk_widget_hide(GTK_WIDGET(data));
    GError *error = NULL;

    GdkPixbufAnimation *animation = gdk_pixbuf_animation_new_from_file("./img/map_invite.gif", &error);
    if (error != NULL) {
        g_error("Failed to load animation: %s", error->message);
    }
    gtk_image_set_from_animation(GTK_IMAGE(ui->map), animation);
    gtk_widget_show(ui->map);
    

    gtk_widget_show_all(new_window);

    

    
    free(ui);
    ui = load_widgets(ui_builder, ssl);
    load_signal(ui_builder, ui);

    gtk_widget_hide(ui->turn);
    gtk_widget_hide(ui->mess_in_game);

    char temp[16];
    sprintf(temp, "g@b@l@%d", id);
    SSL_write(ssl, temp, strlen(temp));

    

    g_object_unref(ui_builder);
    g_source_remove(timer_mes);
    g_source_remove(timer_chats);
}

G_MODULE_EXPORT void acceptChange(GtkButton *btn, gpointer data){
    int image;

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->radio1))){
        image = 1;
    }
    else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->radio2))){
        image = 2;
    }
    else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->radio3))){
        image = 3;
    }
    else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->radio4))){
        image = 4;
    }
    else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->radio5))){
        image = 5;
    }
    else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->radio6))){
        image = 6;
    }

    const gchar *mes = gtk_entry_get_text(GTK_ENTRY(ui->change_name));
    if (strlen(mes) > 0){
        // write(STDOUT_FILENO, mes, strlen(mes));
        char temp[128];
        sprintf(temp, "C@%d@%s@%d", image, mes, id);
        SSL_write(ssl, temp, strlen(temp));
    }
    else{
        char temp[16];
        sprintf(temp, "C@n@%d@%d", image, id);
        SSL_write(ssl, temp, strlen(temp));
    }
    gtk_widget_hide(ui->popover);
    GtkBuilder *ui_builder;
    GtkWidget *new_window;

    ui_builder = gtk_builder_new_from_file("glade_img/gui.glade");

    if (ui_builder == NULL) {
        g_print("Failed to load GtkBuilder from file\n");
        return;
    }
    free(ui);
    ui = load_widgets(ui_builder, ssl);
    load_signal(ui_builder, ui);
    
    gtk_widget_show_all(ui->window_chat);

    g_object_unref(ui_builder);

    char temp[16];
    sprintf(temp, "c@%d", id);
    SSL_write(ssl, temp, strlen(temp));
    sprintf(temp, "u@%d", id);
    SSL_write(ssl, temp, strlen(temp));

}




G_MODULE_EXPORT void on_btn_reg_l(GtkButton *btn, gpointer data){
    GtkBuilder *ui_builder;
    GtkWidget *new_window;

    ui_builder = gtk_builder_new_from_file("glade_img/gui.glade");

    if (ui_builder == NULL) {
        g_print("Failed to load GtkBuilder from file\n");
        return;
    }

    new_window = GTK_WIDGET(gtk_builder_get_object(ui_builder, "registration_window"));
    GdkScreen *Screen = gtk_widget_get_screen(new_window);
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(provider, "style/gtkchat.css", NULL);
    gtk_style_context_add_provider_for_screen(Screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);

    gtk_widget_hide(GTK_WIDGET(data));
    gtk_widget_show_all(new_window);

    free(ui);
    ui = load_widgets(ui_builder, ssl);
    load_signal(ui_builder, ui);

    g_object_unref(ui_builder);
}

G_MODULE_EXPORT void Settings(GtkButton *btn, gpointer data){
    GtkBuilder *ui_builder;
    GtkWidget *new_window;

    ui_builder = gtk_builder_new_from_file("glade_img/gui.glade");

    if (ui_builder == NULL) {
        g_print("Failed to load GtkBuilder from file\n");
        return;
    }
    gtk_widget_hide(ui->window_chat);
    free(ui);
    ui = load_widgets(ui_builder, ssl);
    load_signal(ui_builder, ui);
    char *temp[16];
    sprintf(temp, "x@%d", id);
    SSL_write(ssl, temp, strlen(temp));
    
    gtk_widget_show_all(ui->popover);

}

G_MODULE_EXPORT void on_btn_log_r(GtkButton *btn, gpointer data){
    GtkBuilder *ui_builder;
    GtkWidget *new_window;

    ui_builder = gtk_builder_new_from_file("glade_img/gui.glade");
    if (ui_builder == NULL) {
        g_print("Failed to load GtkBuilder from file\n");
        return;
    }
    new_window = GTK_WIDGET(gtk_builder_get_object(ui_builder, "login_window"));
    GdkScreen *Screen = gtk_widget_get_screen(new_window);
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(provider, "style/gtkchat.css", NULL);
    gtk_style_context_add_provider_for_screen(Screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
    
    gtk_widget_hide(GTK_WIDGET(data));
    gtk_widget_show_all(new_window);

    free(ui);
    ui = load_widgets(ui_builder, ssl);
    load_signal(ui_builder, ui);
    
    g_object_unref(ui_builder);
}

G_MODULE_EXPORT void onBtnLogin(GtkButton *btn, gpointer data) {
    char buff[1024];

    const gchar *mes = gtk_entry_get_text(GTK_ENTRY(ui->login_l));
    const gchar *pass = gtk_entry_get_text(GTK_ENTRY(ui->password_l));

    if ((strlen(mes) == 0 || strlen(pass) == 0) || (strlen(mes) == 0 || strlen(pass) == 0)) gtk_label_set_label(ui->label_l, "All fields must be filled");
    else {
        sprintf(buff, "l@%s@%s", mes, pass);
        
        SSL_write(ssl,buff, strlen(buff));
    }
}

G_MODULE_EXPORT void onBtnRegistration(GtkButton *btn, gpointer data){
    char buff[1024];
    const gchar *mes = gtk_entry_get_text(GTK_ENTRY(ui->login_r));
    const gchar *pass = gtk_entry_get_text(GTK_ENTRY(ui->password_r));
    const gchar *repeat_pass = gtk_entry_get_text(GTK_ENTRY(ui->repeat_password_r));

    
    if (strlen(mes) == 0 || strlen(pass) == 0 || strlen(repeat_pass) == 0){
        gtk_label_set_label(ui->label_r, "All fields must be filled");
    }
    else if (strcmp(pass, repeat_pass) != 0){
        gtk_label_set_label(ui->label_r, "Password do not math");
    }
    else{
        sprintf(buff, "r@%s@%s", mes, pass);
        // write(STDOUT_FILENO, buff, strlen(buff));
        SSL_write(ssl, buff, strlen(buff));
    }
}

G_MODULE_EXPORT void onBtnMes(GtkButton *btn, gpointer data) {
    char buff[1024];
    const gchar *mes = gtk_entry_get_text(GTK_ENTRY(ui->entry_mes));

    if (strlen(mes) == 0){
        // write(STDOUT_FILENO, "mes == 0", strlen("mes == 0"));
    }
    else if (friend == 0){
        // write(STDOUT_FILENO, "chater == 0", strlen("chater == 0"));
    }
    else{
        sprintf(buff, "m@r@%d@%d@%s", id, friend, mes);
        SSL_write(ssl, buff, strlen(buff));
    }
}

G_MODULE_EXPORT void onBtnSearch(GtkButton *btn, gpointer data) {
    char buff[1024];
    const gchar *mes = gtk_entry_get_text(GTK_ENTRY(ui->search_name));

    sprintf(buff, "s@%s@%d", mes, id);
    SSL_write(ssl, buff, strlen(buff));
}


G_MODULE_EXPORT void onSearchName(GtkEntry *btn, gpointer data){
    char buff[1024];
    const gchar *mes = gtk_entry_get_text(GTK_ENTRY(ui->search_name));

    sprintf(buff, "s@%s@%d", mes, id);
    SSL_write(ssl, buff, strlen(buff));
}

G_MODULE_EXPORT void sendMessGame(GtkEntry *btn, gpointer data){
    char buff[1024];
    const gchar *mes = gtk_entry_get_text(GTK_ENTRY(ui->mess_in_game));
    write(STDOUT_FILENO, buff, strlen(buff));

    sprintf(buff, "g@M@%s@%d@%d@", mes, game, id);
    // write(STDOUT_FILENO, buff, strlen(buff));
    SSL_write(ssl, buff, strlen(buff));
    gtk_entry_set_text(GTK_ENTRY(ui->mess_in_game), "");
}


G_MODULE_EXPORT void onEntryMes(GtkEntry *btn, gpointer data){
    char buff[1024];
    const gchar *mes = gtk_entry_get_text(GTK_ENTRY(ui->entry_mes));

    if (strlen(mes) == 0){
        // write(STDOUT_FILENO, "mes == 0", strlen("mes == 0"));
    }
    else if (friend == 0){
        // write(STDOUT_FILENO, "chater == 0", strlen("chater == 0"));
    }
    else{
        sprintf(buff, "m@r@%d@%d@%s", id, friend, mes);
        SSL_write(ssl, buff, strlen(buff));
    }
    gtk_entry_set_text(GTK_ENTRY(ui->entry_mes), "");
}

G_MODULE_EXPORT void onBtnGame(GtkButton *btn, gpointer data){
    GtkBuilder *ui_builder;
    GtkWidget *new_window;

    ui_builder = gtk_builder_new_from_file("glade_img/gui.glade");
    if (ui_builder == NULL) {
        g_print("Failed to load GtkBuilder from file\n");
        return;
    }
    new_window = GTK_WIDGET(gtk_builder_get_object(ui_builder, "loby_window"));

    
    gtk_widget_hide(ui->window_player);
    gtk_widget_hide(ui->window_invite);
    gtk_widget_show_all(new_window);

    free(ui);
    ui = load_widgets(ui_builder, ssl);
    load_signal(ui_builder, ui);
    
    g_object_unref(ui_builder);
    char temp[16];
    sprintf(temp, "g@e@l@%d", id);
    SSL_write(ssl, temp, strlen(temp));

    sprintf(temp, "g@b@l@%d", id);
    SSL_write(ssl, temp, strlen(temp));
}

G_MODULE_EXPORT void checkInvite(GtkButton *btn, gpointer data){
    GtkBuilder *ui_builder;
    GtkWidget *new_window;

    ui_builder = gtk_builder_new_from_file("glade_img/gui.glade");
    if (ui_builder == NULL) {
        g_print("Failed to load GtkBuilder from file\n");
        return;
    }
    new_window = GTK_WIDGET(gtk_builder_get_object(ui_builder, "invite_window"));
    
    gtk_widget_hide(GTK_WIDGET(data));
    gtk_widget_show_all(new_window);

    free(ui);
    ui = load_widgets(ui_builder, ssl);
    load_signal(ui_builder, ui);
    
    g_object_unref(ui_builder);

    g_source_remove(timer_id);

    char temp[16];
    sprintf(temp, "g@c@%d", id);
    SSL_write(ssl, temp, strlen(temp));
}

G_MODULE_EXPORT void RefreshInvite(GtkButton *btn, gpointer data){
    char temp[16];
    sprintf(temp, "g@c@%d", id);
    SSL_write(ssl, temp, strlen(temp));
}


G_MODULE_EXPORT void on_play_activate(GtkButton *btn, gpointer data){
    char temp[32];
    sprintf(temp, "g@b@m@%d", game);
    SSL_write(ssl, temp, strlen(temp));
    // write(STDOUT_FILENO, temp, strlen(temp));
}

G_MODULE_EXPORT void entryInvitePlayer(GtkEntry *btn, gpointer data){
    const gchar *mes = gtk_entry_get_text(GTK_ENTRY(ui->entry_invite_player));

    char buf[512];
    sprintf(buf, "g@i@%s@%d", mes, game);
    SSL_write(ssl, buf, strlen(buf));
    gtk_entry_set_text(GTK_ENTRY(ui->entry_invite_player), "");
}

G_MODULE_EXPORT void turnUp(GtkButton *btn, gpointer data){
    //write(STDOUT_FILENO, "SPACE", strlen("SPACE"));
    char temp[16];
    sprintf(temp, "g@t@%d@%d", game, id);
    SSL_write(ssl, temp, strlen(temp));
    // write(STDOUT_FILENO, temp, strlen(temp));
}
G_MODULE_EXPORT void BtnYes(GtkButton *btn, gpointer data){
    //write(STDOUT_FILENO, "SPACE", strlen("SPACE"));
    char temp[16];
    sprintf(temp, "g@b@y@%d@%d", game, id);
    SSL_write(ssl, temp, strlen(temp));
    // write(STDOUT_FILENO, temp, strlen(temp));
}
G_MODULE_EXPORT void BtnNo(GtkButton *btn, gpointer data){
    //write(STDOUT_FILENO, "SPACE", strlen("SPACE"));
    char temp[16];
    sprintf(temp, "g@b@n@%d@%d", game, id);
    SSL_write(ssl, temp, strlen(temp));
    // write(STDOUT_FILENO, temp, strlen(temp));
}

void settingsImage(char *str){
    char temp_img[8];
    char name[128];
    bool flag = false;
    int idx = 0;
    for (int i = 0; i < strlen(str); i++){
        if (str[i] == '@' && !flag){
            temp_img[i] = '\0';
            flag = true;
        }
        else if (str[i] == '@' && flag){
            name[idx] = '\0';
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
    int img = atoi(temp_img);
    char temp[128];
    sprintf(temp, "./img/avatar/%dimage.jpg", img);
    gtk_image_set_from_file(GTK_IMAGE(ui->your_img), temp);
    gtk_label_set_text(ui->your_name, name);

}

void scroll_in_down(){
    GtkAdjustment* v_adjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(ui->scroll_window_mes));

    // получаем максимальное значение настройки "upper"
    double upper = gtk_adjustment_get_upper(v_adjustment);

    // устанавливаем значение настройки "value" на максимальное значение
    gtk_adjustment_set_value(v_adjustment, upper);
}

void on_button_clicked(GtkWidget *widget, gpointer data) {  //EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE
        
    int id_chater = data;
    char buff[264];
    gtk_container_foreach(GTK_CONTAINER(ui->grid_on_mes), (GtkCallback) gtk_widget_destroy, NULL);
    friend = id_chater;

    sprintf(buff, "w@p@%d@%d", id, friend);
    
    SSL_write(ssl, buff, strlen(buff));

    sprintf(buff, "W@%d@%d", id, friend);

    SSL_write(ssl, buff, strlen(buff));
}
void on_delete_clicked(GtkButton *button, gpointer data) {  //EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE
    int id_chater = data;
    char buff[264];
    

    friend = id_chater;

    sprintf(buff, "d@%d@%d@", id, friend);
    
    SSL_write(ssl, buff, strlen(buff));
    // friend = 0;
}

void accept_invite(GtkButton *button, gpointer data) {  //EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE
    game = data;


    GtkBuilder *ui_builder;
    GtkWidget *new_window;

    char temp[16];
    sprintf(temp, "g@e@l@%d", id);
    SSL_write(ssl, temp, strlen(temp));

    sprintf(temp, "g@a@%d@%d", game, id);
    SSL_write(ssl, temp, strlen(temp));

    g_timeout_add(1000, updatePlayer, NULL);
}

void RecevChat(){
    char temp[16];
    sprintf(temp, "u@%d", id);
    SSL_write(ssl, temp, strlen(temp));
    timer_chats = g_timeout_add(1000, RecevChat, NULL);
    // write(STDOUT_FILENO, buf, strlen(bytes));
}
void RecevMes(){
    char temp[16];
    sprintf(temp, "w@%d@%d", id, friend);
    SSL_write(ssl, temp, strlen(temp));
    timer_mes = g_timeout_add(1000, RecevMes, NULL);
}

void show_chat_window(char *str)
{
    char *t = &str[2];
    id = atoi(t);
    
    
    GtkBuilder *ui_builder;
    GtkWidget *new_window;

    ui_builder = gtk_builder_new_from_file("glade_img/gui.glade");
    if (ui_builder == NULL) {
        g_print("Failed to load GtkBuilder from file\n");
        return;
    }
    new_window = GTK_WIDGET(gtk_builder_get_object(ui_builder, "chat_window"));
    GdkScreen *Screen = gtk_widget_get_screen(new_window);
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(provider, "style/gtkchat.css", NULL);
    gtk_style_context_add_provider_for_screen(Screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);

    
    gtk_widget_hide(ui->window_l);
    gtk_widget_hide(ui->window_r);
    gtk_widget_show_all(new_window);

    free(ui);
    ui = load_widgets(ui_builder, ssl);
    load_signal(ui_builder, ui);
    
    g_object_unref(ui_builder);
    

    char temp[16];
    sprintf(temp, "c@%d", id);
    SSL_write(ssl, temp, strlen(temp));
    sprintf(temp, "u@%d", id);
    SSL_write(ssl, temp, strlen(temp));

    free(str);
}

void printMes(char *str){
    str[5] = '\0';
    GtkWidget *label = gtk_label_new(str);
    gtk_box_pack_end(GTK_BOX(ui->box_in_game), label, FALSE, FALSE, 0);
    // write(STDOUT_FILENO, str, strlen(str));
}

void showChats(char *str){
    char id_chater[10];
    char name[512];
    bool flag = false;
    int idx = 0;
    char *image;

    for (int i = 0; i < strlen(str); i++){
        if (str[i] == '@' && !flag) {
            id_chater[i] = '\0';
            flag = true;
        }
        else if (str[i] == '@' && flag){
            name[idx] = '\0';
            image = &str[i + 1];
            break;
        }
        else if (!flag) id_chater[i] = str[i];
        else {
            name[idx] = str[i];
            idx++;
        }
    }


    GtkWidget *button = gtk_button_new_with_label(name);
    char temp[64];
    sprintf(temp, "./img/avatar/%dimage.jpg", atoi(image));
    GtkWidget *image_1 = gtk_image_new_from_file(temp);
    GtkWidget *delete_chat = gtk_button_new_with_label(NULL);
    GtkWidget *trashbin = gtk_image_new_from_file("./img/avatar/rubbish.png");
    gtk_container_add(GTK_CONTAINER(delete_chat), trashbin);




    
    GtkStyleContext *context = gtk_widget_get_style_context(button);
    gtk_widget_set_name(button, "nameChats");
    gtk_style_context_add_class(context, "nameChats");
    


    // gtk_widget_set_name(image_1, "images");


    int count_i = 0;
    while (gtk_grid_get_child_at(GTK_GRID(ui->grid_on_chats), 0, count_i) != NULL) count_i++;
    gtk_grid_attach(GTK_GRID(ui->grid_on_chats), image_1, 0, count_i, 1, 1);
    gtk_grid_attach(GTK_GRID(ui->grid_on_chats), button, 1, count_i, 1, 1);
    gtk_grid_attach(GTK_GRID(ui->grid_on_chats), delete_chat, 2, count_i, 1, 1);

    g_signal_connect(button, "clicked", G_CALLBACK(on_button_clicked), atoi(id_chater));

    g_signal_connect(delete_chat, "clicked", G_CALLBACK(on_delete_clicked), atoi(id_chater));

    gtk_widget_show(button);
    gtk_widget_show(image_1);
    gtk_widget_show(delete_chat);
    gtk_widget_show(trashbin);

    free(str);
}

void changeMes(GtkEntry *entry, gpointer data){
    char *str = data;
    char *mes = gtk_entry_get_text(entry);
    char temp[512];
    sprintf(temp, "M@%s@%s@%d@%d", str, mes, id, friend);
    SSL_write(ssl, temp, strlen(temp));
    gtk_widget_hide(entry);
    gtk_widget_show(ui->entry_mes);
}

void on_edit(GtkMenuItem *menu_item, gpointer data){
    char *str = gtk_widget_get_name(data);
    char *label = gtk_button_get_label(GTK_BUTTON(data));
    gtk_widget_hide(ui->entry_mes);
    GtkWidget *entry = gtk_entry_new();
    gtk_widget_set_name(entry, "login_entry");
    gtk_grid_attach(GTK_GRID(ui->chatGrid), entry, 6, 9, 4, 1);
    gtk_entry_set_text(GTK_ENTRY(entry), label);
    gtk_widget_show(entry);
    g_signal_connect(entry, "activate", G_CALLBACK(changeMes), str);
}

void on_delete(GtkMenuItem *menu_item, gpointer data){
    char *str = gtk_widget_get_name(data);
    char temp[32];
    sprintf(temp, "d@m@%s@%d@%d", str, id, friend);
    SSL_write(ssl, temp, strlen(temp));
}


static void on_btn_clicked(GtkButton *button, GdkEventButton *event, gpointer data) {
    if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
        GtkWidget *menu = gtk_menu_new();

        GtkWidget *edit = gtk_menu_item_new_with_label("Edit");
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), edit);

        g_signal_connect(edit, "activate", G_CALLBACK(on_edit), data);


        GtkWidget *delete = gtk_menu_item_new_with_label("Delete");
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), delete);

        g_signal_connect(delete, "activate", G_CALLBACK(on_delete), data);

        gtk_widget_show_all(menu);
        gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent*)event);
    }
}

void showMes(char *str){
    // write(STDOUT_FILENO, str, strlen(str));
    char id_chater[10];
    char name[512];
    bool flag = false;
    int idx = 0;
    char *data;
    char id_mes[16];
    char *str_1;
    for (int i = 0; i < strlen(str); i++){
        if (str[i] == '@'){
            id_mes[i] = '\0';
            str_1 = &str[i + 1];
            break; 
        }
        else id_mes[i] = str[i];
    }
    // write(STDOUT_FILENO, id_mes, strlen(id_mes));

    for (int i = 0; i < strlen(str_1); i++){
        if (str_1[i] == '@' && !flag) {
            id_chater[i] = '\0';
            flag = true;
        }
        else if (str_1[i] == '@' && flag){
            name[idx] = '\0';
            data = &str_1[i + 1];
            break;
        }
        else if (!flag) id_chater[i] = str_1[i];
        else {
            name[idx] = str_1[i];
            idx++;
        }
    }


    GtkWidget *label = gtk_button_new_with_label(name);
    gtk_widget_set_name(label, id_mes);

    GtkWidget *label_1 = gtk_label_new(NULL);
    GtkWidget *label_data = gtk_label_new(data);
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    

    int count_i = 0;
    while (gtk_grid_get_child_at(GTK_GRID(ui->grid_on_mes), 1, count_i) != NULL || gtk_grid_get_child_at(GTK_GRID(ui->grid_on_mes), 0, count_i) != NULL) count_i++;
    if (atoi(id_chater) == id){
        gtk_grid_attach(GTK_GRID(ui->grid_on_mes), box, 1, count_i, 1, 1);
        gtk_button_set_alignment(GTK_BUTTON(label), 0.7, 0.5);
        gtk_label_set_xalign(GTK_LABEL(label_data), 0.9);
        gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
        gtk_box_pack_end(GTK_BOX(box), label_data, FALSE, FALSE, 0);
        g_signal_connect(label, "button-press-event", G_CALLBACK(on_btn_clicked), label);
        gtk_grid_attach(GTK_GRID(ui->grid_on_mes), label_1, 0, count_i, 1, 1);
    }
    else{ 
        gtk_grid_attach(GTK_GRID(ui->grid_on_mes), box, 0, count_i, 1, 1);
        gtk_button_set_alignment(GTK_BUTTON(label), 0.3, 0.5);
        gtk_label_set_xalign(GTK_LABEL(label_data), 0.1);
        gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
        gtk_box_pack_end(GTK_BOX(box), label_data, FALSE, FALSE, 0);
        gtk_grid_attach(GTK_GRID(ui->grid_on_mes), label_1, 1, count_i, 1, 1);
    }
    
    

    gtk_widget_show(box);
    gtk_widget_show(label_data);
    gtk_widget_show(label);
    gtk_widget_show(label_1);

    free(str);
}

void notRazrehau(char *str){
    if (str[2] == 'r') gtk_label_set_label(ui->label_r, "User is already registered");
    else if (str[2] == 'l') {
        if (str[4] == 'c') gtk_label_set_label(ui->label_l, "Check the correctness of the password and login");
        else if (str[4] == 'b') gtk_label_set_label(ui->label_l, "Someone is already online from this account");
    }
    free(str);
}

int searchId(char *str){
    str += 2;
    return atoi(str);
}

char *delete_ykazatel(char *str){
    char *temp = malloc(1024 * sizeof(char));
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

void showPlayers(char *str){
    char temp_img[8];
    char name[36];
    char *temp_id;
    bool flag = false;
    int idx = 0;
    for (int i = 0; i < strlen(str); i++){
        if (str[i] == '@' && !flag){
            name[i] = '\0';
            flag = true;
        }
        else if (str[i] == '@' && flag){
            temp_img[idx] = '\0';
            temp_id = &str[i + 1];
            break;
        }
        else if (!flag){
            name[i] = str[i];
        }
        else if (flag){
            temp_img[idx] = str[i];
            idx++;
        }
    }
    // write(STDOUT_FILENO, temp_img, strlen(temp_img));
    // write(STDOUT_FILENO, "\n", 1);
    // write(STDOUT_FILENO, name, strlen(name));
    // write(STDOUT_FILENO, "\n", 1);
    // write(STDOUT_FILENO, temp_id, strlen(temp_id));
    // write(STDOUT_FILENO, "\n", 1);
    gtk_widget_hide(ui->yes);
    gtk_widget_hide(ui->no);

    
    char temp[128];
    if (atoi(temp_id) == 0) {
        gtk_widget_set_opacity(ui->lb_pl_1, 1);
        gtk_widget_set_opacity(ui->image_1, 1);
        gtk_label_set_text(GTK_LABEL(ui->lb_pl_1), name);
        sprintf(temp, "./img/avatar/%dperson1.png", atoi(temp_img));
        gtk_image_set_from_file(GTK_IMAGE(ui->image_1), temp);
    }
    else if (atoi(temp_id) == 1){
        gtk_widget_set_opacity(ui->lb_pl_2, 1);
        gtk_widget_set_opacity(ui->image_2, 1);
        gtk_label_set_text(GTK_LABEL(ui->lb_pl_2), name);
        sprintf(temp, "./img/avatar/%dperson2.png", atoi(temp_img));
        gtk_image_set_from_file(GTK_IMAGE(ui->image_2), temp);
    }
    else if (atoi(temp_id) == 2) {
        gtk_widget_set_opacity(ui->lb_pl_3, 1);
        gtk_widget_set_opacity(ui->image_3, 1);
        gtk_label_set_text(GTK_LABEL(ui->lb_pl_3), name);
        sprintf(temp, "./img/avatar/%dperson3.png", atoi(temp_img));
        gtk_image_set_from_file(GTK_IMAGE(ui->image_3), temp);
    }
    free(str);
}

void hidePlayers(int i){
    if (i == 0) {
        gtk_widget_set_opacity(ui->lb_pl_1, 0);
        gtk_widget_set_opacity(ui->image_1, 0);
        // gtk_label_set_text(GTK_LABEL(ui->lb_pl_1), name);
        // sprintf(temp, "./img/avatar/%dimage.jpg", atoi(temp_img));
        // gtk_image_set_from_file(GTK_IMAGE(ui->image_1), temp);
    }
    else if (i == 1){
        gtk_widget_set_opacity(ui->lb_pl_2, 0);
        gtk_widget_set_opacity(ui->image_2, 0);
        // gtk_label_set_text(GTK_LABEL(ui->lb_pl_2), name);
        // sprintf(temp, "./img/avatar/%dimage.jpg", atoi(temp_img));
        // gtk_image_set_from_file(GTK_IMAGE(ui->image_2), temp);
    }
    else if (i == 2) {
        gtk_widget_set_opacity(ui->lb_pl_3, 0);
        gtk_widget_set_opacity(ui->image_3, 0);
        // gtk_label_set_text(GTK_LABEL(ui->lb_pl_3), name);
        // sprintf(temp, "./img/avatar/%dimage.jpg", atoi(temp_img));
        // gtk_image_set_from_file(GTK_IMAGE(ui->image_3), temp);
    }
}

void Podguzka(char *data){

    char temp[16];
    sprintf(temp, "g@r@%d@", game);
    SSL_write(ssl, temp, strlen(temp));
    // write(STDOUT_FILENO, temp, strlen(temp));
    // write(STDOUT_FILENO, "\n", strlen("\n"));
    // printf("SSL pointer: %p\n", (void*) ssl);
    timer_podg = g_timeout_add(1000, Podguzka, NULL);

}

void PodguzkaMess(char *data){
    
    char temp[16];
    sprintf(temp, "g@M@U@%d@%d@", game, id);
    SSL_write(ssl, temp, strlen(temp));
    // write(STDOUT_FILENO, temp, strlen(temp));
    // write(STDOUT_FILENO, "\n", strlen("\n"));
    timer_podg_mes = g_timeout_add(1000, PodguzkaMess, NULL);
}


void updatePlayer(){
    char temp[16];
    sprintf(temp, "g@u@%d", game);
    // write(STDOUT_FILENO, temp, strlen(temp));
    // write(STDOUT_FILENO, "|||||\n", strlen(temp));
    SSL_write(ssl, temp, strlen(temp));
    timer_id = g_timeout_add(1000, updatePlayer, NULL);
}

void showLobby(char *str){
    char id_game[8];
    char name_game[512];
    bool flag = false;
    int count = 0;

    for (int i = 0; i < strlen(str); i++){
        if (str[i] == '@' && !flag){
            flag = true;
            id_game[i] = '\0';
        }
        else if (str[i] == '@' && flag){
            name_game[count] = ',';
            count++;
            name_game[count] = ' ';
            count++;
        }
        else if (!flag){
            id_game[i] = str[i];
        }
        else if (flag){
            name_game[count] = str[i];
            count++;
        }
    }
    name_game[count] = '\0';

    gtk_widget_set_opacity(ui->box_for_invite, 1);

    GtkWidget *button = gtk_button_new_with_label(name_game);

    gtk_box_pack_start(GTK_BOX(ui->box_for_invite), button, FALSE, FALSE, 0);


    g_signal_connect(button, "clicked", G_CALLBACK(accept_invite), atoi(id_game));


    gtk_widget_show(button);

    free(str);
}

void showMap(){

    g_source_remove(timer_id);

    char temp[16];
    sprintf(temp, "g@m@%d", id);
    SSL_write(ssl, temp, strlen(temp));
}

void printMesInGame(char *str) {
    GtkWidget *label = gtk_label_new(str);

// Устанавливаем значение свойства xalign равным 0,0
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);

    // Добавляем метку на родительский виджет
    gtk_container_add(GTK_CONTAINER(ui->box_in_game), label);
    gtk_widget_show(label);
    free(str);
}

int loadWidgetGame(void *data){
    gtk_widget_hide(ui->entry_invite_player);
    gtk_widget_hide(ui->btn_refresh);
    gtk_widget_hide(ui->box_for_invite);
    gtk_widget_hide(ui->entry_invite_player);
    gtk_widget_show(ui->mess_in_game);
    gtk_widget_show(ui->turn);
    gtk_widget_show(ui->box_in_game);
    gtk_widget_set_opacity(ui->cube, 1);
    
    char *str = (char *) data;

    char c[8];
    int j = 0;
    for (int i = 0; i < strlen(str); i++){
        if (str[i] == '@') {
            j++;
            break;
        }
        else {
            c[i] = str[i];
            j++;
        }
    }
    int count = atoi(c);
    char temp[16];

    for (int count_dog = 0, i = 0; count_dog != count; j++, i++){
        if (str[j] == '@'){
            temp[i] = '\0';
            count_dog++;
            i = -1;
            switch (count_dog){
                case 1:
                    gtk_label_set_text(GTK_LABEL(ui->money_pl_1), "1500");
                    gtk_widget_set_opacity(ui->pers_1, 1);
                    break;
                case 2:
                    gtk_label_set_text(GTK_LABEL(ui->money_pl_2), "1500");
                    gtk_widget_set_opacity(ui->pers_2, 1);
                    break;
                case 3:
                    gtk_label_set_text(GTK_LABEL(ui->money_pl_3), "1500");
                    gtk_widget_set_opacity(ui->pers_3, 1);
                    break;
            }
        }
        else {
            temp[i] = str[j];
        }
    }
}

void reloadWidgets(char *str) {
    char buff_count[8];
    // write(STDOUT_FILENO, str, strlen(str));
    

    for (int i = 0; i < strlen(str); i++){
        if(str[i] == '@'){
            buff_count[i] = '\0';
            str += i + 1;
            break;
        }
        else buff_count[i] = str[i];
        
    }
    int count = atoi(buff_count);
    char pos[8];
    int p = 0;
    for (int j = 0; p < count; str++){
        if (str[0] == '@'){
            pos[j] = '\0';
            j = 0;
            p++;
            // str++;
            int x = 335;
            int y = 20;
            switch (p)
            {
                case 1:
                    x = 345;
                    y = 0;
                    for (int i = 0; i <= atoi(pos); i++){
                        if (i == 1 || i == 9) x += 81;
                        else if (i > 1 && i < 9) x += 54;
                        else if (i == 10 || i == 18) y += 81;
                        else if (i > 10 && i < 18) y += 54;         
                        else if (i == 19 || i == 27) x -= 81;
                        else if (i > 19 && i < 27) x -=54;      
                        else if (i == 28) y -= 81;
                        else if (i > 28 && i < 36) y -= 54;
                    }
                    gtk_fixed_move(GTK_FIXED(ui->fixed), ui->pers_1, x, y);
                    break;
                case 2:
                    x = 300;
                    y = 30;
                    for (int i = 0; i <= atoi(pos); i++){
                        if (i == 1 || i == 9) x += 81;
                        else if (i > 1 && i < 9) x += 54;
                        else if (i == 10 || i == 18) y += 81;
                        else if (i > 10 && i < 18) y += 54;         
                        else if (i == 19 || i == 27) x -= 81;
                        else if (i > 19 && i < 27) x -=54;      
                        else if (i == 28) y -= 81;
                        else if (i > 28 && i < 36) y -= 54;
                    }
                    gtk_fixed_move(GTK_FIXED(ui->fixed), ui->pers_2, x, y);
                    break;
                case 3:
                    x = 300;
                    y = 0;
                    for (int i = 0; i <= atoi(pos); i++){
                        if (i == 1 || i == 9) x += 81;
                        else if (i > 1 && i < 9) x += 54;
                        else if (i == 10 || i == 18) y += 81;
                        else if (i > 10 && i < 18) y += 54;         
                        else if (i == 19 || i == 27) x -= 81;
                        else if (i > 19 && i < 27) x -=54;      
                        else if (i == 28) y -= 81;
                        else if (i > 28 && i < 36) y -= 54;
                    }
                    gtk_fixed_move(GTK_FIXED(ui->fixed), ui->pers_3, x, y);
                    break;
                case 4:
                    x = 375;
                    y = 20;
                    for (int i = 0; i <= atoi(pos); i++){
                        if (i == 1 || i == 9) x += 81;
                        else if (i > 1 && i < 9) x += 54;
                        else if (i == 10 || i == 18) y += 81;
                        else if (i > 10 && i < 18) y += 54;         
                        else if (i == 19 || i == 27) x -= 81;
                        else if (i > 19 && i < 27) x -=54;      
                        else if (i == 28) y -= 81;
                        else if (i > 28 && i < 36) y -= 54;
                    }
                    gtk_fixed_move(GTK_FIXED(ui->fixed), ui->pers_4, x, y);
                    break;
            }
        }
        else {
            pos[j] = str[0];
            j++;
        }
    }
    for (; p < 4; p++, str += 2){}
    // write(STDOUT_FILENO, str, strlen(str));

    str += 2;
    char *temp[2];
    temp[0] = str[0];
    temp[1] = '\0';
    // write(STDOUT_FILENO, str, strlen(str));
    int dice = atoi(temp);
    str += 4;

    // write(STDOUT_FILENO, str, strlen(str));
            // write(STDOUT_FILENO, "||\n", strlen("||\n"));
            // write(STDOUT_FILENO, str,  strlen(str));

    switch (dice){
        case 1:
            gtk_image_set_from_file(GTK_IMAGE(ui->cube), "./img/1 Dice.png");
            break;
        case 2:
            gtk_image_set_from_file(GTK_IMAGE(ui->cube), "./img/2 Dice.png");
            break;
        case 3:
            gtk_image_set_from_file(GTK_IMAGE(ui->cube), "./img/3 Dice.png");
            break;
        case 4:
            gtk_image_set_from_file(GTK_IMAGE(ui->cube), "./img/4 Dice.png");
            break;
        case 5:
            gtk_image_set_from_file(GTK_IMAGE(ui->cube), "./img/5 Dice.png");
            break;
        case 6:
            gtk_image_set_from_file(GTK_IMAGE(ui->cube), "./img/6 Dice.png");
            break;
    }
    // write(STDOUT_FILENO, str, strlen(str));
    
    p = 0;
    for (int i = 0; p < count; str++){
        if (str[0] == '@'){
            pos[i] = '\0';
            i = 0;
            p++;
            switch (p)
            {
                case 1:
                    gtk_label_set_text(GTK_LABEL(ui->money_pl_1), pos);
                    break;
                case 2:
                    gtk_label_set_text(GTK_LABEL(ui->money_pl_2), pos);
                    break;
                case 3:
                    gtk_label_set_text(GTK_LABEL(ui->money_pl_3), pos);
                    break;
            }
        }
        else {
            pos[i] = str[0];
            i++;
        }
    }
    for (; p < 4; p++, str += 2){}
    //write(STDOUT_FILENO, str, strlen(str));
    if (str[0] == 'n'){
        gtk_widget_set_opacity(ui->shans, 0);
        gtk_image_clear(GTK_IMAGE(ui->shans));
        gtk_widget_hide(ui->yes);
        gtk_widget_hide(ui->no);
    }
    else if (str[0] == 'b'){
        gtk_widget_set_opacity(ui->shans, 1);
        gtk_image_set_from_file(GTK_IMAGE(ui->shans), "./img/Are_you_shure.png");
        gtk_widget_show(ui->yes);
        gtk_widget_show(ui->no);
    }
    else if (str[0] == 's'){
        switch(str[2]){
            case '1':
                gtk_widget_set_opacity(ui->shans, 1);
                gtk_image_set_from_file(GTK_IMAGE(ui->shans), "./img/Lost 50 money.png");
                break;
            case '2':
                gtk_widget_set_opacity(ui->shans, 1);
                gtk_image_set_from_file(GTK_IMAGE(ui->shans), "./img/Lost 100 money.png");
                break;
            case '3':
                gtk_widget_set_opacity(ui->shans, 1);
                gtk_image_set_from_file(GTK_IMAGE(ui->shans), "./img/Lost 150 money.png");
                break;
            case '4':
                gtk_widget_set_opacity(ui->shans, 1);
                gtk_image_set_from_file(GTK_IMAGE(ui->shans), "./img/Get 50 money.png");
                break;
            case '5':
                gtk_widget_set_opacity(ui->shans, 1);
                gtk_image_set_from_file(GTK_IMAGE(ui->shans), "./img/Got 100 money.png");
                break;
            case '6':
                gtk_widget_set_opacity(ui->shans, 1);
                gtk_image_set_from_file(GTK_IMAGE(ui->shans), "./img/Got 150 money.png");
                break;
        }
    }
}

void prUp(char *str){
    char name[32];
    char image[32];
    bool flag = false;
    int idx = 0;
    for (int i = 0; i < strlen(str); i++){
        if (str[i] == '@' && !flag){
            name[i] = '\0';
            flag = true;
        }
        else if (!flag){
            name[i] = str[i];
        }
        else if(flag){
            image[idx] = str[i];
            idx++;
        }
    }   
    image[idx] = '\0';
    gtk_label_set_text(ui->nameProfile, name);
    char temp[64];
    sprintf(temp, "./img/avatar/%simage.jpg", image);
    // write(STDOUT_FILENO, temp, strlen(temp));
    gtk_image_set_from_file(ui->profile, temp);
    gtk_widget_set_opacity(ui->nameProfile, 1);
    gtk_widget_set_opacity(ui->profile, 1);
}


void Servlet(){
    char buf[4096] = {0};
    int bytes; 
        
    while(1){
        bytes = SSL_read(ssl, buf, sizeof(buf));
        if ( bytes > 0 )
        {
            buf[bytes] = '\0';
            if (buf[0] == 'Y'){
                char *str = strdup(buf);
                g_idle_add(show_chat_window, str);
                timer_mes = g_timeout_add(1000, RecevMes, NULL);
                timer_chats = g_timeout_add(1000, RecevChat, NULL);
            }
            else if (buf[0] == 'c'){
                char *str = strdup(buf);
                char temp[1024];
                // char temp_1[36];
                // SSL_read(ssl, temp_1, sizeof(temp_1));
                // write(STDOUT_FILENO, temp_1, strlen(temp_1));
                for (int i = 0; i < searchId(str); i++){
                    if (i == 0) gtk_container_foreach(GTK_CONTAINER(ui->grid_on_mes), (GtkCallback) gtk_widget_destroy, NULL);
                    int bt = SSL_read(ssl, temp, sizeof(temp));
                    if (bt > 0){
                        temp[bt] = '\0';
                        char *str_mes_info = strdup(temp);
                        g_idle_add(showMes, str_mes_info);
                        // write(STDOUT_FILENO, temp, bt);
                        // write(STDOUT_FILENO, "\n", 1);
                        g_idle_add(scroll_in_down, NULL);
                        memset(temp, 0, sizeof(temp));
                    }
                    else {
                        ERR_print_errors_fp(stderr);
                        break;
                    }
                }
                
                //g_idle_add(update_window, ui);
                //write(STDOUT_FILENO, buf, strlen(buf));
                //g_idle_add(razbor_chatov, str);
            }
            else if (buf[0] == 'S'){
                char *str = delete_ykazatel(buf);
                g_idle_add(prUp, str);
            }
            else if (buf[0] == 'm'){
                char *str = strdup(buf);
                // write(STDOUT_FILENO, buf, strlen(buf));
                g_idle_add(showMes, str);
            }
            else if (buf[0] == 'N'){
                char *str = strdup(buf);
                g_idle_add(notRazrehau, str);
            }
            else if (buf[0] == 'n'){
                char *str = strdup(buf);
                char temp[1024];
                for (int i = 0; i < searchId(str); i++){
                    if (i == 0) gtk_container_foreach(GTK_CONTAINER(ui->grid_on_chats), (GtkCallback) gtk_widget_destroy, NULL);
                    int bt = SSL_read(ssl, temp, sizeof(temp));
                    if (bt > 0){
                        temp[bt] = '\0';
                        char *str_chats_info = strdup(temp);
                        g_idle_add(showChats, str_chats_info);
                        // sprintf(str_chats_info, "%s|%d", str_chats_info, i);
                        // write(STDOUT_FILENO, str_chats_info, strlen(str_chats_info));
                        // write(STDOUT_FILENO, "\n", 1);
                        memset(temp, 0, sizeof(temp));
                    }
                    else {
                        ERR_print_errors_fp(stderr);
                        break;
                    }
                }
            }
            else if (buf[0] == 's'){
                char *temp = delete_ykazatel(buf);
                settingsImage(temp);
            }
            else if (buf[0] == 'g'){
                char *str = delete_ykazatel(buf);
                if (str[0] == 'i'){
                    game = searchId(str);
                    free(str);
                    char temp[16];
                    sprintf(temp, "g@u@%d", game);
                    SSL_write(ssl, temp, strlen(temp));
                    timer_id = g_timeout_add(1000, updatePlayer, NULL);
                }
                else if(str[0] == 'c'){
                    // write(STDOUT_FILENO, str, strlen(str));
                    // write(STDOUT_FILENO, "\n", 1);
                    char temp[1024];
                    int i = 0;
                    for (; i < searchId(str); i++){
                        // if (i == 0){
                        //     if (gtk_container_get_children(GTK_CONTAINER(ui->box_label))) gtk_container_foreach(GTK_CONTAINER(ui->box_label), (GtkCallback) gtk_label_set_text, "inviting...");
                        // }
                        int bt = SSL_read(ssl, temp, sizeof(temp));
                        if (bt > 0){
                            temp[bt] = '\0';
                            char *str_player_info = strdup(temp);
                            sprintf(str_player_info, "%s@%d", str_player_info, i);
                            g_idle_add(showPlayers, str_player_info);
                            // write(STDOUT_FILENO, str_player_info, strlen(str_player_info));
                            // write(STDOUT_FILENO, "\n", 1);
                            memset(temp, 0, sizeof(temp));
                        }
                        else {
                            ERR_print_errors_fp(stderr);
                            break;
                        }
                    }
                    // i++;
                    for (; i < 3; i++){
                        // write(STDOUT_FILENO, "hey", strlen("hey"));
                        // write(STDOUT_FILENO, "\n", 1);
                        // char t[6];
                        // sprintf(t, "|%i|", i);
                        // write(STDOUT_FILENO, t, strlen(t));
                        // write(STDOUT_FILENO, "\n", 1);
                        g_idle_add(hidePlayers, i);
                    }
                }
                else if (str[0] == 'l'){
                    char temp[1024];
                    for (int i = 0; i < searchId(str); i++){
                        if (i == 0) gtk_container_foreach(GTK_CONTAINER(ui->box_for_invite), (GtkCallback) gtk_widget_destroy, NULL);
                        int bt = SSL_read(ssl, temp, sizeof(temp));
                        if (bt > 0){
                            temp[bt] = '\0';
                            char *str_lobby_info = strdup(temp);
                            //sprintf(str_lobby_info, "%s@%d", str_lobby_info, i);
                            g_idle_add(showLobby, str_lobby_info);
                            // write(STDOUT_FILENO, str_lobby_info, bt);
                            // write(STDOUT_FILENO, "\n", 1);
                            memset(temp, 0, sizeof(temp));

                        }
                        else {
                            ERR_print_errors_fp(stderr);
                            break;
                        }
                    }
                }
                else if (str[0] == 'm'){
                    char *temp = delete_ykazatel(str);
                    if (temp[0] == 'u'){
                        g_idle_add(reloadWidgets, delete_ykazatel(temp));
                        free(temp);
                    }
                    else{
                        g_idle_add(showMap, NULL);
                        char temp[16];
                        sprintf(temp, "g@m@%d", game);
                        SSL_write(ssl, temp, strlen(temp));
                    }
                }
                else if (str[0] == 'M'){
                    char *temp = delete_ykazatel(str);
                    // write(STDOUT_FILENO, temp, strlen(temp));
                    g_idle_add(printMesInGame, temp);
                } 
                else if (str[0] == 'h'){
                    char *temp = delete_ykazatel(str);
                    free(str);
                    g_idle_add(loadWidgetGame, temp);
                    // write(STDOUT_FILENO, "oao", strlen("oao"));
                    timer_podg = g_timeout_add(1000, Podguzka, NULL);
                    timer_podg_mes = g_timeout_add(1000, PodguzkaMess, NULL);
                }
            }
            // write(STDOUT_FILENO, buf, bytes);
            // write(STDOUT_FILENO, "\n", 1);
            memset(buf, 0, sizeof(buf));
        }
        else
        {
            printf("Error reading data from SSL connection\n");
            // Закрытие текущего SSL-соединения и освобождение ресурсов
            // SSL_shutdown(ssl);
            // SSL_free(ssl);
            // close(sockfd);
            while (1) {
                // Ожидание перед попыткой установить новое соединение
                // printf("Trying to reconnect in 1 second...\n");
                // write(STDOUT_FILENO, "yyyyy", strlen("yyyyy"));
                sleep(1);
                // Установка нового соединения
                
                sockfd = OpenConnection("127.0.0.1", 4444);
                ssl = SSL_new(ctx);
                SSL_set_fd(ssl, sockfd);
                if (SSL_connect(ssl) != 1) {
                    // printf("Error establishing SSL connection\n");
                    continue; // Попробовать еще раз
                }
                // Успешный реконнект
                // printf("Reconnected successfully\n");
                // printf("id= %d, game = %d\n", id, game);
                
                break;
            }
            
            char temp[16];
            sprintf(temp, "g@u@%d", game);
            SSL_write(ssl, temp, strlen(temp));
            // printf("SSL pointer: %p\n", (void*) ssl);
            memset(buf, 0, sizeof(buf));

        }
    }
}

int main(int argc, char *argv[])
{
    // SSL_CTX *ctx;
    // int server;
    char buf[1024];
    char acClientRequest[2048] = {0};
    int bytes;
    char *hostname, *portnum;
    if ( argc != 3)
    {
        printf("usage: ./uchat <host> <port>");
        exit(0);
    }
    signal(SIGINT, sigint_handler);
    SSL_library_init();
    portnum=argv[2];
    ctx = InitCTX();
    sockfd = OpenConnection(argv[1], atoi(portnum));
    ssl = SSL_new(ctx);      /* create new SSL connection state */
    SSL_set_fd(ssl, sockfd);    /* attach the socket descriptor */
    // printf("SSL pointer: %p\n", (void*) ssl);

    gtk_init(&argc, &argv);
    GtkBuilder * ui_builder;
    GError * err = NULL;

    ui_builder = gtk_builder_new();

    if(!gtk_builder_add_from_file(ui_builder, "glade_img/gui.glade", &err)) {
        g_critical ("Не вышло загрузить файл с UI : %s", err->message);
        g_error_free (err);
        return -1;
    }
    GtkWidget * window = GTK_WIDGET(gtk_builder_get_object(ui_builder, "login_window"));
    GdkScreen *Screen = gtk_widget_get_screen(window);
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(provider, "style/gtkchat.css", NULL);
    gtk_style_context_add_provider_for_screen(Screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
    ui = load_widgets(ui_builder, ssl);

    load_signal(ui_builder, ui);

    
    if ( SSL_connect(ssl) == FAIL )   /* perform the connection */
        ERR_print_errors_fp(stderr);
    else
    {
        pthread_t tid;
        pthread_create(&tid, NULL,(void *(*)(void *)) &Servlet, NULL);
        gtk_widget_show_all(window);
        gtk_main();
    }
    free(ui);
    close(sockfd);         /* close socket */
    SSL_CTX_free(ctx);        /* release context */
    return 0;

}
