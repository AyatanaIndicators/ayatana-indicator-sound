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
#ifndef __PLAY_BUTTON_H__
#define __PLAY_BUTTON_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PLAY_BUTTON_TYPE            (play_button_get_type ())
#define PLAY_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PLAY_BUTTON_TYPE, PlayButton))
#define PLAY_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PLAY_BUTTON_TYPE, PlayButtonClass))
#define IS_PLAY_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PLAY_BUTTON_TYPE))
#define IS_PLAY_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PLAY_BUTTON_TYPE))
#define PLAY_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PLAY_BUTTON_TYPE, PlayButtonClass))

typedef struct _PlayButton      PlayButton;
typedef struct _PlayButtonClass PlayButtonClass;

typedef enum {
	TRANSPORT_PREVIOUS,
	TRANSPORT_PLAY_PAUSE,
	TRANSPORT_NEXT,
	TRANSPORT_NADA
}PlayButtonEvent;

typedef enum {
	PLAY,
	PAUSE	
}PlayButtonState;

struct _PlayButtonClass {
	  GtkEventBoxClass parent_class;
};

struct _PlayButton {
	  GtkEventBox parent;
};

GType play_button_get_type (void);
void play_button_set_style(GtkWidget* button, GtkStyle* style);
PlayButtonEvent determine_button_event(GtkWidget* button, GdkEventButton* event);
void play_button_react_to_button_press(GtkWidget* button, PlayButtonEvent command);
void play_button_react_to_button_release(GtkWidget* button,  PlayButtonEvent command);
void play_button_toggle_play_pause(GtkWidget* button, PlayButtonState update);

GtkWidget* play_button_new();

G_END_DECLS

#endif

