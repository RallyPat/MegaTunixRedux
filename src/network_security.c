#include "network_security.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/random.h>

/* Forward declarations */
static gboolean on_server_incoming_connection(GSocketService *service,
                                             GSocketConnection *connection,
                                             GObject *source_object,
                                             gpointer user_data);

/* Initialize network security subsystem */
gboolean mtx_network_security_init(void)
{
    g_debug("Initializing network security subsystem");
    
    /* Initialize GnuTLS */
    gint ret = gnutls_global_init();
    if (ret < 0) {
        g_warning("Failed to initialize GnuTLS: %s", gnutls_strerror(ret));
        return FALSE;
    }
    
    g_debug("Network security subsystem initialized successfully");
    return TRUE;
}

/* Cleanup network security subsystem */
void mtx_network_security_cleanup(void)
{
    g_debug("Cleaning up network security subsystem");
    gnutls_global_deinit();
}

/* Create new network server */
MtxNetworkServer *mtx_network_server_new(MtxServerConfig *config)
{
    g_return_val_if_fail(config != NULL, NULL);
    
    MtxNetworkServer *server = g_new0(MtxNetworkServer, 1);
    
    server->config = config;
    server->service = g_socket_service_new();
    server->sessions = g_hash_table_new_full(g_str_hash, g_str_equal,
                                           g_free, (GDestroyNotify)mtx_session_free);
    server->users = g_hash_table_new_full(g_str_hash, g_str_equal,
                                         g_free, (GDestroyNotify)mtx_user_free);
    
    g_mutex_init(&server->sessions_mutex);
    g_mutex_init(&server->users_mutex);
    
    server->next_sequence = 1;
    server->running = FALSE;
    
    return server;
}

/* Free network server */
void mtx_network_server_free(MtxNetworkServer *server)
{
    if (!server) return;
    
    mtx_network_server_stop(server);
    
    g_object_unref(server->service);
    g_hash_table_destroy(server->sessions);
    g_hash_table_destroy(server->users);
    
    g_mutex_clear(&server->sessions_mutex);
    g_mutex_clear(&server->users_mutex);
    
    if (server->tls_creds) {
        gnutls_certificate_free_credentials(server->tls_creds);
    }
    
    g_free(server);
}

/* Start network server */
gboolean mtx_network_server_start(MtxNetworkServer *server, GError **error)
{
    g_return_val_if_fail(server != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);
    
    /* Load TLS certificates if encryption is required */
    if (server->config->require_encryption) {
        if (!mtx_tls_load_certificates(server->config->cert_file,
                                      server->config->key_file,
                                      server->config->ca_file,
                                      &server->tls_creds,
                                      error)) {
            return FALSE;
        }
    }
    
    /* Add listening address */
    GSocketAddress *address = g_inet_socket_address_new_from_string(
        server->config->bind_address, server->config->port);
    
    if (!address) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
                   "Invalid bind address: %s", server->config->bind_address);
        return FALSE;
    }
    
    if (!g_socket_listener_add_address(G_SOCKET_LISTENER(server->service),
                                      address, G_SOCKET_TYPE_STREAM,
                                      G_SOCKET_PROTOCOL_TCP, NULL, NULL, error)) {
        g_object_unref(address);
        return FALSE;
    }
    
    g_object_unref(address);
    
    /* Connect incoming signal */
    g_signal_connect(server->service, "incoming",
                    G_CALLBACK(on_server_incoming_connection), server);
    
    /* Start the service */
    g_socket_service_start(server->service);
    server->running = TRUE;
    
    g_info("Network server started on %s:%d", 
           server->config->bind_address, server->config->port);
    
    return TRUE;
}

/* Stop network server */
void mtx_network_server_stop(MtxNetworkServer *server)
{
    if (!server || !server->running) return;
    
    g_socket_service_stop(server->service);
    server->running = FALSE;
    
    /* Close all sessions */
    g_mutex_lock(&server->sessions_mutex);
    g_hash_table_remove_all(server->sessions);
    g_mutex_unlock(&server->sessions_mutex);
    
    g_info("Network server stopped");
}

/* Create new network client */
MtxNetworkClient *mtx_network_client_new(MtxClientConfig *config)
{
    g_return_val_if_fail(config != NULL, NULL);
    
    MtxNetworkClient *client = g_new0(MtxNetworkClient, 1);
    
    client->config = config;
    client->client = g_socket_client_new();
    client->state = MTX_SESSION_STATE_DISCONNECTED;
    client->next_sequence = 1;
    
    g_mutex_init(&client->state_mutex);
    
    /* Configure socket client */
    g_socket_client_set_timeout(client->client, client->config->connect_timeout);
    
    return client;
}

/* Free network client */
void mtx_network_client_free(MtxNetworkClient *client)
{
    if (!client) return;
    
    mtx_network_client_disconnect(client);
    
    g_object_unref(client->client);
    g_mutex_clear(&client->state_mutex);
    
    if (client->session) {
        mtx_session_free(client->session);
    }
    
    g_free(client);
}

/* Connect network client */
gboolean mtx_network_client_connect(MtxNetworkClient *client, GError **error)
{
    g_return_val_if_fail(client != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);
    
    g_mutex_lock(&client->state_mutex);
    
    if (client->state != MTX_SESSION_STATE_DISCONNECTED) {
        g_mutex_unlock(&client->state_mutex);
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                   "Client is not in disconnected state");
        return FALSE;
    }
    
    client->state = MTX_SESSION_STATE_CONNECTING;
    g_mutex_unlock(&client->state_mutex);
    
    /* Connect to server */
    client->connection = g_socket_client_connect_to_host(
        client->client,
        client->config->server_address,
        client->config->server_port,
        NULL, error);
    
    if (!client->connection) {
        g_mutex_lock(&client->state_mutex);
        client->state = MTX_SESSION_STATE_ERROR;
        g_mutex_unlock(&client->state_mutex);
        return FALSE;
    }
    
    /* Create session */
    GSocket *socket = g_socket_connection_get_socket(client->connection);
    client->session = mtx_session_new(socket, NULL);
    
    /* Setup TLS if certificate verification is enabled */
    if (client->config->verify_certificate) {
        gnutls_certificate_credentials_t creds;
        gnutls_certificate_allocate_credentials(&creds);
        
        if (client->config->ca_file) {
            gnutls_certificate_set_x509_trust_file(creds, client->config->ca_file,
                                                  GNUTLS_X509_FMT_PEM);
        }
        
        if (!mtx_session_setup_tls(client->session, creds, FALSE, error)) {
            gnutls_certificate_free_credentials(creds);
            g_mutex_lock(&client->state_mutex);
            client->state = MTX_SESSION_STATE_ERROR;
            g_mutex_unlock(&client->state_mutex);
            return FALSE;
        }
        
        gnutls_certificate_free_credentials(creds);
    }
    
    g_mutex_lock(&client->state_mutex);
    client->state = MTX_SESSION_STATE_AUTHENTICATING;
    g_mutex_unlock(&client->state_mutex);
    
    /* Send authentication request */
    if (!mtx_protocol_send_auth_request(client->session,
                                       client->config->username,
                                       client->config->password)) {
        g_mutex_lock(&client->state_mutex);
        client->state = MTX_SESSION_STATE_ERROR;
        g_mutex_unlock(&client->state_mutex);
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "Failed to send authentication request");
        return FALSE;
    }
    
    g_info("Connected to server %s:%d", 
           client->config->server_address, client->config->server_port);
    
    return TRUE;
}

/* Disconnect network client */
void mtx_network_client_disconnect(MtxNetworkClient *client)
{
    if (!client) return;
    
    g_mutex_lock(&client->state_mutex);
    
    if (client->state == MTX_SESSION_STATE_DISCONNECTED) {
        g_mutex_unlock(&client->state_mutex);
        return;
    }
    
    client->state = MTX_SESSION_STATE_DISCONNECTED;
    g_mutex_unlock(&client->state_mutex);
    
    if (client->connection) {
        g_object_unref(client->connection);
        client->connection = NULL;
    }
    
    if (client->session) {
        mtx_session_free(client->session);
        client->session = NULL;
    }
    
    g_info("Disconnected from server");
}

/* Create new network message */
MtxNetworkMessage *mtx_network_message_new(MtxMessageType type)
{
    MtxNetworkMessage *message = g_new0(MtxNetworkMessage, 1);
    
    message->type = type;
    message->timestamp = g_get_real_time();
    message->sequence_number = 0; /* Will be set when sending */
    
    return message;
}

/* Free network message */
void mtx_network_message_free(MtxNetworkMessage *message)
{
    if (!message) return;
    
    g_free(message->sender);
    g_free(message->recipient);
    g_free(message->checksum);
    
    if (message->payload) {
        json_node_unref(message->payload);
    }
    
    g_free(message);
}

/* Set message payload */
gboolean mtx_network_message_set_payload(MtxNetworkMessage *message,
                                        JsonNode *payload)
{
    g_return_val_if_fail(message != NULL, FALSE);
    
    if (message->payload) {
        json_node_unref(message->payload);
    }
    
    message->payload = payload ? json_node_ref(payload) : NULL;
    return TRUE;
}

/* Get message payload */
JsonNode *mtx_network_message_get_payload(MtxNetworkMessage *message)
{
    g_return_val_if_fail(message != NULL, NULL);
    return message->payload;
}

/* Create new user */
MtxUser *mtx_user_new(const gchar *username, const gchar *display_name)
{
    g_return_val_if_fail(username != NULL, NULL);
    
    MtxUser *user = g_new0(MtxUser, 1);
    
    user->username = g_strdup(username);
    user->display_name = g_strdup(display_name ? display_name : username);
    user->role = MTX_USER_ROLE_VIEWER;
    user->last_activity = g_get_real_time();
    user->is_online = FALSE;
    
    return user;
}

/* Free user */
void mtx_user_free(MtxUser *user)
{
    if (!user) return;
    
    g_free(user->username);
    g_free(user->display_name);
    g_free(user->email);
    g_free(user->session_token);
    
    if (user->address) {
        g_object_unref(user->address);
    }
    
    g_free(user);
}

/* Create new session */
MtxSession *mtx_session_new(GSocket *socket, MtxUser *user)
{
    g_return_val_if_fail(socket != NULL, NULL);
    
    MtxSession *session = g_new0(MtxSession, 1);
    
    session->session_id = g_uuid_string_random();
    session->user = user;
    session->socket = g_object_ref(socket);
    session->state = MTX_SESSION_STATE_CONNECTED;
    session->read_buffer = g_byte_array_new();
    session->write_buffer = g_byte_array_new();
    session->connect_time = g_get_real_time();
    session->last_activity = session->connect_time;
    session->encrypted = FALSE;
    
    return session;
}

/* Free session */
void mtx_session_free(MtxSession *session)
{
    if (!session) return;
    
    g_free(session->session_id);
    
    if (session->socket) {
        g_object_unref(session->socket);
    }
    
    if (session->tls_session) {
        gnutls_deinit(session->tls_session);
    }
    
    if (session->read_buffer) {
        g_byte_array_free(session->read_buffer, TRUE);
    }
    
    if (session->write_buffer) {
        g_byte_array_free(session->write_buffer, TRUE);
    }
    
    g_free(session);
}

/* Generate salt for password hashing */
gchar *mtx_security_generate_salt(void)
{
    guchar salt[16];
    
    if (getrandom(salt, sizeof(salt), 0) != sizeof(salt)) {
        g_warning("Failed to generate random salt");
        return NULL;
    }
    
    return g_base64_encode(salt, sizeof(salt));
}

/* Hash password with salt */
gchar *mtx_security_hash_password(const gchar *password, const gchar *salt)
{
    g_return_val_if_fail(password != NULL, NULL);
    g_return_val_if_fail(salt != NULL, NULL);
    
    gchar *combined = g_strconcat(password, salt, NULL);
    
    guchar hash[32];
    gnutls_hash_hd_t dig;
    
    if (gnutls_hash_init(&dig, GNUTLS_DIG_SHA256) < 0) {
        g_free(combined);
        return NULL;
    }
    
    gnutls_hash(dig, combined, strlen(combined));
    gnutls_hash_deinit(dig, hash);
    
    g_free(combined);
    
    return g_base64_encode(hash, sizeof(hash));
}

/* Verify password against hash */
gboolean mtx_security_verify_password(const gchar *password,
                                     const gchar *hash,
                                     const gchar *salt)
{
    g_return_val_if_fail(password != NULL, FALSE);
    g_return_val_if_fail(hash != NULL, FALSE);
    g_return_val_if_fail(salt != NULL, FALSE);
    
    gchar *computed_hash = mtx_security_hash_password(password, salt);
    if (!computed_hash) return FALSE;
    
    gboolean result = g_strcmp0(hash, computed_hash) == 0;
    g_free(computed_hash);
    
    return result;
}

/* Generate session token */
gchar *mtx_security_generate_session_token(void)
{
    guchar token[32];
    
    if (getrandom(token, sizeof(token), 0) != sizeof(token)) {
        g_warning("Failed to generate random token");
        return NULL;
    }
    
    return g_base64_encode(token, sizeof(token));
}

/* User role to string conversion */
const gchar *mtx_user_role_to_string(MtxUserRole role)
{
    switch (role) {
        case MTX_USER_ROLE_VIEWER: return "viewer";
        case MTX_USER_ROLE_TUNER: return "tuner";
        case MTX_USER_ROLE_ADMIN: return "admin";
        case MTX_USER_ROLE_OWNER: return "owner";
        default: return "unknown";
    }
}

/* String to user role conversion */
MtxUserRole mtx_user_role_from_string(const gchar *role_str)
{
    if (!role_str) return MTX_USER_ROLE_VIEWER;
    
    if (g_strcmp0(role_str, "viewer") == 0) return MTX_USER_ROLE_VIEWER;
    if (g_strcmp0(role_str, "tuner") == 0) return MTX_USER_ROLE_TUNER;
    if (g_strcmp0(role_str, "admin") == 0) return MTX_USER_ROLE_ADMIN;
    if (g_strcmp0(role_str, "owner") == 0) return MTX_USER_ROLE_OWNER;
    
    return MTX_USER_ROLE_VIEWER;
}

/* Session state to string conversion */
const gchar *mtx_session_state_to_string(MtxSessionState state)
{
    switch (state) {
        case MTX_SESSION_STATE_DISCONNECTED: return "disconnected";
        case MTX_SESSION_STATE_CONNECTING: return "connecting";
        case MTX_SESSION_STATE_AUTHENTICATING: return "authenticating";
        case MTX_SESSION_STATE_CONNECTED: return "connected";
        case MTX_SESSION_STATE_ERROR: return "error";
        default: return "unknown";
    }
}

/* Message type to string conversion */
const gchar *mtx_message_type_to_string(MtxMessageType type)
{
    switch (type) {
        case MTX_MSG_TYPE_AUTH_REQUEST: return "auth_request";
        case MTX_MSG_TYPE_AUTH_RESPONSE: return "auth_response";
        case MTX_MSG_TYPE_DATA_UPDATE: return "data_update";
        case MTX_MSG_TYPE_TUNE_CHANGE: return "tune_change";
        case MTX_MSG_TYPE_REALTIME_DATA: return "realtime_data";
        case MTX_MSG_TYPE_USER_LIST: return "user_list";
        case MTX_MSG_TYPE_CHAT_MESSAGE: return "chat_message";
        case MTX_MSG_TYPE_SYSTEM_STATUS: return "system_status";
        case MTX_MSG_TYPE_ERROR: return "error";
        default: return "unknown";
    }
}

/* Create server configuration */
MtxServerConfig *mtx_server_config_new(void)
{
    MtxServerConfig *config = g_new0(MtxServerConfig, 1);
    
    /* Set defaults */
    config->server_name = g_strdup("MegaTunix Redux Server");
    config->bind_address = g_strdup("0.0.0.0");
    config->port = 8080;
    config->require_encryption = TRUE;
    config->allow_anonymous = FALSE;
    config->max_connections = 100;
    config->session_timeout = 3600; /* 1 hour */
    
    return config;
}

/* Free server configuration */
void mtx_server_config_free(MtxServerConfig *config)
{
    if (!config) return;
    
    g_free(config->server_name);
    g_free(config->bind_address);
    g_free(config->cert_file);
    g_free(config->key_file);
    g_free(config->ca_file);
    g_free(config->user_db_file);
    
    g_free(config);
}

/* Create client configuration */
MtxClientConfig *mtx_client_config_new(void)
{
    MtxClientConfig *config = g_new0(MtxClientConfig, 1);
    
    /* Set defaults */
    config->server_address = g_strdup("localhost");
    config->server_port = 8080;
    config->verify_certificate = TRUE;
    config->connect_timeout = 30;
    config->keepalive_interval = 60;
    
    return config;
}

/* Free client configuration */
void mtx_client_config_free(MtxClientConfig *config)
{
    if (!config) return;
    
    g_free(config->server_address);
    g_free(config->username);
    g_free(config->password);
    g_free(config->ca_file);
    
    g_free(config);
}

/* Signal handlers and protocol functions would be implemented here */
/* These are placeholder implementations */

static gboolean on_server_incoming_connection(GSocketService *service,
                                             GSocketConnection *connection,
                                             GObject *source_object,
                                             gpointer user_data)
{
    MtxNetworkServer *server = (MtxNetworkServer *)user_data;
    
    g_debug("New incoming connection");
    
    /* Handle new connection in a separate thread */
    /* Implementation would create new session and handle authentication */
    
    return TRUE;
}

/* TLS setup function */
gboolean mtx_session_setup_tls(MtxSession *session, 
                              gnutls_certificate_credentials_t creds,
                              gboolean is_server,
                              GError **error)
{
    gint ret;
    
    ret = gnutls_init(&session->tls_session, 
                     is_server ? GNUTLS_SERVER : GNUTLS_CLIENT);
    if (ret < 0) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "Failed to initialize TLS session: %s", gnutls_strerror(ret));
        return FALSE;
    }
    
    ret = gnutls_set_default_priority(session->tls_session);
    if (ret < 0) {
        gnutls_deinit(session->tls_session);
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "Failed to set TLS priority: %s", gnutls_strerror(ret));
        return FALSE;
    }
    
    ret = gnutls_credentials_set(session->tls_session, GNUTLS_CRD_CERTIFICATE, creds);
    if (ret < 0) {
        gnutls_deinit(session->tls_session);
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "Failed to set TLS credentials: %s", gnutls_strerror(ret));
        return FALSE;
    }
    
    /* Set transport functions */
    gnutls_transport_set_ptr(session->tls_session, session->socket);
    
    session->encrypted = TRUE;
    return TRUE;
}

/* Protocol function stubs */
gboolean mtx_protocol_send_auth_request(MtxSession *session,
                                       const gchar *username,
                                       const gchar *password)
{
    /* Implementation would serialize and send authentication request */
    return TRUE;
}

gboolean mtx_protocol_send_auth_response(MtxSession *session,
                                        gboolean success,
                                        const gchar *session_token,
                                        MtxUserRole role)
{
    /* Implementation would serialize and send authentication response */
    return TRUE;
}

gboolean mtx_protocol_send_realtime_data(MtxSession *session,
                                        JsonNode *data)
{
    /* Implementation would serialize and send real-time data */
    return TRUE;
}

gboolean mtx_protocol_send_tune_change(MtxSession *session,
                                      const gchar *parameter,
                                      JsonNode *value)
{
    /* Implementation would serialize and send tune change notification */
    return TRUE;
}

/* TLS certificate loading function */
gboolean mtx_tls_load_certificates(const gchar *cert_file,
                                  const gchar *key_file,
                                  const gchar *ca_file,
                                  gnutls_certificate_credentials_t *creds,
                                  GError **error)
{
    gint ret;
    
    ret = gnutls_certificate_allocate_credentials(creds);
    if (ret < 0) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "Failed to allocate credentials: %s", gnutls_strerror(ret));
        return FALSE;
    }
    
    if (cert_file && key_file) {
        ret = gnutls_certificate_set_x509_key_file(*creds, cert_file, key_file,
                                                  GNUTLS_X509_FMT_PEM);
        if (ret < 0) {
            gnutls_certificate_free_credentials(*creds);
            g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                       "Failed to load certificate/key: %s", gnutls_strerror(ret));
            return FALSE;
        }
    }
    
    if (ca_file) {
        ret = gnutls_certificate_set_x509_trust_file(*creds, ca_file,
                                                    GNUTLS_X509_FMT_PEM);
        if (ret < 0) {
            gnutls_certificate_free_credentials(*creds);
            g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                       "Failed to load CA file: %s", gnutls_strerror(ret));
            return FALSE;
        }
    }
    
    return TRUE;
}
