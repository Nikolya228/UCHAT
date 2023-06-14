#include <gtk/gtk.h>

// Функция-обработчик сигнала "clicked" для кнопки "Редактировать"
void on_edit_button_clicked(GtkButton *button, gpointer user_data)
{
    GtkLabel *label = GTK_LABEL(user_data);
    GtkEntry *entry = GTK_ENTRY(gtk_entry_new());

    // Получаем текущий текст метки и устанавливаем его в качестве текста входного поля
    const char *text = gtk_label_get_text(label);
    gtk_entry_set_text(entry, text);

    // Устанавливаем режим редактирования для входного поля
    gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);

    // Заменяем метку на входное поле
    gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(GTK_WIDGET(label))), GTK_WIDGET(label));
    gtk_container_add(GTK_CONTAINER(gtk_widget_get_parent(GTK_WIDGET(entry))), GTK_WIDGET(entry));

    // Устанавливаем фокус на входное поле и выделяем весь текст в нем
    gtk_widget_grab_focus(GTK_WIDGET(entry));
    gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1);
}

// Функция-обработчик сигнала "activate" для входного поля
void on_entry_activate(GtkEntry *entry, gpointer user_data)
{
    GtkLabel *label = GTK_LABEL(user_data);
    const char *text = gtk_entry_get_text(entry);

    // Устанавливаем текст метки и скрываем входное поле
    gtk_label_set_text(label, text);
    gtk_widget_hide(GTK_WIDGET(entry));
    gtk_container_add(GTK_CONTAINER(gtk_widget_get_parent(GTK_WIDGET(label))), GTK_WIDGET(label));
}

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    // Создаем главное окно
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Редактируемая метка");
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 200);

    // Создаем метку и устанавливаем ее текст
    GtkWidget *label = gtk_label_new("Нажмите кнопку \"Редактировать\", чтобы начать редактирование");
    gtk_container_add(GTK_CONTAINER(window), label);

    // Создаем кнопку "Редактировать" и устанавливаем ее обработчик сигнала "clicked"
    GtkWidget *entry = gtk_entry_new();
    gtk_widget_hide(GTK_WIDGET(entry));
    gtk_container_add(GTK_CONTAINER(window), entry);

    // Устанавливаем обработчик сигнала "activate" для входного поля
    g_signal_connect(entry, "activate", G_CALLBACK(on_entry_activate), label);


    // Отображаем все созданные виджеты
    gtk_widget_show_all(window);

    // Запускаем главный цикл обработки событий
    gtk_main();

    return 0;
}
