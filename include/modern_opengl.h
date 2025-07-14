#ifndef __MODERN_OPENGL_H__
#define __MODERN_OPENGL_H__

#include <gtk/gtk.h>
#include <epoxy/gl.h>
#include <epoxy/egl.h>
#include <glib.h>

G_BEGIN_DECLS

/* Modern OpenGL context and rendering for MegaTunix Redux 2025 */

/* OpenGL Context Types */
typedef enum {
    MTX_GL_CONTEXT_3D_GAUGE,
    MTX_GL_CONTEXT_REAL_TIME_GRAPH,
    MTX_GL_CONTEXT_TABLE_VISUALIZATION,
    MTX_GL_CONTEXT_DASHBOARD
} MtxGLContextType;

/* OpenGL Renderer Structure */
typedef struct {
    GtkGLArea *gl_area;
    MtxGLContextType context_type;
    
    /* Shader programs */
    GLuint vertex_shader;
    GLuint fragment_shader;
    GLuint program;
    
    /* Vertex buffer objects */
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    
    /* Uniforms */
    GLint model_uniform;
    GLint view_uniform;
    GLint projection_uniform;
    GLint time_uniform;
    
    /* Animation and timing */
    gint64 start_time;
    gboolean animate;
    
    /* Data buffers */
    gfloat *vertices;
    guint *indices;
    gsize vertex_count;
    gsize index_count;
    
    /* Callbacks */
    void (*update_callback)(gpointer user_data);
    gpointer user_data;
} MtxGLRenderer;

/* OpenGL Initialization and Management */
gboolean mtx_gl_init(void);
void mtx_gl_cleanup(void);

/* Renderer creation and management */
MtxGLRenderer *mtx_gl_renderer_new(MtxGLContextType type);
void mtx_gl_renderer_free(MtxGLRenderer *renderer);

/* GL Area widget creation */
GtkWidget *mtx_gl_area_new(MtxGLContextType type);
void mtx_gl_area_set_renderer(GtkGLArea *area, MtxGLRenderer *renderer);

/* Shader management */
gboolean mtx_gl_load_shaders(MtxGLRenderer *renderer, 
                            const gchar *vertex_src,
                            const gchar *fragment_src);
void mtx_gl_update_uniforms(MtxGLRenderer *renderer);

/* 3D Gauge rendering */
void mtx_gl_render_3d_gauge(MtxGLRenderer *renderer, 
                           gfloat value, 
                           gfloat min_val, 
                           gfloat max_val);

/* Real-time graph rendering */
void mtx_gl_render_realtime_graph(MtxGLRenderer *renderer,
                                 gfloat *data,
                                 gsize data_count,
                                 gfloat time_span);

/* Table visualization */
void mtx_gl_render_table_3d(MtxGLRenderer *renderer,
                           gfloat **table_data,
                           gsize rows,
                           gsize cols);

/* Dashboard rendering */
void mtx_gl_render_dashboard(MtxGLRenderer *renderer);

/* Animation control */
void mtx_gl_start_animation(MtxGLRenderer *renderer);
void mtx_gl_stop_animation(MtxGLRenderer *renderer);
void mtx_gl_set_animation_speed(MtxGLRenderer *renderer, gfloat speed);

/* Utility functions */
gboolean mtx_gl_check_extensions(void);
void mtx_gl_print_info(void);
gchar *mtx_gl_get_error_string(GLenum error);

/* Matrix operations */
void mtx_gl_matrix_identity(gfloat *matrix);
void mtx_gl_matrix_perspective(gfloat *matrix, 
                              gfloat fovy, 
                              gfloat aspect, 
                              gfloat near, 
                              gfloat far);
void mtx_gl_matrix_rotate(gfloat *matrix, 
                         gfloat angle, 
                         gfloat x, 
                         gfloat y, 
                         gfloat z);
void mtx_gl_matrix_translate(gfloat *matrix, 
                            gfloat x, 
                            gfloat y, 
                            gfloat z);

/* Default shaders */
extern const gchar *mtx_gl_default_vertex_shader;
extern const gchar *mtx_gl_default_fragment_shader;
extern const gchar *mtx_gl_gauge_vertex_shader;
extern const gchar *mtx_gl_gauge_fragment_shader;
extern const gchar *mtx_gl_graph_vertex_shader;
extern const gchar *mtx_gl_graph_fragment_shader;

G_END_DECLS

#endif /* __MODERN_OPENGL_H__ */
