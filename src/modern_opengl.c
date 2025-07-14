#include "modern_opengl.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

/* Forward declarations */
static void on_gl_area_realize(GtkGLArea *area, gpointer user_data);
static void on_gl_area_unrealize(GtkGLArea *area, gpointer user_data);
static gboolean on_gl_area_render(GtkGLArea *area, GdkGLContext *context, gpointer user_data);

/* Default vertex shader for basic 3D rendering */
const gchar *mtx_gl_default_vertex_shader = 
"#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec3 aNormal;\n"
"layout (location = 2) in vec2 aTexCoord;\n"
"\n"
"out vec3 FragPos;\n"
"out vec3 Normal;\n"
"out vec2 TexCoord;\n"
"\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"\n"
"void main()\n"
"{\n"
"    FragPos = vec3(model * vec4(aPos, 1.0));\n"
"    Normal = mat3(transpose(inverse(model))) * aNormal;\n"
"    TexCoord = aTexCoord;\n"
"    \n"
"    gl_Position = projection * view * vec4(FragPos, 1.0);\n"
"}\n";

/* Default fragment shader for basic 3D rendering */
const gchar *mtx_gl_default_fragment_shader = 
"#version 330 core\n"
"out vec4 FragColor;\n"
"\n"
"in vec3 FragPos;\n"
"in vec3 Normal;\n"
"in vec2 TexCoord;\n"
"\n"
"uniform vec3 lightPos;\n"
"uniform vec3 lightColor;\n"
"uniform vec3 objectColor;\n"
"uniform float time;\n"
"\n"
"void main()\n"
"{\n"
"    // Ambient\n"
"    float ambientStrength = 0.1;\n"
"    vec3 ambient = ambientStrength * lightColor;\n"
"    \n"
"    // Diffuse\n"
"    vec3 norm = normalize(Normal);\n"
"    vec3 lightDir = normalize(lightPos - FragPos);\n"
"    float diff = max(dot(norm, lightDir), 0.0);\n"
"    vec3 diffuse = diff * lightColor;\n"
"    \n"
"    vec3 result = (ambient + diffuse) * objectColor;\n"
"    FragColor = vec4(result, 1.0);\n"
"}\n";

/* Gauge-specific vertex shader */
const gchar *mtx_gl_gauge_vertex_shader = 
"#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec3 aColor;\n"
"\n"
"out vec3 vertexColor;\n"
"\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"uniform float gaugeValue;\n"
"uniform float time;\n"
"\n"
"void main()\n"
"{\n"
"    vertexColor = aColor;\n"
"    \n"
"    // Apply gauge rotation based on value\n"
"    float angle = gaugeValue * 3.14159 * 1.5; // 270 degrees max\n"
"    mat3 rotation = mat3(\n"
"        cos(angle), -sin(angle), 0.0,\n"
"        sin(angle), cos(angle), 0.0,\n"
"        0.0, 0.0, 1.0\n"
"    );\n"
"    \n"
"    vec3 rotatedPos = rotation * aPos;\n"
"    gl_Position = projection * view * model * vec4(rotatedPos, 1.0);\n"
"}\n";

/* Gauge-specific fragment shader */
const gchar *mtx_gl_gauge_fragment_shader = 
"#version 330 core\n"
"out vec4 FragColor;\n"
"\n"
"in vec3 vertexColor;\n"
"\n"
"uniform float gaugeValue;\n"
"uniform float time;\n"
"\n"
"void main()\n"
"{\n"
"    // Color based on gauge value\n"
"    vec3 color = vertexColor;\n"
"    if (gaugeValue > 0.8) {\n"
"        color = mix(color, vec3(1.0, 0.0, 0.0), 0.5); // Red zone\n"
"    } else if (gaugeValue > 0.6) {\n"
"        color = mix(color, vec3(1.0, 1.0, 0.0), 0.3); // Yellow zone\n"
"    }\n"
"    \n"
"    FragColor = vec4(color, 1.0);\n"
"}\n";

/* Graph-specific vertex shader */
const gchar *mtx_gl_graph_vertex_shader = 
"#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in float aValue;\n"
"\n"
"out float dataValue;\n"
"\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"uniform float timeOffset;\n"
"\n"
"void main()\n"
"{\n"
"    dataValue = aValue;\n"
"    \n"
"    vec3 pos = aPos;\n"
"    pos.x += timeOffset; // Scroll effect\n"
"    \n"
"    gl_Position = projection * view * model * vec4(pos, 1.0);\n"
"}\n";

/* Graph-specific fragment shader */
const gchar *mtx_gl_graph_fragment_shader = 
"#version 330 core\n"
"out vec4 FragColor;\n"
"\n"
"in float dataValue;\n"
"\n"
"uniform float minValue;\n"
"uniform float maxValue;\n"
"\n"
"void main()\n"
"{\n"
"    // Color based on data value\n"
"    float normalized = (dataValue - minValue) / (maxValue - minValue);\n"
"    vec3 color = mix(vec3(0.0, 0.0, 1.0), vec3(1.0, 0.0, 0.0), normalized);\n"
"    \n"
"    FragColor = vec4(color, 1.0);\n"
"}\n";

/* OpenGL initialization */
gboolean mtx_gl_init(void)
{
    g_debug("Initializing modern OpenGL support");
    
    /* Check if OpenGL is available */
    if (!epoxy_has_egl()) {
        g_warning("OpenGL not available");
        return FALSE;
    }
    
    /* Check OpenGL version */
    int version = epoxy_gl_version();
    int major = version / 10;
    int minor = version % 10;
    g_debug("OpenGL version: %d.%d", major, minor);
    
    if (major < 3 || (major == 3 && minor < 3)) {
        g_warning("OpenGL 3.3 or higher required, found %d.%d", major, minor);
        return FALSE;
    }
    
    /* Check required extensions */
    if (!mtx_gl_check_extensions()) {
        g_warning("Required OpenGL extensions not available");
        return FALSE;
    }
    
    /* Print OpenGL info */
    mtx_gl_print_info();
    
    return TRUE;
}

/* OpenGL cleanup */
void mtx_gl_cleanup(void)
{
    g_debug("Cleaning up OpenGL resources");
    /* Cleanup will be handled by individual renderers */
}

/* Create new OpenGL renderer */
MtxGLRenderer *mtx_gl_renderer_new(MtxGLContextType type)
{
    MtxGLRenderer *renderer = g_new0(MtxGLRenderer, 1);
    
    renderer->context_type = type;
    renderer->start_time = g_get_monotonic_time();
    renderer->animate = FALSE;
    
    /* Initialize OpenGL objects to 0 */
    renderer->vertex_shader = 0;
    renderer->fragment_shader = 0;
    renderer->program = 0;
    renderer->vao = 0;
    renderer->vbo = 0;
    renderer->ebo = 0;
    
    return renderer;
}

/* Free OpenGL renderer */
void mtx_gl_renderer_free(MtxGLRenderer *renderer)
{
    if (!renderer) return;
    
    /* Clean up OpenGL objects */
    if (renderer->program) {
        glDeleteProgram(renderer->program);
    }
    if (renderer->vertex_shader) {
        glDeleteShader(renderer->vertex_shader);
    }
    if (renderer->fragment_shader) {
        glDeleteShader(renderer->fragment_shader);
    }
    if (renderer->vao) {
        glDeleteVertexArrays(1, &renderer->vao);
    }
    if (renderer->vbo) {
        glDeleteBuffers(1, &renderer->vbo);
    }
    if (renderer->ebo) {
        glDeleteBuffers(1, &renderer->ebo);
    }
    
    /* Free data arrays */
    g_free(renderer->vertices);
    g_free(renderer->indices);
    
    g_free(renderer);
}

/* Create GL area widget */
GtkWidget *mtx_gl_area_new(MtxGLContextType type)
{
    GtkWidget *gl_area = gtk_gl_area_new();
    
    /* Set required OpenGL version */
    gtk_gl_area_set_required_version(GTK_GL_AREA(gl_area), 3, 3);
    
    /* Enable auto-render */
    gtk_gl_area_set_auto_render(GTK_GL_AREA(gl_area), TRUE);
    
    /* Set up signal handlers */
    g_signal_connect(gl_area, "realize", G_CALLBACK(on_gl_area_realize), GINT_TO_POINTER(type));
    g_signal_connect(gl_area, "unrealize", G_CALLBACK(on_gl_area_unrealize), NULL);
    g_signal_connect(gl_area, "render", G_CALLBACK(on_gl_area_render), NULL);
    
    return gl_area;
}

/* Set renderer for GL area */
void mtx_gl_area_set_renderer(GtkGLArea *area, MtxGLRenderer *renderer)
{
    renderer->gl_area = area;
    g_object_set_data(G_OBJECT(area), "mtx-renderer", renderer);
}

/* Load shaders */
gboolean mtx_gl_load_shaders(MtxGLRenderer *renderer, 
                            const gchar *vertex_src,
                            const gchar *fragment_src)
{
    GLint success;
    GLchar info_log[512];
    
    /* Create vertex shader */
    renderer->vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(renderer->vertex_shader, 1, &vertex_src, NULL);
    glCompileShader(renderer->vertex_shader);
    
    glGetShaderiv(renderer->vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(renderer->vertex_shader, 512, NULL, info_log);
        g_warning("Vertex shader compilation failed: %s", info_log);
        return FALSE;
    }
    
    /* Create fragment shader */
    renderer->fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(renderer->fragment_shader, 1, &fragment_src, NULL);
    glCompileShader(renderer->fragment_shader);
    
    glGetShaderiv(renderer->fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(renderer->fragment_shader, 512, NULL, info_log);
        g_warning("Fragment shader compilation failed: %s", info_log);
        return FALSE;
    }
    
    /* Create shader program */
    renderer->program = glCreateProgram();
    glAttachShader(renderer->program, renderer->vertex_shader);
    glAttachShader(renderer->program, renderer->fragment_shader);
    glLinkProgram(renderer->program);
    
    glGetProgramiv(renderer->program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(renderer->program, 512, NULL, info_log);
        g_warning("Shader program linking failed: %s", info_log);
        return FALSE;
    }
    
    /* Get uniform locations */
    renderer->model_uniform = glGetUniformLocation(renderer->program, "model");
    renderer->view_uniform = glGetUniformLocation(renderer->program, "view");
    renderer->projection_uniform = glGetUniformLocation(renderer->program, "projection");
    renderer->time_uniform = glGetUniformLocation(renderer->program, "time");
    
    return TRUE;
}

/* Update uniforms */
void mtx_gl_update_uniforms(MtxGLRenderer *renderer)
{
    if (!renderer->program) return;
    
    glUseProgram(renderer->program);
    
    /* Update time uniform */
    if (renderer->time_uniform != -1) {
        gfloat time = (g_get_monotonic_time() - renderer->start_time) / 1000000.0f;
        glUniform1f(renderer->time_uniform, time);
    }
    
    /* Set up matrices */
    gfloat model[16], view[16], projection[16];
    
    mtx_gl_matrix_identity(model);
    mtx_gl_matrix_identity(view);
    mtx_gl_matrix_perspective(projection, 45.0f, 1.0f, 0.1f, 100.0f);
    
    /* Translate view matrix */
    mtx_gl_matrix_translate(view, 0.0f, 0.0f, -3.0f);
    
    if (renderer->model_uniform != -1) {
        glUniformMatrix4fv(renderer->model_uniform, 1, GL_FALSE, model);
    }
    if (renderer->view_uniform != -1) {
        glUniformMatrix4fv(renderer->view_uniform, 1, GL_FALSE, view);
    }
    if (renderer->projection_uniform != -1) {
        glUniformMatrix4fv(renderer->projection_uniform, 1, GL_FALSE, projection);
    }
}

/* Render 3D gauge */
void mtx_gl_render_3d_gauge(MtxGLRenderer *renderer, 
                           gfloat value, 
                           gfloat min_val, 
                           gfloat max_val)
{
    if (!renderer || !renderer->program) return;
    
    /* Normalize value */
    gfloat normalized = (value - min_val) / (max_val - min_val);
    normalized = CLAMP(normalized, 0.0f, 1.0f);
    
    glUseProgram(renderer->program);
    
    /* Set gauge-specific uniforms */
    GLint gauge_uniform = glGetUniformLocation(renderer->program, "gaugeValue");
    if (gauge_uniform != -1) {
        glUniform1f(gauge_uniform, normalized);
    }
    
    mtx_gl_update_uniforms(renderer);
    
    /* Render gauge geometry */
    if (renderer->vao) {
        glBindVertexArray(renderer->vao);
        if (renderer->ebo) {
            glDrawElements(GL_TRIANGLES, renderer->index_count, GL_UNSIGNED_INT, 0);
        } else {
            glDrawArrays(GL_TRIANGLES, 0, renderer->vertex_count);
        }
        glBindVertexArray(0);
    }
}

/* Render real-time graph */
void mtx_gl_render_realtime_graph(MtxGLRenderer *renderer,
                                 gfloat *data,
                                 gsize data_count,
                                 gfloat time_span)
{
    if (!renderer || !renderer->program || !data) return;
    
    glUseProgram(renderer->program);
    
    /* Update time offset for scrolling */
    gfloat time_offset = fmodf((g_get_monotonic_time() - renderer->start_time) / 1000000.0f, time_span);
    GLint offset_uniform = glGetUniformLocation(renderer->program, "timeOffset");
    if (offset_uniform != -1) {
        glUniform1f(offset_uniform, -time_offset);
    }
    
    /* Find min/max values */
    gfloat min_val = data[0], max_val = data[0];
    for (gsize i = 1; i < data_count; i++) {
        if (data[i] < min_val) min_val = data[i];
        if (data[i] > max_val) max_val = data[i];
    }
    
    GLint min_uniform = glGetUniformLocation(renderer->program, "minValue");
    GLint max_uniform = glGetUniformLocation(renderer->program, "maxValue");
    if (min_uniform != -1) glUniform1f(min_uniform, min_val);
    if (max_uniform != -1) glUniform1f(max_uniform, max_val);
    
    mtx_gl_update_uniforms(renderer);
    
    /* Render graph */
    if (renderer->vao) {
        glBindVertexArray(renderer->vao);
        glDrawArrays(GL_LINE_STRIP, 0, data_count);
        glBindVertexArray(0);
    }
}

/* Render 3D table */
void mtx_gl_render_table_3d(MtxGLRenderer *renderer,
                           gfloat **table_data,
                           gsize rows,
                           gsize cols)
{
    if (!renderer || !renderer->program || !table_data) return;
    
    glUseProgram(renderer->program);
    mtx_gl_update_uniforms(renderer);
    
    /* Render table as height map */
    if (renderer->vao) {
        glBindVertexArray(renderer->vao);
        glDrawElements(GL_TRIANGLES, renderer->index_count, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
}

/* Render dashboard */
void mtx_gl_render_dashboard(MtxGLRenderer *renderer)
{
    if (!renderer || !renderer->program) return;
    
    glUseProgram(renderer->program);
    mtx_gl_update_uniforms(renderer);
    
    /* Render dashboard elements */
    if (renderer->vao) {
        glBindVertexArray(renderer->vao);
        glDrawArrays(GL_TRIANGLES, 0, renderer->vertex_count);
        glBindVertexArray(0);
    }
}

/* Animation control */
void mtx_gl_start_animation(MtxGLRenderer *renderer)
{
    if (renderer) {
        renderer->animate = TRUE;
        if (renderer->gl_area) {
            gtk_gl_area_set_auto_render(renderer->gl_area, TRUE);
        }
    }
}

void mtx_gl_stop_animation(MtxGLRenderer *renderer)
{
    if (renderer) {
        renderer->animate = FALSE;
        if (renderer->gl_area) {
            gtk_gl_area_set_auto_render(renderer->gl_area, FALSE);
        }
    }
}

void mtx_gl_set_animation_speed(MtxGLRenderer *renderer, gfloat speed)
{
    /* Speed control could be implemented by adjusting time scaling */
    /* For now, this is a placeholder */
}

/* Check extensions */
gboolean mtx_gl_check_extensions(void)
{
    /* Check for required extensions */
    if (!epoxy_has_gl_extension("GL_ARB_vertex_array_object")) {
        g_warning("GL_ARB_vertex_array_object not available");
        return FALSE;
    }
    
    if (!epoxy_has_gl_extension("GL_ARB_vertex_buffer_object")) {
        g_warning("GL_ARB_vertex_buffer_object not available");
        return FALSE;
    }
    
    return TRUE;
}

/* Print OpenGL info */
void mtx_gl_print_info(void)
{
    g_debug("OpenGL Vendor: %s", glGetString(GL_VENDOR));
    g_debug("OpenGL Renderer: %s", glGetString(GL_RENDERER));
    g_debug("OpenGL Version: %s", glGetString(GL_VERSION));
    g_debug("GLSL Version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
}

/* Get error string */
gchar *mtx_gl_get_error_string(GLenum error)
{
    switch (error) {
        case GL_NO_ERROR:
            return g_strdup("No error");
        case GL_INVALID_ENUM:
            return g_strdup("Invalid enum");
        case GL_INVALID_VALUE:
            return g_strdup("Invalid value");
        case GL_INVALID_OPERATION:
            return g_strdup("Invalid operation");
        case GL_OUT_OF_MEMORY:
            return g_strdup("Out of memory");
        default:
            return g_strdup_printf("Unknown error: 0x%x", error);
    }
}

/* Matrix operations */
void mtx_gl_matrix_identity(gfloat *matrix)
{
    memset(matrix, 0, 16 * sizeof(gfloat));
    matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1.0f;
}

void mtx_gl_matrix_perspective(gfloat *matrix, 
                              gfloat fovy, 
                              gfloat aspect, 
                              gfloat near, 
                              gfloat far)
{
    gfloat f = 1.0f / tanf(fovy * G_PI / 180.0f);
    
    memset(matrix, 0, 16 * sizeof(gfloat));
    matrix[0] = f / aspect;
    matrix[5] = f;
    matrix[10] = (far + near) / (near - far);
    matrix[11] = -1.0f;
    matrix[14] = (2.0f * far * near) / (near - far);
}

void mtx_gl_matrix_rotate(gfloat *matrix, 
                         gfloat angle, 
                         gfloat x, 
                         gfloat y, 
                         gfloat z)
{
    gfloat c = cosf(angle);
    gfloat s = sinf(angle);
    gfloat len = sqrtf(x*x + y*y + z*z);
    
    if (len > 0.0f) {
        x /= len;
        y /= len;
        z /= len;
    }
    
    gfloat rot[16];
    memset(rot, 0, 16 * sizeof(gfloat));
    
    rot[0] = c + x*x*(1-c);
    rot[1] = x*y*(1-c) - z*s;
    rot[2] = x*z*(1-c) + y*s;
    rot[4] = y*x*(1-c) + z*s;
    rot[5] = c + y*y*(1-c);
    rot[6] = y*z*(1-c) - x*s;
    rot[8] = z*x*(1-c) - y*s;
    rot[9] = z*y*(1-c) + x*s;
    rot[10] = c + z*z*(1-c);
    rot[15] = 1.0f;
    
    /* Multiply matrices */
    gfloat result[16];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result[i*4 + j] = 0.0f;
            for (int k = 0; k < 4; k++) {
                result[i*4 + j] += matrix[i*4 + k] * rot[k*4 + j];
            }
        }
    }
    
    memcpy(matrix, result, 16 * sizeof(gfloat));
}

void mtx_gl_matrix_translate(gfloat *matrix, 
                            gfloat x, 
                            gfloat y, 
                            gfloat z)
{
    matrix[12] += matrix[0] * x + matrix[4] * y + matrix[8] * z;
    matrix[13] += matrix[1] * x + matrix[5] * y + matrix[9] * z;
    matrix[14] += matrix[2] * x + matrix[6] * y + matrix[10] * z;
    matrix[15] += matrix[3] * x + matrix[7] * y + matrix[11] * z;
}

/* Signal handlers (to be implemented) */
static void on_gl_area_realize(GtkGLArea *area, gpointer user_data)
{
    MtxGLContextType type = GPOINTER_TO_INT(user_data);
    g_debug("GL area realized for context type: %d", type);
    
    /* Make context current */
    gtk_gl_area_make_current(area);
    
    if (gtk_gl_area_get_error(area) != NULL) {
        g_warning("GL area error on realize");
        return;
    }
    
    /* Initialize OpenGL state */
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    /* Set up renderer */
    MtxGLRenderer *renderer = mtx_gl_renderer_new(type);
    mtx_gl_area_set_renderer(area, renderer);
    
    /* Load appropriate shaders based on context type */
    const gchar *vertex_src = mtx_gl_default_vertex_shader;
    const gchar *fragment_src = mtx_gl_default_fragment_shader;
    
    switch (type) {
        case MTX_GL_CONTEXT_3D_GAUGE:
            vertex_src = mtx_gl_gauge_vertex_shader;
            fragment_src = mtx_gl_gauge_fragment_shader;
            break;
        case MTX_GL_CONTEXT_REAL_TIME_GRAPH:
            vertex_src = mtx_gl_graph_vertex_shader;
            fragment_src = mtx_gl_graph_fragment_shader;
            break;
        default:
            break;
    }
    
    mtx_gl_load_shaders(renderer, vertex_src, fragment_src);
}

static void on_gl_area_unrealize(GtkGLArea *area, gpointer user_data)
{
    g_debug("GL area unrealized");
    
    /* Make context current */
    gtk_gl_area_make_current(area);
    
    /* Clean up renderer */
    MtxGLRenderer *renderer = g_object_get_data(G_OBJECT(area), "mtx-renderer");
    if (renderer) {
        mtx_gl_renderer_free(renderer);
    }
}

static gboolean on_gl_area_render(GtkGLArea *area, GdkGLContext *context, gpointer user_data)
{
    /* Clear the framebuffer */
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    /* Get renderer */
    MtxGLRenderer *renderer = g_object_get_data(G_OBJECT(area), "mtx-renderer");
    if (!renderer) return FALSE;
    
    /* Render based on context type */
    switch (renderer->context_type) {
        case MTX_GL_CONTEXT_3D_GAUGE:
            mtx_gl_render_3d_gauge(renderer, 0.5f, 0.0f, 1.0f);
            break;
        case MTX_GL_CONTEXT_REAL_TIME_GRAPH:
            /* Example data rendering */
            break;
        case MTX_GL_CONTEXT_TABLE_VISUALIZATION:
            /* Table rendering */
            break;
        case MTX_GL_CONTEXT_DASHBOARD:
            mtx_gl_render_dashboard(renderer);
            break;
    }
    
    return TRUE;
}
