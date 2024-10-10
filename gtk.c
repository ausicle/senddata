#include "encryption.h"
#include "networking.h"
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef ENCRYPTED_FILE_EXT
#define ENCRYPTED_FILE_EXT ".enc"
#endif

GtkCssProvider *provider;

typedef struct {
	GtkWidget *main_window;
	GtkWidget *main_box;
	GtkWidget *file_label;
	GtkWidget *progress_bar;
	GtkWidget *password_entry;
	GtkWidget *target_entry;
	char *selected_filename;
	GArray *sent_files;
} AppWidgets;

static const char *css = "progressbar {"
                         "    background-color: #ffffff;"
                         "    min-height: 30px;"
                         "}"
                         "progressbar > trough > progress {"
                         "    background-color: #99ccff;"
                         "    border-radius: 10px;"
                         "    min-height: 25px;"
                         "}"
                         "progressbar > trough {"
                         "    background-color: #ffffff;"
                         "    border-radius: 10px;"
                         "    min-height: 30px;"
                         "}";

static void
help_function(GtkWidget *widget, GVariant *parameter, gpointer user_data)
{
	GtkWidget *new_window = gtk_window_new();
	gtk_window_set_title(GTK_WINDOW(new_window), "Help");
	gtk_window_set_default_size(GTK_WINDOW(new_window), 450, 300);
	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_window_set_child(GTK_WINDOW(new_window), box);
	GtkWidget *scroll = gtk_scrolled_window_new();
	GtkWidget *text_view = gtk_text_view_new();
	gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_view), FALSE);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD_CHAR);
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
	gtk_text_buffer_set_text(
	    buffer,
	    "\n         How to use \n\n   1.Select your file on your device\n\n   "
	    "2.Input the password witch you need to encrytion\n\n   3.Press the "
	    "send file button\n\n  (If the file is alredy to send please check in "
	    "\"Receive file button\")",
	    -1);
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), text_view);
	gtk_widget_set_vexpand(scroll, TRUE);
	gtk_box_append(GTK_BOX(box), scroll);
	gtk_window_present(GTK_WINDOW(new_window));
}

static void
on_open_response(GtkDialog *dialog, int response, gpointer user_data)
{
	AppWidgets *widgets = (AppWidgets *)user_data;
	if (response == GTK_RESPONSE_ACCEPT) {
		GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
		GFile *file = gtk_file_chooser_get_file(chooser);

		g_free(widgets->selected_filename);
		widgets->selected_filename = g_file_get_path(file);
		if (widgets->selected_filename) {
			g_print("Selected file: %s\n", widgets->selected_filename);
			gtk_editable_set_text(GTK_EDITABLE(widgets->file_label),
			                      widgets->selected_filename);
		} else {
			g_print("Failed to get file path\n");
			gtk_editable_set_text(GTK_EDITABLE(widgets->file_label),
			                      "No file selected");
		}
		g_object_unref(file);
	}
	gtk_window_destroy(GTK_WINDOW(dialog));
}

static void
show_error_message(GtkWindow *parent, const char *message)
{
	// Create a new message dialog
	GtkWidget *dialog =
	    gtk_message_dialog_new(parent, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
	                           GTK_BUTTONS_OK, "%s", message);

	// Set the dialog to be transient for the parent window
	gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);

	// Connect the response signal to automatically close the dialog
	g_signal_connect_swapped(dialog, "response", G_CALLBACK(gtk_window_destroy),
	                         dialog);

	// Show the dialog
	gtk_widget_show(dialog);
}

static void
select_file(GtkButton *button, gpointer user_data)
{
	GtkWindow *window = GTK_WINDOW(gtk_widget_get_root(GTK_WIDGET(button)));
	GtkFileChooserDialog *dialog =
	    GTK_FILE_CHOOSER_DIALOG(gtk_file_chooser_dialog_new(
	        "Open File", window, GTK_FILE_CHOOSER_ACTION_OPEN, "_Cancel",
	        GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL));
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
	g_signal_connect(dialog, "response", G_CALLBACK(on_open_response),
	                 user_data);
	gtk_window_present(GTK_WINDOW(dialog));
}

void
close_file_progress_windows(GtkWidget *widget, gpointer data)
{
	GList *toplevels = gtk_window_list_toplevels();
	for (GList *iter = toplevels; iter != NULL; iter = iter->next) {
		GtkWindow *window = GTK_WINDOW(iter->data);
		const gchar *title = gtk_window_get_title(window);
		if (g_strcmp0(title, "File progress") == 0) {
			gtk_window_destroy(window);
		}
	}
	g_list_free(toplevels);
}

static gboolean
update_progress(gpointer user_data)
{
	GtkProgressBar *progress_bar = GTK_PROGRESS_BAR(user_data);
	gdouble current_value = gtk_progress_bar_get_fraction(progress_bar);
	if (current_value < 1.0) {
		current_value += 0.047;
		gtk_progress_bar_set_fraction(progress_bar, current_value);
		return TRUE;
	} else {
		GtkWidget *done_window = gtk_window_new();
		gtk_window_set_title(GTK_WINDOW(done_window), "File progress");
		gtk_window_set_default_size(GTK_WINDOW(done_window), 250, 150);
		GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
		gtk_window_set_child(GTK_WINDOW(done_window), vbox);
		GtkWidget *label = gtk_label_new("\nDone");
		gtk_box_append(GTK_BOX(vbox), label);
		GtkWidget *quit_button = gtk_button_new_with_label("Quit");
		gtk_box_append(GTK_BOX(vbox), quit_button);
		gtk_widget_set_size_request(quit_button, 50, 30);
		gtk_widget_set_halign(quit_button, GTK_ALIGN_CENTER);
		gtk_widget_set_valign(quit_button, GTK_ALIGN_CENTER);
		g_signal_connect(quit_button, "clicked",
		                 G_CALLBACK(close_file_progress_windows), NULL);
		gtk_window_present(GTK_WINDOW(done_window));
		return FALSE;
	}
}

static void
send_function(GtkWidget *widget, gpointer user_data)
{
	AppWidgets *widgets = (AppWidgets *)user_data;
	if (widgets->selected_filename == NULL ||
	    strlen(widgets->selected_filename) == 0) {
		show_error_message(GTK_WINDOW(gtk_widget_get_root(widget)),
		                   "No file selected\n(Please choose the file first)");
		return;
	}
	char filename[256];
	strncpy(filename, widgets->selected_filename, sizeof(filename));
	unsigned char md[SHA256_DIGEST_LENGTH];
	char password[256];
	strncpy(password,
	        gtk_editable_get_text(GTK_EDITABLE(widgets->password_entry)),
	        sizeof(password));
	size_t password_len = strlen(password);
	SHA256((unsigned char *)password, password_len, md);
	char filename_encrypted[256];
	if (password_len != 0) {
		strncpy(filename_encrypted, widgets->selected_filename,
		        sizeof(filename_encrypted));
		strncat(filename_encrypted, ENCRYPTED_FILE_EXT,
		        sizeof(filename_encrypted) - sizeof(ENCRYPTED_FILE_EXT));
		if (file_encrypt(md, filename, filename_encrypted) < 0) {
			show_error_message(GTK_WINDOW(gtk_widget_get_root(widget)),
			                   "File cannot be encrypted.");
			unlink(filename_encrypted);
			return;
		}
		strncpy(filename, filename_encrypted, sizeof(filename));
		printf("FN: %s\n", filename);
	}
	if (strlen(gtk_editable_get_text(GTK_EDITABLE(widgets->target_entry))) ==
	    0) {
		show_error_message(
		    GTK_WINDOW(gtk_widget_get_root(widget)),
		    "Address invalid.\n(Please check the destination first)");
		return;
	}
	int sockfd = initialize_socket();
	char address[256];
	strncpy(address, gtk_editable_get_text(GTK_EDITABLE(widgets->target_entry)),
	        sizeof(address));
	struct sockaddr_in addr_in;
	initialize_addr_in(&addr_in, address, 43337);
	if (connect(sockfd, (struct sockaddr *)&addr_in, sizeof(addr_in)) < 0) {
		show_error_message(GTK_WINDOW(gtk_widget_get_root(widget)),
		                   "Could not connect to address");
		return;
	}
	sendfile_name(filename, sockfd);
	GtkWidget *window = gtk_window_new();
	gtk_window_set_title(GTK_WINDOW(window), "File progress");
	gtk_window_set_default_size(GTK_WINDOW(window), 325, 150);
	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
	gtk_widget_set_margin_start(box, 10);
	gtk_widget_set_margin_end(box, 10);
	gtk_widget_set_margin_top(box, 10);
	gtk_widget_set_margin_bottom(box, 10);
	gtk_window_set_child(GTK_WINDOW(window), box);
	GtkWidget *info_label = gtk_label_new(NULL);
	gchar *info_text = g_strdup_printf(
	    "File : %s\n\nEncrytion Password : %s\n\nDestination : %s\n\nSend "
	    "completed!",
	    widgets->selected_filename ? widgets->selected_filename : "None",
	    gtk_editable_get_text(GTK_EDITABLE(widgets->password_entry)),
	    gtk_editable_get_text(GTK_EDITABLE(widgets->target_entry)));
	gtk_label_set_text(GTK_LABEL(info_label), info_text);
	g_free(info_text);
	gtk_box_append(GTK_BOX(box), info_label);
	gtk_window_present(GTK_WINDOW(window));
	shutdown(sockfd, SHUT_WR);
	if (password_len != 0) {
		unlink(filename_encrypted);
	}
	close(sockfd);
}

static void
on_accept_clicked(GtkWidget *widget, gpointer user_data)
{
	GtkWidget *new_window = GTK_WIDGET(user_data);
	gtk_window_destroy(GTK_WINDOW(new_window));
	const gchar *source_path = "/path/to/received/file/example_file.txt";
	const gchar *destination_path = "/path/to/save/file/example_file.txt";
	GFile *source_file = g_file_new_for_path(source_path);
	GFile *destination_file = g_file_new_for_path(destination_path);
	GError *error = NULL;
	gboolean result = g_file_copy(source_file, destination_file,
	                              G_FILE_COPY_NONE, NULL, NULL, NULL, &error);
	if (result) {
		g_print("File download complete\n");
	} else {
		g_print("Failed to download file : %s\n", error->message);
		g_error_free(error);
	}
	g_object_unref(source_file);
	g_object_unref(destination_file);
}

static void
on_decline_clicked(GtkWidget *widget, gpointer user_data)
{
	GtkWidget *new_window = GTK_WIDGET(user_data);
	gtk_window_destroy(GTK_WINDOW(new_window));
	g_print("File declined\n");
}

static void
receive_function(GtkWidget *widget, gpointer user_data)
{
	AppWidgets *widgets = (AppWidgets *)user_data;
	GtkWidget *new_window = gtk_window_new();
	gtk_window_set_title(GTK_WINDOW(new_window), "Received file");
	gtk_window_set_default_size(GTK_WINDOW(new_window), 700, 350);

	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_window_set_child(GTK_WINDOW(new_window), box);

	GtkWidget *scroll = gtk_scrolled_window_new();
	GtkWidget *text_view = gtk_text_view_new();
	gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_view), FALSE);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD_CHAR);

	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
	gchar *text_to_display =
	    g_strdup_printf("\n   Received file list :\n\n       %s\n");
	gtk_text_buffer_set_text(buffer, text_to_display, -2);
	g_free(text_to_display);

	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), text_view);
	gtk_widget_set_vexpand(scroll, TRUE);
	gtk_box_append(GTK_BOX(box), scroll);

	GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
	gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(button_box, GTK_ALIGN_CENTER);
	gtk_widget_set_margin_bottom(button_box, 5);
	gtk_box_append(GTK_BOX(box), button_box);

	GtkWidget *accept_button = gtk_button_new_with_label("Accept");
	gtk_widget_set_size_request(accept_button, 80, 35);
	gtk_box_append(GTK_BOX(button_box), accept_button);
	g_signal_connect(accept_button, "clicked", G_CALLBACK(on_accept_clicked),
	                 new_window);

	GtkWidget *decline_button = gtk_button_new_with_label("Decline");
	gtk_widget_set_size_request(decline_button, 80, 35);
	gtk_box_append(GTK_BOX(button_box), decline_button);
	g_signal_connect(decline_button, "clicked", G_CALLBACK(on_decline_clicked),
	                 new_window);
	gtk_window_present(GTK_WINDOW(new_window));
}

static void
fullscreen_changed(GSimpleAction *action, GVariant *value, GtkWindow *win)
{
	if (g_variant_get_boolean(value)) {
		gtk_window_maximize(win);
	} else {
		gtk_window_unmaximize(win);
	}
	g_simple_action_set_state(action, value);
}

static void
app_shutdown(GApplication *app, GtkCssProvider *provider)
{
	gtk_style_context_remove_provider_for_display(gdk_display_get_default(),
	                                              GTK_STYLE_PROVIDER(provider));
}

static void
activate(GtkApplication *app, G_GNUC_UNUSED gpointer user_data,
         GApplication *application)
{
	AppWidgets *widgets = g_new(AppWidgets, 1);
	widgets->selected_filename = NULL;
	widgets->sent_files = g_array_new(FALSE, FALSE, sizeof(gchar *));

	GtkWidget *window;
	GtkWidget *grid;
	GtkWidget *name_label, *name_entry;
	GtkWidget *password_label, *password_entry;
	GtkWidget *file_label, *file_entry;
	GtkWidget *select_button, *send_button, *receive_button;
	GSimpleAction *act_fullscreen;

	GtkWidget *start_button;
	GtkWidget *progress_bar;
	GtkWidget *vbox;
	GtkWidget *update_progress;

	window = gtk_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(window), "Ecrytion & Decrytion File");
	gtk_window_set_default_size(GTK_WINDOW(window), 800, 500);

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	gtk_window_set_child(GTK_WINDOW(window), vbox);
	progress_bar = gtk_progress_bar_new();
	gtk_box_append(GTK_BOX(vbox), progress_bar);
	start_button = gtk_button_new_with_label("Start Loading");
	gtk_box_append(GTK_BOX(vbox), start_button);
	g_signal_connect(start_button, "clicked", G_CALLBACK(update_progress),
	                 progress_bar);
	gtk_widget_show(window);

	act_fullscreen = g_simple_action_new_stateful("fullscreen", NULL,
	                                              g_variant_new_boolean(FALSE));
	g_signal_connect(act_fullscreen, "change-state",
	                 G_CALLBACK(fullscreen_changed), window);
	g_action_map_add_action(G_ACTION_MAP(window), G_ACTION(act_fullscreen));

	grid = gtk_grid_new();
	gtk_window_set_child(GTK_WINDOW(window), grid);
	gtk_widget_set_halign(grid, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(grid, GTK_ALIGN_CENTER);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 30);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 30);

	name_label = gtk_label_new("File :");
	widgets->file_label = gtk_entry_new();
	gtk_entry_set_placeholder_text(GTK_ENTRY(widgets->file_label),
	                               " Enter file ");
	gtk_editable_set_editable(GTK_EDITABLE(widgets->file_label), FALSE);
	gtk_widget_set_margin_start(name_label, 20);
	gtk_widget_set_margin_end(widgets->file_label, 20);
	gtk_grid_attach(GTK_GRID(grid), name_label, 0, 5, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), widgets->file_label, 1, 5, 1, 1);
	gtk_widget_set_hexpand(widgets->file_label, TRUE);

	select_button = gtk_button_new_with_label("Select file");
	gtk_grid_attach(GTK_GRID(grid), select_button, 0, 6, 2, 1);
	g_signal_connect(select_button, "clicked", G_CALLBACK(select_file),
	                 widgets);
	gtk_widget_set_hexpand(widgets->file_label, TRUE);

	password_label = gtk_label_new("Password :");
	widgets->password_entry = gtk_password_entry_new();
	gtk_widget_set_margin_start(password_label, 20);
	gtk_widget_set_margin_end(widgets->password_entry, 20);
	gtk_password_entry_set_show_peek_icon(
	    GTK_PASSWORD_ENTRY(widgets->password_entry), TRUE);
	gtk_grid_attach(GTK_GRID(grid), password_label, 0, 7, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), widgets->password_entry, 1, 7, 1, 1);
	gtk_widget_set_hexpand(widgets->password_entry, TRUE);

	file_label = gtk_label_new("Dest. IP Address / Hostname :");
	widgets->target_entry = gtk_entry_new();
	gtk_widget_set_margin_start(file_label, 20);
	gtk_widget_set_margin_end(widgets->target_entry, 20);
	gtk_grid_attach(GTK_GRID(grid), file_label, 0, 8, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), widgets->target_entry, 1, 8, 1, 1);
	gtk_widget_set_hexpand(widgets->target_entry, TRUE);

	send_button = gtk_button_new_with_label("Send");
	gtk_widget_set_margin_start(send_button, 20);
	gtk_widget_set_margin_end(send_button, 20);
	gtk_grid_attach(GTK_GRID(grid), send_button, 0, 9, 2, 1);
	g_signal_connect(send_button, "clicked", G_CALLBACK(send_function),
	                 widgets);
	gtk_widget_set_hexpand(send_button, TRUE);

	/*
	receive_button = gtk_button_new_with_label("Received file");
	gtk_grid_attach(GTK_GRID(grid), receive_button, 0, 10, 2, 1);
	g_signal_connect(receive_button, "clicked", G_CALLBACK(receive_function),
	                 NULL);
	gtk_widget_set_hexpand(receive_button, TRUE);
	gtk_application_window_set_show_menubar(GTK_APPLICATION_WINDOW(window),
	                                        TRUE);
	                    */

	GtkCssProvider *provider = gtk_css_provider_new();
	gtk_css_provider_load_from_data(provider, css, -1);
	gtk_style_context_add_provider_for_display(
	    gdk_display_get_default(), GTK_STYLE_PROVIDER(provider),
	    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	g_object_unref(provider);

	gtk_window_present(GTK_WINDOW(window));
}

static void
app_startup(GApplication *app)
{
	GVariantType *vtype;
	GSimpleAction *act_quit;
	GSimpleAction *act_help;
	GMenu *setting_bar;
	GMenu *setting;
	GMenu *section1;
	GMenu *section2;
	GMenu *section3;
	GMenuItem *setting_item_help;
	GMenuItem *setting_item_fullscreen;
	GMenuItem *setting_item_quit;

	vtype = g_variant_type_new("s");

	act_help = g_simple_action_new("help", NULL);
	g_signal_connect_swapped(act_help, "activate", G_CALLBACK(help_function),
	                         NULL);
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(act_help));

	act_quit = g_simple_action_new("quit", NULL);
	g_signal_connect_swapped(act_quit, "activate",
	                         G_CALLBACK(g_application_quit), app);
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(act_quit));

	setting_bar = g_menu_new();
	setting = g_menu_new();
	section1 = g_menu_new();
	section2 = g_menu_new();
	section3 = g_menu_new();
	setting_item_help = g_menu_item_new("Help", "app.help");
	setting_item_fullscreen = g_menu_item_new("Full Screen", "win.fullscreen");
	setting_item_quit = g_menu_item_new("Quit", "app.quit");

	g_menu_append_item(section1, setting_item_help);
	g_menu_append_item(section2, setting_item_fullscreen);
	g_menu_append_item(section3, setting_item_quit);
	g_object_unref(setting_item_help);
	g_object_unref(setting_item_fullscreen);
	g_object_unref(setting_item_quit);

	g_menu_append_section(setting, NULL, G_MENU_MODEL(section1));
	g_menu_append_section(setting, NULL, G_MENU_MODEL(section2));
	g_menu_append_section(setting, NULL, G_MENU_MODEL(section3));
	g_menu_append_submenu(setting_bar, "Setting", G_MENU_MODEL(setting));
	g_object_unref(section1);
	g_object_unref(section2);
	g_object_unref(section3);
	g_object_unref(setting);

	gtk_application_set_menubar(GTK_APPLICATION(app),
	                            G_MENU_MODEL(setting_bar));
}

int
start_gui(int argc, char **argv)
{
	GtkApplication *app;
	int status;
	app = gtk_application_new("dev.ntrain.senddata", G_APPLICATION_FLAGS_NONE);
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	g_signal_connect(app, "startup", G_CALLBACK(app_startup), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);
	return status;
}
