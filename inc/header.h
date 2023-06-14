#ifndef HEADER_H
#define HEADER_H

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <gtk/gtk.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

typedef struct Widgets{
    GtkWidget *btn_l;
    GtkWidget *login_l;
    GtkWidget *password_l;
    GtkWidget *btn_r;
    GtkWidget *login_r;
    GtkWidget *password_r;
    GtkWidget *repeat_password_r;
    GtkWidget *label_r;
    GtkWidget *btn_log_r;
    GtkWidget *btn_reg_l;
    GtkWidget *window_l;
    GtkWidget *window_r;
    GtkWidget *window_chat;
    GtkWidget *window_player;
    GtkWidget *search_name;
    GtkWidget *grid_on_chats;
    GtkWidget *grid_on_mes;
    GtkWidget *btn_entry_mes;
    GtkWidget *entry_mes;
    GtkWidget *label_l;
    GtkWidget *btn_play;
    GtkWidget *btn_exit_invite_game;
    GtkWidget *lb_pl_1;
    GtkWidget *lb_pl_2;
    GtkWidget *lb_pl_3;
    GtkWidget *entry_invite_player;
    GtkWidget *btn_play_game;
    GtkWidget *box_label;
    GtkWidget *btn_check_invite;
    GtkWidget *window_invite;
    GtkWidget *box_for_invite;
    GtkWidget *btn_refresh;

    GtkWidget *hud_players;
    GtkWidget *map;
    GtkWidget *pers_1;
    GtkWidget *pers_2;
    GtkWidget *pers_3;
    GtkWidget *pers_4;
    GtkWidget *chest_1;
    GtkWidget *chest_2;
    GtkWidget *chest_3;
    GtkWidget *chest_4;
    GtkWidget *cube;
    GtkWidget *mes;
    GtkWidget *fixed;
    GtkWidget *money_pl_1;
    GtkWidget *money_pl_2;
    GtkWidget *money_pl_3;
    GtkWidget *shans;
    GtkWidget *avatar;
    GtkWidget *scroll_window_mes;
    GtkWidget *BtnReserch;

    GtkWidget *image_1;
    GtkWidget *image_2;
    GtkWidget *image_3;
    GtkWidget *popover;
    GtkWidget *settings;
    GtkRadioButton *radio1;
    GtkRadioButton *radio2;
    GtkRadioButton *radio3;
    GtkRadioButton *radio4;
    GtkRadioButton *radio5;
    GtkRadioButton *radio6;
    GtkWidget *btn_accept;
    GtkWidget *change_name;
    GtkWidget *your_img;
    GtkWidget *your_name;
    GtkWidget *turn;
    GtkWidget *yes;
    GtkWidget *no;
    GtkWidget *mess_in_game;
    GtkWidget *box_in_game;
    GtkWidget *chatGrid;
    GtkWidget *profile;
    GtkWidget *nameProfile;
    

    // SSL *ssl;
} Widgets;

void load_signal(GtkBuilder *ui_builder, Widgets *ui);
void on_btn_reg_l(GtkButton *btn, gpointer data);
void on_btn_log_r(GtkButton *btn, gpointer data);
void onBtnLogin(GtkButton *btn, gpointer data);
void onBtnRegistration(GtkButton *btn, gpointer data);
Widgets* load_widgets(GtkBuilder *ui_builder, SSL *ssl);
void updatePlayer();

#endif