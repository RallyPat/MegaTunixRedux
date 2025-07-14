#ifndef __NETWORK_SECURITY_H__
#define __NETWORK_SECURITY_H__

#include <glib.h>
#include <gio/gio.h>
#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>
#include <json-glib/json-glib.h>

G_BEGIN_DECLS

/* Network Security and Multi-User Support for MegaTunix Redux */

/* User roles and permissions */
typedef enum {
    MTX_USER_ROLE_VIEWER,     /* Read-only access */
    MTX_USER_ROLE_TUNER,      /* Can modify tune parameters */
    MTX_USER_ROLE_ADMIN,      /* Full system access */
    MTX_USER_ROLE_OWNER       /* System owner */
} MtxUserRole;

/* Session states */
typedef enum {
    MTX_SESSION_STATE_DISCONNECTED,
    MTX_SESSION_STATE_CONNECTING,
    MTX_SESSION_STATE_AUTHENTICATING,
    MTX_SESSION_STATE_CONNECTED,
    MTX_SESSION_STATE_ERROR
} MtxSessionState;

/* Message types */
typedef enum {
    MTX_MSG_TYPE_AUTH_REQUEST,
    MTX_MSG_TYPE_AUTH_RESPONSE,
    MTX_MSG_TYPE_DATA_UPDATE,
    MTX_MSG_TYPE_TUNE_CHANGE,
    MTX_MSG_TYPE_REALTIME_DATA,
    MTX_MSG_TYPE_USER_LIST,
    MTX_MSG_TYPE_CHAT_MESSAGE,
    MTX_MSG_TYPE_SYSTEM_STATUS,
    MTX_MSG_TYPE_ERROR
} MtxMessageType;

/* User information */
typedef struct {
    gchar *username;
    gchar *display_name;
    gchar *email;
    MtxUserRole role;
    gchar *session_token;
    gint64 last_activity;
    gboolean is_online;
    GSocketAddress *address;
} MtxUser;

/* Session information */
typedef struct {
    gchar *session_id;
    MtxUser *user;
    GSocket *socket;
    gnutls_session_t tls_session;
    MtxSessionState state;
    GByteArray *read_buffer;
    GByteArray *write_buffer;
    gint64 connect_time;
    gint64 last_activity;
    gboolean encrypted;
} MtxSession;

/* Network message */
typedef struct {
    MtxMessageType type;
    gchar *sender;
    gchar *recipient;  /* NULL for broadcast */
    gint64 timestamp;
    JsonNode *payload;
    gsize payload_size;
    guint32 sequence_number;
    gchar *checksum;
} MtxNetworkMessage;

/* Server configuration */
typedef struct {
    gchar *server_name;
    gchar *bind_address;
    guint16 port;
    gchar *cert_file;
    gchar *key_file;
    gchar *ca_file;
    gboolean require_encryption;
    gboolean allow_anonymous;
    guint max_connections;
    guint session_timeout;
    gchar *user_db_file;
} MtxServerConfig;

/* Client configuration */
typedef struct {
    gchar *server_address;
    guint16 server_port;
    gchar *username;
    gchar *password;
    gchar *ca_file;
    gboolean verify_certificate;
    guint connect_timeout;
    guint keepalive_interval;
} MtxClientConfig;

/* Server instance */
typedef struct {
    MtxServerConfig *config;
    GSocketService *service;
    gnutls_certificate_credentials_t tls_creds;
    GHashTable *sessions;      /* session_id -> MtxSession */
    GHashTable *users;         /* username -> MtxUser */
    GMutex sessions_mutex;
    GMutex users_mutex;
    guint32 next_sequence;
    gboolean running;
    
    /* Callbacks */
    void (*user_connected)(MtxUser *user, gpointer user_data);
    void (*user_disconnected)(MtxUser *user, gpointer user_data);
    void (*message_received)(MtxSession *session, MtxNetworkMessage *message, gpointer user_data);
    gpointer callback_data;
} MtxNetworkServer;

/* Client instance */
typedef struct {
    MtxClientConfig *config;
    GSocketClient *client;
    GSocketConnection *connection;
    gnutls_session_t tls_session;
    MtxSession *session;
    MtxSessionState state;
    GMutex state_mutex;
    guint32 next_sequence;
    
    /* Callbacks */
    void (*state_changed)(MtxSessionState state, gpointer user_data);
    void (*message_received)(MtxNetworkMessage *message, gpointer user_data);
    void (*user_list_updated)(GList *users, gpointer user_data);
    gpointer callback_data;
} MtxNetworkClient;

/* Initialization and cleanup */
gboolean mtx_network_security_init(void);
void mtx_network_security_cleanup(void);

/* Server functions */
MtxNetworkServer *mtx_network_server_new(MtxServerConfig *config);
void mtx_network_server_free(MtxNetworkServer *server);
gboolean mtx_network_server_start(MtxNetworkServer *server, GError **error);
void mtx_network_server_stop(MtxNetworkServer *server);
void mtx_network_server_broadcast_message(MtxNetworkServer *server, 
                                         MtxNetworkMessage *message);
void mtx_network_server_send_to_user(MtxNetworkServer *server,
                                    const gchar *username,
                                    MtxNetworkMessage *message);

/* Client functions */
MtxNetworkClient *mtx_network_client_new(MtxClientConfig *config);
void mtx_network_client_free(MtxNetworkClient *client);
gboolean mtx_network_client_connect(MtxNetworkClient *client, GError **error);
void mtx_network_client_disconnect(MtxNetworkClient *client);
gboolean mtx_network_client_send_message(MtxNetworkClient *client,
                                        MtxNetworkMessage *message,
                                        GError **error);

/* Message functions */
MtxNetworkMessage *mtx_network_message_new(MtxMessageType type);
void mtx_network_message_free(MtxNetworkMessage *message);
gboolean mtx_network_message_set_payload(MtxNetworkMessage *message,
                                        JsonNode *payload);
JsonNode *mtx_network_message_get_payload(MtxNetworkMessage *message);
gchar *mtx_network_message_serialize(MtxNetworkMessage *message);
MtxNetworkMessage *mtx_network_message_deserialize(const gchar *data);

/* User management */
MtxUser *mtx_user_new(const gchar *username, const gchar *display_name);
void mtx_user_free(MtxUser *user);
gboolean mtx_user_authenticate(const gchar *username, 
                              const gchar *password,
                              MtxUser **user);
gboolean mtx_user_has_permission(MtxUser *user, const gchar *permission);
gchar *mtx_user_generate_session_token(MtxUser *user);

/* Session management */
MtxSession *mtx_session_new(GSocket *socket, MtxUser *user);
void mtx_session_free(MtxSession *session);
gboolean mtx_session_setup_tls(MtxSession *session, 
                              gnutls_certificate_credentials_t creds,
                              gboolean is_server,
                              GError **error);
void mtx_session_update_activity(MtxSession *session);
gboolean mtx_session_is_expired(MtxSession *session, guint timeout_seconds);

/* Security utilities */
gchar *mtx_security_generate_salt(void);
gchar *mtx_security_hash_password(const gchar *password, const gchar *salt);
gboolean mtx_security_verify_password(const gchar *password,
                                     const gchar *hash,
                                     const gchar *salt);
gchar *mtx_security_generate_session_token(void);
gboolean mtx_security_validate_session_token(const gchar *token);

/* Encryption utilities */
gboolean mtx_crypto_encrypt_data(const guchar *data,
                                gsize data_len,
                                const guchar *key,
                                guchar **encrypted_data,
                                gsize *encrypted_len,
                                GError **error);
gboolean mtx_crypto_decrypt_data(const guchar *encrypted_data,
                                gsize encrypted_len,
                                const guchar *key,
                                guchar **decrypted_data,
                                gsize *decrypted_len,
                                GError **error);

/* Certificate management */
gboolean mtx_tls_load_certificates(const gchar *cert_file,
                                  const gchar *key_file,
                                  const gchar *ca_file,
                                  gnutls_certificate_credentials_t *creds,
                                  GError **error);
gboolean mtx_tls_verify_certificate(gnutls_session_t session);

/* Protocol helpers */
gboolean mtx_protocol_send_auth_request(MtxSession *session,
                                       const gchar *username,
                                       const gchar *password);
gboolean mtx_protocol_send_auth_response(MtxSession *session,
                                        gboolean success,
                                        const gchar *session_token,
                                        MtxUserRole role);
gboolean mtx_protocol_send_realtime_data(MtxSession *session,
                                        JsonNode *data);
gboolean mtx_protocol_send_tune_change(MtxSession *session,
                                      const gchar *parameter,
                                      JsonNode *value);

/* Configuration helpers */
MtxServerConfig *mtx_server_config_new(void);
void mtx_server_config_free(MtxServerConfig *config);
gboolean mtx_server_config_load_from_file(MtxServerConfig *config,
                                         const gchar *filename,
                                         GError **error);

MtxClientConfig *mtx_client_config_new(void);
void mtx_client_config_free(MtxClientConfig *config);
gboolean mtx_client_config_load_from_file(MtxClientConfig *config,
                                         const gchar *filename,
                                         GError **error);

/* Utility functions */
const gchar *mtx_user_role_to_string(MtxUserRole role);
MtxUserRole mtx_user_role_from_string(const gchar *role_str);
const gchar *mtx_session_state_to_string(MtxSessionState state);
const gchar *mtx_message_type_to_string(MtxMessageType type);

G_END_DECLS

#endif /* __NETWORK_SECURITY_H__ */
