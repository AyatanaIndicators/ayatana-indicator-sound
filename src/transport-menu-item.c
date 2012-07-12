/* transport-menu-item.c generated by valac 0.16.1, the Vala compiler
 * generated from transport-menu-item.vala, do not modify */

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

#include <glib.h>
#include <glib-object.h>
#include <libdbusmenu-glib/client.h>
#include <libdbusmenu-glib/dbusmenu-glib.h>
#include <libdbusmenu-glib/enum-types.h>
#include <libdbusmenu-glib/menuitem-proxy.h>
#include <libdbusmenu-glib/menuitem.h>
#include <libdbusmenu-glib/server.h>
#include <libdbusmenu-glib/types.h>
#include <common-defs.h>
#include <gee.h>
#include <stdlib.h>
#include <string.h>
#include <gio/gio.h>


#define TYPE_PLAYER_ITEM (player_item_get_type ())
#define PLAYER_ITEM(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_PLAYER_ITEM, PlayerItem))
#define PLAYER_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_PLAYER_ITEM, PlayerItemClass))
#define IS_PLAYER_ITEM(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_PLAYER_ITEM))
#define IS_PLAYER_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_PLAYER_ITEM))
#define PLAYER_ITEM_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_PLAYER_ITEM, PlayerItemClass))

typedef struct _PlayerItem PlayerItem;
typedef struct _PlayerItemClass PlayerItemClass;
typedef struct _PlayerItemPrivate PlayerItemPrivate;

#define TYPE_TRANSPORT_MENUITEM (transport_menuitem_get_type ())
#define TRANSPORT_MENUITEM(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_TRANSPORT_MENUITEM, TransportMenuitem))
#define TRANSPORT_MENUITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_TRANSPORT_MENUITEM, TransportMenuitemClass))
#define IS_TRANSPORT_MENUITEM(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_TRANSPORT_MENUITEM))
#define IS_TRANSPORT_MENUITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_TRANSPORT_MENUITEM))
#define TRANSPORT_MENUITEM_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_TRANSPORT_MENUITEM, TransportMenuitemClass))

typedef struct _TransportMenuitem TransportMenuitem;
typedef struct _TransportMenuitemClass TransportMenuitemClass;
typedef struct _TransportMenuitemPrivate TransportMenuitemPrivate;

#define TYPE_PLAYER_CONTROLLER (player_controller_get_type ())
#define PLAYER_CONTROLLER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_PLAYER_CONTROLLER, PlayerController))
#define PLAYER_CONTROLLER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_PLAYER_CONTROLLER, PlayerControllerClass))
#define IS_PLAYER_CONTROLLER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_PLAYER_CONTROLLER))
#define IS_PLAYER_CONTROLLER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_PLAYER_CONTROLLER))
#define PLAYER_CONTROLLER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_PLAYER_CONTROLLER, PlayerControllerClass))

typedef struct _PlayerController PlayerController;
typedef struct _PlayerControllerClass PlayerControllerClass;
typedef struct _PlayerControllerPrivate PlayerControllerPrivate;

#define TYPE_MPRIS2_CONTROLLER (mpris2_controller_get_type ())
#define MPRIS2_CONTROLLER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_MPRIS2_CONTROLLER, Mpris2Controller))
#define MPRIS2_CONTROLLER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_MPRIS2_CONTROLLER, Mpris2ControllerClass))
#define IS_MPRIS2_CONTROLLER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_MPRIS2_CONTROLLER))
#define IS_MPRIS2_CONTROLLER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_MPRIS2_CONTROLLER))
#define MPRIS2_CONTROLLER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_MPRIS2_CONTROLLER, Mpris2ControllerClass))

typedef struct _Mpris2Controller Mpris2Controller;
typedef struct _Mpris2ControllerClass Mpris2ControllerClass;
#define _g_variant_unref0(var) ((var == NULL) ? NULL : (var = (g_variant_unref (var), NULL)))
#define _g_object_unref0(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))

#define PLAYER_CONTROLLER_TYPE_STATE (player_controller_state_get_type ())

struct _PlayerItem {
	DbusmenuMenuitem parent_instance;
	PlayerItemPrivate * priv;
};

struct _PlayerItemClass {
	DbusmenuMenuitemClass parent_class;
};

struct _TransportMenuitem {
	PlayerItem parent_instance;
	TransportMenuitemPrivate * priv;
};

struct _TransportMenuitemClass {
	PlayerItemClass parent_class;
};

struct _TransportMenuitemPrivate {
	TransportAction cached_action;
};

struct _PlayerController {
	GObject parent_instance;
	PlayerControllerPrivate * priv;
	gint current_state;
	DbusmenuMenuitem* root_menu;
	GeeArrayList* custom_items;
	Mpris2Controller* mpris_bridge;
	gboolean* use_playlists;
};

struct _PlayerControllerClass {
	GObjectClass parent_class;
};

typedef enum  {
	PLAYER_CONTROLLER_STATE_OFFLINE,
	PLAYER_CONTROLLER_STATE_INSTANTIATING,
	PLAYER_CONTROLLER_STATE_READY,
	PLAYER_CONTROLLER_STATE_CONNECTED,
	PLAYER_CONTROLLER_STATE_DISCONNECTED
} PlayerControllerstate;


static gpointer transport_menuitem_parent_class = NULL;

GType player_item_get_type (void) G_GNUC_CONST;
GType transport_menuitem_get_type (void) G_GNUC_CONST;
#define TRANSPORT_MENUITEM_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_TRANSPORT_MENUITEM, TransportMenuitemPrivate))
enum  {
	TRANSPORT_MENUITEM_DUMMY_PROPERTY
};
GType player_controller_get_type (void) G_GNUC_CONST;
TransportMenuitem* transport_menuitem_new (PlayerController* parent);
TransportMenuitem* transport_menuitem_construct (GType object_type, PlayerController* parent);
void transport_menuitem_handle_cached_action (TransportMenuitem* self);
static gboolean transport_menuitem_send_cached_action (TransportMenuitem* self);
static gboolean _transport_menuitem_send_cached_action_gsource_func (gpointer self);
PlayerController* player_item_get_owner (PlayerItem* self);
GType mpris2_controller_get_type (void) G_GNUC_CONST;
void mpris2_controller_transport_update (Mpris2Controller* self, TransportAction command);
void transport_menuitem_change_play_state (TransportMenuitem* self, TransportState update);
static void transport_menuitem_real_handle_event (DbusmenuMenuitem* base, const gchar* name, GVariant* input_value, guint timestamp);
static gboolean transport_menuitem_get_running (TransportMenuitem* self);
void player_controller_instantiate (PlayerController* self);
GeeHashSet* transport_menuitem_attributes_format (void);
GType player_controller_state_get_type (void) G_GNUC_CONST;
static GObject * transport_menuitem_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
GAppInfo* player_controller_get_app_info (PlayerController* self);
static void transport_menuitem_finalize (GObject* obj);
static void _vala_transport_menuitem_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec);


TransportMenuitem* transport_menuitem_construct (GType object_type, PlayerController* parent) {
	TransportMenuitem * self = NULL;
	PlayerController* _tmp0_;
	g_return_val_if_fail (parent != NULL, NULL);
	_tmp0_ = parent;
	self = (TransportMenuitem*) g_object_new (object_type, "item-type", DBUSMENU_TRANSPORT_MENUITEM_TYPE, "owner", _tmp0_, NULL);
	return self;
}


TransportMenuitem* transport_menuitem_new (PlayerController* parent) {
	return transport_menuitem_construct (TYPE_TRANSPORT_MENUITEM, parent);
}


/**
  Please remove this timeout when the default player can handle mpris commands
  immediately once it raises its dbus interface
  **/
static gboolean _transport_menuitem_send_cached_action_gsource_func (gpointer self) {
	gboolean result;
	result = transport_menuitem_send_cached_action (self);
	return result;
}


void transport_menuitem_handle_cached_action (TransportMenuitem* self) {
	TransportAction _tmp0_;
	g_return_if_fail (self != NULL);
	_tmp0_ = self->priv->cached_action;
	if (_tmp0_ != TRANSPORT_ACTION_NO_ACTION) {
		g_timeout_add_seconds_full (G_PRIORITY_DEFAULT, (guint) 1, _transport_menuitem_send_cached_action_gsource_func, g_object_ref (self), g_object_unref);
	}
}


static gboolean transport_menuitem_send_cached_action (TransportMenuitem* self) {
	gboolean result = FALSE;
	PlayerController* _tmp0_;
	PlayerController* _tmp1_;
	Mpris2Controller* _tmp2_;
	TransportAction _tmp3_;
	g_return_val_if_fail (self != NULL, FALSE);
	_tmp0_ = player_item_get_owner ((PlayerItem*) self);
	_tmp1_ = _tmp0_;
	_tmp2_ = _tmp1_->mpris_bridge;
	_tmp3_ = self->priv->cached_action;
	mpris2_controller_transport_update (_tmp2_, _tmp3_);
	self->priv->cached_action = TRANSPORT_ACTION_NO_ACTION;
	result = FALSE;
	return result;
}


void transport_menuitem_change_play_state (TransportMenuitem* self, TransportState update) {
	TransportState _tmp0_;
	gint temp;
	g_return_if_fail (self != NULL);
	_tmp0_ = update;
	temp = (gint) _tmp0_;
	dbusmenu_menuitem_property_set_int ((DbusmenuMenuitem*) self, DBUSMENU_TRANSPORT_MENUITEM_PLAY_STATE, temp);
}


static gpointer _g_variant_ref0 (gpointer self) {
	return self ? g_variant_ref (self) : NULL;
}


static void transport_menuitem_real_handle_event (DbusmenuMenuitem* base, const gchar* name, GVariant* input_value, guint timestamp) {
	TransportMenuitem * self;
	GVariant* _tmp0_;
	GVariant* _tmp1_;
	GVariant* v;
	GVariant* _tmp2_;
	const GVariantType* _tmp3_;
	gboolean _tmp4_ = FALSE;
	GVariant* _tmp7_;
	gint32 _tmp8_ = 0;
	gint32 input;
	gboolean _tmp9_;
	gboolean _tmp10_;
	self = (TransportMenuitem*) base;
	g_return_if_fail (name != NULL);
	g_return_if_fail (input_value != NULL);
	_tmp0_ = input_value;
	_tmp1_ = _g_variant_ref0 (_tmp0_);
	v = _tmp1_;
	_tmp2_ = input_value;
	_tmp3_ = G_VARIANT_TYPE_VARIANT;
	_tmp4_ = g_variant_is_of_type (_tmp2_, _tmp3_);
	if (_tmp4_) {
		GVariant* _tmp5_;
		GVariant* _tmp6_ = NULL;
		_tmp5_ = input_value;
		_tmp6_ = g_variant_get_variant (_tmp5_);
		_g_variant_unref0 (v);
		v = _tmp6_;
	}
	_tmp7_ = v;
	_tmp8_ = g_variant_get_int32 (_tmp7_);
	input = _tmp8_;
	_tmp9_ = transport_menuitem_get_running (self);
	_tmp10_ = _tmp9_;
	if (_tmp10_ == TRUE) {
		PlayerController* _tmp11_;
		PlayerController* _tmp12_;
		Mpris2Controller* _tmp13_;
		gint32 _tmp14_;
		_tmp11_ = player_item_get_owner ((PlayerItem*) self);
		_tmp12_ = _tmp11_;
		_tmp13_ = _tmp12_->mpris_bridge;
		_tmp14_ = input;
		mpris2_controller_transport_update (_tmp13_, (TransportAction) _tmp14_);
	} else {
		gint32 _tmp15_;
		PlayerController* _tmp16_;
		PlayerController* _tmp17_;
		_tmp15_ = input;
		self->priv->cached_action = (TransportAction) _tmp15_;
		_tmp16_ = player_item_get_owner ((PlayerItem*) self);
		_tmp17_ = _tmp16_;
		player_controller_instantiate (_tmp17_);
		dbusmenu_menuitem_property_set_int ((DbusmenuMenuitem*) self, DBUSMENU_TRANSPORT_MENUITEM_PLAY_STATE, (gint) TRANSPORT_STATE_LAUNCHING);
	}
	_g_variant_unref0 (v);
}


GeeHashSet* transport_menuitem_attributes_format (void) {
	GeeHashSet* result = NULL;
	GeeHashSet* _tmp0_;
	GeeHashSet* attrs;
	_tmp0_ = gee_hash_set_new (G_TYPE_STRING, (GBoxedCopyFunc) g_strdup, g_free, NULL, NULL);
	attrs = _tmp0_;
	gee_abstract_collection_add ((GeeAbstractCollection*) attrs, DBUSMENU_TRANSPORT_MENUITEM_PLAY_STATE);
	result = attrs;
	return result;
}


static gboolean transport_menuitem_get_running (TransportMenuitem* self) {
	gboolean result;
	PlayerController* _tmp0_;
	PlayerController* _tmp1_;
	gint _tmp2_;
	g_return_val_if_fail (self != NULL, FALSE);
	_tmp0_ = player_item_get_owner ((PlayerItem*) self);
	_tmp1_ = _tmp0_;
	_tmp2_ = _tmp1_->current_state;
	result = _tmp2_ == ((gint) PLAYER_CONTROLLER_STATE_CONNECTED);
	return result;
}


static GObject * transport_menuitem_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties) {
	GObject * obj;
	GObjectClass * parent_class;
	TransportMenuitem * self;
	PlayerController* _tmp0_;
	PlayerController* _tmp1_;
	GAppInfo* _tmp2_;
	GAppInfo* _tmp3_;
	const gchar* _tmp4_ = NULL;
	parent_class = G_OBJECT_CLASS (transport_menuitem_parent_class);
	obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	self = TRANSPORT_MENUITEM (obj);
	dbusmenu_menuitem_property_set_int ((DbusmenuMenuitem*) self, DBUSMENU_TRANSPORT_MENUITEM_PLAY_STATE, (gint) TRANSPORT_STATE_PAUSED);
	_tmp0_ = player_item_get_owner ((PlayerItem*) self);
	_tmp1_ = _tmp0_;
	_tmp2_ = player_controller_get_app_info (_tmp1_);
	_tmp3_ = _tmp2_;
	_tmp4_ = g_app_info_get_name (_tmp3_);
	dbusmenu_menuitem_property_set ((DbusmenuMenuitem*) self, DBUSMENU_MENUITEM_PROP_LABEL, _tmp4_);
	self->priv->cached_action = TRANSPORT_ACTION_NO_ACTION;
	return obj;
}


static void transport_menuitem_class_init (TransportMenuitemClass * klass) {
	transport_menuitem_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (TransportMenuitemPrivate));
	DBUSMENU_MENUITEM_CLASS (klass)->handle_event = transport_menuitem_real_handle_event;
	G_OBJECT_CLASS (klass)->get_property = _vala_transport_menuitem_get_property;
	G_OBJECT_CLASS (klass)->constructor = transport_menuitem_constructor;
	G_OBJECT_CLASS (klass)->finalize = transport_menuitem_finalize;
}


static void transport_menuitem_instance_init (TransportMenuitem * self) {
	self->priv = TRANSPORT_MENUITEM_GET_PRIVATE (self);
}


static void transport_menuitem_finalize (GObject* obj) {
	TransportMenuitem * self;
	self = TRANSPORT_MENUITEM (obj);
	G_OBJECT_CLASS (transport_menuitem_parent_class)->finalize (obj);
}


GType transport_menuitem_get_type (void) {
	static volatile gsize transport_menuitem_type_id__volatile = 0;
	if (g_once_init_enter (&transport_menuitem_type_id__volatile)) {
		static const GTypeInfo g_define_type_info = { sizeof (TransportMenuitemClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) transport_menuitem_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (TransportMenuitem), 0, (GInstanceInitFunc) transport_menuitem_instance_init, NULL };
		GType transport_menuitem_type_id;
		transport_menuitem_type_id = g_type_register_static (TYPE_PLAYER_ITEM, "TransportMenuitem", &g_define_type_info, 0);
		g_once_init_leave (&transport_menuitem_type_id__volatile, transport_menuitem_type_id);
	}
	return transport_menuitem_type_id__volatile;
}


static void _vala_transport_menuitem_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec) {
	TransportMenuitem * self;
	self = TRANSPORT_MENUITEM (object);
	switch (property_id) {
		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}



