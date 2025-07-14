/*
 * Security utilities header for MegaTunix Redux
 */

#ifndef __SECURITY_UTILS_H__
#define __SECURITY_UTILS_H__

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Secure string operations */
gboolean mtx_strncpy_secure(gchar *dest, const gchar *src, gsize dest_size);
gboolean mtx_strncat_secure(gchar *dest, const gchar *src, gsize dest_size);

/* Input validation */
gboolean mtx_validate_filename(const gchar *filename);
gboolean mtx_validate_numeric_input(const gchar *input, gdouble min_val, gdouble max_val, gdouble *result);
gchar *mtx_sanitize_user_input(const gchar *input, gsize max_length);

/* Memory security */
void mtx_secure_zero_memory(void *ptr, gsize size);

/* Rate limiting */
typedef struct MtxRateLimiter MtxRateLimiter;

MtxRateLimiter *mtx_rate_limiter_new(guint max_ops_per_second);
gboolean mtx_rate_limiter_check(MtxRateLimiter *limiter, const gchar *operation_id);
void mtx_rate_limiter_free(MtxRateLimiter *limiter);

/* Security macros */
#define MTX_MAX_FILENAME_LEN 255
#define MTX_MAX_INPUT_LEN 1024
#define MTX_MAX_PATH_LEN 4096

/* Safe string operations */
#define SAFE_FREE(ptr) do { \
    if (ptr) { \
        g_free(ptr); \
        ptr = NULL; \
    } \
} while(0)

#define SAFE_UNREF(obj) do { \
    if (obj) { \
        g_object_unref(obj); \
        obj = NULL; \
    } \
} while(0)

#ifdef __cplusplus
}
#endif

#endif /* __SECURITY_UTILS_H__ */
