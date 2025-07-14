/*
 * Network Security Demo for MegaTunix Redux
 * Demonstrates multi-user collaborative tuning capabilities
 */

#include <gtk/gtk.h>
#include <glib.h>
#include "network_security.h"

/* Demo application structure */
typedef struct {
    GtkWidget *window;
    GtkWidget *notebook;
    GtkWidget *status_label;
    GtkWidget *user_list;
    GtkWidget *chat_view;
    GtkWidget *chat_entry;
    GtkWidget *server_controls;
    GtkWidget *client_controls;
    
    MtxNetworkServer *server;
    MtxNetworkClient *client;
    
    gboolean is_server;
    gchar *username;
    MtxUserRole role;
} NetworkDemo;

/* GUI setup functions */
static void setup_server_tab(NetworkDemo *demo);
static void setup_client_tab(NetworkDemo *demo);
static void setup_chat_tab(NetworkDemo *demo);
static void setup_users_tab(NetworkDemo *demo);

/* Event handlers */
static void on_start_server_clicked(GtkButton *button, NetworkDemo *demo);
static void on_connect_client_clicked(GtkButton *button, NetworkDemo *demo);
static void on_send_chat_clicked(GtkButton *button, NetworkDemo *demo);
static void on_user_role_changed(GtkComboBox *combo, NetworkDemo *demo);

/* Network callbacks */
static void on_server_user_connected(MtxUser *user, NetworkDemo *demo);
static void on_server_user_disconnected(MtxUser *user, NetworkDemo *demo);
static void on_server_message_received(MtxSession *session, MtxNetworkMessage *message, NetworkDemo *demo);
static void on_client_state_changed(MtxSessionState state, NetworkDemo *demo);
static void on_client_message_received(MtxNetworkMessage *message, NetworkDemo *demo);

/* Demo implementation */
static void setup_demo_window(NetworkDemo *demo)
{
    demo->window = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(demo->window), "MegaTunix Redux - Network Security Demo");
    gtk_window_set_default_size(GTK_WINDOW(demo->window), 800, 600);
    
    demo->notebook = gtk_notebook_new();
    gtk_window_set_child(GTK_WINDOW(demo->window), demo->notebook);
    
    setup_server_tab(demo);
    setup_client_tab(demo);
    setup_chat_tab(demo);
    setup_users_tab(demo);
    
    demo->status_label = gtk_label_new("Ready - Select Server or Client mode");
    gtk_notebook_append_page(GTK_NOTEBOOK(demo->notebook), demo->status_label, gtk_label_new("Status"));
    
    /* Apply modern styling */
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(provider, 
        "window { background-color: #2b2b2b; color: #ffffff; }\n"
        "notebook { background-color: #3b3b3b; }\n"
        "button { background-color: #4a4a4a; color: #ffffff; border: 1px solid #666666; }\n"
        "button:hover { background-color: #5a5a5a; }\n"
        "entry { background-color: #4a4a4a; color: #ffffff; }\n"
        "textview { background-color: #333333; color: #ffffff; }\n"
        "label { color: #ffffff; }\n");
    
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    
    g_object_unref(provider);
}

static void setup_server_tab(NetworkDemo *demo)
{
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(vbox, 10);
    gtk_widget_set_margin_end(vbox, 10);
    gtk_widget_set_margin_top(vbox, 10);
    gtk_widget_set_margin_bottom(vbox, 10);
    
    /* Server configuration */
    GtkWidget *config_frame = gtk_frame_new("Server Configuration");
    GtkWidget *config_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(config_grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(config_grid), 5);
    gtk_frame_set_child(GTK_FRAME(config_frame), config_grid);
    
    gtk_grid_attach(GTK_GRID(config_grid), gtk_label_new("Server Name:"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(config_grid), gtk_entry_new(), 1, 0, 1, 1);
    
    gtk_grid_attach(GTK_GRID(config_grid), gtk_label_new("Port:"), 0, 1, 1, 1);
    GtkWidget *port_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(port_entry), "8080");
    gtk_grid_attach(GTK_GRID(config_grid), port_entry, 1, 1, 1, 1);
    
    gtk_grid_attach(GTK_GRID(config_grid), gtk_label_new("Max Connections:"), 0, 2, 1, 1);
    GtkWidget *max_conn_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(max_conn_entry), "10");
    gtk_grid_attach(GTK_GRID(config_grid), max_conn_entry, 1, 2, 1, 1);
    
    /* Server controls */
    GtkWidget *controls_frame = gtk_frame_new("Server Controls");
    demo->server_controls = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_frame_set_child(GTK_FRAME(controls_frame), demo->server_controls);
    
    GtkWidget *start_server_btn = gtk_button_new_with_label("Start Server");
    g_signal_connect(start_server_btn, "clicked", G_CALLBACK(on_start_server_clicked), demo);
    gtk_box_append(GTK_BOX(demo->server_controls), start_server_btn);
    
    GtkWidget *stop_server_btn = gtk_button_new_with_label("Stop Server");
    gtk_box_append(GTK_BOX(demo->server_controls), stop_server_btn);
    
    /* Server statistics */
    GtkWidget *stats_frame = gtk_frame_new("Server Statistics");
    GtkWidget *stats_grid = gtk_grid_new();
    gtk_frame_set_child(GTK_FRAME(stats_frame), stats_grid);
    
    gtk_grid_attach(GTK_GRID(stats_grid), gtk_label_new("Active Users:"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(stats_grid), gtk_label_new("0"), 1, 0, 1, 1);
    
    gtk_grid_attach(GTK_GRID(stats_grid), gtk_label_new("Messages Sent:"), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(stats_grid), gtk_label_new("0"), 1, 1, 1, 1);
    
    gtk_box_append(GTK_BOX(vbox), config_frame);
    gtk_box_append(GTK_BOX(vbox), controls_frame);
    gtk_box_append(GTK_BOX(vbox), stats_frame);
    
    gtk_notebook_append_page(GTK_NOTEBOOK(demo->notebook), vbox, gtk_label_new("Server"));
}

static void setup_client_tab(NetworkDemo *demo)
{
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(vbox, 10);
    gtk_widget_set_margin_end(vbox, 10);
    gtk_widget_set_margin_top(vbox, 10);
    gtk_widget_set_margin_bottom(vbox, 10);
    
    /* Client configuration */
    GtkWidget *config_frame = gtk_frame_new("Client Configuration");
    GtkWidget *config_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(config_grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(config_grid), 5);
    gtk_frame_set_child(GTK_FRAME(config_frame), config_grid);
    
    gtk_grid_attach(GTK_GRID(config_grid), gtk_label_new("Username:"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(config_grid), gtk_entry_new(), 1, 0, 1, 1);
    
    gtk_grid_attach(GTK_GRID(config_grid), gtk_label_new("Server:"), 0, 1, 1, 1);
    GtkWidget *server_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(server_entry), "localhost");
    gtk_grid_attach(GTK_GRID(config_grid), server_entry, 1, 1, 1, 1);
    
    gtk_grid_attach(GTK_GRID(config_grid), gtk_label_new("Port:"), 0, 2, 1, 1);
    GtkWidget *port_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(port_entry), "8080");
    gtk_grid_attach(GTK_GRID(config_grid), port_entry, 1, 2, 1, 1);
    
    gtk_grid_attach(GTK_GRID(config_grid), gtk_label_new("Role:"), 0, 3, 1, 1);
    GtkWidget *role_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(role_combo), "Viewer");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(role_combo), "Tuner");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(role_combo), "Admin");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(role_combo), "Owner");
    gtk_combo_box_set_active(GTK_COMBO_BOX(role_combo), 0);
    gtk_grid_attach(GTK_GRID(config_grid), role_combo, 1, 3, 1, 1);
    
    /* Client controls */
    GtkWidget *controls_frame = gtk_frame_new("Client Controls");
    demo->client_controls = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_frame_set_child(GTK_FRAME(controls_frame), demo->client_controls);
    
    GtkWidget *connect_btn = gtk_button_new_with_label("Connect");
    g_signal_connect(connect_btn, "clicked", G_CALLBACK(on_connect_client_clicked), demo);
    gtk_box_append(GTK_BOX(demo->client_controls), connect_btn);
    
    GtkWidget *disconnect_btn = gtk_button_new_with_label("Disconnect");
    gtk_box_append(GTK_BOX(demo->client_controls), disconnect_btn);
    
    gtk_box_append(GTK_BOX(vbox), config_frame);
    gtk_box_append(GTK_BOX(vbox), controls_frame);
    
    gtk_notebook_append_page(GTK_NOTEBOOK(demo->notebook), vbox, gtk_label_new("Client"));
}

static void setup_chat_tab(NetworkDemo *demo)
{
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(vbox, 10);
    gtk_widget_set_margin_end(vbox, 10);
    gtk_widget_set_margin_top(vbox, 10);
    gtk_widget_set_margin_bottom(vbox, 10);
    
    /* Chat view */
    demo->chat_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(demo->chat_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(demo->chat_view), FALSE);
    
    GtkWidget *chat_scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(chat_scroll), 
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(chat_scroll), demo->chat_view);
    
    /* Chat input */
    GtkWidget *input_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    demo->chat_entry = gtk_entry_new();
    gtk_widget_set_hexpand(demo->chat_entry, TRUE);
    
    GtkWidget *send_btn = gtk_button_new_with_label("Send");
    g_signal_connect(send_btn, "clicked", G_CALLBACK(on_send_chat_clicked), demo);
    
    gtk_box_append(GTK_BOX(input_box), demo->chat_entry);
    gtk_box_append(GTK_BOX(input_box), send_btn);
    
    gtk_box_append(GTK_BOX(vbox), chat_scroll);
    gtk_box_append(GTK_BOX(vbox), input_box);
    
    gtk_notebook_append_page(GTK_NOTEBOOK(demo->notebook), vbox, gtk_label_new("Chat"));
}

static void setup_users_tab(NetworkDemo *demo)
{
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(vbox, 10);
    gtk_widget_set_margin_end(vbox, 10);
    gtk_widget_set_margin_top(vbox, 10);
    gtk_widget_set_margin_bottom(vbox, 10);
    
    /* Users list */
    demo->user_list = gtk_list_box_new();
    
    GtkWidget *users_scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(users_scroll), 
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(users_scroll), demo->user_list);
    
    gtk_box_append(GTK_BOX(vbox), users_scroll);
    
    gtk_notebook_append_page(GTK_NOTEBOOK(demo->notebook), vbox, gtk_label_new("Users"));
}

/* Event handlers */
static void on_start_server_clicked(GtkButton *button, NetworkDemo *demo)
{
    /* Initialize server configuration */
    MtxServerConfig *config = mtx_server_config_new();
    config->server_name = g_strdup("MegaTunix Redux Demo Server");
    config->bind_address = g_strdup("127.0.0.1");
    config->port = 8080;
    config->max_connections = 10;
    config->require_encryption = TRUE;
    config->allow_anonymous = FALSE;
    
    /* Create and start server */
    demo->server = mtx_network_server_new(config);
    if (demo->server) {
        demo->server->user_connected = (void(*)(MtxUser*, gpointer))on_server_user_connected;
        demo->server->user_disconnected = (void(*)(MtxUser*, gpointer))on_server_user_disconnected;
        demo->server->message_received = (void(*)(MtxSession*, MtxNetworkMessage*, gpointer))on_server_message_received;
        demo->server->callback_data = demo;
        
        GError *error = NULL;
        if (mtx_network_server_start(demo->server, &error)) {
            gtk_label_set_text(GTK_LABEL(demo->status_label), "Server started successfully");
        } else {
            gtk_label_set_text(GTK_LABEL(demo->status_label), 
                             error ? error->message : "Failed to start server");
            g_clear_error(&error);
        }
    }
}

static void on_connect_client_clicked(GtkButton *button, NetworkDemo *demo)
{
    /* Initialize client configuration */
    MtxClientConfig *config = mtx_client_config_new();
    config->server_address = g_strdup("127.0.0.1");
    config->server_port = 8080;
    config->username = g_strdup("demo_user");
    config->password = g_strdup("demo_pass");
    
    /* Create and connect client */
    demo->client = mtx_network_client_new(config);
    if (demo->client) {
        demo->client->state_changed = (void(*)(MtxSessionState, gpointer))on_client_state_changed;
        demo->client->message_received = (void(*)(MtxNetworkMessage*, gpointer))on_client_message_received;
        demo->client->callback_data = demo;
        
        GError *error = NULL;
        if (mtx_network_client_connect(demo->client, &error)) {
            gtk_label_set_text(GTK_LABEL(demo->status_label), "Connecting to server...");
        } else {
            gtk_label_set_text(GTK_LABEL(demo->status_label), 
                             error ? error->message : "Failed to connect to server");
            g_clear_error(&error);
        }
    }
}

static void on_send_chat_clicked(GtkButton *button, NetworkDemo *demo)
{
    const gchar *message_text = gtk_editable_get_text(GTK_EDITABLE(demo->chat_entry));
    if (message_text && strlen(message_text) > 0) {
        MtxNetworkMessage *message = mtx_network_message_new(MTX_MSG_TYPE_CHAT_MESSAGE);
        if (message) {
            /* Add message payload */
            JsonNode *payload = json_node_new(JSON_NODE_OBJECT);
            JsonObject *obj = json_object_new();
            json_object_set_string_member(obj, "message", message_text);
            json_object_set_string_member(obj, "sender", demo->username ? demo->username : "unknown");
            json_node_set_object(payload, obj);
            
            mtx_network_message_set_payload(message, payload);
            
            if (demo->client) {
                GError *error = NULL;
                if (mtx_network_client_send_message(demo->client, message, &error)) {
                    gtk_editable_set_text(GTK_EDITABLE(demo->chat_entry), "");
                } else {
                    gtk_label_set_text(GTK_LABEL(demo->status_label), 
                                     error ? error->message : "Failed to send message");
                    g_clear_error(&error);
                }
            }
            
            mtx_network_message_free(message);
        }
    }
}

/* Network callbacks */
static void on_server_user_connected(MtxUser *user, NetworkDemo *demo)
{
    gchar *status_text = g_strdup_printf("User connected: %s (%s)", 
                                        user->display_name, 
                                        mtx_user_role_to_string(user->role));
    gtk_label_set_text(GTK_LABEL(demo->status_label), status_text);
    g_free(status_text);
}

static void on_server_user_disconnected(MtxUser *user, NetworkDemo *demo)
{
    gchar *status_text = g_strdup_printf("User disconnected: %s", user->display_name);
    gtk_label_set_text(GTK_LABEL(demo->status_label), status_text);
    g_free(status_text);
}

static void on_server_message_received(MtxSession *session, MtxNetworkMessage *message, NetworkDemo *demo)
{
    gchar *msg_type = g_strdup(mtx_message_type_to_string(message->type));
    gchar *status_text = g_strdup_printf("Message received: %s from %s", msg_type, message->sender);
    gtk_label_set_text(GTK_LABEL(demo->status_label), status_text);
    g_free(msg_type);
    g_free(status_text);
}

static void on_client_state_changed(MtxSessionState state, NetworkDemo *demo)
{
    const gchar *state_text = mtx_session_state_to_string(state);
    gchar *status_text = g_strdup_printf("Client state: %s", state_text);
    gtk_label_set_text(GTK_LABEL(demo->status_label), status_text);
    g_free(status_text);
}

static void on_client_message_received(MtxNetworkMessage *message, NetworkDemo *demo)
{
    if (message->type == MTX_MSG_TYPE_CHAT_MESSAGE) {
        JsonNode *payload = mtx_network_message_get_payload(message);
        if (payload && JSON_NODE_HOLDS_OBJECT(payload)) {
            JsonObject *obj = json_node_get_object(payload);
            const gchar *msg_text = json_object_get_string_member(obj, "message");
            const gchar *sender = json_object_get_string_member(obj, "sender");
            
            if (msg_text && sender) {
                GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(demo->chat_view));
                GtkTextIter end;
                gtk_text_buffer_get_end_iter(buffer, &end);
                
                gchar *formatted_msg = g_strdup_printf("%s: %s\n", sender, msg_text);
                gtk_text_buffer_insert(buffer, &end, formatted_msg, -1);
                g_free(formatted_msg);
            }
        }
    }
}

/* Main demo function */
static void activate_demo(GtkApplication *app, NetworkDemo *demo)
{
    setup_demo_window(demo);
    gtk_window_set_application(GTK_WINDOW(demo->window), app);
    gtk_widget_set_visible(demo->window, TRUE);
}

int main(int argc, char *argv[])
{
    /* Initialize network security */
    if (!mtx_network_security_init()) {
        g_error("Failed to initialize network security subsystem");
        return 1;
    }
    
    /* Create demo application */
    GtkApplication *app = gtk_application_new("com.megatunix.redux.networkdemo", G_APPLICATION_DEFAULT_FLAGS);
    
    NetworkDemo demo = {0};
    demo.username = g_strdup("demo_user");
    demo.role = MTX_USER_ROLE_TUNER;
    
    g_signal_connect(app, "activate", G_CALLBACK(activate_demo), &demo);
    
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    
    /* Cleanup */
    if (demo.server) {
        mtx_network_server_stop(demo.server);
        mtx_network_server_free(demo.server);
    }
    
    if (demo.client) {
        mtx_network_client_disconnect(demo.client);
        mtx_network_client_free(demo.client);
    }
    
    g_free(demo.username);
    g_object_unref(app);
    
    mtx_network_security_cleanup();
    
    return status;
}
