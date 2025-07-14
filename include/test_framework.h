#ifndef __TEST_FRAMEWORK_H__
#define __TEST_FRAMEWORK_H__

#include <glib.h>
#include <gtk/gtk.h>
#include <json-glib/json-glib.h>

G_BEGIN_DECLS

/* Comprehensive Test Framework for MegaTunix Redux 2025 */

/* Test types */
typedef enum {
    MTX_TEST_TYPE_UNIT,           /* Unit tests */
    MTX_TEST_TYPE_INTEGRATION,    /* Integration tests */
    MTX_TEST_TYPE_FUNCTIONAL,     /* Functional tests */
    MTX_TEST_TYPE_PERFORMANCE,    /* Performance tests */
    MTX_TEST_TYPE_SECURITY,       /* Security tests */
    MTX_TEST_TYPE_UI,             /* User interface tests */
    MTX_TEST_TYPE_NETWORK,        /* Network tests */
    MTX_TEST_TYPE_PLUGIN          /* Plugin tests */
} MtxTestType;

/* Test result status */
typedef enum {
    MTX_TEST_RESULT_PENDING,
    MTX_TEST_RESULT_RUNNING,
    MTX_TEST_RESULT_PASSED,
    MTX_TEST_RESULT_FAILED,
    MTX_TEST_RESULT_SKIPPED,
    MTX_TEST_RESULT_ERROR
} MtxTestResult;

/* Test priority levels */
typedef enum {
    MTX_TEST_PRIORITY_LOW,
    MTX_TEST_PRIORITY_NORMAL,
    MTX_TEST_PRIORITY_HIGH,
    MTX_TEST_PRIORITY_CRITICAL
} MtxTestPriority;

/* Forward declarations */
typedef struct _MtxTestCase MtxTestCase;
typedef struct _MtxTestSuite MtxTestSuite;
typedef struct _MtxTestRunner MtxTestRunner;
typedef struct _MtxTestFixture MtxTestFixture;
typedef struct _MtxTestContext MtxTestContext;
typedef struct _MtxTestReport MtxTestReport;
typedef struct _MtxMockObject MtxMockObject;

/* Test function signatures */
typedef gboolean (*MtxTestFunc)(MtxTestContext *context, GError **error);
typedef void (*MtxTestSetupFunc)(MtxTestContext *context);
typedef void (*MtxTestTeardownFunc)(MtxTestContext *context);
typedef gboolean (*MtxTestAssertFunc)(gpointer actual, gpointer expected, const gchar *message);

/* Test case structure */
struct _MtxTestCase {
    gchar *name;
    gchar *description;
    MtxTestType type;
    MtxTestPriority priority;
    MtxTestFunc test_func;
    MtxTestSetupFunc setup_func;
    MtxTestTeardownFunc teardown_func;
    MtxTestResult result;
    GError *error;
    gdouble execution_time;
    gint64 timestamp;
    GHashTable *metadata;
    gchar **tags;
    gchar **dependencies;
    gboolean enabled;
    gint timeout_seconds;
};

/* Test suite structure */
struct _MtxTestSuite {
    gchar *name;
    gchar *description;
    GList *test_cases;
    MtxTestSetupFunc suite_setup;
    MtxTestTeardownFunc suite_teardown;
    GHashTable *shared_data;
    gint total_tests;
    gint passed_tests;
    gint failed_tests;
    gint skipped_tests;
    gdouble total_time;
};

/* Test fixture structure */
struct _MtxTestFixture {
    gchar *name;
    gpointer data;
    GDestroyNotify data_free_func;
    MtxTestSetupFunc setup;
    MtxTestTeardownFunc teardown;
    GHashTable *properties;
};

/* Test context structure */
struct _MtxTestContext {
    MtxTestCase *current_test;
    MtxTestSuite *current_suite;
    MtxTestRunner *runner;
    MtxTestFixture *fixture;
    GHashTable *mock_objects;
    GHashTable *test_data;
    GString *output_buffer;
    GString *error_buffer;
    GCancellable *cancellable;
    gboolean verbose;
    gboolean stop_on_failure;
};

/* Test runner structure */
struct _MtxTestRunner {
    GList *test_suites;
    MtxTestContext *context;
    GHashTable *global_fixtures;
    GHashTable *results;
    GString *report_buffer;
    gchar *output_directory;
    gboolean parallel_execution;
    gint max_threads;
    GThreadPool *thread_pool;
    GMutex results_mutex;
    
    /* Callbacks */
    void (*test_started)(MtxTestCase *test, gpointer user_data);
    void (*test_completed)(MtxTestCase *test, MtxTestResult result, gpointer user_data);
    void (*suite_started)(MtxTestSuite *suite, gpointer user_data);
    void (*suite_completed)(MtxTestSuite *suite, gpointer user_data);
    gpointer callback_data;
};

/* Test report structure */
struct _MtxTestReport {
    gchar *name;
    gint64 timestamp;
    gdouble total_time;
    gint total_suites;
    gint total_tests;
    gint passed_tests;
    gint failed_tests;
    gint skipped_tests;
    GList *suite_results;
    JsonNode *json_data;
    gchar *html_report;
    gchar *xml_report;
    gchar *json_report;
};

/* Mock object structure */
struct _MtxMockObject {
    gchar *name;
    GType object_type;
    gpointer mock_data;
    GHashTable *method_implementations;
    GHashTable *call_counts;
    GHashTable *return_values;
    gboolean strict_mode;
};

/* Test framework initialization */
gboolean mtx_test_framework_init(void);
void mtx_test_framework_cleanup(void);

/* Test case management */
MtxTestCase *mtx_test_case_new(const gchar *name, MtxTestFunc test_func);
void mtx_test_case_free(MtxTestCase *test_case);
void mtx_test_case_set_description(MtxTestCase *test_case, const gchar *description);
void mtx_test_case_set_type(MtxTestCase *test_case, MtxTestType type);
void mtx_test_case_set_priority(MtxTestCase *test_case, MtxTestPriority priority);
void mtx_test_case_set_timeout(MtxTestCase *test_case, gint timeout_seconds);
void mtx_test_case_add_tag(MtxTestCase *test_case, const gchar *tag);
void mtx_test_case_add_dependency(MtxTestCase *test_case, const gchar *dependency);
void mtx_test_case_set_setup(MtxTestCase *test_case, MtxTestSetupFunc setup_func);
void mtx_test_case_set_teardown(MtxTestCase *test_case, MtxTestTeardownFunc teardown_func);

/* Test suite management */
MtxTestSuite *mtx_test_suite_new(const gchar *name);
void mtx_test_suite_free(MtxTestSuite *suite);
void mtx_test_suite_add_test(MtxTestSuite *suite, MtxTestCase *test_case);
void mtx_test_suite_set_setup(MtxTestSuite *suite, MtxTestSetupFunc setup_func);
void mtx_test_suite_set_teardown(MtxTestSuite *suite, MtxTestTeardownFunc teardown_func);
void mtx_test_suite_set_shared_data(MtxTestSuite *suite, const gchar *key, gpointer data);
gpointer mtx_test_suite_get_shared_data(MtxTestSuite *suite, const gchar *key);

/* Test runner management */
MtxTestRunner *mtx_test_runner_new(void);
void mtx_test_runner_free(MtxTestRunner *runner);
void mtx_test_runner_add_suite(MtxTestRunner *runner, MtxTestSuite *suite);
void mtx_test_runner_set_output_directory(MtxTestRunner *runner, const gchar *directory);
void mtx_test_runner_set_parallel_execution(MtxTestRunner *runner, gboolean enabled, gint max_threads);
gboolean mtx_test_runner_run_all(MtxTestRunner *runner, GError **error);
gboolean mtx_test_runner_run_suite(MtxTestRunner *runner, const gchar *suite_name, GError **error);
gboolean mtx_test_runner_run_test(MtxTestRunner *runner, const gchar *test_name, GError **error);
gboolean mtx_test_runner_run_by_tag(MtxTestRunner *runner, const gchar *tag, GError **error);

/* Test context functions */
MtxTestContext *mtx_test_context_new(MtxTestRunner *runner);
void mtx_test_context_free(MtxTestContext *context);
void mtx_test_context_set_test_data(MtxTestContext *context, const gchar *key, gpointer data);
gpointer mtx_test_context_get_test_data(MtxTestContext *context, const gchar *key);
void mtx_test_context_log(MtxTestContext *context, const gchar *message);
void mtx_test_context_log_error(MtxTestContext *context, const gchar *message);

/* Assertion functions */
gboolean mtx_assert_true(gboolean condition, const gchar *message);
gboolean mtx_assert_false(gboolean condition, const gchar *message);
gboolean mtx_assert_null(gpointer pointer, const gchar *message);
gboolean mtx_assert_not_null(gpointer pointer, const gchar *message);
gboolean mtx_assert_equal_int(gint actual, gint expected, const gchar *message);
gboolean mtx_assert_equal_uint(guint actual, guint expected, const gchar *message);
gboolean mtx_assert_equal_double(gdouble actual, gdouble expected, gdouble epsilon, const gchar *message);
gboolean mtx_assert_equal_string(const gchar *actual, const gchar *expected, const gchar *message);
gboolean mtx_assert_equal_pointer(gpointer actual, gpointer expected, const gchar *message);
gboolean mtx_assert_greater_than_int(gint actual, gint threshold, const gchar *message);
gboolean mtx_assert_less_than_int(gint actual, gint threshold, const gchar *message);
gboolean mtx_assert_in_range_double(gdouble value, gdouble min, gdouble max, const gchar *message);
gboolean mtx_assert_string_contains(const gchar *haystack, const gchar *needle, const gchar *message);
gboolean mtx_assert_string_starts_with(const gchar *string, const gchar *prefix, const gchar *message);
gboolean mtx_assert_string_ends_with(const gchar *string, const gchar *suffix, const gchar *message);

/* Mock object functions */
MtxMockObject *mtx_mock_object_new(const gchar *name, GType object_type);
void mtx_mock_object_free(MtxMockObject *mock);
void mtx_mock_object_set_return_value(MtxMockObject *mock, const gchar *method, gpointer return_value);
void mtx_mock_object_set_method_implementation(MtxMockObject *mock, const gchar *method, GCallback implementation);
gint mtx_mock_object_get_call_count(MtxMockObject *mock, const gchar *method);
void mtx_mock_object_reset_calls(MtxMockObject *mock);
gboolean mtx_mock_object_was_called(MtxMockObject *mock, const gchar *method);
gboolean mtx_mock_object_was_called_with(MtxMockObject *mock, const gchar *method, ...);

/* Test fixture functions */
MtxTestFixture *mtx_test_fixture_new(const gchar *name);
void mtx_test_fixture_free(MtxTestFixture *fixture);
void mtx_test_fixture_set_data(MtxTestFixture *fixture, gpointer data, GDestroyNotify free_func);
void mtx_test_fixture_set_setup(MtxTestFixture *fixture, MtxTestSetupFunc setup);
void mtx_test_fixture_set_teardown(MtxTestFixture *fixture, MtxTestTeardownFunc teardown);
void mtx_test_fixture_set_property(MtxTestFixture *fixture, const gchar *key, gpointer value);
gpointer mtx_test_fixture_get_property(MtxTestFixture *fixture, const gchar *key);

/* Performance testing functions */
gdouble mtx_test_measure_execution_time(GFunc func, gpointer data);
gboolean mtx_test_performance_benchmark(const gchar *name, GFunc func, gpointer data, 
                                       gint iterations, gdouble max_time_seconds);
void mtx_test_performance_start_timer(MtxTestContext *context, const gchar *timer_name);
gdouble mtx_test_performance_stop_timer(MtxTestContext *context, const gchar *timer_name);

/* Memory testing functions */
gsize mtx_test_get_memory_usage(void);
gboolean mtx_test_check_memory_leaks(MtxTestContext *context);
void mtx_test_memory_mark_start(MtxTestContext *context);
gboolean mtx_test_memory_check_leaks(MtxTestContext *context);

/* UI testing functions */
gboolean mtx_test_ui_widget_exists(GtkWidget *parent, const gchar *widget_name);
gboolean mtx_test_ui_button_click(GtkButton *button);
gboolean mtx_test_ui_entry_set_text(GtkEntry *entry, const gchar *text);
gchar *mtx_test_ui_entry_get_text(GtkEntry *entry);
gboolean mtx_test_ui_window_is_visible(GtkWindow *window);
gboolean mtx_test_ui_wait_for_widget(GtkWidget *widget, guint timeout_ms);

/* Network testing functions */
gboolean mtx_test_network_server_start(guint16 port, GError **error);
void mtx_test_network_server_stop(void);
gboolean mtx_test_network_client_connect(const gchar *host, guint16 port, GError **error);
void mtx_test_network_client_disconnect(void);
gboolean mtx_test_network_send_data(const gchar *data, GError **error);
gchar *mtx_test_network_receive_data(guint timeout_ms, GError **error);

/* File testing functions */
gboolean mtx_test_file_create_temp(gchar **filename, const gchar *content, GError **error);
gboolean mtx_test_file_cleanup_temp(const gchar *filename);
gboolean mtx_test_file_compare_contents(const gchar *file1, const gchar *file2, GError **error);
gboolean mtx_test_directory_create_temp(gchar **dirname, GError **error);
gboolean mtx_test_directory_cleanup_temp(const gchar *dirname);

/* Report generation */
MtxTestReport *mtx_test_report_new(const gchar *name);
void mtx_test_report_free(MtxTestReport *report);
void mtx_test_report_add_suite_result(MtxTestReport *report, MtxTestSuite *suite);
gboolean mtx_test_report_generate_html(MtxTestReport *report, const gchar *filename, GError **error);
gboolean mtx_test_report_generate_xml(MtxTestReport *report, const gchar *filename, GError **error);
gboolean mtx_test_report_generate_json(MtxTestReport *report, const gchar *filename, GError **error);

/* Test discovery */
GList *mtx_test_discover_tests(const gchar *directory);
GList *mtx_test_discover_tests_by_pattern(const gchar *directory, const gchar *pattern);

/* Test utilities */
const gchar *mtx_test_type_to_string(MtxTestType type);
const gchar *mtx_test_result_to_string(MtxTestResult result);
const gchar *mtx_test_priority_to_string(MtxTestPriority priority);

/* Test macros for convenience */
#define MTX_TEST_CASE(name, func) mtx_test_case_new(name, func)
#define MTX_TEST_SUITE(name) mtx_test_suite_new(name)
#define MTX_ASSERT_TRUE(condition) mtx_assert_true(condition, #condition)
#define MTX_ASSERT_FALSE(condition) mtx_assert_false(condition, #condition)
#define MTX_ASSERT_NULL(pointer) mtx_assert_null(pointer, #pointer " should be NULL")
#define MTX_ASSERT_NOT_NULL(pointer) mtx_assert_not_null(pointer, #pointer " should not be NULL")
#define MTX_ASSERT_EQUAL(actual, expected) _Generic((actual), \
    int: mtx_assert_equal_int, \
    unsigned int: mtx_assert_equal_uint, \
    double: mtx_assert_equal_double, \
    char*: mtx_assert_equal_string, \
    const char*: mtx_assert_equal_string, \
    default: mtx_assert_equal_pointer \
)(actual, expected, #actual " == " #expected)

G_END_DECLS

#endif /* __TEST_FRAMEWORK_H__ */
