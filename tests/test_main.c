#include "test_framework.h"
#include "modern_widgets.h"
#include "security_utils.h"
#include "plugin_system.h"
#include "network_security.h"

/* Example unit tests for MegaTunix Redux 2025 */

/* Test security utilities */
static gboolean test_security_validate_input(MtxTestContext *context, GError **error)
{
    /* Test input validation */
    if (!MTX_ASSERT_TRUE(mtx_security_validate_input("valid_input", MTX_INPUT_TYPE_ALPHANUMERIC))) {
        return FALSE;
    }
    
    if (!MTX_ASSERT_FALSE(mtx_security_validate_input("invalid@input", MTX_INPUT_TYPE_ALPHANUMERIC))) {
        return FALSE;
    }
    
    if (!MTX_ASSERT_FALSE(mtx_security_validate_input(NULL, MTX_INPUT_TYPE_ALPHANUMERIC))) {
        return FALSE;
    }
    
    return TRUE;
}

static gboolean test_security_safe_string_operations(MtxTestContext *context, GError **error)
{
    gchar buffer[32];
    
    /* Test safe string copy */
    if (!MTX_ASSERT_TRUE(mtx_security_safe_strcpy(buffer, sizeof(buffer), "test string"))) {
        return FALSE;
    }
    
    if (!MTX_ASSERT_EQUAL(buffer, "test string")) {
        return FALSE;
    }
    
    /* Test buffer overflow protection */
    if (!MTX_ASSERT_FALSE(mtx_security_safe_strcpy(buffer, sizeof(buffer), 
                                                  "this is a very long string that should not fit in the buffer and should be rejected"))) {
        return FALSE;
    }
    
    return TRUE;
}

/* Test modern widgets */
static gboolean test_modern_widgets_creation(MtxTestContext *context, GError **error)
{
    gtk_init();
    
    /* Test gauge creation */
    GtkWidget *gauge = mtx_modern_gauge_new();
    if (!MTX_ASSERT_NOT_NULL(gauge)) {
        return FALSE;
    }
    
    /* Test setting gauge value */
    mtx_modern_gauge_set_value(MTX_MODERN_GAUGE(gauge), 0.5);
    gdouble value = mtx_modern_gauge_get_value(MTX_MODERN_GAUGE(gauge));
    if (!MTX_ASSERT_EQUAL(value, 0.5)) {
        return FALSE;
    }
    
    g_object_unref(gauge);
    return TRUE;
}

static gboolean test_modern_widgets_theming(MtxTestContext *context, GError **error)
{
    gtk_init();
    
    /* Test theme application */
    GtkWidget *widget = gtk_label_new("Test");
    if (!MTX_ASSERT_NOT_NULL(widget)) {
        return FALSE;
    }
    
    mtx_modern_widgets_apply_theme(widget, "dark");
    
    /* Test that CSS classes were applied */
    GtkStyleContext *style_context = gtk_widget_get_style_context(widget);
    if (!MTX_ASSERT_TRUE(gtk_style_context_has_class(style_context, "mtx-dark-theme"))) {
        return FALSE;
    }
    
    g_object_unref(widget);
    return TRUE;
}

/* Test plugin system */
static gboolean test_plugin_manager_creation(MtxTestContext *context, GError **error)
{
    MtxPluginManager *manager = mtx_plugin_manager_new();
    if (!MTX_ASSERT_NOT_NULL(manager)) {
        return FALSE;
    }
    
    /* Test initialization */
    gchar *temp_dir;
    if (!mtx_test_directory_create_temp(&temp_dir, error)) {
        mtx_plugin_manager_free(manager);
        return FALSE;
    }
    
    if (!MTX_ASSERT_TRUE(mtx_plugin_manager_initialize(manager, temp_dir, NULL))) {
        mtx_plugin_manager_free(manager);
        mtx_test_directory_cleanup_temp(temp_dir);
        g_free(temp_dir);
        return FALSE;
    }
    
    mtx_plugin_manager_cleanup(manager);
    mtx_plugin_manager_free(manager);
    mtx_test_directory_cleanup_temp(temp_dir);
    g_free(temp_dir);
    
    return TRUE;
}

/* Test network security */
static gboolean test_network_security_init(MtxTestContext *context, GError **error)
{
    /* Test network security initialization */
    if (!MTX_ASSERT_TRUE(mtx_network_security_init())) {
        return FALSE;
    }
    
    mtx_network_security_cleanup();
    return TRUE;
}

static gboolean test_network_password_hashing(MtxTestContext *context, GError **error)
{
    const gchar *password = "test_password";
    gchar *salt = mtx_security_generate_salt();
    
    if (!MTX_ASSERT_NOT_NULL(salt)) {
        return FALSE;
    }
    
    gchar *hash = mtx_security_hash_password(password, salt);
    if (!MTX_ASSERT_NOT_NULL(hash)) {
        g_free(salt);
        return FALSE;
    }
    
    /* Test password verification */
    if (!MTX_ASSERT_TRUE(mtx_security_verify_password(password, hash, salt))) {
        g_free(salt);
        g_free(hash);
        return FALSE;
    }
    
    /* Test wrong password */
    if (!MTX_ASSERT_FALSE(mtx_security_verify_password("wrong_password", hash, salt))) {
        g_free(salt);
        g_free(hash);
        return FALSE;
    }
    
    g_free(salt);
    g_free(hash);
    return TRUE;
}

/* Performance test example */
static gboolean test_performance_widget_creation(MtxTestContext *context, GError **error)
{
    gtk_init();
    
    mtx_test_performance_start_timer(context, "widget_creation");
    
    /* Create 1000 widgets */
    GList *widgets = NULL;
    for (int i = 0; i < 1000; i++) {
        GtkWidget *widget = gtk_label_new("Test Label");
        widgets = g_list_prepend(widgets, widget);
    }
    
    gdouble elapsed = mtx_test_performance_stop_timer(context, "widget_creation");
    
    /* Clean up */
    g_list_free_full(widgets, (GDestroyNotify)g_object_unref);
    
    /* Assert performance requirement: should complete in less than 1 second */
    if (!MTX_ASSERT_TRUE(elapsed < 1.0)) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "Widget creation took too long: %f seconds", elapsed);
        return FALSE;
    }
    
    mtx_test_context_log(context, g_strdup_printf("Created 1000 widgets in %f seconds", elapsed));
    return TRUE;
}

/* Test setup function */
static void test_setup(MtxTestContext *context)
{
    /* Initialize any global test resources */
    if (!mtx_test_framework_init()) {
        g_error("Failed to initialize test framework");
    }
}

/* Test teardown function */
static void test_teardown(MtxTestContext *context)
{
    /* Clean up global test resources */
    mtx_test_framework_cleanup();
}

/* External test suite declarations */
extern void speeduino_plugin_test_suite(MtxTestFramework *framework);

/* Main test function */
int main(int argc, char *argv[])
{
    /* Initialize test framework */
    if (!mtx_test_framework_init()) {
        g_error("Failed to initialize test framework");
        return 1;
    }
    
    /* Create test runner */
    MtxTestRunner *runner = mtx_test_runner_new();
    
    /* Create security test suite */
    MtxTestSuite *security_suite = mtx_test_suite_new("Security Tests");
    mtx_test_suite_set_setup(security_suite, test_setup);
    mtx_test_suite_set_teardown(security_suite, test_teardown);
    
    /* Add security tests */
    MtxTestCase *input_validation_test = mtx_test_case_new("Input Validation", test_security_validate_input);
    mtx_test_case_set_description(input_validation_test, "Test input validation functions");
    mtx_test_case_set_type(input_validation_test, MTX_TEST_TYPE_SECURITY);
    mtx_test_case_add_tag(input_validation_test, "security");
    mtx_test_case_add_tag(input_validation_test, "validation");
    mtx_test_suite_add_test(security_suite, input_validation_test);
    
    MtxTestCase *string_ops_test = mtx_test_case_new("Safe String Operations", test_security_safe_string_operations);
    mtx_test_case_set_description(string_ops_test, "Test secure string handling functions");
    mtx_test_case_set_type(string_ops_test, MTX_TEST_TYPE_SECURITY);
    mtx_test_case_add_tag(string_ops_test, "security");
    mtx_test_case_add_tag(string_ops_test, "strings");
    mtx_test_suite_add_test(security_suite, string_ops_test);
    
    /* Create UI test suite */
    MtxTestSuite *ui_suite = mtx_test_suite_new("UI Tests");
    
    /* Add UI tests */
    MtxTestCase *widget_creation_test = mtx_test_case_new("Widget Creation", test_modern_widgets_creation);
    mtx_test_case_set_description(widget_creation_test, "Test modern widget creation and basic operations");
    mtx_test_case_set_type(widget_creation_test, MTX_TEST_TYPE_UI);
    mtx_test_case_add_tag(widget_creation_test, "ui");
    mtx_test_case_add_tag(widget_creation_test, "widgets");
    mtx_test_suite_add_test(ui_suite, widget_creation_test);
    
    MtxTestCase *theming_test = mtx_test_case_new("Widget Theming", test_modern_widgets_theming);
    mtx_test_case_set_description(theming_test, "Test widget theming functionality");
    mtx_test_case_set_type(theming_test, MTX_TEST_TYPE_UI);
    mtx_test_case_add_tag(theming_test, "ui");
    mtx_test_case_add_tag(theming_test, "theming");
    mtx_test_suite_add_test(ui_suite, theming_test);
    
    /* Create plugin test suite */
    MtxTestSuite *plugin_suite = mtx_test_suite_new("Plugin Tests");
    
    MtxTestCase *plugin_manager_test = mtx_test_case_new("Plugin Manager", test_plugin_manager_creation);
    mtx_test_case_set_description(plugin_manager_test, "Test plugin manager creation and initialization");
    mtx_test_case_set_type(plugin_manager_test, MTX_TEST_TYPE_PLUGIN);
    mtx_test_case_add_tag(plugin_manager_test, "plugins");
    mtx_test_suite_add_test(plugin_suite, plugin_manager_test);
    
    /* Create network test suite */
    MtxTestSuite *network_suite = mtx_test_suite_new("Network Tests");
    
    MtxTestCase *network_init_test = mtx_test_case_new("Network Security Init", test_network_security_init);
    mtx_test_case_set_description(network_init_test, "Test network security subsystem initialization");
    mtx_test_case_set_type(network_init_test, MTX_TEST_TYPE_NETWORK);
    mtx_test_case_add_tag(network_init_test, "network");
    mtx_test_case_add_tag(network_init_test, "security");
    mtx_test_suite_add_test(network_suite, network_init_test);
    
    MtxTestCase *password_test = mtx_test_case_new("Password Hashing", test_network_password_hashing);
    mtx_test_case_set_description(password_test, "Test password hashing and verification");
    mtx_test_case_set_type(password_test, MTX_TEST_TYPE_SECURITY);
    mtx_test_case_set_priority(password_test, MTX_TEST_PRIORITY_HIGH);
    mtx_test_case_add_tag(password_test, "security");
    mtx_test_case_add_tag(password_test, "crypto");
    mtx_test_suite_add_test(network_suite, password_test);
    
    /* Create performance test suite */
    MtxTestSuite *performance_suite = mtx_test_suite_new("Performance Tests");
    
    MtxTestCase *perf_widget_test = mtx_test_case_new("Widget Creation Performance", test_performance_widget_creation);
    mtx_test_case_set_description(perf_widget_test, "Test widget creation performance");
    mtx_test_case_set_type(perf_widget_test, MTX_TEST_TYPE_PERFORMANCE);
    mtx_test_case_set_priority(perf_widget_test, MTX_TEST_PRIORITY_LOW);
    mtx_test_case_add_tag(perf_widget_test, "performance");
    mtx_test_case_add_tag(perf_widget_test, "ui");
    mtx_test_suite_add_test(performance_suite, perf_widget_test);
    
    /* Add suites to runner */
    mtx_test_runner_add_suite(runner, security_suite);
    mtx_test_runner_add_suite(runner, ui_suite);
    mtx_test_runner_add_suite(runner, plugin_suite);
    mtx_test_runner_add_suite(runner, network_suite);
    mtx_test_runner_add_suite(runner, performance_suite);
    
    /* Set output directory */
    mtx_test_runner_set_output_directory(runner, "./test-results");
    
    /* Run all tests */
    GError *error = NULL;
    gboolean success = mtx_test_runner_run_all(runner, &error);
    
    if (!success) {
        g_printerr("Test execution failed: %s\n", error ? error->message : "Unknown error");
        if (error) g_error_free(error);
    }
    
    /* Generate reports */
    MtxTestReport *report = mtx_test_report_new("MegaTunix Redux 2025 Test Report");
    
    /* Add results from all suites */
    GList *suites = mtx_test_runner_get_all_plugins(runner);  /* This should be get_all_suites */
    for (GList *l = suites; l != NULL; l = l->next) {
        mtx_test_report_add_suite_result(report, (MtxTestSuite *)l->data);
    }
    
    /* Generate HTML report */
    if (!mtx_test_report_generate_html(report, "./test-results/report.html", &error)) {
        g_printerr("Failed to generate HTML report: %s\n", error ? error->message : "Unknown error");
        if (error) {
            g_error_free(error);
            error = NULL;
        }
    }
    
    /* Generate XML report */
    if (!mtx_test_report_generate_xml(report, "./test-results/report.xml", &error)) {
        g_printerr("Failed to generate XML report: %s\n", error ? error->message : "Unknown error");
        if (error) {
            g_error_free(error);
            error = NULL;
        }
    }
    
    /* Cleanup */
    mtx_test_report_free(report);
    mtx_test_runner_free(runner);
    mtx_test_framework_cleanup();
    
    return success ? 0 : 1;
}
