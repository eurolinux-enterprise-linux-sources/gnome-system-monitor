/* -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* Procman - main window
 * Copyright (C) 2001 Kevin Vandersloot
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */


#include <config.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <gdk/gdkkeysyms.h>
#include <math.h>

#include "interface.h"
#include "proctable.h"
#include "procactions.h"
#include "procdialogs.h"
#include "memmaps.h"
#include "openfiles.h"
#include "procproperties.h"
#include "load-graph.h"
#include "util.h"
#include "disks.h"
#include "settings-keys.h"
#include "gsm_color_button.h"


static gboolean
cb_window_key_press_event (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    const char *current_page = gtk_stack_get_visible_child_name (GTK_STACK (GsmApplication::get()->stack));

    if (strcmp (current_page, "processes") == 0)
        return gtk_search_bar_handle_event (GTK_SEARCH_BAR (user_data), event);
    
    return FALSE;
}

static void
search_text_changed (GtkEditable *entry, gpointer data)
{
    GsmApplication * const app = static_cast<GsmApplication *>(data);
    gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (gtk_tree_model_sort_get_model (
                                    GTK_TREE_MODEL_SORT (gtk_tree_view_get_model(
                                    GTK_TREE_VIEW (app->tree))))));
}

static void 
create_proc_view(GsmApplication *app, GtkBuilder * builder)
{
    GtkWidget *proctree;
    GtkWidget *scrolled;

    proctree = proctable_new (app);
    scrolled = GTK_WIDGET (gtk_builder_get_object (builder, "processes_scrolled"));

    gtk_container_add (GTK_CONTAINER (scrolled), proctree);

    app->proc_actionbar_revealer = GTK_WIDGET (gtk_builder_get_object (builder, "proc_actionbar_revealer"));

    /* create popup_menu for the processes tab */
    GMenuModel *menu_model = G_MENU_MODEL (gtk_builder_get_object (builder, "process-popup-menu"));
    app->popup_menu = gtk_menu_new_from_model (menu_model);
    gtk_menu_attach_to_widget (GTK_MENU (app->popup_menu), app->main_window, NULL);
    
    app->search_bar = GTK_WIDGET (gtk_builder_get_object (builder, "proc_searchbar"));
    app->search_entry = GTK_WIDGET (gtk_builder_get_object (builder, "proc_searchentry"));
    
    gtk_search_bar_connect_entry (GTK_SEARCH_BAR(app->search_bar), GTK_ENTRY (app->search_entry));
    g_signal_connect (app->main_window, "key-press-event",
                      G_CALLBACK (cb_window_key_press_event), app->search_bar);
                  
    g_signal_connect (app->search_entry, "changed", G_CALLBACK (search_text_changed), app);

    g_object_bind_property (app->search_bar, "search-mode-enabled", app->search_button, "active", (GBindingFlags)(G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE));
}

void
cb_cpu_color_changed (GsmColorButton *cp, gpointer data)
{
    guint cpu_i = GPOINTER_TO_UINT (data);
    GSettings *settings = g_settings_new (GSM_GSETTINGS_SCHEMA);

    /* Get current values */
    GVariant *cpu_colors_var = g_settings_get_value (settings, GSM_SETTING_CPU_COLORS);
    gsize children_n = g_variant_n_children(cpu_colors_var);

    /* Create builder to contruct new setting with updated value for cpu i */
    GVariantBuilder builder;
    g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);

    for (guint i = 0; i < children_n; i++) {
        if(cpu_i == i) {
            gchar *color;
            GdkRGBA button_color;
            gsm_color_button_get_color(cp, &button_color);
            color = gdk_rgba_to_string (&button_color);
            g_variant_builder_add(&builder, "(us)", i, color);
            g_free (color);
        } else {
            g_variant_builder_add_value(&builder,
                                        g_variant_get_child_value(cpu_colors_var, i));
        }
    }

    /* Just set the value and let the changed::cpu-colors signal callback do the rest. */
    g_settings_set_value (settings, GSM_SETTING_CPU_COLORS,
                          g_variant_builder_end(&builder));
}

static void change_settings_color(GSettings *settings, const char *key,
                                  GsmColorButton *cp)
{
    GdkRGBA c;
    char *color;

    gsm_color_button_get_color(cp, &c);
    color = gdk_rgba_to_string (&c);
    g_settings_set_string (settings, key, color);
    g_free (color);
}

static void
cb_mem_color_changed (GsmColorButton *cp, gpointer data)
{
    GsmApplication *app = (GsmApplication *) data;
    change_settings_color (app->settings, GSM_SETTING_MEM_COLOR, cp);
}


static void
cb_swap_color_changed (GsmColorButton *cp, gpointer data)
{
    GsmApplication *app = (GsmApplication *) data;
    change_settings_color (app->settings, GSM_SETTING_SWAP_COLOR, cp);
}

static void
cb_net_in_color_changed (GsmColorButton *cp, gpointer data)
{
    GsmApplication *app = (GsmApplication *) data;
    change_settings_color (app->settings, GSM_SETTING_NET_IN_COLOR, cp);
}

static void
cb_net_out_color_changed (GsmColorButton *cp, gpointer data)
{
    GsmApplication *app = (GsmApplication *) data;
    change_settings_color(app->settings, GSM_SETTING_NET_OUT_COLOR, cp);
}

static void
create_sys_view (GsmApplication *app, GtkBuilder * builder)
{
    GtkWidget *cpu_graph_box, *mem_graph_box, *net_graph_box;
    GtkWidget *label,*cpu_label;
    GtkWidget *table;
    GtkWidget *color_picker;
    GtkWidget *picker_alignment;
    LoadGraph *cpu_graph, *mem_graph, *net_graph;

    gint i;
    gchar *title_text;
    gchar *label_text;
    gchar *title_template;

    // Translators: color picker title, %s is CPU, Memory, Swap, Receiving, Sending
    title_template = g_strdup(_("Pick a Color for '%s'"));

    /* The CPU BOX */
    
    cpu_graph_box = GTK_WIDGET (gtk_builder_get_object (builder, "cpu_graph_box"));

    cpu_graph = new LoadGraph(LOAD_GRAPH_CPU);
    gtk_box_pack_start (GTK_BOX (cpu_graph_box),
                        load_graph_get_widget(cpu_graph),
                        TRUE,
                        TRUE,
                        0);

    GtkWidget* cpu_table = GTK_WIDGET (gtk_builder_get_object (builder, "cpu_table"));
    gint cols = 4;
    for (i=0;i<app->config.num_cpus; i++) {
        GtkWidget *temp_hbox;

        temp_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_widget_show (temp_hbox);
        if (i < cols) {
            gtk_grid_insert_column(GTK_GRID(cpu_table), i%cols);
        }
        if ((i+1)%cols ==cols) {
            gtk_grid_insert_row(GTK_GRID(cpu_table), (i+1)/cols);
        }
        gtk_grid_attach(GTK_GRID (cpu_table), temp_hbox, i%cols, i/cols, 1, 1);
        color_picker = gsm_color_button_new (&cpu_graph->colors.at(i), GSMCP_TYPE_CPU);
        g_signal_connect (G_OBJECT (color_picker), "color-set",
                          G_CALLBACK (cb_cpu_color_changed), GINT_TO_POINTER (i));
        gtk_box_pack_start (GTK_BOX (temp_hbox), color_picker, FALSE, TRUE, 0);
        gtk_widget_set_size_request(GTK_WIDGET(color_picker), 32, -1);
        if(app->config.num_cpus == 1) {
            label_text = g_strdup (_("CPU"));
        } else {
            label_text = g_strdup_printf (_("CPU%d"), i+1);
        }
        title_text = g_strdup_printf(title_template, label_text);
        label = gtk_label_new (label_text);
        gsm_color_button_set_title(GSM_COLOR_BUTTON(color_picker), title_text);
        g_free(title_text);
        gtk_box_pack_start (GTK_BOX (temp_hbox), label, FALSE, FALSE, 6);
        gtk_widget_show (label);
        g_free (label_text);

        cpu_label = gtk_label_new (NULL);

        gtk_widget_set_valign (cpu_label, GTK_ALIGN_CENTER);
        gtk_widget_set_halign (cpu_label, GTK_ALIGN_START);
        gtk_box_pack_start (GTK_BOX (temp_hbox), cpu_label, FALSE, FALSE, 0);
        gtk_widget_show (cpu_label);
        load_graph_get_labels(cpu_graph)->cpu[i] = cpu_label;

    }

    app->cpu_graph = cpu_graph;

    /** The memory box */
    
    mem_graph_box = GTK_WIDGET (gtk_builder_get_object (builder, "mem_graph_box"));

    mem_graph = new LoadGraph(LOAD_GRAPH_MEM);
    gtk_box_pack_start (GTK_BOX (mem_graph_box),
                        load_graph_get_widget(mem_graph),
                        TRUE,
                        TRUE,
                        0);

    table = GTK_WIDGET (gtk_builder_get_object (builder, "mem_table"));

    color_picker = load_graph_get_mem_color_picker(mem_graph);
    g_signal_connect (G_OBJECT (color_picker), "color-set",
                      G_CALLBACK (cb_mem_color_changed), app);
    title_text = g_strdup_printf(title_template, _("Memory"));
    gsm_color_button_set_title(GSM_COLOR_BUTTON(color_picker), title_text);
    g_free(title_text);

    label = GTK_WIDGET(gtk_builder_get_object(builder, "memory_label"));

    gtk_grid_attach_next_to (GTK_GRID (table), color_picker, label, GTK_POS_LEFT, 1, 2);
    gtk_grid_attach_next_to (GTK_GRID (table), load_graph_get_labels(mem_graph)->memory, label, GTK_POS_BOTTOM, 1, 1);

    color_picker = load_graph_get_swap_color_picker(mem_graph);
    g_signal_connect (G_OBJECT (color_picker), "color-set",
                      G_CALLBACK (cb_swap_color_changed), app);
    title_text = g_strdup_printf(title_template, _("Swap"));
    gsm_color_button_set_title(GSM_COLOR_BUTTON(color_picker), title_text);
    g_free(title_text);

    label = GTK_WIDGET(gtk_builder_get_object(builder, "swap_label"));

    gtk_grid_attach_next_to (GTK_GRID (table), color_picker, label, GTK_POS_LEFT, 1, 2);
    gtk_grid_attach_next_to (GTK_GRID (table), load_graph_get_labels(mem_graph)->swap, label, GTK_POS_BOTTOM, 1, 1);

    app->mem_graph = mem_graph;

    /* The net box */
    
    net_graph_box = GTK_WIDGET (gtk_builder_get_object (builder, "net_graph_box"));

    net_graph = new LoadGraph(LOAD_GRAPH_NET);
    gtk_box_pack_start (GTK_BOX (net_graph_box),
                        load_graph_get_widget(net_graph),
                        TRUE,
                        TRUE,
                        0);

    table = GTK_WIDGET (gtk_builder_get_object (builder, "net_table"));

    color_picker = gsm_color_button_new (
        &net_graph->colors.at(0), GSMCP_TYPE_NETWORK_IN);
    g_signal_connect (G_OBJECT (color_picker), "color-set",
                      G_CALLBACK (cb_net_in_color_changed), app);
    title_text = g_strdup_printf(title_template, _("Receiving"));
    gsm_color_button_set_title(GSM_COLOR_BUTTON(color_picker), title_text);
    g_free(title_text);
    picker_alignment = GTK_WIDGET (gtk_builder_get_object (builder, "receiving_picker_alignment"));
    gtk_container_add (GTK_CONTAINER (picker_alignment), color_picker);

    label = GTK_WIDGET (gtk_builder_get_object(builder, "receiving_label"));
    gtk_grid_attach_next_to (GTK_GRID (table), load_graph_get_labels(net_graph)->net_in, label, GTK_POS_RIGHT, 1, 1);
    label = GTK_WIDGET (gtk_builder_get_object(builder, "total_received_label"));
    gtk_grid_attach_next_to (GTK_GRID (table), load_graph_get_labels(net_graph)->net_in_total, label, GTK_POS_RIGHT, 1, 1);

    color_picker = gsm_color_button_new (
        &net_graph->colors.at(1), GSMCP_TYPE_NETWORK_OUT);
    g_signal_connect (G_OBJECT (color_picker), "color-set",
                      G_CALLBACK (cb_net_out_color_changed), app);
    title_text = g_strdup_printf(title_template, _("Sending"));
    gsm_color_button_set_title(GSM_COLOR_BUTTON(color_picker), title_text);
    g_free(title_text);

    picker_alignment = GTK_WIDGET (gtk_builder_get_object (builder, "sending_picker_alignment"));
    gtk_container_add (GTK_CONTAINER (picker_alignment), color_picker);

    label = GTK_WIDGET (gtk_builder_get_object(builder, "sending_label"));
    gtk_grid_attach_next_to (GTK_GRID (table), load_graph_get_labels(net_graph)->net_out, label, GTK_POS_RIGHT, 1, 1);
    label = GTK_WIDGET (gtk_builder_get_object(builder, "total_sent_label"));
    gtk_grid_attach_next_to (GTK_GRID (table),  load_graph_get_labels(net_graph)->net_out_total, label, GTK_POS_RIGHT, 1, 1);

    app->net_graph = net_graph;
    g_free (title_template);

}

static void
on_activate_about (GSimpleAction *, GVariant *, gpointer data)
{
    GsmApplication *app = (GsmApplication *) data;

    const gchar * const authors[] = {
        "Kevin Vandersloot",
        "Erik Johnsson",
        "Jorgen Scheibengruber",
        "Benoît Dejean",
        "Paolo Borelli",
        "Karl Lattimer",
        "Chris Kühl",
        "Robert Roth",
        "Stefano Facchini",
        NULL
    };

    const gchar * const documenters[] = {
        "Bill Day",
        "Sun Microsystems",
        NULL
    };

    const gchar * const artists[] = {
        "Baptiste Mille-Mathias",
        NULL
    };

    gtk_show_about_dialog (
        GTK_WINDOW (app->main_window),
        "name",                 _("System Monitor"),
        "comments",             _("View current processes and monitor "
                                  "system state"),
        "version",              VERSION,
        "copyright",            "Copyright \xc2\xa9 2001-2004 Kevin Vandersloot\n"
                                "Copyright \xc2\xa9 2005-2007 Benoît Dejean\n"
                                "Copyright \xc2\xa9 2011 Chris Kühl",
        "logo-icon-name",       "utilities-system-monitor",
        "authors",              authors,
        "artists",              artists,
        "documenters",          documenters,
        "translator-credits",   _("translator-credits"),
        "license",              "GPL 2+",
        "wrap-license",         TRUE,
        NULL
        );
}

static void
on_activate_refresh (GSimpleAction *, GVariant *, gpointer data)
{
    GsmApplication *app = (GsmApplication *) data;
    proctable_update (app);
}

static void
kill_process_with_confirmation (GsmApplication *app, int signal) {
    gboolean kill_dialog = g_settings_get_boolean (app->settings, GSM_SETTING_SHOW_KILL_DIALOG);

    if (kill_dialog)
        procdialog_create_kill_dialog (app, signal);
    else
        kill_process (app, signal);
}

static void
on_activate_send_signal (GSimpleAction *, GVariant *parameter, gpointer data)
{
    GsmApplication *app = (GsmApplication *) data;

    /* no confirmation */
    gint32 signal = g_variant_get_int32(parameter);
    switch (signal) {
        case SIGCONT:
            kill_process (app, signal);
            break;
        case SIGSTOP:
        case SIGTERM:
        case SIGKILL:
            kill_process_with_confirmation (app, signal);
            break;
    }
}

static void
on_activate_memory_maps (GSimpleAction *, GVariant *, gpointer data)
{
    GsmApplication *app = (GsmApplication *) data;

    create_memmaps_dialog (app);
}

static void
on_activate_open_files (GSimpleAction *, GVariant *, gpointer data)
{
    GsmApplication *app = (GsmApplication *) data;

    create_openfiles_dialog (app);
}

static void
on_activate_process_properties (GSimpleAction *, GVariant *, gpointer data)
{
    GsmApplication *app = (GsmApplication *) data;

    create_procproperties_dialog (app);
}

static void
on_activate_radio (GSimpleAction *action, GVariant *parameter, gpointer data)
{
    g_action_change_state (G_ACTION (action), parameter);
}

static void
on_activate_toggle (GSimpleAction *action, GVariant *parameter, gpointer data)
{
    GVariant *state = g_action_get_state (G_ACTION (action));
    g_action_change_state (G_ACTION (action), g_variant_new_boolean (!g_variant_get_boolean (state)));    
    g_variant_unref (state);
}

static void
on_activate_search (GSimpleAction *action, GVariant *parameter, gpointer data)
{
    GsmApplication *app = (GsmApplication *) data;
    GVariant *state = g_action_get_state (G_ACTION (action));
    gboolean is_search_shortcut = g_variant_get_boolean (parameter);
    gboolean is_search_bar = gtk_search_bar_get_search_mode (GTK_SEARCH_BAR (app->search_bar));
    gtk_widget_set_visible (app->search_bar, is_search_bar || is_search_shortcut);
    if (is_search_shortcut && is_search_bar) {
        gtk_widget_grab_focus (app->search_entry);
    } else {
        g_action_change_state (G_ACTION (action), g_variant_new_boolean (!g_variant_get_boolean (state)));
    }
    g_variant_unref (state);
}

static void
change_show_page_state (GSimpleAction *action, GVariant *state, gpointer data)
{
    GsmApplication *app = (GsmApplication *) data;

    g_simple_action_set_state (action, state);
    g_settings_set_value (app->settings, GSM_SETTING_CURRENT_TAB, state);
}

static void
change_show_processes_state (GSimpleAction *action, GVariant *state, gpointer data)
{
    GsmApplication *app = (GsmApplication *) data;

    g_simple_action_set_state (action, state);
    g_settings_set_value (app->settings, GSM_SETTING_SHOW_WHOSE_PROCESSES, state);
}

static void
change_show_dependencies_state (GSimpleAction *action, GVariant *state, gpointer data)
{
    GsmApplication *app = (GsmApplication *) data;

    g_simple_action_set_state (action, state);
    g_settings_set_value (app->settings, GSM_SETTING_SHOW_DEPENDENCIES, state);
}

static void
on_activate_priority (GSimpleAction *action, GVariant *parameter, gpointer data)
{
    GsmApplication *app = (GsmApplication *) data;

    g_action_change_state (G_ACTION (action), parameter);

    const gint32 priority = g_variant_get_int32 (parameter);
    switch (priority) {
        case 32: 
            procdialog_create_renice_dialog (app);
            break;
        default:
            renice (app, priority);
            break;
    }

}

static void
change_priority_state (GSimpleAction *action, GVariant *state, gpointer data)
{
    g_simple_action_set_state (action, state);
}

void
update_page_activities (GsmApplication *app)
{
    const char *current_page = gtk_stack_get_visible_child_name (GTK_STACK (app->stack));

    if (strcmp (current_page, "processes") == 0) {
        GAction *search_action = g_action_map_lookup_action (G_ACTION_MAP (app->main_window),
                                                             "search");
        proctable_update (app);
        proctable_thaw (app);

        gtk_widget_show (app->end_process_button);
        gtk_widget_show (app->search_button);
        gtk_widget_show (app->process_menu_button);

        update_sensitivity (app);

        if (g_variant_get_boolean (g_action_get_state (search_action)))
            gtk_widget_grab_focus (app->search_entry);
        else
            gtk_widget_grab_focus (app->tree);
    } else {
        proctable_freeze (app);

        gtk_widget_hide (app->end_process_button);
        gtk_widget_hide (app->search_button);
        gtk_widget_hide (app->process_menu_button);

        update_sensitivity (app);
    }

    if (strcmp (current_page, "resources") == 0) {
        load_graph_start (app->cpu_graph);
        load_graph_start (app->mem_graph);
        load_graph_start (app->net_graph);
    } else {
        load_graph_stop (app->cpu_graph);
        load_graph_stop (app->mem_graph);
        load_graph_stop (app->net_graph);
    }

    if (strcmp (current_page, "disks") == 0) {
        disks_update (app);
        disks_thaw (app);
    } else {
        disks_freeze (app);
    }
}

static void
cb_change_current_page (GtkStack *stack, GParamSpec *pspec, gpointer data)
{
    update_page_activities ((GsmApplication *)data);
}

static gboolean
cb_main_window_delete (GtkWidget *window, GdkEvent *event, gpointer data)
{
    GsmApplication *app = (GsmApplication *) data;

    app->shutdown ();

    return TRUE;
}

static gboolean
cb_main_window_state_changed (GtkWidget *window, GdkEventWindowState *event, gpointer data)
{
    GsmApplication *app = (GsmApplication *) data;
    gchar * current_page = g_settings_get_string (app->settings, GSM_SETTING_CURRENT_TAB);
    if (event->new_window_state & GDK_WINDOW_STATE_BELOW ||
        event->new_window_state & GDK_WINDOW_STATE_ICONIFIED ||
        event->new_window_state & GDK_WINDOW_STATE_WITHDRAWN)
    {
        if (strcmp (current_page, "processes") == 0) {
            proctable_freeze (app);
        } else if (strcmp (current_page, "resources") == 0) {
            load_graph_stop (app->cpu_graph);
            load_graph_stop (app->mem_graph);
            load_graph_stop (app->net_graph);
        } else if (strcmp (current_page, "disks") == 0) {
            disks_freeze (app);
        }
    } else  {
        if (strcmp (current_page, "processes") == 0) {
            proctable_update (app);
            proctable_thaw (app);
        } else if (strcmp (current_page, "resources") == 0) {
            load_graph_start (app->cpu_graph);
            load_graph_start (app->mem_graph);
            load_graph_start (app->net_graph);
        } else if (strcmp (current_page, "disks") == 0) {
            disks_update (app);
            disks_thaw (app);
        }
    }
    g_free (current_page);
    return FALSE;
}

void
create_main_window (GsmApplication *app)
{
    GtkWidget *main_window;
    GtkWidget *stack;
    GtkWidget *process_menu_button;
    GMenuModel *process_menu_model;

    const char* session;

    int width, height, xpos, ypos;

    GtkBuilder *builder = gtk_builder_new();
    gtk_builder_add_from_resource (builder, "/org/gnome/gnome-system-monitor/data/interface.ui", NULL);
    gtk_builder_add_from_resource (builder, "/org/gnome/gnome-system-monitor/data/menus.ui", NULL);

    main_window = GTK_WIDGET (gtk_builder_get_object (builder, "main_window"));
    gtk_window_set_application (GTK_WINDOW (main_window), app->gobj());
    gtk_widget_set_name (main_window, "gnome-system-monitor");
    app->main_window = main_window;

    session = g_getenv ("XDG_CURRENT_DESKTOP");
    if (session && !strstr (session, "GNOME")){
        GtkWidget *mainbox;
        GtkWidget *headerbar;

        mainbox = GTK_WIDGET (gtk_builder_get_object (builder, "main_box"));
        headerbar = GTK_WIDGET (gtk_builder_get_object (builder, "header_bar"));
        gtk_style_context_remove_class (gtk_widget_get_style_context (headerbar), "titlebar");
        gtk_window_set_titlebar (GTK_WINDOW (main_window), NULL);
        gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (headerbar), FALSE);
        gtk_box_pack_start (GTK_BOX(mainbox), headerbar, FALSE, FALSE, 0);
    }

    g_settings_get (app->settings, GSM_SETTING_WINDOW_STATE, "(iiii)",
                    &width, &height, &xpos, &ypos);
    width = CLAMP (width, 50, gdk_screen_width ());
    height = CLAMP (height, 50, gdk_screen_height ());

    gtk_window_set_default_size (GTK_WINDOW (main_window), width, height);
    gtk_window_move (GTK_WINDOW (main_window), xpos, ypos);
    if (g_settings_get_boolean (app->settings, GSM_SETTING_MAXIMIZED))
        gtk_window_maximize (GTK_WINDOW (main_window));

    app->process_menu_button = process_menu_button = GTK_WIDGET (gtk_builder_get_object (builder, "process_menu_button"));
    process_menu_model = G_MENU_MODEL (gtk_builder_get_object (builder, "process-window-menu"));
    gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (process_menu_button), process_menu_model);

    app->end_process_button = GTK_WIDGET (gtk_builder_get_object (builder, "end_process_button"));

    app->search_button = GTK_WIDGET (gtk_builder_get_object (builder, "search_button"));

    GActionEntry win_action_entries[] = {
        { "about", on_activate_about, NULL, NULL, NULL },
        { "search", on_activate_search, "b", "false", NULL },
        { "send-signal-stop", on_activate_send_signal, "i", NULL, NULL },
        { "send-signal-cont", on_activate_send_signal, "i", NULL, NULL },
        { "send-signal-end", on_activate_send_signal, "i", NULL, NULL },
        { "send-signal-kill", on_activate_send_signal, "i", NULL, NULL },
        { "priority", on_activate_priority, "i", "@i 0", change_priority_state },
        { "memory-maps", on_activate_memory_maps, NULL, NULL, NULL },
        { "open-files", on_activate_open_files, NULL, NULL, NULL },
        { "process-properties", on_activate_process_properties, NULL, NULL, NULL },
        { "refresh", on_activate_refresh, NULL, NULL, NULL },
        { "show-page", on_activate_radio, "s", "'resources'", change_show_page_state },
        { "show-whose-processes", on_activate_radio, "s", "'all'", change_show_processes_state },
        { "show-dependencies", on_activate_toggle, NULL, "false", change_show_dependencies_state }
    };

    g_action_map_add_action_entries (G_ACTION_MAP (main_window),
                                     win_action_entries,
                                     G_N_ELEMENTS (win_action_entries),
                                     app);

    GdkScreen* screen = gtk_widget_get_screen(main_window);
    GdkVisual* visual = gdk_screen_get_rgba_visual(screen);

    /* use visual, if available */
    if (visual)
        gtk_widget_set_visual(main_window, visual);

    /* create the main stack */
    app->stack = stack = GTK_WIDGET (gtk_builder_get_object (builder, "stack"));

    create_proc_view(app, builder);

    create_sys_view (app, builder);
    
    create_disk_view (app, builder);

    g_settings_bind (app->settings, GSM_SETTING_CURRENT_TAB, stack, "visible-child-name", G_SETTINGS_BIND_DEFAULT);

    g_signal_connect (G_OBJECT (stack), "notify::visible-child",
                      G_CALLBACK (cb_change_current_page), app);

    g_signal_connect (G_OBJECT (main_window), "delete_event",
                      G_CALLBACK (cb_main_window_delete),
                      app);
    g_signal_connect (G_OBJECT (main_window), "window-state-event",
                      G_CALLBACK (cb_main_window_state_changed),
                      app);

    GAction *action;
    action = g_action_map_lookup_action (G_ACTION_MAP (main_window),
                                         "show-dependencies");
    g_action_change_state (action,
                           g_settings_get_value (app->settings, GSM_SETTING_SHOW_DEPENDENCIES));


    action = g_action_map_lookup_action (G_ACTION_MAP (main_window),
                                         "show-whose-processes");
    g_action_change_state (action,
                           g_settings_get_value (app->settings, GSM_SETTING_SHOW_WHOSE_PROCESSES));

    gtk_widget_show (main_window);
    
    update_page_activities (app);

    g_object_unref (G_OBJECT (builder));
}

void
update_sensitivity(GsmApplication *app)
{
    const char * const selected_actions[] = { "send-signal-stop",
                                              "send-signal-cont",
                                              "send-signal-end",
                                              "send-signal-kill",
                                              "priority",
                                              "memory-maps",
                                              "open-files",
                                              "process-properties" };

    const char * const processes_actions[] = { "refresh",
                                               "search",
                                               "show-whose-processes",
                                               "show-dependencies" };

    size_t i;
    gboolean processes_sensitivity, selected_sensitivity;
    GAction *action;

    processes_sensitivity = (strcmp (gtk_stack_get_visible_child_name (GTK_STACK (app->stack)), "processes") == 0);
    selected_sensitivity = gtk_tree_selection_count_selected_rows (app->selection) > 0;

    for (i = 0; i != G_N_ELEMENTS (processes_actions); ++i) {
        action = g_action_map_lookup_action (G_ACTION_MAP (app->main_window),
                                             processes_actions[i]);
        g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
                                     processes_sensitivity);
    }

    for (i = 0; i != G_N_ELEMENTS (selected_actions); ++i) {
        action = g_action_map_lookup_action (G_ACTION_MAP (app->main_window),
                                             selected_actions[i]);
        g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
                                     processes_sensitivity & selected_sensitivity);
    }

    gtk_revealer_set_reveal_child (GTK_REVEALER (app->proc_actionbar_revealer),
                                   selected_sensitivity);
}
