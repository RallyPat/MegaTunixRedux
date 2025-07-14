/*
 * Minimal MegaTunix Redux Test Application
 * Tests core functionality without full GUI
 */

#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <string.h>

// Minimal application to test plugin loading and basic functionality
static void activate(GtkApplication *app, gpointer user_data)
{
    GtkWidget *window;
    GtkWidget *grid;
    GtkWidget *button;
    GtkWidget *label;
    
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "MegaTunix Redux 2025 - Test");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
    
    grid = gtk_grid_new();
    gtk_window_set_child(GTK_WINDOW(window), grid);
    
    label = gtk_label_new("MegaTunix Redux 2025 - Speeduino Plugin Test");
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 2, 1);
    
    button = gtk_button_new_with_label("Test Speeduino Connection");
    gtk_grid_attach(GTK_GRID(grid), button, 0, 1, 1, 1);
    
    button = gtk_button_new_with_label("Load Plugin");
    gtk_grid_attach(GTK_GRID(grid), button, 1, 1, 1, 1);
    
    // Add status area
    GtkWidget *status_label = gtk_label_new("Ready to test Speeduino plugin");
    gtk_grid_attach(GTK_GRID(grid), status_label, 0, 2, 2, 1);
    
    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char *argv[])
{
    GtkApplication *app;
    int status;
    
    printf("=== MegaTunix Redux 2025 - Minimal Test Application ===\n");
    printf("Testing GTK4 application startup...\n");
    
    app = gtk_application_new("com.megatunix.redux.test", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    
    return status;
}
