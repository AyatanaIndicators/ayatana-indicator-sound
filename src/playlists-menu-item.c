/* playlists-menu-item.c generated by valac 0.14.0, the Vala compiler
 * generated from playlists-menu-item.vala, do not modify */

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
#include <gee.h>
#include <common-defs.h>
#include <stdlib.h>
#include <string.h>
#include <glib/gi18n-lib.h>


#define TYPE_PLAYER_ITEM (player_item_get_type ())
#define PLAYER_ITEM(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_PLAYER_ITEM, PlayerItem))
#define PLAYER_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_PLAYER_ITEM, PlayerItemClass))
#define IS_PLAYER_ITEM(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_PLAYER_ITEM))
#define IS_PLAYER_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_PLAYER_ITEM))
#define PLAYER_ITEM_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_PLAYER_ITEM, PlayerItemClass))

typedef struct _PlayerItem PlayerItem;
typedef struct _PlayerItemClass PlayerItemClass;
typedef struct _PlayerItemPrivate PlayerItemPrivate;

#define TYPE_PLAYLISTS_MENUITEM (playlists_menuitem_get_type ())
#define PLAYLISTS_MENUITEM(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_PLAYLISTS_MENUITEM, PlaylistsMenuitem))
#define PLAYLISTS_MENUITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_PLAYLISTS_MENUITEM, PlaylistsMenuitemClass))
#define IS_PLAYLISTS_MENUITEM(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_PLAYLISTS_MENUITEM))
#define IS_PLAYLISTS_MENUITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_PLAYLISTS_MENUITEM))
#define PLAYLISTS_MENUITEM_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_PLAYLISTS_MENUITEM, PlaylistsMenuitemClass))

typedef struct _PlaylistsMenuitem PlaylistsMenuitem;
typedef struct _PlaylistsMenuitemClass PlaylistsMenuitemClass;
typedef struct _PlaylistsMenuitemPrivate PlaylistsMenuitemPrivate;
#define _g_object_unref0(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))

#define TYPE_PLAYER_CONTROLLER (player_controller_get_type ())
#define PLAYER_CONTROLLER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_PLAYER_CONTROLLER, PlayerController))
#define PLAYER_CONTROLLER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_PLAYER_CONTROLLER, PlayerControllerClass))
#define IS_PLAYER_CONTROLLER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_PLAYER_CONTROLLER))
#define IS_PLAYER_CONTROLLER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_PLAYER_CONTROLLER))
#define PLAYER_CONTROLLER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_PLAYER_CONTROLLER, PlayerControllerClass))

typedef struct _PlayerController PlayerController;
typedef struct _PlayerControllerClass PlayerControllerClass;

#define TYPE_PLAYLIST_DETAILS (playlist_details_get_type ())
typedef struct _PlaylistDetails PlaylistDetails;
typedef struct _Block1Data Block1Data;
#define _g_free0(var) (var = (g_free (var), NULL))
typedef struct _PlayerControllerPrivate PlayerControllerPrivate;

#define TYPE_MPRIS2_CONTROLLER (mpris2_controller_get_type ())
#define MPRIS2_CONTROLLER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_MPRIS2_CONTROLLER, Mpris2Controller))
#define MPRIS2_CONTROLLER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_MPRIS2_CONTROLLER, Mpris2ControllerClass))
#define IS_MPRIS2_CONTROLLER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_MPRIS2_CONTROLLER))
#define IS_MPRIS2_CONTROLLER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_MPRIS2_CONTROLLER))
#define MPRIS2_CONTROLLER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_MPRIS2_CONTROLLER, Mpris2ControllerClass))

typedef struct _Mpris2Controller Mpris2Controller;
typedef struct _Mpris2ControllerClass Mpris2ControllerClass;

struct _PlayerItem {
	DbusmenuMenuitem parent_instance;
	PlayerItemPrivate * priv;
};

struct _PlayerItemClass {
	DbusmenuMenuitemClass parent_class;
};

struct _PlaylistsMenuitem {
	PlayerItem parent_instance;
	PlaylistsMenuitemPrivate * priv;
	DbusmenuMenuitem* root_item;
};

struct _PlaylistsMenuitemClass {
	PlayerItemClass parent_class;
};

struct _PlaylistsMenuitemPrivate {
	GeeHashMap* current_playlists;
};

struct _PlaylistDetails {
	char* path;
	gchar* name;
	gchar* icon_path;
};

struct _Block1Data {
	int _ref_count_;
	PlaylistsMenuitem * self;
	DbusmenuMenuitem* menuitem;
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


static gpointer playlists_menuitem_parent_class = NULL;

GType player_item_get_type (void) G_GNUC_CONST;
GType playlists_menuitem_get_type (void) G_GNUC_CONST;
#define PLAYLISTS_MENUITEM_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_PLAYLISTS_MENUITEM, PlaylistsMenuitemPrivate))
enum  {
	PLAYLISTS_MENUITEM_DUMMY_PROPERTY
};
GType player_controller_get_type (void) G_GNUC_CONST;
PlaylistsMenuitem* playlists_menuitem_new (PlayerController* parent);
PlaylistsMenuitem* playlists_menuitem_construct (GType object_type, PlayerController* parent);
GType playlist_details_get_type (void) G_GNUC_CONST;
PlaylistDetails* playlist_details_dup (const PlaylistDetails* self);
void playlist_details_free (PlaylistDetails* self);
void playlist_details_copy (const PlaylistDetails* self, PlaylistDetails* dest);
void playlist_details_destroy (PlaylistDetails* self);
void playlists_menuitem_update (PlaylistsMenuitem* self, PlaylistDetails* playlists, int playlists_length1);
static Block1Data* block1_data_ref (Block1Data* _data1_);
static void block1_data_unref (Block1Data* _data1_);
static gboolean playlists_menuitem_already_observed (PlaylistsMenuitem* self, PlaylistDetails* new_detail);
static gboolean playlists_menuitem_is_video_related (PlaylistsMenuitem* self, PlaylistDetails* new_detail);
static void ____lambda1_ (Block1Data* _data1_);
static void playlists_menuitem_submenu_item_activated (PlaylistsMenuitem* self, gint menu_item_id);
static void _____lambda1__dbusmenu_menuitem_item_activated (DbusmenuMenuitem* _sender, guint object, gpointer self);
void playlists_menuitem_update_individual_playlist (PlaylistsMenuitem* self, PlaylistDetails* new_detail);
void playlists_menuitem_active_playlist_update (PlaylistsMenuitem* self, PlaylistDetails* detail);
PlayerController* player_item_get_owner (PlayerItem* self);
GType mpris2_controller_get_type (void) G_GNUC_CONST;
void mpris2_controller_activate_playlist (Mpris2Controller* self, const char* path);
GeeHashSet* playlists_menuitem_attributes_format (void);
static GObject * playlists_menuitem_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
static void playlists_menuitem_finalize (GObject* obj);


PlaylistsMenuitem* playlists_menuitem_construct (GType object_type, PlayerController* parent) {
	PlaylistsMenuitem * self = NULL;
	PlayerController* _tmp0_;
	g_return_val_if_fail (parent != NULL, NULL);
	_tmp0_ = parent;
	self = (PlaylistsMenuitem*) g_object_new (object_type, "item-type", DBUSMENU_PLAYLISTS_MENUITEM_TYPE, "owner", _tmp0_, NULL);
	return self;
}


PlaylistsMenuitem* playlists_menuitem_new (PlayerController* parent) {
	return playlists_menuitem_construct (TYPE_PLAYLISTS_MENUITEM, parent);
}


static Block1Data* block1_data_ref (Block1Data* _data1_) {
	g_atomic_int_inc (&_data1_->_ref_count_);
	return _data1_;
}


static void block1_data_unref (Block1Data* _data1_) {
	if (g_atomic_int_dec_and_test (&_data1_->_ref_count_)) {
		_g_object_unref0 (_data1_->self);
		_g_object_unref0 (_data1_->menuitem);
		g_slice_free (Block1Data, _data1_);
	}
}


static void ____lambda1_ (Block1Data* _data1_) {
	PlaylistsMenuitem * self;
	DbusmenuMenuitem* _tmp0_;
	gint _tmp1_;
	gint _tmp2_;
	self = _data1_->self;
	_tmp0_ = _data1_->menuitem;
	_tmp1_ = dbusmenu_menuitem_get_id (_tmp0_);
	_tmp2_ = _tmp1_;
	playlists_menuitem_submenu_item_activated (self, _tmp2_);
}


static void _____lambda1__dbusmenu_menuitem_item_activated (DbusmenuMenuitem* _sender, guint object, gpointer self) {
	____lambda1_ (self);
}


void playlists_menuitem_update (PlaylistsMenuitem* self, PlaylistDetails* playlists, int playlists_length1) {
	PlaylistDetails* _tmp0_;
	gint _tmp0__length1;
	g_return_if_fail (self != NULL);
	_tmp0_ = playlists;
	_tmp0__length1 = playlists_length1;
	{
		PlaylistDetails* detail_collection = NULL;
		gint detail_collection_length1 = 0;
		gint _detail_collection_size_ = 0;
		gint detail_it = 0;
		detail_collection = _tmp0_;
		detail_collection_length1 = _tmp0__length1;
		for (detail_it = 0; detail_it < _tmp0__length1; detail_it = detail_it + 1) {
			PlaylistDetails _tmp1_ = {0};
			PlaylistDetails detail = {0};
			playlist_details_copy (&detail_collection[detail_it], &_tmp1_);
			detail = _tmp1_;
			{
				Block1Data* _data1_;
				gboolean _tmp2_ = FALSE;
				PlaylistDetails _tmp3_;
				gboolean _tmp4_ = FALSE;
				gboolean _tmp7_;
				DbusmenuMenuitem* _tmp8_;
				DbusmenuMenuitem* _tmp9_;
				PlaylistDetails _tmp10_;
				const gchar* _tmp11_;
				DbusmenuMenuitem* _tmp12_;
				DbusmenuMenuitem* _tmp13_;
				PlaylistDetails _tmp14_;
				const char* _tmp15_;
				DbusmenuMenuitem* _tmp16_;
				DbusmenuMenuitem* _tmp17_;
				DbusmenuMenuitem* _tmp18_;
				GeeHashMap* _tmp19_;
				DbusmenuMenuitem* _tmp20_;
				gint _tmp21_;
				gint _tmp22_;
				DbusmenuMenuitem* _tmp23_;
				DbusmenuMenuitem* _tmp24_;
				DbusmenuMenuitem* _tmp25_;
				PlaylistDetails _tmp26_;
				const gchar* _tmp27_;
				_data1_ = g_slice_new0 (Block1Data);
				_data1_->_ref_count_ = 1;
				_data1_->self = g_object_ref (self);
				_tmp3_ = detail;
				_tmp4_ = playlists_menuitem_already_observed (self, &_tmp3_);
				if (_tmp4_) {
					_tmp2_ = TRUE;
				} else {
					PlaylistDetails _tmp5_;
					gboolean _tmp6_ = FALSE;
					_tmp5_ = detail;
					_tmp6_ = playlists_menuitem_is_video_related (self, &_tmp5_);
					_tmp2_ = _tmp6_;
				}
				_tmp7_ = _tmp2_;
				if (_tmp7_) {
					playlist_details_destroy (&detail);
					block1_data_unref (_data1_);
					_data1_ = NULL;
					continue;
				}
				_tmp8_ = dbusmenu_menuitem_new ();
				_data1_->menuitem = _tmp8_;
				_tmp9_ = _data1_->menuitem;
				_tmp10_ = detail;
				_tmp11_ = _tmp10_.name;
				dbusmenu_menuitem_property_set (_tmp9_, DBUSMENU_MENUITEM_PROP_LABEL, _tmp11_);
				_tmp12_ = _data1_->menuitem;
				dbusmenu_menuitem_property_set (_tmp12_, DBUSMENU_MENUITEM_PROP_ICON_NAME, "playlist-symbolic");
				_tmp13_ = _data1_->menuitem;
				_tmp14_ = detail;
				_tmp15_ = _tmp14_.path;
				dbusmenu_menuitem_property_set (_tmp13_, DBUSMENU_PLAYLIST_MENUITEM_PATH, (const gchar*) _tmp15_);
				_tmp16_ = _data1_->menuitem;
				dbusmenu_menuitem_property_set_bool (_tmp16_, DBUSMENU_MENUITEM_PROP_VISIBLE, TRUE);
				_tmp17_ = _data1_->menuitem;
				dbusmenu_menuitem_property_set_bool (_tmp17_, DBUSMENU_MENUITEM_PROP_ENABLED, TRUE);
				_tmp18_ = _data1_->menuitem;
				g_signal_connect_data (_tmp18_, "item-activated", (GCallback) _____lambda1__dbusmenu_menuitem_item_activated, block1_data_ref (_data1_), (GClosureNotify) block1_data_unref, 0);
				_tmp19_ = self->priv->current_playlists;
				_tmp20_ = _data1_->menuitem;
				_tmp21_ = dbusmenu_menuitem_get_id (_tmp20_);
				_tmp22_ = _tmp21_;
				_tmp23_ = _data1_->menuitem;
				gee_abstract_map_set ((GeeAbstractMap*) _tmp19_, GINT_TO_POINTER (_tmp22_), _tmp23_);
				_tmp24_ = self->root_item;
				_tmp25_ = _data1_->menuitem;
				dbusmenu_menuitem_child_append (_tmp24_, _tmp25_);
				_tmp26_ = detail;
				_tmp27_ = _tmp26_.name;
				g_debug ("playlists-menu-item.vala:65: populating valid playlists %s", _tmp27_);
				playlist_details_destroy (&detail);
				block1_data_unref (_data1_);
				_data1_ = NULL;
			}
		}
	}
	{
		GeeHashMap* _tmp28_;
		GeeCollection* _tmp29_;
		GeeCollection* _tmp30_;
		GeeCollection* _tmp31_;
		GeeIterator* _tmp32_ = NULL;
		GeeIterator* _tmp33_;
		GeeIterator* _item_it;
		_tmp28_ = self->priv->current_playlists;
		_tmp29_ = gee_map_get_values ((GeeMap*) _tmp28_);
		_tmp30_ = _tmp29_;
		_tmp31_ = _tmp30_;
		_tmp32_ = gee_iterable_iterator ((GeeIterable*) _tmp31_);
		_tmp33_ = _tmp32_;
		_g_object_unref0 (_tmp31_);
		_item_it = _tmp33_;
		while (TRUE) {
			GeeIterator* _tmp34_;
			gboolean _tmp35_ = FALSE;
			GeeIterator* _tmp36_;
			gpointer _tmp37_ = NULL;
			DbusmenuMenuitem* item;
			gboolean within;
			PlaylistDetails* _tmp38_;
			gint _tmp38__length1;
			gboolean _tmp44_;
			_tmp34_ = _item_it;
			_tmp35_ = gee_iterator_next (_tmp34_);
			if (!_tmp35_) {
				break;
			}
			_tmp36_ = _item_it;
			_tmp37_ = gee_iterator_get (_tmp36_);
			item = (DbusmenuMenuitem*) _tmp37_;
			within = FALSE;
			_tmp38_ = playlists;
			_tmp38__length1 = playlists_length1;
			{
				PlaylistDetails* detail_collection = NULL;
				gint detail_collection_length1 = 0;
				gint _detail_collection_size_ = 0;
				gint detail_it = 0;
				detail_collection = _tmp38_;
				detail_collection_length1 = _tmp38__length1;
				for (detail_it = 0; detail_it < _tmp38__length1; detail_it = detail_it + 1) {
					PlaylistDetails _tmp39_ = {0};
					PlaylistDetails detail = {0};
					playlist_details_copy (&detail_collection[detail_it], &_tmp39_);
					detail = _tmp39_;
					{
						PlaylistDetails _tmp40_;
						const char* _tmp41_;
						DbusmenuMenuitem* _tmp42_;
						const gchar* _tmp43_ = NULL;
						_tmp40_ = detail;
						_tmp41_ = _tmp40_.path;
						_tmp42_ = item;
						_tmp43_ = dbusmenu_menuitem_property_get (_tmp42_, DBUSMENU_PLAYLIST_MENUITEM_PATH);
						if (g_strcmp0 (_tmp41_, _tmp43_) == 0) {
							within = TRUE;
							playlist_details_destroy (&detail);
							break;
						}
						playlist_details_destroy (&detail);
					}
				}
			}
			_tmp44_ = within;
			if (_tmp44_ == FALSE) {
				DbusmenuMenuitem* _tmp45_;
				const gchar* _tmp46_ = NULL;
				DbusmenuMenuitem* _tmp47_;
				const gchar* _tmp48_ = NULL;
				DbusmenuMenuitem* _tmp51_;
				DbusmenuMenuitem* _tmp52_;
				_tmp45_ = self->root_item;
				_tmp46_ = dbusmenu_menuitem_property_get (_tmp45_, DBUSMENU_PLAYLIST_MENUITEM_PATH);
				_tmp47_ = item;
				_tmp48_ = dbusmenu_menuitem_property_get (_tmp47_, DBUSMENU_PLAYLIST_MENUITEM_PATH);
				if (g_strcmp0 (_tmp46_, _tmp48_) == 0) {
					DbusmenuMenuitem* _tmp49_;
					const gchar* _tmp50_ = NULL;
					_tmp49_ = self->root_item;
					_tmp50_ = _ ("Choose Playlist");
					dbusmenu_menuitem_property_set (_tmp49_, DBUSMENU_MENUITEM_PROP_LABEL, _tmp50_);
				}
				_tmp51_ = self->root_item;
				_tmp52_ = item;
				dbusmenu_menuitem_child_delete (_tmp51_, _tmp52_);
			}
			_g_object_unref0 (item);
		}
		_g_object_unref0 (_item_it);
	}
}


void playlists_menuitem_update_individual_playlist (PlaylistsMenuitem* self, PlaylistDetails* new_detail) {
	DbusmenuMenuitem* _tmp17_;
	const gchar* _tmp18_ = NULL;
	PlaylistDetails _tmp19_;
	const char* _tmp20_;
	g_return_if_fail (self != NULL);
	g_return_if_fail (new_detail != NULL);
	{
		GeeHashMap* _tmp0_;
		GeeCollection* _tmp1_;
		GeeCollection* _tmp2_;
		GeeCollection* _tmp3_;
		GeeIterator* _tmp4_ = NULL;
		GeeIterator* _tmp5_;
		GeeIterator* _item_it;
		_tmp0_ = self->priv->current_playlists;
		_tmp1_ = gee_map_get_values ((GeeMap*) _tmp0_);
		_tmp2_ = _tmp1_;
		_tmp3_ = _tmp2_;
		_tmp4_ = gee_iterable_iterator ((GeeIterable*) _tmp3_);
		_tmp5_ = _tmp4_;
		_g_object_unref0 (_tmp3_);
		_item_it = _tmp5_;
		while (TRUE) {
			GeeIterator* _tmp6_;
			gboolean _tmp7_ = FALSE;
			GeeIterator* _tmp8_;
			gpointer _tmp9_ = NULL;
			DbusmenuMenuitem* item;
			PlaylistDetails _tmp10_;
			const char* _tmp11_;
			DbusmenuMenuitem* _tmp12_;
			const gchar* _tmp13_ = NULL;
			_tmp6_ = _item_it;
			_tmp7_ = gee_iterator_next (_tmp6_);
			if (!_tmp7_) {
				break;
			}
			_tmp8_ = _item_it;
			_tmp9_ = gee_iterator_get (_tmp8_);
			item = (DbusmenuMenuitem*) _tmp9_;
			_tmp10_ = *new_detail;
			_tmp11_ = _tmp10_.path;
			_tmp12_ = item;
			_tmp13_ = dbusmenu_menuitem_property_get (_tmp12_, DBUSMENU_PLAYLIST_MENUITEM_PATH);
			if (g_strcmp0 (_tmp11_, _tmp13_) == 0) {
				DbusmenuMenuitem* _tmp14_;
				PlaylistDetails _tmp15_;
				const gchar* _tmp16_;
				_tmp14_ = item;
				_tmp15_ = *new_detail;
				_tmp16_ = _tmp15_.name;
				dbusmenu_menuitem_property_set (_tmp14_, DBUSMENU_MENUITEM_PROP_LABEL, _tmp16_);
			}
			_g_object_unref0 (item);
		}
		_g_object_unref0 (_item_it);
	}
	_tmp17_ = self->root_item;
	_tmp18_ = dbusmenu_menuitem_property_get (_tmp17_, DBUSMENU_PLAYLIST_MENUITEM_PATH);
	_tmp19_ = *new_detail;
	_tmp20_ = _tmp19_.path;
	if (g_strcmp0 (_tmp18_, _tmp20_) == 0) {
		DbusmenuMenuitem* _tmp21_;
		PlaylistDetails _tmp22_;
		const gchar* _tmp23_;
		_tmp21_ = self->root_item;
		_tmp22_ = *new_detail;
		_tmp23_ = _tmp22_.name;
		dbusmenu_menuitem_property_set (_tmp21_, DBUSMENU_MENUITEM_PROP_LABEL, _tmp23_);
	}
}


static gboolean playlists_menuitem_already_observed (PlaylistsMenuitem* self, PlaylistDetails* new_detail) {
	gboolean result = FALSE;
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (new_detail != NULL, FALSE);
	{
		GeeHashMap* _tmp0_;
		GeeCollection* _tmp1_;
		GeeCollection* _tmp2_;
		GeeCollection* _tmp3_;
		GeeIterator* _tmp4_ = NULL;
		GeeIterator* _tmp5_;
		GeeIterator* _item_it;
		_tmp0_ = self->priv->current_playlists;
		_tmp1_ = gee_map_get_values ((GeeMap*) _tmp0_);
		_tmp2_ = _tmp1_;
		_tmp3_ = _tmp2_;
		_tmp4_ = gee_iterable_iterator ((GeeIterable*) _tmp3_);
		_tmp5_ = _tmp4_;
		_g_object_unref0 (_tmp3_);
		_item_it = _tmp5_;
		while (TRUE) {
			GeeIterator* _tmp6_;
			gboolean _tmp7_ = FALSE;
			GeeIterator* _tmp8_;
			gpointer _tmp9_ = NULL;
			DbusmenuMenuitem* item;
			DbusmenuMenuitem* _tmp10_;
			const gchar* _tmp11_ = NULL;
			gchar* _tmp12_;
			gchar* path;
			PlaylistDetails _tmp13_;
			const char* _tmp14_;
			const gchar* _tmp15_;
			_tmp6_ = _item_it;
			_tmp7_ = gee_iterator_next (_tmp6_);
			if (!_tmp7_) {
				break;
			}
			_tmp8_ = _item_it;
			_tmp9_ = gee_iterator_get (_tmp8_);
			item = (DbusmenuMenuitem*) _tmp9_;
			_tmp10_ = item;
			_tmp11_ = dbusmenu_menuitem_property_get (_tmp10_, DBUSMENU_PLAYLIST_MENUITEM_PATH);
			_tmp12_ = g_strdup (_tmp11_);
			path = _tmp12_;
			_tmp13_ = *new_detail;
			_tmp14_ = _tmp13_.path;
			_tmp15_ = path;
			if (g_strcmp0 (_tmp14_, _tmp15_) == 0) {
				result = TRUE;
				_g_free0 (path);
				_g_object_unref0 (item);
				_g_object_unref0 (_item_it);
				return result;
			}
			_g_free0 (path);
			_g_object_unref0 (item);
		}
		_g_object_unref0 (_item_it);
	}
	result = FALSE;
	return result;
}


static gboolean string_contains (const gchar* self, const gchar* needle) {
	gboolean result = FALSE;
	const gchar* _tmp0_;
	gchar* _tmp1_ = NULL;
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (needle != NULL, FALSE);
	_tmp0_ = needle;
	_tmp1_ = strstr ((gchar*) self, (gchar*) _tmp0_);
	result = _tmp1_ != NULL;
	return result;
}


static gboolean playlists_menuitem_is_video_related (PlaylistsMenuitem* self, PlaylistDetails* new_detail) {
	gboolean result = FALSE;
	PlaylistDetails _tmp0_;
	const char* _tmp1_;
	gchar* _tmp2_;
	gchar* location;
	const gchar* _tmp3_;
	gboolean _tmp4_ = FALSE;
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (new_detail != NULL, FALSE);
	_tmp0_ = *new_detail;
	_tmp1_ = _tmp0_.path;
	_tmp2_ = g_strdup ((const gchar*) _tmp1_);
	location = _tmp2_;
	_tmp3_ = location;
	_tmp4_ = string_contains (_tmp3_, "/VideoLibrarySource/");
	if (_tmp4_) {
		result = TRUE;
		_g_free0 (location);
		return result;
	}
	result = FALSE;
	_g_free0 (location);
	return result;
}


void playlists_menuitem_active_playlist_update (PlaylistsMenuitem* self, PlaylistDetails* detail) {
	PlaylistDetails _tmp0_;
	const gchar* _tmp1_;
	gchar* _tmp2_;
	gchar* update;
	const gchar* _tmp3_;
	DbusmenuMenuitem* _tmp6_;
	const gchar* _tmp7_;
	DbusmenuMenuitem* _tmp8_;
	PlaylistDetails _tmp9_;
	const char* _tmp10_;
	g_return_if_fail (self != NULL);
	g_return_if_fail (detail != NULL);
	_tmp0_ = *detail;
	_tmp1_ = _tmp0_.name;
	_tmp2_ = g_strdup (_tmp1_);
	update = _tmp2_;
	_tmp3_ = update;
	if (g_strcmp0 (_tmp3_, "") == 0) {
		const gchar* _tmp4_ = NULL;
		gchar* _tmp5_;
		_tmp4_ = _ ("Choose Playlist");
		_tmp5_ = g_strdup (_tmp4_);
		_g_free0 (update);
		update = _tmp5_;
	}
	_tmp6_ = self->root_item;
	_tmp7_ = update;
	dbusmenu_menuitem_property_set (_tmp6_, DBUSMENU_MENUITEM_PROP_LABEL, _tmp7_);
	_tmp8_ = self->root_item;
	_tmp9_ = *detail;
	_tmp10_ = _tmp9_.path;
	dbusmenu_menuitem_property_set (_tmp8_, DBUSMENU_PLAYLIST_MENUITEM_PATH, (const gchar*) _tmp10_);
	_g_free0 (update);
}


static void playlists_menuitem_submenu_item_activated (PlaylistsMenuitem* self, gint menu_item_id) {
	GeeHashMap* _tmp0_;
	gint _tmp1_;
	gboolean _tmp2_ = FALSE;
	PlayerController* _tmp4_;
	PlayerController* _tmp5_;
	Mpris2Controller* _tmp6_;
	GeeHashMap* _tmp7_;
	gint _tmp8_;
	gpointer _tmp9_ = NULL;
	DbusmenuMenuitem* _tmp10_;
	const gchar* _tmp11_ = NULL;
	g_return_if_fail (self != NULL);
	_tmp0_ = self->priv->current_playlists;
	_tmp1_ = menu_item_id;
	_tmp2_ = gee_abstract_map_has_key ((GeeAbstractMap*) _tmp0_, GINT_TO_POINTER (_tmp1_));
	if (!_tmp2_) {
		gint _tmp3_;
		_tmp3_ = menu_item_id;
		g_warning ("playlists-menu-item.vala:125: item %i was activated but we don't have " \
"a corresponding playlist", _tmp3_);
		return;
	}
	_tmp4_ = player_item_get_owner ((PlayerItem*) self);
	_tmp5_ = _tmp4_;
	_tmp6_ = _tmp5_->mpris_bridge;
	_tmp7_ = self->priv->current_playlists;
	_tmp8_ = menu_item_id;
	_tmp9_ = gee_abstract_map_get ((GeeAbstractMap*) _tmp7_, GINT_TO_POINTER (_tmp8_));
	_tmp10_ = (DbusmenuMenuitem*) _tmp9_;
	_tmp11_ = dbusmenu_menuitem_property_get (_tmp10_, DBUSMENU_PLAYLIST_MENUITEM_PATH);
	mpris2_controller_activate_playlist (_tmp6_, (const char*) _tmp11_);
	_g_object_unref0 (_tmp10_);
}


GeeHashSet* playlists_menuitem_attributes_format (void) {
	GeeHashSet* result = NULL;
	GeeHashSet* _tmp0_;
	GeeHashSet* attrs;
	_tmp0_ = gee_hash_set_new (G_TYPE_STRING, (GBoxedCopyFunc) g_strdup, g_free, NULL, NULL);
	attrs = _tmp0_;
	gee_abstract_collection_add ((GeeAbstractCollection*) attrs, DBUSMENU_PLAYLISTS_MENUITEM_TITLE);
	gee_abstract_collection_add ((GeeAbstractCollection*) attrs, DBUSMENU_PLAYLISTS_MENUITEM_PLAYLISTS);
	result = attrs;
	return result;
}


static GObject * playlists_menuitem_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties) {
	GObject * obj;
	GObjectClass * parent_class;
	PlaylistsMenuitem * self;
	GeeHashMap* _tmp0_;
	DbusmenuMenuitem* _tmp1_;
	DbusmenuMenuitem* _tmp2_;
	const gchar* _tmp3_ = NULL;
	DbusmenuMenuitem* _tmp4_;
	parent_class = G_OBJECT_CLASS (playlists_menuitem_parent_class);
	obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	self = PLAYLISTS_MENUITEM (obj);
	_tmp0_ = gee_hash_map_new (G_TYPE_INT, NULL, NULL, dbusmenu_menuitem_get_type (), (GBoxedCopyFunc) g_object_ref, g_object_unref, NULL, NULL, NULL);
	_g_object_unref0 (self->priv->current_playlists);
	self->priv->current_playlists = _tmp0_;
	_tmp1_ = dbusmenu_menuitem_new ();
	_g_object_unref0 (self->root_item);
	self->root_item = _tmp1_;
	_tmp2_ = self->root_item;
	_tmp3_ = _ ("Choose Playlist");
	dbusmenu_menuitem_property_set (_tmp2_, DBUSMENU_MENUITEM_PROP_LABEL, _tmp3_);
	_tmp4_ = self->root_item;
	dbusmenu_menuitem_property_set (_tmp4_, DBUSMENU_PLAYLIST_MENUITEM_PATH, "");
	return obj;
}


static void playlists_menuitem_class_init (PlaylistsMenuitemClass * klass) {
	playlists_menuitem_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (PlaylistsMenuitemPrivate));
	G_OBJECT_CLASS (klass)->constructor = playlists_menuitem_constructor;
	G_OBJECT_CLASS (klass)->finalize = playlists_menuitem_finalize;
}


static void playlists_menuitem_instance_init (PlaylistsMenuitem * self) {
	self->priv = PLAYLISTS_MENUITEM_GET_PRIVATE (self);
}


static void playlists_menuitem_finalize (GObject* obj) {
	PlaylistsMenuitem * self;
	self = PLAYLISTS_MENUITEM (obj);
	_g_object_unref0 (self->priv->current_playlists);
	_g_object_unref0 (self->root_item);
	G_OBJECT_CLASS (playlists_menuitem_parent_class)->finalize (obj);
}


GType playlists_menuitem_get_type (void) {
	static volatile gsize playlists_menuitem_type_id__volatile = 0;
	if (g_once_init_enter (&playlists_menuitem_type_id__volatile)) {
		static const GTypeInfo g_define_type_info = { sizeof (PlaylistsMenuitemClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) playlists_menuitem_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (PlaylistsMenuitem), 0, (GInstanceInitFunc) playlists_menuitem_instance_init, NULL };
		GType playlists_menuitem_type_id;
		playlists_menuitem_type_id = g_type_register_static (TYPE_PLAYER_ITEM, "PlaylistsMenuitem", &g_define_type_info, 0);
		g_once_init_leave (&playlists_menuitem_type_id__volatile, playlists_menuitem_type_id);
	}
	return playlists_menuitem_type_id__volatile;
}



