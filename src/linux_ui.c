#include <gtk/gtk.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "file_io.h"
#include "linux_ui.h"
#include "localization.h"

typedef struct {
    GtkWidget *window;
    GtkWidget *text_view;
    GtkWidget *status_line_col;
    GtkWidget *status_size;
    GtkWidget *status_encoding;
    GtkWidget *menu_file;
    GtkWidget *menu_edit;
    GtkWidget *menu_view;
    GtkWidget *menu_language;
    GtkWidget *menu_new;
    GtkWidget *menu_open;
    GtkWidget *menu_save;
    GtkWidget *menu_save_as;
    GtkWidget *menu_exit;
    GtkWidget *menu_undo;
    GtkWidget *menu_cut;
    GtkWidget *menu_copy;
    GtkWidget *menu_paste;
    GtkWidget *menu_select_all;
    GtkWidget *menu_always_on_top;
    GtkWidget *menu_lang_english;
    GtkWidget *menu_lang_bangla;
    gboolean always_on_top;
    char *file_path;
    FILE_ENCODING encoding;
} AppState;

static void update_status(AppState *state);
static void update_menu_labels(AppState *state);
static void update_window_title(AppState *state);

static void free_file_path(AppState *state)
{
    if (state->file_path) {
        g_free(state->file_path);
        state->file_path = NULL;
    }
}

static void set_file_path(AppState *state, const char *path)
{
    free_file_path(state);
    if (path && path[0]) {
        state->file_path = g_strdup(path);
    }
}

static void open_file(AppState *state, const char *path)
{
    if (!path || !path[0]) {
        return;
    }

    LPWSTR buffer = NULL;
    DWORD size = 0;
    FILE_ENCODING enc = ENC_UTF8;
    if (!file_read(NULL, path, &buffer, &size, &enc)) {
        return;
    }

    GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->text_view));
    gtk_text_buffer_set_text(text_buffer, buffer ? buffer : "", (gint)size);
    free(buffer);

    set_file_path(state, path);
    state->encoding = enc;
    update_window_title(state);
    update_status(state);
}

static bool save_to_path(AppState *state, const char *path)
{
    if (!path || !path[0]) {
        return false;
    }

    GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->text_view));
    GtkTextIter start;
    GtkTextIter end;
    gtk_text_buffer_get_bounds(text_buffer, &start, &end);
    gchar *text = gtk_text_buffer_get_text(text_buffer, &start, &end, FALSE);
    if (!text) {
        return false;
    }

    DWORD len = (DWORD)strlen(text);
    bool ok = file_write(NULL, path, text, len, ENC_UTF8);
    g_free(text);
    if (ok) {
        set_file_path(state, path);
        state->encoding = ENC_UTF8;
        update_window_title(state);
        update_status(state);
    }
    return ok;
}

static void on_new_activate(GtkWidget *widget, gpointer user_data)
{
    (void)widget;
    AppState *state = user_data;
    GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->text_view));
    gtk_text_buffer_set_text(text_buffer, "", 0);
    free_file_path(state);
    state->encoding = ENC_UTF8;
    update_window_title(state);
    update_status(state);
}

static void on_open_activate(GtkWidget *widget, gpointer user_data)
{
    (void)widget;
    AppState *state = user_data;
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        loc_str(LOC_MENU_OPEN),
        GTK_WINDOW(state->window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (path) {
            open_file(state, path);
            g_free(path);
        }
    }
    gtk_widget_destroy(dialog);
}

static void on_save_as_activate(GtkWidget *widget, gpointer user_data)
{
    (void)widget;
    AppState *state = user_data;
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        loc_str(LOC_MENU_SAVE_AS),
        GTK_WINDOW(state->window),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Save", GTK_RESPONSE_ACCEPT,
        NULL);

    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (path) {
            save_to_path(state, path);
            g_free(path);
        }
    }
    gtk_widget_destroy(dialog);
}

static void on_save_activate(GtkWidget *widget, gpointer user_data)
{
    (void)widget;
    AppState *state = user_data;
    if (state->file_path) {
        if (save_to_path(state, state->file_path)) {
            return;
        }
    }
    on_save_as_activate(widget, user_data);
}

static void on_exit_activate(GtkWidget *widget, gpointer user_data)
{
    (void)widget;
    AppState *state = user_data;
    gtk_widget_destroy(state->window);
}

static void on_cut_activate(GtkWidget *widget, gpointer user_data)
{
    (void)widget;
    AppState *state = user_data;
    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->text_view));
    gtk_text_buffer_cut_clipboard(buffer, clipboard, TRUE);
}

static void on_copy_activate(GtkWidget *widget, gpointer user_data)
{
    (void)widget;
    AppState *state = user_data;
    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->text_view));
    gtk_text_buffer_copy_clipboard(buffer, clipboard);
}

static void on_paste_activate(GtkWidget *widget, gpointer user_data)
{
    (void)widget;
    AppState *state = user_data;
    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->text_view));
    gtk_text_buffer_paste_clipboard(buffer, clipboard, NULL, TRUE);
}

static void on_select_all_activate(GtkWidget *widget, gpointer user_data)
{
    (void)widget;
    AppState *state = user_data;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->text_view));
    GtkTextIter start;
    GtkTextIter end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    gtk_text_buffer_select_range(buffer, &start, &end);
}

static void on_always_on_top_toggled(GtkWidget *widget, gpointer user_data)
{
    AppState *state = user_data;
    gboolean active = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));
    state->always_on_top = active;
    gtk_window_set_keep_above(GTK_WINDOW(state->window), active);
}

static void on_language_toggled(GtkWidget *widget, gpointer user_data)
{
    AppState *state = user_data;
    if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget))) {
        return;
    }

    if (widget == state->menu_lang_bangla) {
        localization_set_bangla(true);
    } else {
        localization_set_bangla(false);
    }

    update_menu_labels(state);
    update_window_title(state);
    update_status(state);
}

static void on_buffer_changed(GtkTextBuffer *buffer, gpointer user_data)
{
    (void)buffer;
    update_status(user_data);
}

static void on_mark_set(GtkTextBuffer *buffer, GtkTextIter *location, GtkTextMark *mark, gpointer user_data)
{
    (void)buffer;
    (void)location;
    const char *name = gtk_text_mark_get_name(mark);
    if (name && strcmp(name, "insert") == 0) {
        update_status(user_data);
    }
}

static GtkWidget *create_menu_bar(AppState *state)
{
    GtkWidget *menu_bar = gtk_menu_bar_new();

    state->menu_file = gtk_menu_item_new();
    state->menu_edit = gtk_menu_item_new();
    state->menu_view = gtk_menu_item_new();

    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), state->menu_file);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), state->menu_edit);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), state->menu_view);

    GtkWidget *file_menu = gtk_menu_new();
    GtkWidget *edit_menu = gtk_menu_new();
    GtkWidget *view_menu = gtk_menu_new();

    state->menu_new = gtk_menu_item_new();
    state->menu_open = gtk_menu_item_new();
    state->menu_save = gtk_menu_item_new();
    state->menu_save_as = gtk_menu_item_new();
    state->menu_exit = gtk_menu_item_new();

    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), state->menu_new);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), state->menu_open);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), state->menu_save);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), state->menu_save_as);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), state->menu_exit);

    state->menu_undo = gtk_menu_item_new();
    state->menu_cut = gtk_menu_item_new();
    state->menu_copy = gtk_menu_item_new();
    state->menu_paste = gtk_menu_item_new();
    state->menu_select_all = gtk_menu_item_new();

    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), state->menu_undo);
    gtk_widget_set_sensitive(state->menu_undo, FALSE);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), state->menu_cut);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), state->menu_copy);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), state->menu_paste);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), state->menu_select_all);

    state->menu_always_on_top = gtk_check_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(view_menu), state->menu_always_on_top);

    state->menu_language = gtk_menu_item_new();
    GtkWidget *language_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(state->menu_language), language_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(view_menu), state->menu_language);

    state->menu_lang_english = gtk_radio_menu_item_new_with_label(NULL, "English");
    GSList *group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(state->menu_lang_english));
    state->menu_lang_bangla = gtk_radio_menu_item_new_with_label(group, "বাংলা");
    gtk_menu_shell_append(GTK_MENU_SHELL(language_menu), state->menu_lang_english);
    gtk_menu_shell_append(GTK_MENU_SHELL(language_menu), state->menu_lang_bangla);

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(state->menu_file), file_menu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(state->menu_edit), edit_menu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(state->menu_view), view_menu);

    g_signal_connect(state->menu_new, "activate", G_CALLBACK(on_new_activate), state);
    g_signal_connect(state->menu_open, "activate", G_CALLBACK(on_open_activate), state);
    g_signal_connect(state->menu_save, "activate", G_CALLBACK(on_save_activate), state);
    g_signal_connect(state->menu_save_as, "activate", G_CALLBACK(on_save_as_activate), state);
    g_signal_connect(state->menu_exit, "activate", G_CALLBACK(on_exit_activate), state);
    g_signal_connect(state->menu_cut, "activate", G_CALLBACK(on_cut_activate), state);
    g_signal_connect(state->menu_copy, "activate", G_CALLBACK(on_copy_activate), state);
    g_signal_connect(state->menu_paste, "activate", G_CALLBACK(on_paste_activate), state);
    g_signal_connect(state->menu_select_all, "activate", G_CALLBACK(on_select_all_activate), state);
    g_signal_connect(state->menu_always_on_top, "toggled", G_CALLBACK(on_always_on_top_toggled), state);
    g_signal_connect(state->menu_lang_english, "toggled", G_CALLBACK(on_language_toggled), state);
    g_signal_connect(state->menu_lang_bangla, "toggled", G_CALLBACK(on_language_toggled), state);

    update_menu_labels(state);
    return menu_bar;
}

static void update_menu_labels(AppState *state)
{
    gtk_menu_item_set_label(GTK_MENU_ITEM(state->menu_file), loc_str(LOC_MENU_FILE));
    gtk_menu_item_set_label(GTK_MENU_ITEM(state->menu_edit), loc_str(LOC_MENU_EDIT));
    gtk_menu_item_set_label(GTK_MENU_ITEM(state->menu_view), loc_str(LOC_MENU_VIEW));

    gtk_menu_item_set_label(GTK_MENU_ITEM(state->menu_new), loc_str(LOC_MENU_NEW));
    gtk_menu_item_set_label(GTK_MENU_ITEM(state->menu_open), loc_str(LOC_MENU_OPEN));
    gtk_menu_item_set_label(GTK_MENU_ITEM(state->menu_save), loc_str(LOC_MENU_SAVE));
    gtk_menu_item_set_label(GTK_MENU_ITEM(state->menu_save_as), loc_str(LOC_MENU_SAVE_AS));
    gtk_menu_item_set_label(GTK_MENU_ITEM(state->menu_exit), loc_str(LOC_MENU_EXIT));

    gtk_menu_item_set_label(GTK_MENU_ITEM(state->menu_undo), loc_str(LOC_MENU_UNDO));
    gtk_menu_item_set_label(GTK_MENU_ITEM(state->menu_cut), loc_str(LOC_MENU_CUT));
    gtk_menu_item_set_label(GTK_MENU_ITEM(state->menu_copy), loc_str(LOC_MENU_COPY));
    gtk_menu_item_set_label(GTK_MENU_ITEM(state->menu_paste), loc_str(LOC_MENU_PASTE));
    gtk_menu_item_set_label(GTK_MENU_ITEM(state->menu_select_all), loc_str(LOC_MENU_SELECT_ALL));
    gtk_menu_item_set_label(GTK_MENU_ITEM(state->menu_always_on_top), loc_str(LOC_MENU_ALWAYS_ON_TOP));
    gtk_menu_item_set_label(GTK_MENU_ITEM(state->menu_language), loc_str(LOC_MENU_LANGUAGE));
    gtk_menu_item_set_label(GTK_MENU_ITEM(state->menu_lang_english), loc_str(LOC_MENU_LANG_ENGLISH));
    gtk_menu_item_set_label(GTK_MENU_ITEM(state->menu_lang_bangla), loc_str(LOC_MENU_LANG_BANGLA));

    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(state->menu_lang_bangla), localization_is_bangla());
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(state->menu_lang_english), !localization_is_bangla());
}

static void update_window_title(AppState *state)
{
    if (state->file_path) {
        char *base = g_path_get_basename(state->file_path);
        gtk_window_set_title(GTK_WINDOW(state->window), base ? base : loc_str(LOC_APP_TITLE));
        g_free(base);
    } else {
        gtk_window_set_title(GTK_WINDOW(state->window), loc_str(LOC_APP_TITLE));
    }
}

static void update_status(AppState *state)
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->text_view));
    GtkTextIter iter;
    GtkTextMark *mark = gtk_text_buffer_get_insert(buffer);
    gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);
    int line = gtk_text_iter_get_line(&iter) + 1;
    int col = gtk_text_iter_get_line_offset(&iter) + 1;

    char text[128];
    snprintf(text, sizeof(text), loc_str(LOC_STATUS_LN_COL), line, col);
    gtk_label_set_text(GTK_LABEL(state->status_line_col), text);

    GtkTextIter start;
    GtkTextIter end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    gchar *contents = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    size_t bytes = contents ? strlen(contents) : 0;
    g_free(contents);

    double size = (double)bytes;
    const char *unit = "B";
    if (size >= 1024 * 1024) {
        size /= (1024 * 1024);
        unit = "MB";
    } else if (size >= 1024) {
        size /= 1024;
        unit = "KB";
    }
    snprintf(text, sizeof(text), loc_str(LOC_STATUS_SIZE), size, unit);
    gtk_label_set_text(GTK_LABEL(state->status_size), text);

    const char *enc = loc_str(LOC_STATUS_ENCODING_UNKNOWN);
    switch (state->encoding) {
        case ENC_UTF8:
            enc = loc_str(LOC_STATUS_ENCODING_UTF8);
            break;
        case ENC_UTF16_LE:
            enc = loc_str(LOC_STATUS_ENCODING_UTF16_LE);
            break;
        case ENC_UTF16_BE:
            enc = loc_str(LOC_STATUS_ENCODING_UTF16_BE);
            break;
        case ENC_ANSI:
            enc = loc_str(LOC_STATUS_ENCODING_ANSI);
            break;
    }
    gtk_label_set_text(GTK_LABEL(state->status_encoding), enc);
}

int linux_ui_run(int argc, char **argv, const char *startup_path)
{
    gtk_init(&argc, &argv);

    AppState state = {0};
    state.encoding = ENC_UTF8;

    state.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(state.window), 1000, 700);
    gtk_window_set_position(GTK_WINDOW(state.window), GTK_WIN_POS_CENTER);
    g_signal_connect(state.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(state.window), root);

    GtkWidget *menu_bar = create_menu_bar(&state);
    gtk_box_pack_start(GTK_BOX(root), menu_bar, FALSE, FALSE, 0);

    state.text_view = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(state.text_view), GTK_WRAP_NONE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(state.text_view), TRUE);
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, "textview { font-family: monospace; font-size: 11pt; }", -1, NULL);
    GtkStyleContext *context = gtk_widget_get_style_context(state.text_view);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scroll), state.text_view);
    gtk_widget_set_hexpand(scroll, TRUE);
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_box_pack_start(GTK_BOX(root), scroll, TRUE, TRUE, 0);

    GtkWidget *status = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_margin_start(status, 8);
    gtk_widget_set_margin_end(status, 8);
    gtk_widget_set_margin_top(status, 6);
    gtk_widget_set_margin_bottom(status, 6);

    state.status_line_col = gtk_label_new("");
    state.status_size = gtk_label_new("");
    state.status_encoding = gtk_label_new("");

    gtk_box_pack_start(GTK_BOX(status), state.status_line_col, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(status), state.status_size, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(status), state.status_encoding, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(root), status, FALSE, FALSE, 0);

    GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(state.text_view));
    g_signal_connect(text_buffer, "changed", G_CALLBACK(on_buffer_changed), &state);
    g_signal_connect(text_buffer, "mark-set", G_CALLBACK(on_mark_set), &state);

    update_window_title(&state);
    update_status(&state);

    if (startup_path && startup_path[0]) {
        open_file(&state, startup_path);
    }

    gtk_widget_show_all(state.window);
    gtk_main();

    free_file_path(&state);
    return 0;
}
