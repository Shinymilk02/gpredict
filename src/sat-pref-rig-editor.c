/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
  Gpredict: Real-time satellite tracking and orbit prediction program

  Copyright (C)  2001-2007  Alexandru Csete, OZ9AEC.

  Authors: Alexandru Csete <oz9aec@gmail.com>

  Comments, questions and bugreports should be submitted via
  http://sourceforge.net/projects/groundstation/
  More details can be found at the project home page:

  http://groundstation.sourceforge.net/
 
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, visit http://www.fsf.org/
*/

/** \brief Edit radio configuration.
 *
 */

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <math.h>
#ifdef HAVE_CONFIG_H
#  include <build-config.h>
#endif
#include "gpredict-utils.h"
#include "sat-cfg.h"
#include "sat-log.h"
#include "radio-conf.h"
#include "sat-pref-rig-editor.h"



extern GtkWidget *window; /* dialog window defined in sat-pref.c */



/* private widgets */
static GtkWidget *dialog;   /* dialog window */
static GtkWidget *name;     /* config name */
static GtkWidget *host;     /* host */


static GtkWidget    *create_editor_widgets (radio_conf_t *conf);
static void          update_widgets        (radio_conf_t *conf);
static void          clear_widgets         (void);
static gboolean      apply_changes         (radio_conf_t *conf);
static void          name_changed          (GtkWidget *widget, gpointer data);


/** \brief Add or edit a radio configuration.
 * \param conf Pointer to a radio configuration.
 *
 * Of conf->name is not NULL the widgets will be populated with the data.
 */
void
sat_pref_rig_editor_run (radio_conf_t *conf)
{
	gint       response;
	gboolean   finished = FALSE;


	/* crate dialog and add contents */
	dialog = gtk_dialog_new_with_buttons (_("Edit radio configuration"),
										  GTK_WINDOW (window),
										  GTK_DIALOG_MODAL |
										  GTK_DIALOG_DESTROY_WITH_PARENT,
										  GTK_STOCK_CLEAR,
										  GTK_RESPONSE_REJECT,
										  GTK_STOCK_CANCEL,
										  GTK_RESPONSE_CANCEL,
										  GTK_STOCK_OK,
										  GTK_RESPONSE_OK,
										  NULL);

	/* disable OK button to begin with */
	gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
									   GTK_RESPONSE_OK,
									   FALSE);

	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox),
					   create_editor_widgets (conf));

	/* this hacky-thing is to keep the dialog running in case the
	   CLEAR button is plressed. OK and CANCEL will exit the loop
	*/
	while (!finished) {

		response = gtk_dialog_run (GTK_DIALOG (dialog));

		switch (response) {

			/* OK */
		case GTK_RESPONSE_OK:
			if (apply_changes (conf)) {
				finished = TRUE;
			}
			else {
				finished = FALSE;
			}
			break;

			/* CLEAR */
		case GTK_RESPONSE_REJECT:
			clear_widgets ();
			break;

			/* Everything else is considered CANCEL */
		default:
			finished = TRUE;
			break;
		}
	}

	gtk_widget_destroy (dialog);
}


/** \brief Create and initialise widgets */
static GtkWidget *
create_editor_widgets (radio_conf_t *conf)
{
	GtkWidget    *table;
	GtkWidget    *label;



	table = gtk_table_new (2, 4, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (table), 5);
	gtk_table_set_col_spacings (GTK_TABLE (table), 5);
	gtk_table_set_row_spacings (GTK_TABLE (table), 5);

	/* Config name */
	label = gtk_label_new (_("Name"));
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
	
	name = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (name), 25);
    gtk_widget_set_tooltip_text (name,
                                 _("Enter a short name for this configuration, e.g. IC910-1.\n"\
                                   "Allowed charachters: 0..9, a..z, A..Z, - and _"));
	gtk_table_attach_defaults (GTK_TABLE (table), name, 1, 4, 0, 1);

	/* attach changed signal so that we can enable OK button when
	   a proper name has been entered
	*/
	g_signal_connect (name, "changed", G_CALLBACK (name_changed), NULL);

	/* Host */
	label = gtk_label_new (_("Host"));
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);
	
    host = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (host), 50);
    gtk_widget_set_tooltip_text (host,
                                 _("Enter the host and port where rigctld is running, e.g. 192.168.1.100:15123"));
    gtk_table_attach_defaults (GTK_TABLE (table), host, 1, 4, 1, 2);

    
	if (conf->name != NULL)
		update_widgets (conf);

	gtk_widget_show_all (table);

	return table;
}


/** \brief Update widgets from the currently selected row in the treeview
 */
static void
update_widgets (radio_conf_t *conf)
{
    
    /* configuration name */
    gtk_entry_set_text (GTK_ENTRY (name), conf->name);
    
    /* host name and port */
    if (conf->host)
        gtk_entry_set_text (GTK_ENTRY (host), conf->host);

}



/** \brief Clear the contents of all widgets.
 *
 * This function is usually called when the user clicks on the CLEAR button
 *
 */
static void
clear_widgets () 
{
    gtk_entry_set_text (GTK_ENTRY (name), "");
    gtk_entry_set_text (GTK_ENTRY (host), "");

}


/** \brief Apply changes.
 *  \return TRUE if things are ok, FALSE otherwise.
 *
 * This function is usually called when the user clicks the OK button.
 */
static gboolean
apply_changes         (radio_conf_t *conf)
{

    
    /* name */
    if (conf->name)
        g_free (conf->name);

    conf->name = g_strdup (gtk_entry_get_text (GTK_ENTRY (name)));
    
    /* host */
    if (conf->host)
        g_free (conf->host);

    conf->host = g_strdup (gtk_entry_get_text (GTK_ENTRY (host)));


	return TRUE;
}



/** \brief Manage name changes.
 *
 * This function is called when the contents of the name entry changes.
 * The primary purpose of this function is to check whether the char length
 * of the name is greater than zero, if yes enable the OK button of the dialog.
 */
static void
name_changed          (GtkWidget *widget, gpointer data)
{
	const gchar *text;
	gchar *entry, *end, *j;
	gint len, pos;


	/* step 1: ensure that only valid characters are entered
	   (stolen from xlog, tnx pg4i)
	*/
	entry = gtk_editable_get_chars (GTK_EDITABLE (widget), 0, -1);
	if ((len = g_utf8_strlen (entry, -1)) > 0)
		{
			end = entry + g_utf8_strlen (entry, -1);
			for (j = entry; j < end; ++j)
				{
					switch (*j)
						{
						case '0' ... '9':
						case 'a' ... 'z':
						case 'A' ... 'Z':
						case '-':
						case '_':
							break;
						default:
							gdk_beep ();
							pos = gtk_editable_get_position (GTK_EDITABLE (widget));
							gtk_editable_delete_text (GTK_EDITABLE (widget),
													  pos, pos+1);
							break;
						}
				}
		}


	/* step 2: if name seems all right, enable OK button */
	text = gtk_entry_get_text (GTK_ENTRY (widget));

	if (g_utf8_strlen (text, -1) > 0) {
		gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
										   GTK_RESPONSE_OK,
										   TRUE);
	}
	else {
		gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
										   GTK_RESPONSE_OK,
										   FALSE);
	}
}


