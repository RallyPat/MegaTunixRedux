/*
 * Security utilities for MegaTunix Redux
 * Secure string handling and input validation
 */

#include <glib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* Secure string copy with bounds checking */
gboolean mtx_strncpy_secure(gchar *dest, const gchar *src, gsize dest_size)
{
    if (!dest || !src || dest_size == 0) {
        return FALSE;
    }
    
    gsize src_len = strlen(src);
    if (src_len >= dest_size) {
        return FALSE;  /* Source too long */
    }
    
    memcpy(dest, src, src_len + 1);  /* Include null terminator */
    return TRUE;
}

/* Secure string concatenation */
gboolean mtx_strncat_secure(gchar *dest, const gchar *src, gsize dest_size)
{
    if (!dest || !src || dest_size == 0) {
        return FALSE;
    }
    
    gsize dest_len = strlen(dest);
    gsize src_len = strlen(src);
    
    if (dest_len + src_len >= dest_size) {
        return FALSE;  /* Result would be too long */
    }
    
    memcpy(dest + dest_len, src, src_len + 1);
    return TRUE;
}

/* Validate filename for security */
gboolean mtx_validate_filename(const gchar *filename)
{
    if (!filename || *filename == '\0') {
        return FALSE;
    }
    
    /* Check for path traversal attempts */
    if (strstr(filename, "..") || 
        strstr(filename, "//") ||
        strchr(filename, '\0') != filename + strlen(filename)) {
        return FALSE;
    }
    
    /* Check for invalid characters */
    for (const gchar *p = filename; *p; p++) {
        if (*p < 32 || *p == 127 || strchr("<>:\"|?*", *p)) {
            return FALSE;
        }
    }
    
    return TRUE;
}

/* Validate numeric input */
gboolean mtx_validate_numeric_input(const gchar *input, gdouble min_val, gdouble max_val, gdouble *result)
{
    if (!input || !result) {
        return FALSE;
    }
    
    gchar *endptr;
    gdouble value = g_strtod(input, &endptr);
    
    /* Check for conversion errors */
    if (endptr == input || *endptr != '\0') {
        return FALSE;
    }
    
    /* Check range */
    if (value < min_val || value > max_val) {
        return FALSE;
    }
    
    *result = value;
    return TRUE;
}

/* Sanitize user input for display */
gchar *mtx_sanitize_user_input(const gchar *input, gsize max_length)
{
    if (!input) {
        return NULL;
    }
    
    gsize input_len = strlen(input);
    if (input_len > max_length) {
        input_len = max_length;
    }
    
    gchar *sanitized = g_malloc0(input_len + 1);
    gsize out_pos = 0;
    
    for (gsize i = 0; i < input_len && out_pos < max_length; i++) {
        guchar c = (guchar)input[i];
        
        /* Only allow printable ASCII characters */
        if (c >= 32 && c <= 126) {
            sanitized[out_pos++] = c;
        } else {
            /* Replace with safe character */
            sanitized[out_pos++] = '?';
        }
    }
    
    return sanitized;
}

/* Secure memory cleanup */
void mtx_secure_zero_memory(void *ptr, gsize size)
{
    if (ptr && size > 0) {
        volatile guchar *p = (volatile guchar *)ptr;
        for (gsize i = 0; i < size; i++) {
            p[i] = 0;
        }
    }
}

/* Rate limiting for operations */
typedef struct {
    GHashTable *operation_counts;
    GHashTable *last_operation_time;
    guint max_operations_per_second;
} MtxRateLimiter;

MtxRateLimiter *mtx_rate_limiter_new(guint max_ops_per_second)
{
    MtxRateLimiter *limiter = g_malloc0(sizeof(MtxRateLimiter));
    limiter->operation_counts = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    limiter->last_operation_time = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    limiter->max_operations_per_second = max_ops_per_second;
    return limiter;
}

gboolean mtx_rate_limiter_check(MtxRateLimiter *limiter, const gchar *operation_id)
{
    if (!limiter || !operation_id) {
        return FALSE;
    }
    
    gint64 current_time = g_get_monotonic_time();
    gint64 *last_time = g_hash_table_lookup(limiter->last_operation_time, operation_id);
    
    if (last_time) {
        gint64 time_diff = current_time - *last_time;
        if (time_diff < (G_USEC_PER_SEC / limiter->max_operations_per_second)) {
            return FALSE;  /* Rate limit exceeded */
        }
    }
    
    /* Update last operation time */
    gint64 *new_time = g_malloc(sizeof(gint64));
    *new_time = current_time;
    g_hash_table_replace(limiter->last_operation_time, g_strdup(operation_id), new_time);
    
    return TRUE;
}

void mtx_rate_limiter_free(MtxRateLimiter *limiter)
{
    if (limiter) {
        g_hash_table_destroy(limiter->operation_counts);
        g_hash_table_destroy(limiter->last_operation_time);
        g_free(limiter);
    }
}
