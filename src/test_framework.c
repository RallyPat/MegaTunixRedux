#include "test_framework.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

/* Static global variables for test framework */
static gboolean framework_initialized = FALSE;
static GHashTable *global_timers = NULL;
static GMutex timers_mutex;

/* Initialize test framework */
gboolean mtx_test_framework_init(void)
{
    if (framework_initialized) {
        return TRUE;
    }
    
    g_debug("Initializing MegaTunix Redux test framework");
    
    /* Initialize global timers */
    global_timers = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    g_mutex_init(&timers_mutex);
    
    framework_initialized = TRUE;
    
    g_info("Test framework initialized successfully");
    return TRUE;
}

/* Cleanup test framework */
void mtx_test_framework_cleanup(void)
{
    if (!framework_initialized) {
        return;
    }
    
    g_debug("Cleaning up test framework");
    
    if (global_timers) {
        g_hash_table_destroy(global_timers);
        global_timers = NULL;
    }
    
    g_mutex_clear(&timers_mutex);
    
    framework_initialized = FALSE;
    
    g_info("Test framework cleanup completed");
}

/* Create new test case */
MtxTestCase *mtx_test_case_new(const gchar *name, MtxTestFunc test_func)
{
    g_return_val_if_fail(name != NULL, NULL);
    g_return_val_if_fail(test_func != NULL, NULL);
    
    MtxTestCase *test_case = g_new0(MtxTestCase, 1);
    
    test_case->name = g_strdup(name);
    test_case->test_func = test_func;
    test_case->type = MTX_TEST_TYPE_UNIT;
    test_case->priority = MTX_TEST_PRIORITY_NORMAL;
    test_case->result = MTX_TEST_RESULT_PENDING;
    test_case->metadata = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    test_case->enabled = TRUE;
    test_case->timeout_seconds = 30; /* Default timeout */
    
    return test_case;
}

/* Free test case */
void mtx_test_case_free(MtxTestCase *test_case)
{
    if (!test_case) return;
    
    g_free(test_case->name);
    g_free(test_case->description);
    g_strfreev(test_case->tags);
    g_strfreev(test_case->dependencies);
    g_hash_table_destroy(test_case->metadata);
    
    if (test_case->error) {
        g_error_free(test_case->error);
    }
    
    g_free(test_case);
}

/* Set test case description */
void mtx_test_case_set_description(MtxTestCase *test_case, const gchar *description)
{
    g_return_if_fail(test_case != NULL);
    
    g_free(test_case->description);
    test_case->description = g_strdup(description);
}

/* Set test case type */
void mtx_test_case_set_type(MtxTestCase *test_case, MtxTestType type)
{
    g_return_if_fail(test_case != NULL);
    test_case->type = type;
}

/* Set test case priority */
void mtx_test_case_set_priority(MtxTestCase *test_case, MtxTestPriority priority)
{
    g_return_if_fail(test_case != NULL);
    test_case->priority = priority;
}

/* Set test case timeout */
void mtx_test_case_set_timeout(MtxTestCase *test_case, gint timeout_seconds)
{
    g_return_if_fail(test_case != NULL);
    test_case->timeout_seconds = timeout_seconds;
}

/* Add tag to test case */
void mtx_test_case_add_tag(MtxTestCase *test_case, const gchar *tag)
{
    g_return_if_fail(test_case != NULL);
    g_return_if_fail(tag != NULL);
    
    gint len = test_case->tags ? g_strv_length(test_case->tags) : 0;
    test_case->tags = g_realloc(test_case->tags, (len + 2) * sizeof(gchar*));
    test_case->tags[len] = g_strdup(tag);
    test_case->tags[len + 1] = NULL;
}

/* Create new test suite */
MtxTestSuite *mtx_test_suite_new(const gchar *name)
{
    g_return_val_if_fail(name != NULL, NULL);
    
    MtxTestSuite *suite = g_new0(MtxTestSuite, 1);
    
    suite->name = g_strdup(name);
    suite->shared_data = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    
    return suite;
}

/* Free test suite */
void mtx_test_suite_free(MtxTestSuite *suite)
{
    if (!suite) return;
    
    g_free(suite->name);
    g_free(suite->description);
    g_list_free_full(suite->test_cases, (GDestroyNotify)mtx_test_case_free);
    g_hash_table_destroy(suite->shared_data);
    
    g_free(suite);
}

/* Add test to suite */
void mtx_test_suite_add_test(MtxTestSuite *suite, MtxTestCase *test_case)
{
    g_return_if_fail(suite != NULL);
    g_return_if_fail(test_case != NULL);
    
    suite->test_cases = g_list_append(suite->test_cases, test_case);
    suite->total_tests++;
}

/* Create new test runner */
MtxTestRunner *mtx_test_runner_new(void)
{
    MtxTestRunner *runner = g_new0(MtxTestRunner, 1);
    
    runner->global_fixtures = g_hash_table_new_full(g_str_hash, g_str_equal,
                                                   g_free, (GDestroyNotify)mtx_test_fixture_free);
    runner->results = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    runner->report_buffer = g_string_new(NULL);
    runner->parallel_execution = FALSE;
    runner->max_threads = 1;
    
    g_mutex_init(&runner->results_mutex);
    
    return runner;
}

/* Free test runner */
void mtx_test_runner_free(MtxTestRunner *runner)
{
    if (!runner) return;
    
    g_list_free_full(runner->test_suites, (GDestroyNotify)mtx_test_suite_free);
    g_hash_table_destroy(runner->global_fixtures);
    g_hash_table_destroy(runner->results);
    g_string_free(runner->report_buffer, TRUE);
    g_free(runner->output_directory);
    
    if (runner->context) {
        mtx_test_context_free(runner->context);
    }
    
    if (runner->thread_pool) {
        g_thread_pool_free(runner->thread_pool, TRUE, TRUE);
    }
    
    g_mutex_clear(&runner->results_mutex);
    
    g_free(runner);
}

/* Add suite to runner */
void mtx_test_runner_add_suite(MtxTestRunner *runner, MtxTestSuite *suite)
{
    g_return_if_fail(runner != NULL);
    g_return_if_fail(suite != NULL);
    
    runner->test_suites = g_list_append(runner->test_suites, suite);
}

/* Create new test context */
MtxTestContext *mtx_test_context_new(MtxTestRunner *runner)
{
    MtxTestContext *context = g_new0(MtxTestContext, 1);
    
    context->runner = runner;
    context->mock_objects = g_hash_table_new_full(g_str_hash, g_str_equal,
                                                 g_free, (GDestroyNotify)mtx_mock_object_free);
    context->test_data = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    context->output_buffer = g_string_new(NULL);
    context->error_buffer = g_string_new(NULL);
    context->cancellable = g_cancellable_new();
    context->verbose = FALSE;
    context->stop_on_failure = FALSE;
    
    return context;
}

/* Free test context */
void mtx_test_context_free(MtxTestContext *context)
{
    if (!context) return;
    
    g_hash_table_destroy(context->mock_objects);
    g_hash_table_destroy(context->test_data);
    g_string_free(context->output_buffer, TRUE);
    g_string_free(context->error_buffer, TRUE);
    g_object_unref(context->cancellable);
    
    g_free(context);
}

/* Set test data */
void mtx_test_context_set_test_data(MtxTestContext *context, const gchar *key, gpointer data)
{
    g_return_if_fail(context != NULL);
    g_return_if_fail(key != NULL);
    
    g_hash_table_insert(context->test_data, g_strdup(key), data);
}

/* Get test data */
gpointer mtx_test_context_get_test_data(MtxTestContext *context, const gchar *key)
{
    g_return_val_if_fail(context != NULL, NULL);
    g_return_val_if_fail(key != NULL, NULL);
    
    return g_hash_table_lookup(context->test_data, key);
}

/* Log message */
void mtx_test_context_log(MtxTestContext *context, const gchar *message)
{
    g_return_if_fail(context != NULL);
    g_return_if_fail(message != NULL);
    
    g_string_append(context->output_buffer, message);
    g_string_append_c(context->output_buffer, '\n');
    
    if (context->verbose) {
        g_print("%s\n", message);
    }
}

/* Log error message */
void mtx_test_context_log_error(MtxTestContext *context, const gchar *message)
{
    g_return_if_fail(context != NULL);
    g_return_if_fail(message != NULL);
    
    g_string_append(context->error_buffer, message);
    g_string_append_c(context->error_buffer, '\n');
    
    g_printerr("ERROR: %s\n", message);
}

/* Assertion functions */
gboolean mtx_assert_true(gboolean condition, const gchar *message)
{
    if (!condition) {
        g_warning("Assertion failed: %s", message ? message : "condition should be TRUE");
        return FALSE;
    }
    return TRUE;
}

gboolean mtx_assert_false(gboolean condition, const gchar *message)
{
    if (condition) {
        g_warning("Assertion failed: %s", message ? message : "condition should be FALSE");
        return FALSE;
    }
    return TRUE;
}

gboolean mtx_assert_null(gpointer pointer, const gchar *message)
{
    if (pointer != NULL) {
        g_warning("Assertion failed: %s", message ? message : "pointer should be NULL");
        return FALSE;
    }
    return TRUE;
}

gboolean mtx_assert_not_null(gpointer pointer, const gchar *message)
{
    if (pointer == NULL) {
        g_warning("Assertion failed: %s", message ? message : "pointer should not be NULL");
        return FALSE;
    }
    return TRUE;
}

gboolean mtx_assert_equal_int(gint actual, gint expected, const gchar *message)
{
    if (actual != expected) {
        g_warning("Assertion failed: %s (expected: %d, actual: %d)", 
                 message ? message : "integers should be equal", expected, actual);
        return FALSE;
    }
    return TRUE;
}

gboolean mtx_assert_equal_string(const gchar *actual, const gchar *expected, const gchar *message)
{
    if (g_strcmp0(actual, expected) != 0) {
        g_warning("Assertion failed: %s (expected: '%s', actual: '%s')", 
                 message ? message : "strings should be equal", expected, actual);
        return FALSE;
    }
    return TRUE;
}

gboolean mtx_assert_equal_double(gdouble actual, gdouble expected, gdouble epsilon, const gchar *message)
{
    if (fabs(actual - expected) > epsilon) {
        g_warning("Assertion failed: %s (expected: %f, actual: %f, epsilon: %f)", 
                 message ? message : "doubles should be equal", expected, actual, epsilon);
        return FALSE;
    }
    return TRUE;
}

/* Mock object functions */
MtxMockObject *mtx_mock_object_new(const gchar *name, GType object_type)
{
    g_return_val_if_fail(name != NULL, NULL);
    
    MtxMockObject *mock = g_new0(MtxMockObject, 1);
    
    mock->name = g_strdup(name);
    mock->object_type = object_type;
    mock->method_implementations = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    mock->call_counts = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    mock->return_values = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    mock->strict_mode = FALSE;
    
    return mock;
}

/* Free mock object */
void mtx_mock_object_free(MtxMockObject *mock)
{
    if (!mock) return;
    
    g_free(mock->name);
    g_hash_table_destroy(mock->method_implementations);
    g_hash_table_destroy(mock->call_counts);
    g_hash_table_destroy(mock->return_values);
    
    g_free(mock);
}

/* Performance testing functions */
gdouble mtx_test_measure_execution_time(GFunc func, gpointer data)
{
    g_return_val_if_fail(func != NULL, 0.0);
    
    GTimer *timer = g_timer_new();
    g_timer_start(timer);
    
    func(data, NULL);
    
    g_timer_stop(timer);
    gdouble elapsed = g_timer_elapsed(timer, NULL);
    g_timer_destroy(timer);
    
    return elapsed;
}

/* Start performance timer */
void mtx_test_performance_start_timer(MtxTestContext *context, const gchar *timer_name)
{
    g_return_if_fail(context != NULL);
    g_return_if_fail(timer_name != NULL);
    
    g_mutex_lock(&timers_mutex);
    
    GTimer *timer = g_timer_new();
    g_timer_start(timer);
    g_hash_table_insert(global_timers, g_strdup(timer_name), timer);
    
    g_mutex_unlock(&timers_mutex);
}

/* Stop performance timer */
gdouble mtx_test_performance_stop_timer(MtxTestContext *context, const gchar *timer_name)
{
    g_return_val_if_fail(context != NULL, 0.0);
    g_return_val_if_fail(timer_name != NULL, 0.0);
    
    g_mutex_lock(&timers_mutex);
    
    GTimer *timer = g_hash_table_lookup(global_timers, timer_name);
    if (!timer) {
        g_mutex_unlock(&timers_mutex);
        return 0.0;
    }
    
    g_timer_stop(timer);
    gdouble elapsed = g_timer_elapsed(timer, NULL);
    
    g_hash_table_remove(global_timers, timer_name);
    g_timer_destroy(timer);
    
    g_mutex_unlock(&timers_mutex);
    
    return elapsed;
}

/* Memory testing functions */
gsize mtx_test_get_memory_usage(void)
{
    /* This would need platform-specific implementation */
    /* For now, return 0 as placeholder */
    return 0;
}

/* UI testing functions */
gboolean mtx_test_ui_widget_exists(GtkWidget *parent, const gchar *widget_name)
{
    g_return_val_if_fail(GTK_IS_WIDGET(parent), FALSE);
    g_return_val_if_fail(widget_name != NULL, FALSE);
    
    /* Implementation would search for widget by name in parent */
    /* This is a placeholder implementation */
    return FALSE;
}

gboolean mtx_test_ui_button_click(GtkButton *button)
{
    g_return_val_if_fail(GTK_IS_BUTTON(button), FALSE);
    
    /* Simulate button click */
    g_signal_emit_by_name(button, "clicked");
    return TRUE;
}

/* File testing functions */
gboolean mtx_test_file_create_temp(gchar **filename, const gchar *content, GError **error)
{
    g_return_val_if_fail(filename != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);
    
    gint fd = g_file_open_tmp("mtx_test_XXXXXX", filename, error);
    if (fd < 0) {
        return FALSE;
    }
    
    if (content) {
        gssize written = write(fd, content, strlen(content));
        if (written < 0) {
            close(fd);
            g_unlink(*filename);
            g_free(*filename);
            *filename = NULL;
            g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(errno),
                       "Failed to write to temp file: %s", g_strerror(errno));
            return FALSE;
        }
    }
    
    close(fd);
    return TRUE;
}

gboolean mtx_test_file_cleanup_temp(const gchar *filename)
{
    g_return_val_if_fail(filename != NULL, FALSE);
    
    return g_unlink(filename) == 0;
}

/* Test type to string conversion */
const gchar *mtx_test_type_to_string(MtxTestType type)
{
    switch (type) {
        case MTX_TEST_TYPE_UNIT: return "unit";
        case MTX_TEST_TYPE_INTEGRATION: return "integration";
        case MTX_TEST_TYPE_FUNCTIONAL: return "functional";
        case MTX_TEST_TYPE_PERFORMANCE: return "performance";
        case MTX_TEST_TYPE_SECURITY: return "security";
        case MTX_TEST_TYPE_UI: return "ui";
        case MTX_TEST_TYPE_NETWORK: return "network";
        case MTX_TEST_TYPE_PLUGIN: return "plugin";
        default: return "unknown";
    }
}

/* Test result to string conversion */
const gchar *mtx_test_result_to_string(MtxTestResult result)
{
    switch (result) {
        case MTX_TEST_RESULT_PENDING: return "pending";
        case MTX_TEST_RESULT_RUNNING: return "running";
        case MTX_TEST_RESULT_PASSED: return "passed";
        case MTX_TEST_RESULT_FAILED: return "failed";
        case MTX_TEST_RESULT_SKIPPED: return "skipped";
        case MTX_TEST_RESULT_ERROR: return "error";
        default: return "unknown";
    }
}

/* Test priority to string conversion */
const gchar *mtx_test_priority_to_string(MtxTestPriority priority)
{
    switch (priority) {
        case MTX_TEST_PRIORITY_LOW: return "low";
        case MTX_TEST_PRIORITY_NORMAL: return "normal";
        case MTX_TEST_PRIORITY_HIGH: return "high";
        case MTX_TEST_PRIORITY_CRITICAL: return "critical";
        default: return "unknown";
    }
}

/* Test fixture functions */
MtxTestFixture *mtx_test_fixture_new(const gchar *name)
{
    g_return_val_if_fail(name != NULL, NULL);
    
    MtxTestFixture *fixture = g_new0(MtxTestFixture, 1);
    
    fixture->name = g_strdup(name);
    fixture->properties = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    
    return fixture;
}

void mtx_test_fixture_free(MtxTestFixture *fixture)
{
    if (!fixture) return;
    
    g_free(fixture->name);
    
    if (fixture->data && fixture->data_free_func) {
        fixture->data_free_func(fixture->data);
    }
    
    g_hash_table_destroy(fixture->properties);
    g_free(fixture);
}

/* Stub implementations for remaining functions */
gboolean mtx_test_runner_run_all(MtxTestRunner *runner, GError **error)
{
    /* Implementation would run all test suites */
    g_info("Running all test suites");
    return TRUE;
}

MtxTestReport *mtx_test_report_new(const gchar *name)
{
    MtxTestReport *report = g_new0(MtxTestReport, 1);
    report->name = g_strdup(name);
    report->timestamp = g_get_real_time();
    return report;
}

void mtx_test_report_free(MtxTestReport *report)
{
    if (!report) return;
    
    g_free(report->name);
    g_free(report->html_report);
    g_free(report->xml_report);
    g_free(report->json_report);
    
    if (report->json_data) {
        json_node_unref(report->json_data);
    }
    
    g_list_free(report->suite_results);
    g_free(report);
}
