#include "../inc/header.h"

void load_signal(GtkBuilder *ui_builder, Widgets *ui){
    gtk_builder_connect_signals(ui_builder, NULL);
}

Widgets* load_widgets(GtkBuilder *ui_builder, SSL *ssl){
    Widgets *ui = malloc(sizeof(Widgets));

    ui->btn_l = GTK_WIDGET(gtk_builder_get_object(ui_builder, "send_l"));
    ui->login_l = GTK_WIDGET(gtk_builder_get_object(ui_builder, "login_l"));
    ui->password_l = GTK_WIDGET(gtk_builder_get_object(ui_builder, "password_l"));
    ui->btn_r = GTK_WIDGET(gtk_builder_get_object(ui_builder, "send_r"));
    ui->login_r = GTK_WIDGET(gtk_builder_get_object(ui_builder, "login_r"));
    ui->password_r = GTK_WIDGET(gtk_builder_get_object(ui_builder, "password_r"));
    ui->repeat_password_r = GTK_WIDGET(gtk_builder_get_object(ui_builder, "repeat_password_r"));
    ui->label_r = GTK_WIDGET(gtk_builder_get_object(ui_builder, "label_r"));
    ui->label_l = GTK_WIDGET(gtk_builder_get_object(ui_builder, "label_l"));
    ui->btn_log_r = GTK_WIDGET(gtk_builder_get_object(ui_builder, "btn_lor_r"));
    ui->btn_reg_l = GTK_WIDGET(gtk_builder_get_object(ui_builder, "btn_reg_l"));
    ui->window_l = GTK_WIDGET(gtk_builder_get_object(ui_builder, "login_window"));
    ui->window_r = GTK_WIDGET(gtk_builder_get_object(ui_builder, "registration_window"));
    ui->window_chat = GTK_WIDGET(gtk_builder_get_object(ui_builder, "chat_window"));
    ui->search_name = GTK_WIDGET(gtk_builder_get_object(ui_builder, "search_name"));
    ui->grid_on_chats = GTK_WIDGET(gtk_builder_get_object(ui_builder, "name_chat_grid"));
    ui->grid_on_mes = GTK_WIDGET(gtk_builder_get_object(ui_builder, "mess_grid"));
    ui->entry_mes = GTK_WIDGET(gtk_builder_get_object(ui_builder, "entry_mes"));
    ui->btn_entry_mes = GTK_WIDGET(gtk_builder_get_object(ui_builder, "btn_entry_mes"));
    ui->btn_play = GTK_WIDGET(gtk_builder_get_object(ui_builder, "btn_game"));
    ui->btn_exit_invite_game = GTK_WIDGET(gtk_builder_get_object(ui_builder, "exit_onBtn_invite_game"));
    ui->entry_invite_player = GTK_WIDGET(gtk_builder_get_object(ui_builder, "invite_player"));
    ui->lb_pl_1 = GTK_WIDGET(gtk_builder_get_object(ui_builder, "name_1"));
    ui->lb_pl_2 = GTK_WIDGET(gtk_builder_get_object(ui_builder, "name_2"));
    ui->lb_pl_3 = GTK_WIDGET(gtk_builder_get_object(ui_builder, "name_3"));
    ui->btn_play = GTK_WIDGET(gtk_builder_get_object(ui_builder, "play"));
    ui->box_label = GTK_WIDGET(gtk_builder_get_object(ui_builder, "label_box"));
    ui->btn_check_invite = GTK_WIDGET(gtk_builder_get_object(ui_builder, "check_invite"));
    ui->window_invite = GTK_WIDGET(gtk_builder_get_object(ui_builder, "invite_window"));
    ui->box_for_invite = GTK_WIDGET(gtk_builder_get_object(ui_builder, "box_for_invite"));
    ui->btn_refresh = GTK_WIDGET(gtk_builder_get_object(ui_builder, "refresh"));
    ui->window_player = GTK_WIDGET(gtk_builder_get_object(ui_builder, "loby_window"));
    ui->hud_players = GTK_WIDGET(gtk_builder_get_object(ui_builder, "hud_players"));
    ui->map = GTK_WIDGET(gtk_builder_get_object(ui_builder, "maps"));
    ui->pers_1 = GTK_WIDGET(gtk_builder_get_object(ui_builder, "pers_1"));
    ui->pers_2 = GTK_WIDGET(gtk_builder_get_object(ui_builder, "pers_2"));
    ui->pers_3 = GTK_WIDGET(gtk_builder_get_object(ui_builder, "pers_3"));
    ui->pers_4 = GTK_WIDGET(gtk_builder_get_object(ui_builder, "pers_4"));
    ui->chest_1 = GTK_WIDGET(gtk_builder_get_object(ui_builder, "chest_1"));
    ui->chest_2 = GTK_WIDGET(gtk_builder_get_object(ui_builder, "chest_2"));
    ui->chest_3 = GTK_WIDGET(gtk_builder_get_object(ui_builder, "chest_3"));
    ui->chest_4 = GTK_WIDGET(gtk_builder_get_object(ui_builder, "chest_4"));
    ui->cube = GTK_WIDGET(gtk_builder_get_object(ui_builder, "cube"));
    ui->mes = GTK_WIDGET(gtk_builder_get_object(ui_builder, "mes"));
    ui->fixed = GTK_WIDGET(gtk_builder_get_object(ui_builder, "fixed"));
    ui->money_pl_1 = GTK_WIDGET(gtk_builder_get_object(ui_builder, "money_pl_1"));
    ui->money_pl_2 = GTK_WIDGET(gtk_builder_get_object(ui_builder, "money_pl_2"));
    ui->money_pl_3 = GTK_WIDGET(gtk_builder_get_object(ui_builder, "money_pl_3"));
    ui->shans = GTK_WIDGET(gtk_builder_get_object(ui_builder, "shans"));
    ui->avatar = GTK_WIDGET(gtk_builder_get_object(ui_builder, "avatar"));
    ui->scroll_window_mes = GTK_WIDGET(gtk_builder_get_object(ui_builder, "mess_window"));
    ui->BtnReserch = GTK_WIDGET(gtk_builder_get_object(ui_builder, "BtnSearch"));
    ui->image_1 = GTK_WIDGET(gtk_builder_get_object(ui_builder, "1_person"));
    ui->image_2 = GTK_WIDGET(gtk_builder_get_object(ui_builder, "2_person"));
    ui->image_3 = GTK_WIDGET(gtk_builder_get_object(ui_builder, "3_person"));
    ui->popover = GTK_WIDGET(gtk_builder_get_object(ui_builder, "popover"));
    ui->settings = GTK_WIDGET(gtk_builder_get_object(ui_builder, "settings"));
    ui->radio1 =  GTK_RADIO_BUTTON(gtk_builder_get_object(ui_builder, "radio1"));
    ui->radio2 =  GTK_RADIO_BUTTON(gtk_builder_get_object(ui_builder, "radio2"));
    ui->radio3 =  GTK_RADIO_BUTTON(gtk_builder_get_object(ui_builder, "radio3"));
    ui->radio4 =  GTK_RADIO_BUTTON(gtk_builder_get_object(ui_builder, "radio4"));
    ui->radio5 =  GTK_RADIO_BUTTON(gtk_builder_get_object(ui_builder, "radio5"));
    ui->radio6 =  GTK_RADIO_BUTTON(gtk_builder_get_object(ui_builder, "radio6"));
    ui->btn_accept = GTK_WIDGET(gtk_builder_get_object(ui_builder, "btnAccept"));
    ui->change_name = GTK_WIDGET(gtk_builder_get_object(ui_builder, "change_name"));
    ui->your_img = GTK_WIDGET(gtk_builder_get_object(ui_builder, "your_img"));
    ui->your_name = GTK_WIDGET(gtk_builder_get_object(ui_builder, "your_name"));
    ui->turn = GTK_WIDGET(gtk_builder_get_object(ui_builder, "turn"));
    ui->yes = GTK_WIDGET(gtk_builder_get_object(ui_builder, "yes"));
    ui->no = GTK_WIDGET(gtk_builder_get_object(ui_builder, "no"));
    ui->mess_in_game = GTK_WIDGET(gtk_builder_get_object(ui_builder, "mess_in_game"));
    ui->box_in_game = GTK_WIDGET(gtk_builder_get_object(ui_builder, "box_mes_game"));
    ui->chatGrid = GTK_WIDGET(gtk_builder_get_object(ui_builder, "chatGrid"));
    ui->profile = GTK_WIDGET(gtk_builder_get_object(ui_builder, "profile"));
    ui->nameProfile = GTK_WIDGET(gtk_builder_get_object(ui_builder, "name_prof"));
    // ui->ssl = ssl;
    return ui;
}
