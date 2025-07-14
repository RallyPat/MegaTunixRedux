/*
 * Copyright (C) 2002-2012 by Dave J. Andruczyk <djandruczyk at yahoo dot com>
 * Modernized as MegaTunix Redux
 *
 * Linux Megasquirt tuning software
 * 
 * 
 * This software comes under the GPL (GNU Public License)
 * You may freely copy,distribute etc. this as long as the source code
 * is made available for FREE.
 * 
 * No warranty is made or implied. You use this program at your own risk.
 */

/*!
  \file src/main.c
  \ingroup CoreMtx
  \brief The holder of the main() function which gets the ball rolling...
  \author David Andruczyk
  */

#ifdef GTK_DISABLE_SINGLE_INCLUDES
#undef GTK_DISABLE_SINGLE_INCLUDES
#endif


#include <args.h>
#include <binlogger.h>
#include <core_gui.h>
#include <debugging.h>
#include <dispatcher.h>
#include <locale.h>
#include <locking.h>
#include <init.h>
#include <serialio.h>
#include <stdio.h>
#include <sleep_calib.h>
#include <stringmatch.h>
#include <timeout_handlers.h>
#include <speeduino_bridge.h>

gboolean gl_ability = FALSE;
gconstpointer *global_data = NULL;

/* Forward declaration */
static void activate_app(GtkApplication *app, gpointer user_data);

/* External function declaration */
extern CmdLineArgs *init_args(void);

/*!
  \brief main() is the typical main function in a C program, it performs
  all core initialization, loading of all main parameters, initializing handlers
  and entering gtk_main to process events until program close
  \param argc is the count of command line arguments
  \param argv is the array of command line args
  \returns TRUE
  */
gint main(gint argc, gchar ** argv)
{
	setlocale(LC_ALL,"");
#ifdef __WIN32__
	bindtextdomain(PACKAGE, "C:\\Program Files\\MegaTunix Redux\\dist\\locale");
#else
	bindtextdomain(PACKAGE, LOCALEDIR);
#endif
	textdomain (PACKAGE);

#ifdef DEBUG
	printf("This is a debug release, Git hash: %s\n",GIT_HASH);
#endif
	
	/* Use GtkApplication for GTK4 */
	GtkApplication *app;
	app = gtk_application_new("org.megasquirt.megatunix-redux", G_APPLICATION_FLAGS_NONE);
	g_signal_connect(app, "activate", G_CALLBACK(activate_app), NULL);
	
	int status = g_application_run(G_APPLICATION(app), argc, argv);
	
	/* Cleanup bridge on exit */
	speeduino_bridge_shutdown();
	
	g_object_unref(app);
	
	EXIT();
	return status;
}

/*!
  \brief activate_app() is called when the GtkApplication is activated
  \param app is the GtkApplication
  \param user_data is user data
  */
static void activate_app(GtkApplication *app, gpointer user_data)
{
	g_print("Application activated\n");
	
	/* Set up all the initialization that was previously in main() */
	Serial_Params *serial_params = NULL;
	GAsyncQueue *queue = NULL;
	GTimer *timer = NULL;
	GCond *rtv_thread_cond = NULL;
	GMutex *dash_mutex = NULL;
	GMutex *rtt_mutex = NULL;
	GMutex *rtv_mutex = NULL;
	GMutex *rtv_thread_mutex = NULL;
	GMutex *serio_mutex = NULL;
	gint id = 0;
	
	/* OpenGL support will be handled via modern GTK4 GL context */
	if (gdk_gl_query() == FALSE) {
		g_print("OpenGL not supported\n");
		return;
	}
	else
		gl_ability = TRUE;

	global_data = g_new0(gconstpointer, 1);

	/* Condition Variables */
	rtv_thread_cond = g_new0(GCond,1);
	g_cond_init(rtv_thread_cond);
	DATA_SET(global_data,"rtv_thread_cond",rtv_thread_cond);
	/* Mutexes */
	dash_mutex = g_new0(GMutex, 1);
	g_mutex_init(dash_mutex);
	DATA_SET(global_data,"dash_mutex",dash_mutex);
	rtt_mutex = g_new0(GMutex, 1);
	g_mutex_init(rtt_mutex);
	DATA_SET(global_data,"rtt_mutex",rtt_mutex);
	rtv_mutex = g_new0(GMutex, 1);
	g_mutex_init(rtv_mutex);
	DATA_SET(global_data,"rtv_mutex",rtv_mutex);
	rtv_thread_mutex = g_new0(GMutex, 1);
	g_mutex_init(rtv_thread_mutex);
	DATA_SET(global_data,"rtv_thread_mutex",rtv_thread_mutex);
	serio_mutex = g_new0(GMutex, 1);
	g_mutex_init(serio_mutex);
	DATA_SET(global_data,"serio_mutex",serio_mutex);

	/* Build table of strings to enum values */
	build_string_2_enum_table();
	serial_params = (Serial_Params *)g_malloc0(sizeof(Serial_Params));
	DATA_SET(global_data,"serial_params",serial_params);

	/* Initialize args structure */
	CmdLineArgs *args = init_args();
	DATA_SET(global_data,"args",args);

	/* This will exit mtx if the locking fails! */
	/* Prevents multiple instances  but stops esoteric usess too 
	 * create_mtx_lock();
	 * */
	open_debug();		/* Open debug log */

	ENTER();
	/* Allocate memory  */
	init();			/* Initialize global vars */
	make_mtx_dirs();	/* Create config file dirs if missing */

	/* Create Message passing queues */
	queue = g_async_queue_new();
	DATA_SET_FULL(global_data,"io_data_queue",queue,g_async_queue_unref);
	queue = g_async_queue_new();
	DATA_SET_FULL(global_data,"slave_msg_queue",queue,g_async_queue_unref);
	queue = g_async_queue_new();
	DATA_SET_FULL(global_data,"io_repair_queue",queue,g_async_queue_unref);

	read_config();
	setup_gui(app);		

	gtk_rc_parse_string("style \"override\"\n{\n\tGtkTreeView::horizontal-separator = 0\n\tGtkTreeView::vertical-separator = 0\n}\nwidget_class \"*\" style \"override\"");

	/* This doesn't do any GUI stuff so can run as is... */
	id = g_timeout_add(2000,(GSourceFunc)flush_binary_logs,NULL);
    DATA_SET(global_data,"binlog_flush_id",GINT_TO_POINTER(id));

	sleep_calib();
	/* Check for first_time flag, if so, run first time wizard, otherwise
	   load personality choice
	   */
	timer = g_timer_new();
	DATA_SET_FULL(global_data,"mtx_uptime_timer",timer,g_timer_destroy);
	g_idle_add((GSourceFunc)check_for_first_time,NULL);
	
	DATA_SET(global_data,"ready",GINT_TO_POINTER(TRUE));
	
	g_print("Application initialization complete\n");
}
