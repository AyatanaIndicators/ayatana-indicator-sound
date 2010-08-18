/*
Copyright 2010 Canonical Ltd.

Authors:
    Conor Curran <conor.curran@canonical.com>

This program is free software: you can redistribute it and/or modify it 
under the terms of the GNU General Public License version 3, as published 
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but 
WITHOUT ANY WARRANTY; without even the implied warranties of 
MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR 
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along 
with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n.h>
#include "scrub-widget.h"
#include "common-defs.h"
#include <libido/idoscalemenuitem.h>
#include <libido/idotimeline.h>

typedef struct _ScrubWidgetPrivate ScrubWidgetPrivate;

struct _ScrubWidgetPrivate
{
	DbusmenuMenuitem* twin_item;	
	GtkWidget* ido_scrub_bar;
	IdoTimeline* time_line;
	gboolean scrubbing;
};

#define SCRUB_WIDGET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), SCRUB_WIDGET_TYPE, ScrubWidgetPrivate))

/* Prototypes */
static void scrub_widget_class_init (ScrubWidgetClass *klass);
static void scrub_widget_init       (ScrubWidget *self);
static void scrub_widget_dispose    (GObject *object);
static void scrub_widget_finalize   (GObject *object);
static void scrub_widget_property_update( DbusmenuMenuitem* item, gchar* property, 
                                       	  GValue* value, gpointer userdata);
static void scrub_widget_set_twin_item(	ScrubWidget* self,
                           							DbusmenuMenuitem* twin_item);
static gchar* scrub_widget_format_time(gint time);
static void scrub_widget_set_ido_position(ScrubWidget* self,
                                          gint position,
                                          gint duration);
static gboolean scrub_widget_change_value_cb (GtkRange     *range,
                                 							GtkScrollType scroll,
                                 							gdouble       value,
                                 							gpointer      user_data);

static void scrub_widget_timeline_frame_cb(IdoTimeline *timeline,
			      												 				gdouble      progress,
                                     				gpointer 		userdata);
static void scrub_widget_timeline_started_cb(IdoTimeline *timeline,
                                     				gpointer 		userdata);
static void scrub_widget_timeline_finished_cb(IdoTimeline *timeline,
                                     				gpointer 		userdata);
static gdouble scrub_widget_calculate_progress(ScrubWidget* widget);
static void scrub_widget_check_play_state(ScrubWidget* self);
static void scrub_widget_slider_grabbed(GtkWidget *widget, gpointer user_data);
static void scrub_widget_slider_released(GtkWidget *widget, gpointer user_data);



G_DEFINE_TYPE (ScrubWidget, scrub_widget, G_TYPE_OBJECT);

static void
scrub_widget_class_init (ScrubWidgetClass *klass)
{
	GObjectClass 			*gobject_class = G_OBJECT_CLASS (klass);
	
	g_type_class_add_private (klass, sizeof (ScrubWidgetPrivate));

	gobject_class->dispose = scrub_widget_dispose;
	gobject_class->finalize = scrub_widget_finalize;
}

static void
scrub_widget_init (ScrubWidget *self)
{
	g_debug("ScrubWidget::scrub_widget_init");
	ScrubWidgetPrivate * priv = SCRUB_WIDGET_GET_PRIVATE(self);

  priv->ido_scrub_bar = ido_scale_menu_item_new_with_range ("Scrub", IDO_RANGE_STYLE_SMALL,  0, 0, 100, 1);
	priv->time_line = ido_timeline_new(0);
	
	ido_scale_menu_item_set_style (IDO_SCALE_MENU_ITEM(priv->ido_scrub_bar), IDO_SCALE_MENU_ITEM_STYLE_LABEL);		

	g_object_set(priv->ido_scrub_bar, "reverse-scroll-events", TRUE, NULL);
	priv->scrubbing = FALSE;

	gtk_widget_set_size_request(GTK_WIDGET(priv->ido_scrub_bar), 100, 25); 

  // register slider changes listening on the range
  GtkWidget* scrub_widget = ido_scale_menu_item_get_scale((IdoScaleMenuItem*)priv->ido_scrub_bar);	
  g_signal_connect(scrub_widget, "change-value", G_CALLBACK(scrub_widget_change_value_cb), self);	
	g_signal_connect(priv->time_line, "frame", G_CALLBACK(scrub_widget_timeline_frame_cb), self);	
	g_signal_connect(priv->time_line, "started", G_CALLBACK(scrub_widget_timeline_started_cb), self);		
	g_signal_connect(priv->time_line, "finished", G_CALLBACK(scrub_widget_timeline_finished_cb), self);		
  g_signal_connect(priv->ido_scrub_bar, "slider-grabbed", G_CALLBACK(scrub_widget_slider_grabbed), self);
  g_signal_connect(priv->ido_scrub_bar, "slider-released", G_CALLBACK(scrub_widget_slider_released), self);
	
}

static void
scrub_widget_dispose (GObject *object)
{
	G_OBJECT_CLASS (scrub_widget_parent_class)->dispose (object);
}

static void
scrub_widget_finalize (GObject *object)
{
	G_OBJECT_CLASS (scrub_widget_parent_class)->finalize (object);
}

static void 
scrub_widget_property_update(DbusmenuMenuitem* item, gchar* property, 
                             GValue* value, gpointer userdata)
{
	g_debug("scrub-widget::property_update"); 
	
	g_return_if_fail (IS_SCRUB_WIDGET (userdata));	
	ScrubWidget* mitem = SCRUB_WIDGET(userdata);
	ScrubWidgetPrivate * priv = SCRUB_WIDGET_GET_PRIVATE(mitem);
	
	if(g_ascii_strcasecmp(DBUSMENU_SCRUB_MENUITEM_DURATION, property) == 0){
		g_debug("scrub-widget::update length = %i", g_value_get_int(value)); 

    ido_scale_menu_item_set_secondary_label(IDO_SCALE_MENU_ITEM(priv->ido_scrub_bar),
		                                      	scrub_widget_format_time(g_value_get_int(value))); 			
		
		ido_timeline_set_duration(priv->time_line, g_value_get_int(value) * 1000); 	
		ido_timeline_rewind(priv->time_line);
		scrub_widget_check_play_state(mitem);
		//g_debug("timeline is running: %i", (gint)ido_timeline_is_running(priv->time_line));
		//g_debug("timeline duration = %i", ido_timeline_get_duration(priv->time_line));

		scrub_widget_set_ido_position(mitem, 
	                              dbusmenu_menuitem_property_get_int(priv->twin_item, DBUSMENU_SCRUB_MENUITEM_POSITION)/1000,
																dbusmenu_menuitem_property_get_int(priv->twin_item, DBUSMENU_SCRUB_MENUITEM_DURATION));	                                		
	}
	else if(g_ascii_strcasecmp(DBUSMENU_SCRUB_MENUITEM_POSITION, property) == 0){
		g_debug("scrub-widget::update position = %i", g_value_get_int(value)); 
		ido_timeline_pause(priv->time_line);
		ido_scale_menu_item_set_primary_label(IDO_SCALE_MENU_ITEM(priv->ido_scrub_bar),
		                                      scrub_widget_format_time(g_value_get_int(value)/1000)); 					

		g_debug("scrub-widget::update progress = %f", scrub_widget_calculate_progress(mitem)*100); 
																					
		ido_timeline_set_progress(priv->time_line, scrub_widget_calculate_progress(mitem));
		scrub_widget_set_ido_position(mitem, g_value_get_int(value)/1000,
																dbusmenu_menuitem_property_get_int(priv->twin_item, DBUSMENU_SCRUB_MENUITEM_DURATION));

		scrub_widget_check_play_state(mitem);
	}
	else if(g_ascii_strcasecmp(DBUSMENU_SCRUB_MENUITEM_PLAY_STATE, property) == 0){
		scrub_widget_check_play_state(mitem);
	}
}

static void 
scrub_widget_check_play_state(ScrubWidget* self)
{
	ScrubWidgetPrivate * priv = SCRUB_WIDGET_GET_PRIVATE(self);
	gint play_state = dbusmenu_menuitem_property_get_int(priv->twin_item,
	                                                     DBUSMENU_SCRUB_MENUITEM_PLAY_STATE);
	g_debug("play-state = %i", play_state);
	if(play_state == 0){
		g_debug("START TIMELINE");
		ido_timeline_start(priv->time_line);
	}
	else{
		g_debug("PAUSE TIMELINE");			
		ido_timeline_pause(priv->time_line);
	}		
}

static void
scrub_widget_set_twin_item(ScrubWidget* self,
                           DbusmenuMenuitem* twin_item)
{
	ScrubWidgetPrivate * priv = SCRUB_WIDGET_GET_PRIVATE(self);
	priv->twin_item = twin_item;

	g_signal_connect(G_OBJECT(twin_item), "property-changed", 
	                 G_CALLBACK(scrub_widget_property_update), self);

	gchar* left_text = scrub_widget_format_time(dbusmenu_menuitem_property_get_int(priv->twin_item,
	                                                                               DBUSMENU_SCRUB_MENUITEM_POSITION)/1000); 	
	gchar* right_text = scrub_widget_format_time(dbusmenu_menuitem_property_get_int(priv->twin_item, 
	                                                                                DBUSMENU_SCRUB_MENUITEM_DURATION)); 	
	scrub_widget_set_ido_position(self, 
	                              dbusmenu_menuitem_property_get_int(priv->twin_item, DBUSMENU_SCRUB_MENUITEM_POSITION)/1000,
																dbusmenu_menuitem_property_get_int(priv->twin_item, DBUSMENU_SCRUB_MENUITEM_DURATION));	                                
	                                
	ido_scale_menu_item_set_primary_label(IDO_SCALE_MENU_ITEM(priv->ido_scrub_bar), left_text); 	
	ido_scale_menu_item_set_secondary_label(IDO_SCALE_MENU_ITEM(priv->ido_scrub_bar), right_text);
	g_free(left_text);
	g_free(right_text);
}

static gboolean
scrub_widget_change_value_cb (GtkRange     *range,
                 							GtkScrollType scroll,
                 							gdouble       new_value,
                 							gpointer      user_data)
{
	g_return_val_if_fail (IS_SCRUB_WIDGET (user_data), FALSE);
	ScrubWidget* mitem = SCRUB_WIDGET(user_data);
	ScrubWidgetPrivate * priv = SCRUB_WIDGET_GET_PRIVATE(mitem);

  // Don't bother when the slider is grabbed  
  if(priv->scrubbing == TRUE)
    return FALSE;

	GValue value = {0};
  g_value_init(&value, G_TYPE_DOUBLE);
	gdouble clamped = CLAMP(new_value, 0, 100);
  g_value_set_double(&value, clamped);
  dbusmenu_menuitem_handle_event (priv->twin_item, "scrubbing", &value, 0);
	return TRUE;
}

GtkWidget*
scrub_widget_get_ido_bar(ScrubWidget* self)
{
	ScrubWidgetPrivate * priv = SCRUB_WIDGET_GET_PRIVATE(self);
	return priv->ido_scrub_bar;	
}

static gchar*
scrub_widget_format_time(gint time)
{
// Assuming its in seconds for now ...
	gchar* prefix = "-";
	gchar* seconds_prefix = "-";

	if(time != DBUSMENU_PROPERTY_EMPTY){
		gint minutes = time/60;
		gint seconds = time % 60;
		prefix="0";
		seconds_prefix="0";
		if(minutes > 9)
			prefix="";
		if(seconds > 9)
			seconds_prefix="";
		return g_strdup_printf("%s%i:%s%i", prefix, minutes, seconds_prefix, seconds);	
		
	}	
	else{
		return g_strdup_printf("%s-:%s-", prefix, seconds_prefix);	
	}
}
 
static void
scrub_widget_set_ido_position(ScrubWidget* self,
                              gint position,
                              gint duration)
{
	ScrubWidgetPrivate * priv = SCRUB_WIDGET_GET_PRIVATE(self);
	gdouble ido_position = position/(gdouble)duration	* 100.0;
	g_debug("scrub_widget_set_ido_position - pos: %i, duration: %i, ido_pos: %f", position, duration, ido_position);
  GtkWidget *slider = ido_scale_menu_item_get_scale((IdoScaleMenuItem*)priv->ido_scrub_bar);
  GtkRange *range = (GtkRange*)slider;
	if(duration == 0)
		ido_position = 0.0;
 	gtk_range_set_value(range, ido_position);
}

static gdouble 
scrub_widget_calculate_progress(ScrubWidget* widget)
{
	ScrubWidgetPrivate * priv = SCRUB_WIDGET_GET_PRIVATE(widget);
	gint position = dbusmenu_menuitem_property_get_int(priv->twin_item,
	                                                   DBUSMENU_SCRUB_MENUITEM_POSITION)/1000;
	gint duration = dbusmenu_menuitem_property_get_int(priv->twin_item,
	                                                   DBUSMENU_SCRUB_MENUITEM_DURATION);
	gdouble ido_position = position/(gdouble)duration;
	g_debug("scrub_widget_calculate_progress %f", ido_position);
	        
	return ido_position;
}


static void
scrub_widget_timeline_frame_cb(	IdoTimeline *timeline,
			      						 				gdouble     	progress,
                         				gpointer 		user_data)
{
	
	//g_debug("Timeline CB : %f", progress);
	g_return_if_fail (IS_SCRUB_WIDGET (user_data));
	ScrubWidget* mitem = SCRUB_WIDGET(user_data);
	ScrubWidgetPrivate * priv = SCRUB_WIDGET_GET_PRIVATE(mitem);
	if(priv->scrubbing == TRUE)
	{
		g_debug("don't update the slider or timeline, slider is being scrubbed");
		return;
	}
	gint position = progress * dbusmenu_menuitem_property_get_int(priv->twin_item, 
	                                   														DBUSMENU_SCRUB_MENUITEM_DURATION);	
	gchar* left_text = scrub_widget_format_time(position); 	
	ido_scale_menu_item_set_primary_label(IDO_SCALE_MENU_ITEM(priv->ido_scrub_bar), left_text); 	
  GtkWidget *slider = ido_scale_menu_item_get_scale((IdoScaleMenuItem*)priv->ido_scrub_bar);
  GtkRange *range = (GtkRange*)slider;
  gtk_range_set_value(range, progress * 100);
	/*g_debug("position in seconds %i and in words %s", position, left_text);
	g_debug("timeline is running: %i", (gint)ido_timeline_is_running(priv->time_line));
	g_debug("timeline duration = %i", ido_timeline_get_duration(priv->time_line));	
	*/
	//g_debug("timeline-update - progress = %f", progress);
	g_free(left_text);
}


static void
scrub_widget_slider_released(GtkWidget *widget, gpointer user_data)
{
	ScrubWidget* mitem = SCRUB_WIDGET(user_data);
	ScrubWidgetPrivate * priv = SCRUB_WIDGET_GET_PRIVATE(mitem);
	priv->scrubbing = FALSE;
	GtkWidget *slider = ido_scale_menu_item_get_scale((IdoScaleMenuItem*)priv->ido_scrub_bar);
  gdouble new_value = gtk_range_get_value(GTK_RANGE(slider));
	g_debug("okay set the scrub position with %f", new_value);	
	GValue value = {0};
  g_value_init(&value, G_TYPE_DOUBLE);
	gdouble clamped = CLAMP(new_value, 0, 100);
  g_value_set_double(&value, clamped);
  dbusmenu_menuitem_handle_event (priv->twin_item, "scrubbing", &value, 0);
}

static void
scrub_widget_slider_grabbed(GtkWidget *widget, gpointer user_data)
{
	ScrubWidget* mitem = SCRUB_WIDGET(user_data);
	ScrubWidgetPrivate * priv = SCRUB_WIDGET_GET_PRIVATE(mitem);
	priv->scrubbing = TRUE;		
}

static void
scrub_widget_timeline_started_cb(	IdoTimeline *timeline,
                         			 		gpointer 		user_data)
{
	g_debug("Timeline Started!");	
}
                           
static void
scrub_widget_timeline_finished_cb(IdoTimeline *timeline,
                         			 		gpointer 		user_data)
{
	g_debug("Timeline Finished!");	
}

/**
 * scrub_widget_new:
 * @returns: a new #ScrubWidget.
 **/
GtkWidget* 
scrub_widget_new(DbusmenuMenuitem *item)
{
	GtkWidget* widget = g_object_new(SCRUB_WIDGET_TYPE, NULL);
	scrub_widget_set_twin_item((ScrubWidget*)widget, item);
	return widget;
}


