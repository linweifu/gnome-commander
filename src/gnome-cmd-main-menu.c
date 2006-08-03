/*
    GNOME Commander - A GNOME based file manager
    Copyright (C) 2001-2006 Marcus Bjurman

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include <config.h>
#include "gnome-cmd-includes.h"
#include "gnome-cmd-main-menu.h"
#include "gnome-cmd-types.h"
#include "gnome-cmd-bookmark-dialog.h"
#include "gnome-cmd-main-win.h"
#include "gnome-cmd-file-selector.h"
#include "gnome-cmd-con.h"
#include "gnome-cmd-data.h"
#include "utils.h"
#include "imageloader.h"
#include "useractions.h"

#include "../pixmaps/exec_wheel.xpm"
#include "../pixmaps/menu_ftp_connect.xpm"


/* Theese following types are slightly changed from the originals in the GnomeUI library
   We need special types because we neeed to place non-changeable shortcuts in the
   menus. Another difference is that we want only mouse-clicks in the menu to generate an
   action, keyboard shortcuts are caught by the different components by them self */

typedef enum {
    MENU_TYPE_END,        /* No more items, use it at the end of an array */
    MENU_TYPE_ITEM,       /* Normal item, or radio item if it is inside a radioitems group */
    MENU_TYPE_BASIC,
    MENU_TYPE_TOGGLEITEM, /* Toggle (check box) item */
    MENU_TYPE_RADIOITEMS, /* Radio item group */
    MENU_TYPE_SUBTREE,    /* Item that defines a subtree/submenu */
    MENU_TYPE_SEPARATOR   /* Separator line (menus) or blank space (toolbars) */
} MenuType;


typedef struct {
    MenuType type;          /* Type of item */
    gchar *label;            /* The text to use for this menu-item */
    gchar *shortcut;        /* The shortcut for this menu-item */
    gchar *tooltip;         /* The tooltip of this menu-item */
    gpointer moreinfo;        /* For an item, toggleitem, this is a pointer to the
                               function to call when the item is activated. */
    gpointer user_data;        /* Data pointer to pass to callbacks */
    GnomeUIPixmapType pixmap_type;    /* Type of pixmap for the item */
    gconstpointer pixmap_info;      /* Pointer to the pixmap information:
                                     *
                                     * For GNOME_APP_PIXMAP_STOCK, a
                                     * pointer to the stock icon name.
                                     *
                                     * For GNOME_APP_PIXMAP_DATA, a
                                     * pointer to the inline xpm data.
                                     *
                                     * For GNOME_APP_PIXMAP_FILENAME, a
                                     * pointer to the filename string.
                                     */

    GtkWidget *widget;        /* Filled in by gnome_app_create*, you can use this
                               to tweak the widgets once they have been created */
} MenuData;

#define MENUTYPE_END { \
    MENU_TYPE_END, \
    NULL, NULL, NULL, \
    NULL, NULL, \
    (GnomeUIPixmapType) 0, NULL, \
    NULL }

#define MENUTYPE_SEPARATOR { \
    MENU_TYPE_SEPARATOR, \
    NULL, NULL, NULL, \
    NULL, NULL, \
    (GnomeUIPixmapType) 0, NULL, \
    NULL }




struct _GnomeCmdMainMenuPrivate
{
    GtkWidget *file_menu;
    GtkWidget *edit_menu;
    GtkWidget *mark_menu;
    GtkWidget *view_menu;
    GtkWidget *options_menu;
    GtkWidget *connections_menu;
    GtkWidget *bookmarks_menu;
    GtkWidget *plugins_menu;
    GtkWidget *help_menu;

    GtkWidget *menu_edit_paste;
    GtkWidget *menu_view_conbuttons;
    GtkWidget *menu_view_toolbar;
    GtkWidget *menu_view_buttonbar;
    GtkWidget *menu_view_cmdline;
    GtkWidget *menu_view_hidden_files;
    GtkWidget *menu_view_backup_files;
    GtkWidget *menu_view_back;
    GtkWidget *menu_view_forward;

    GList *connections_menuitems;

    GList *bookmark_menuitems;
    GList *group_menuitems;

    GList *view_menuitems;

    GtkTooltips *tooltips;
};


static GtkMenuBarClass *parent_class = NULL;


/*******************************
 * Private functions
 *******************************/

static void
on_bookmark_selected (GtkMenuItem *menuitem, GnomeCmdBookmark *bookmark)
{
    g_return_if_fail (bookmark != NULL);

    gnome_cmd_bookmark_goto (bookmark);
}


static void
on_con_list_list_changed (GnomeCmdConList *con_list, GnomeCmdMainMenu *main_menu)
{
    gnome_cmd_main_menu_update_connections (main_menu);
}


static GtkWidget*
create_menu_item (GnomeCmdMainMenu *main_menu, GtkMenu *parent, MenuData *spec)
{
    GtkWidget *item=NULL;
    GtkWidget *desc, *shortcut;
    GtkWidget *content = NULL;
    GtkWidget *pixmap = NULL;

    switch (spec->type)
    {
        case MENU_TYPE_BASIC:
            item = gtk_menu_item_new ();
            desc = gtk_label_new_with_mnemonic (spec->label);
            gtk_widget_ref (desc);
            gtk_widget_show (desc);

            gtk_container_add (GTK_CONTAINER (item), desc);
            break;

        case MENU_TYPE_ITEM:
            item = gtk_image_menu_item_new ();
            content = create_hbox (GTK_WIDGET (main_win), FALSE, 12);

            desc = gtk_label_new_with_mnemonic (spec->label);
            gtk_widget_ref (desc);
            gtk_widget_show (desc);
            gtk_box_pack_start (GTK_BOX (content), desc, FALSE, FALSE, 0);

            shortcut = create_label (GTK_WIDGET (main_win), spec->shortcut);
            gtk_misc_set_alignment (GTK_MISC (shortcut), 1.0, 0.5);
            gtk_box_pack_start (GTK_BOX (content), shortcut, TRUE, TRUE, 0);

            if (spec->pixmap_type != 0 && spec->pixmap_info) {
                pixmap = create_ui_pixmap (GTK_WIDGET (main_win),
                                           spec->pixmap_type,
                                           spec->pixmap_info,
                                           GTK_ICON_SIZE_MENU);
                if (pixmap) {
                    gtk_widget_show (pixmap);
                    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), pixmap);
                }
            }
            if (spec->tooltip)
                gtk_tooltips_set_tip (main_menu->priv->tooltips, item, spec->tooltip, NULL);
            gtk_container_add (GTK_CONTAINER (item), content);
            break;

        case MENU_TYPE_TOGGLEITEM:
            item = gtk_check_menu_item_new_with_label (spec->label);
            gtk_signal_connect (GTK_OBJECT (item), "toggled", GTK_SIGNAL_FUNC (spec->moreinfo), spec->user_data);
            break;

        case MENU_TYPE_SEPARATOR:
            item = gtk_menu_item_new ();
            gtk_widget_set_sensitive (item, FALSE);
            break;

        default:
            g_warning ("This MENU_TYPE is not implemented\n");
            return NULL;
    }


    gtk_widget_show (item);

    if (spec->type == MENU_TYPE_ITEM) {
        /* Connect to the signal and set user data */
        gtk_object_set_data (GTK_OBJECT (item), GNOMEUIINFO_KEY_UIDATA, spec->user_data);

        gtk_signal_connect (GTK_OBJECT (item), "activate", GTK_SIGNAL_FUNC (spec->moreinfo), spec->user_data);
    }

    spec->widget = item;

    return item;
}


static GtkWidget*
create_menu (GnomeCmdMainMenu *main_menu, MenuData *spec, MenuData *childs)
{
    gint i=0;
    GtkWidget *submenu, *menu_item;

    submenu = gtk_menu_new ();
    menu_item = create_menu_item (main_menu, NULL, spec);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), submenu);

    gtk_widget_ref (menu_item);
    gtk_widget_show (menu_item);

    while (childs[i].type != MENU_TYPE_END) {
        GtkWidget *child = create_menu_item (main_menu, GTK_MENU (submenu), &childs[i]);
        gtk_menu_shell_append (GTK_MENU_SHELL (submenu), child);
        i++;
    }

    return menu_item;
}


static GtkWidget *
add_menu_item (GnomeCmdMainMenu *main_menu,
               GtkMenuShell *menu,
               const gchar *text,
               const gchar *tooltip,
               GdkPixmap *pixmap,
               GdkBitmap *mask,
               GtkSignalFunc callback,
               gpointer user_data)
{
    GtkWidget *item, *label;
    GtkWidget *pixmap_widget = NULL;

    g_return_val_if_fail (GTK_IS_MENU_SHELL (menu), NULL);

    item = gtk_image_menu_item_new ();

    if (tooltip)
        gtk_tooltips_set_tip (main_menu->priv->tooltips, item, tooltip, NULL);

    if (pixmap && mask)
        pixmap_widget = gtk_pixmap_new (pixmap, mask);

    if (pixmap_widget) {
        gtk_widget_show (pixmap_widget);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), pixmap_widget);
    }

    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);


    /* Create the contents of the menu item */
    label = gtk_label_new (text);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_widget_show (label);
    gtk_container_add (GTK_CONTAINER (item), label);


    /* Connect to the signal and set user data */
    if (callback) {
        gtk_object_set_data (GTK_OBJECT (item), GNOMEUIINFO_KEY_UIDATA, user_data);
        gtk_signal_connect (GTK_OBJECT (item), "activate", callback, user_data);
    }

    return item;
}


static GtkWidget *
add_separator (GnomeCmdMainMenu *main_menu, GtkMenuShell *menu)
{
    MenuData t = MENUTYPE_SEPARATOR;

    GtkWidget *child = create_menu_item (main_menu, GTK_MENU (menu), &t);
    gtk_menu_shell_append (menu, child);

    return child;
}


static void
add_bookmark_menu_item (GnomeCmdMainMenu *main_menu, GtkMenuShell *menu, GnomeCmdBookmark *bookmark)
{
    GtkWidget *item;

    item = add_menu_item (main_menu, menu, bookmark->name, NULL,
                          IMAGE_get_pixmap (PIXMAP_BOOKMARK), IMAGE_get_mask (PIXMAP_BOOKMARK),
                          GTK_SIGNAL_FUNC (on_bookmark_selected), bookmark);

    /* Remeber this bookmarks item-widget so that we can remove it later */
    main_menu->priv->bookmark_menuitems = g_list_append (main_menu->priv->bookmark_menuitems, item);
}


static void
add_bookmark_group (GnomeCmdMainMenu *main_menu, GtkMenuShell *menu, GnomeCmdBookmarkGroup *group)
{
    GtkWidget *submenu;
    GtkWidget *item;
    GnomeCmdPixmap *pixmap;
    GList *bookmarks = group->bookmarks;

    g_return_if_fail (GTK_IS_MENU_SHELL (menu));
    g_return_if_fail (bookmarks != NULL);

    pixmap = gnome_cmd_con_get_go_pixmap (group->con);
    item = add_menu_item (main_menu, menu, gnome_cmd_con_get_alias (group->con), NULL,
                          pixmap?pixmap->pixmap:NULL, pixmap?pixmap->mask:NULL,
                          NULL, NULL);

    /* Remeber this bookmarks item-widget so that we can remove it later */
    main_menu->priv->group_menuitems = g_list_append (main_menu->priv->group_menuitems, item);


    /* Add bookmarks for this group */
    submenu = gtk_menu_new ();
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);
    while (bookmarks) {
        GnomeCmdBookmark *bookmark = (GnomeCmdBookmark*)bookmarks->data;
        add_bookmark_menu_item (main_menu, GTK_MENU_SHELL (submenu), bookmark);
        bookmarks = bookmarks->next;
    }
}

static void
update_view_menu (GnomeCmdMainMenu *main_menu);


static void
on_switch_orientation (GtkMenuItem *menu_item, GnomeCmdMainMenu *main_menu)
{
    gnome_cmd_data_set_list_orientation (!gnome_cmd_data_get_list_orientation ());

    gnome_cmd_main_win_update_list_orientation (main_win);

    update_view_menu (main_menu);
}


static void
update_view_menu (GnomeCmdMainMenu *main_menu)
{
    gchar *label;
    GtkWidget *item;
    GdkPixmap *pm;
    GdkBitmap *bm;

    if (gnome_cmd_data_get_list_orientation ()) {
        label = g_strdup (_("Switch to Horizontal Layout"));
        pm = IMAGE_get_pixmap (PIXMAP_SWITCH_V);
        bm = IMAGE_get_mask (PIXMAP_SWITCH_V);
    }
    else {
        label = g_strdup (_("Switch to Vertical Layout"));
        pm = IMAGE_get_pixmap (PIXMAP_SWITCH_H);
        bm = IMAGE_get_mask (PIXMAP_SWITCH_H);
    }

    g_list_foreach (main_menu->priv->view_menuitems, (GFunc)gtk_object_destroy, NULL);
    g_list_free (main_menu->priv->view_menuitems);
    main_menu->priv->view_menuitems = NULL;

    item = add_menu_item (
        main_menu,
        GTK_MENU_SHELL (GTK_MENU_ITEM (main_menu->priv->view_menu)->submenu),
        label, NULL,
        pm, bm,
        GTK_SIGNAL_FUNC (on_switch_orientation), main_menu);

    g_free (label);

    main_menu->priv->view_menuitems = g_list_append (
        main_menu->priv->view_menuitems, item);
}


/*******************************
 * Gtk class implementation
 *******************************/

static void
destroy (GtkObject *object)
{
    GnomeCmdMainMenu *menu = GNOME_CMD_MAIN_MENU (object);

    g_free (menu->priv);

    if (GTK_OBJECT_CLASS (parent_class)->destroy)
        (*GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}


static void
map (GtkWidget *widget)
{
    if (GTK_WIDGET_CLASS (parent_class)->map != NULL)
        GTK_WIDGET_CLASS (parent_class)->map (widget);
}


static void
class_init (GnomeCmdMainMenuClass *class)
{
    GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

    parent_class = gtk_type_class (gtk_menu_bar_get_type ());
    object_class->destroy = destroy;
    widget_class->map = map;
}


static void
init (GnomeCmdMainMenu *main_menu)
{
    MenuData files_menu_uiinfo[] =
    {
        {
            MENU_TYPE_ITEM, _("Change _Owner/Group"), "", NULL,
            file_chown, NULL,
            GNOME_APP_PIXMAP_NONE, NULL,
            NULL
        },
        {
            MENU_TYPE_ITEM, _("Change Per_missions"), "", NULL,
            file_chmod, NULL,
            GNOME_APP_PIXMAP_NONE, NULL,
            NULL
        },
        {
            MENU_TYPE_ITEM, _("Advanced _Rename Tool"), "Ctrl+M", NULL,
            file_advrename, NULL,
            GNOME_APP_PIXMAP_NONE, NULL,
            NULL
        },
        {
            MENU_TYPE_ITEM, _("Create _Symbolic Link"), "Ctrl+Shift+F5", NULL,
            file_create_symlink, NULL,
            GNOME_APP_PIXMAP_NONE, NULL,
            NULL
        },
        {
            MENU_TYPE_ITEM, _("_Properties..."), "Alt+ENTER", NULL,
            file_properties, NULL,
            GNOME_APP_PIXMAP_STOCK, GTK_STOCK_PROPERTIES,
            NULL
        },
        MENUTYPE_SEPARATOR,
        {
            MENU_TYPE_ITEM, _("_Diff"), "", NULL,
            file_diff, NULL,
            GNOME_APP_PIXMAP_NONE, NULL,
            NULL
        },
        MENUTYPE_SEPARATOR,
        {
            MENU_TYPE_ITEM, _("_Quit"), "Ctrl+Q", NULL,
            file_exit, NULL,
            GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_QUIT,
            NULL
        },
        MENUTYPE_END
    };

    MenuData mark_menu_uiinfo[] =
    {
        {
            MENU_TYPE_ITEM, _("_Select All"), "Ctrl++", NULL,
            mark_select_all, NULL,
            GNOME_APP_PIXMAP_NONE, NULL,
            NULL
        },
        {
            MENU_TYPE_ITEM, _("_Unselect All"), "Ctrl+-", NULL,
            mark_unselect_all, NULL,
            GNOME_APP_PIXMAP_NONE, NULL,
            NULL
        },
        {
            MENU_TYPE_ITEM, _("Select with _Pattern"), "+", NULL,
            mark_select_with_pattern, NULL,
            GNOME_APP_PIXMAP_NONE, NULL,
            NULL
        },
        {
            MENU_TYPE_ITEM, _("Unselect with P_attern"), "-", NULL,
            mark_unselect_with_pattern, NULL,
            GNOME_APP_PIXMAP_NONE, NULL,
            NULL
        },
        {
            MENU_TYPE_ITEM, _("_Invert Selection"), "*", NULL,
            mark_invert_selection, NULL,
            GNOME_APP_PIXMAP_NONE, NULL,
            NULL
        },
        {
            MENU_TYPE_ITEM, _("_Restore Selection"), "", NULL,
            mark_restore_selection, NULL,
            GNOME_APP_PIXMAP_NONE, NULL,
            NULL
        },
        MENUTYPE_SEPARATOR,
        {
            MENU_TYPE_ITEM, _("_Compare Directories"), "Shift+F2", NULL,
            mark_compare_directories, NULL,
            GNOME_APP_PIXMAP_NONE, NULL,
            NULL
        },
        MENUTYPE_END
    };

    MenuData edit_menu_uiinfo[] =
    {
        {
            MENU_TYPE_ITEM, _("Cu_t"), "Ctrl+X", NULL,
            file_cap_cut, NULL,
            GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_CUT,
            NULL
        },
        {
            MENU_TYPE_ITEM, _("_Copy"), "Ctrl+C", NULL,
            file_cap_copy, NULL,
            GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_COPY,
            NULL
        },
        {
            MENU_TYPE_ITEM, _("_Paste"), "Ctrl+V", NULL,
            file_cap_paste, NULL,
            GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PASTE,
            NULL
        },
        {
            MENU_TYPE_ITEM, _("_Delete"), "Delete", NULL,
            file_delete, NULL,
            GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_TRASH,
            NULL
        },
        MENUTYPE_SEPARATOR,
        {
            MENU_TYPE_ITEM, _("Copy _File Names"), "", NULL,
            edit_copy_fnames, NULL,
            GNOME_APP_PIXMAP_NONE, 0,
            NULL
        },
        MENUTYPE_SEPARATOR,
        {
            MENU_TYPE_ITEM, _("_Search..."), "Alt+F7", NULL,
            edit_search, NULL,
            GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SEARCH,
            NULL
        },
        {
            MENU_TYPE_ITEM, _("_Quick Search..."), "", NULL,
            edit_quick_search, NULL,
            GNOME_APP_PIXMAP_NONE, 0,
            NULL
        },
        {
            MENU_TYPE_ITEM, _("_Enable Filter..."), "", NULL,
            edit_filter, NULL,
            GNOME_APP_PIXMAP_NONE, 0,
            NULL
        },
        MENUTYPE_END
    };

    MenuData view_menu_uiinfo[] =
    {
        {
            MENU_TYPE_ITEM, _("_Back"), "Alt+Left", NULL,
            view_back, NULL,
            GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BACK,
            NULL
        },
        {
            MENU_TYPE_ITEM, _("_Forward"), "Alt+Right", NULL,
            view_forward, NULL,
            GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_FORWARD,
            NULL
        },
        {
            MENU_TYPE_ITEM, _("_Refresh"), "Ctrl+R", NULL,
            view_refresh, NULL,
            GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_REFRESH,
            NULL
        },
        MENUTYPE_SEPARATOR,
        {
            MENU_TYPE_TOGGLEITEM, _("Show Device Buttons"), "", NULL,
            view_conbuttons, NULL,
            GNOME_APP_PIXMAP_NONE, NULL,
            NULL
        },
        {
            MENU_TYPE_TOGGLEITEM, _("Show Toolbar"), "", NULL,
            view_toolbar, NULL,
            GNOME_APP_PIXMAP_NONE, NULL,
            NULL
        },
        {
            MENU_TYPE_TOGGLEITEM, _("Show Commandline"), "", NULL,
            view_cmdline, NULL,
            GNOME_APP_PIXMAP_NONE, NULL,
            NULL
        },
        {
            MENU_TYPE_TOGGLEITEM, _("Show Buttonbar"), "", NULL,
            view_buttonbar, NULL,
            GNOME_APP_PIXMAP_NONE, NULL,
            NULL
        },
        MENUTYPE_SEPARATOR,
        {
            MENU_TYPE_TOGGLEITEM, _("Show Hidden Files"), "", NULL,
            view_hidden_files, NULL,
            GNOME_APP_PIXMAP_NONE, NULL,
            NULL
        },
        {
            MENU_TYPE_TOGGLEITEM, _("Show Backup Files"), "", NULL,
            view_backup_files, NULL,
            GNOME_APP_PIXMAP_NONE, NULL,
            NULL
        },
        MENUTYPE_SEPARATOR,
        MENUTYPE_END
    };

    MenuData bookmarks_menu_uiinfo[] =
    {
        {
            MENU_TYPE_ITEM, _("_Bookmark this Directory..."), "", NULL,
            bookmarks_add_current, NULL,
            GNOME_APP_PIXMAP_STOCK, GTK_STOCK_ADD,
            NULL
        },
        {
            MENU_TYPE_ITEM, _("_Manage Bookmarks..."), "Ctrl+D", NULL,
            bookmarks_edit, NULL,
            GNOME_APP_PIXMAP_NONE, NULL,
            NULL
        },
        MENUTYPE_SEPARATOR,
        MENUTYPE_END
    };

    MenuData plugins_menu_uiinfo[] =
    {
        {
            MENU_TYPE_ITEM, _("_Configure Plugins..."), "", NULL,
            plugins_configure, NULL,
            GNOME_APP_PIXMAP_DATA, exec_wheel_xpm,
            NULL
        },
        MENUTYPE_SEPARATOR,
        MENUTYPE_END
    };

    MenuData options_menu_uiinfo[] =
    {
        {
            MENU_TYPE_ITEM, _("_Options..."), "Ctrl+O", NULL,
            options_edit, NULL,
            GNOME_APP_PIXMAP_STOCK, GTK_STOCK_PREFERENCES,
            NULL
        },
        {
            MENU_TYPE_ITEM, _("Edit _Mime Types..."), "", NULL,
            options_edit_mime_types, NULL,
            GNOME_APP_PIXMAP_NONE, 0,
            NULL
        },
        MENUTYPE_END
    };

    MenuData connections_menu_uiinfo[] =
    {
        {
            MENU_TYPE_ITEM, _("FTP _Connect..."), "Ctrl+F", NULL,
            connections_ftp_connect, NULL,
            GNOME_APP_PIXMAP_STOCK, GTK_STOCK_CONNECT,
            NULL
        },
        {
            MENU_TYPE_ITEM, _("FTP _Quick Connect..."), "Ctrl+G", NULL,
            connections_ftp_quick_connect, NULL,
            GNOME_APP_PIXMAP_STOCK, GTK_STOCK_CONNECT,
            NULL
        },
        MENUTYPE_END
    };

    MenuData help_menu_uiinfo[] =
    {
        {
            MENU_TYPE_ITEM, _("_Contents"), "F1", NULL,
            help_help, NULL,
            GNOME_APP_PIXMAP_STOCK, GTK_STOCK_HELP,
            NULL
        },
        {
            MENU_TYPE_ITEM, _("_Keyboard Shortcuts"), "", NULL,
            help_keyboard, NULL,
            GNOME_APP_PIXMAP_NONE, 0,
            NULL
        },
        {
            MENU_TYPE_ITEM, _("GNOME Commander on the _Web"), "", NULL,
            help_web, NULL,
            GNOME_APP_PIXMAP_NONE, 0,
            NULL
        },
        {
            MENU_TYPE_ITEM, _("Report a _Problem"), "", NULL,
            help_problem, NULL,
            GNOME_APP_PIXMAP_NONE, 0,
            NULL
        },
        MENUTYPE_SEPARATOR,
        {
            MENU_TYPE_ITEM, _("_About"), "", NULL,
            help_about, NULL,
            GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT,
            NULL
        },
        MENUTYPE_END
    };

    MenuData spec = { MENU_TYPE_BASIC, "", "", "", NULL, NULL, 0, NULL, NULL };

    main_menu->priv = g_new (GnomeCmdMainMenuPrivate, 1);
    main_menu->priv->tooltips = gtk_tooltips_new ();
    main_menu->priv->bookmark_menuitems = NULL;
    main_menu->priv->connections_menuitems = NULL;
    main_menu->priv->group_menuitems = NULL;
    main_menu->priv->view_menuitems = NULL;

    //gtk_menu_bar_set_shadow_type (GTK_MENU_BAR (main_menu), GTK_SHADOW_NONE);

    spec.label = _("_File");
    main_menu->priv->file_menu = create_menu (main_menu, &spec, files_menu_uiinfo);
    gtk_menu_shell_append (GTK_MENU_SHELL (main_menu), main_menu->priv->file_menu);

    spec.label = _("_Edit");
    main_menu->priv->edit_menu = create_menu (main_menu, &spec, edit_menu_uiinfo);
    gtk_menu_shell_append (GTK_MENU_SHELL (main_menu), main_menu->priv->edit_menu);

    spec.label = _("_Mark");
    main_menu->priv->mark_menu = create_menu (main_menu, &spec, mark_menu_uiinfo);
    gtk_menu_shell_append (GTK_MENU_SHELL (main_menu), main_menu->priv->mark_menu);

    spec.label = _("_View");
    main_menu->priv->view_menu = create_menu (main_menu, &spec, view_menu_uiinfo);
    gtk_menu_shell_append (GTK_MENU_SHELL (main_menu), main_menu->priv->view_menu);
    update_view_menu (main_menu);

    spec.label = _("_Settings");
    main_menu->priv->options_menu = create_menu (main_menu, &spec, options_menu_uiinfo);
    gtk_menu_shell_append (GTK_MENU_SHELL (main_menu), main_menu->priv->options_menu);

    spec.label = _("_Connections");
    main_menu->priv->connections_menu = create_menu (main_menu, &spec, connections_menu_uiinfo);
    gtk_menu_shell_append (GTK_MENU_SHELL (main_menu), main_menu->priv->connections_menu);

    spec.label = _("_Bookmarks");
    main_menu->priv->bookmarks_menu = create_menu (main_menu, &spec, bookmarks_menu_uiinfo);
    gtk_menu_shell_append (GTK_MENU_SHELL (main_menu), main_menu->priv->bookmarks_menu);

    spec.label = _("_Plugins");
    main_menu->priv->plugins_menu = create_menu (main_menu, &spec, plugins_menu_uiinfo);
    gtk_menu_shell_append (GTK_MENU_SHELL (main_menu), main_menu->priv->plugins_menu);

    spec.label = _("_Help");
    main_menu->priv->help_menu = create_menu (main_menu, &spec, help_menu_uiinfo);
    gtk_menu_shell_append (GTK_MENU_SHELL (main_menu), main_menu->priv->help_menu);

    main_menu->priv->menu_edit_paste = edit_menu_uiinfo[2].widget;
    main_menu->priv->menu_view_conbuttons = view_menu_uiinfo[4].widget;
    main_menu->priv->menu_view_toolbar = view_menu_uiinfo[5].widget;
    main_menu->priv->menu_view_cmdline = view_menu_uiinfo[6].widget;
    main_menu->priv->menu_view_buttonbar = view_menu_uiinfo[7].widget;
    main_menu->priv->menu_view_hidden_files = view_menu_uiinfo[9].widget;
    main_menu->priv->menu_view_backup_files = view_menu_uiinfo[10].widget;
    main_menu->priv->menu_view_back = view_menu_uiinfo[0].widget;
    main_menu->priv->menu_view_forward = view_menu_uiinfo[1].widget;

    gtk_check_menu_item_set_active (
        GTK_CHECK_MENU_ITEM (main_menu->priv->menu_view_conbuttons),
        gnome_cmd_data_get_conbuttons_visibility ());
    gtk_check_menu_item_set_active (
        GTK_CHECK_MENU_ITEM (main_menu->priv->menu_view_toolbar),
        gnome_cmd_data_get_toolbar_visibility ());
    gtk_check_menu_item_set_active (
        GTK_CHECK_MENU_ITEM (main_menu->priv->menu_view_buttonbar),
        gnome_cmd_data_get_buttonbar_visibility ());
    gtk_check_menu_item_set_active (
        GTK_CHECK_MENU_ITEM (main_menu->priv->menu_view_cmdline),
        gnome_cmd_data_get_cmdline_visibility ());
    gtk_check_menu_item_set_active (
        GTK_CHECK_MENU_ITEM (main_menu->priv->menu_view_hidden_files),
        !gnome_cmd_data_get_hidden_filter ());
    gtk_check_menu_item_set_active (
        GTK_CHECK_MENU_ITEM (main_menu->priv->menu_view_backup_files),
        !gnome_cmd_data_get_backup_filter ());

    gtk_signal_connect (GTK_OBJECT (gnome_cmd_data_get_con_list ()), "list-changed",
                        GTK_SIGNAL_FUNC (on_con_list_list_changed), main_menu);

    gnome_cmd_main_menu_update_bookmarks (main_menu);
    gnome_cmd_main_menu_update_connections (main_menu);
}



/***********************************
 * Public functions
 ***********************************/

GtkWidget*
gnome_cmd_main_menu_new (void)
{
    return gtk_type_new (gnome_cmd_main_menu_get_type ());
}


GtkType
gnome_cmd_main_menu_get_type         (void)
{
    static GtkType dlg_type = 0;

    if (dlg_type == 0)
    {
        GtkTypeInfo dlg_info =
        {
            "GnomeCmdMainMenu",
            sizeof (GnomeCmdMainMenu),
            sizeof (GnomeCmdMainMenuClass),
            (GtkClassInitFunc) class_init,
            (GtkObjectInitFunc) init,
            /* reserved_1 */ NULL,
            /* reserved_2 */ NULL,
            (GtkClassInitFunc) NULL
        };

        dlg_type = gtk_type_unique (gtk_menu_bar_get_type (), &dlg_info);
    }
    return dlg_type;
}


static void
add_connection (GnomeCmdMainMenu *main_menu, GnomeCmdCon *con,
                const gchar *text, GnomeCmdPixmap *pixmap,
                GtkSignalFunc func)
{
    GtkMenuShell *connections_menu =GTK_MENU_SHELL (GTK_MENU_ITEM (main_menu->priv->connections_menu)->submenu);
    GtkWidget *item;

    item = add_menu_item (main_menu, connections_menu, text, NULL, pixmap?pixmap->pixmap:NULL, pixmap?pixmap->mask:NULL, func, con);

    main_menu->priv->connections_menuitems = g_list_append (main_menu->priv->connections_menuitems, item);
}


void
gnome_cmd_main_menu_update_connections (GnomeCmdMainMenu *main_menu)
{
    GList *all_cons, *ftp_cons, *dev_cons, *tmp;
    GnomeCmdConList *con_list;
    GtkMenuShell *connections_menu;
    gint match_count;

    g_return_if_fail (GNOME_CMD_IS_MAIN_MENU (main_menu));

    connections_menu = GTK_MENU_SHELL (GTK_MENU_ITEM (main_menu->priv->connections_menu)->submenu);
    con_list = gnome_cmd_data_get_con_list ();
    all_cons = gnome_cmd_con_list_get_all (con_list);
    ftp_cons = gnome_cmd_con_list_get_all_ftp (con_list);
    dev_cons = gnome_cmd_con_list_get_all_dev (con_list);

    /* Remove all old items */
    g_list_foreach (main_menu->priv->connections_menuitems, (GFunc)gtk_widget_destroy, NULL);
    g_list_free (main_menu->priv->connections_menuitems);
    main_menu->priv->connections_menuitems = NULL;

    /* separator */
    main_menu->priv->connections_menuitems = g_list_append (main_menu->priv->connections_menuitems, add_separator (main_menu, connections_menu));

    /* Add all open connections */
    match_count = 0;
    tmp = all_cons;
    while (tmp) {
        GnomeCmdCon *con = GNOME_CMD_CON (tmp->data);
        if (!GNOME_CMD_IS_CON_FTP (con) || gnome_cmd_con_is_open (con)) {
            add_connection (
                main_menu, con,
                gnome_cmd_con_get_go_text (con),
                gnome_cmd_con_get_go_pixmap (con),
                GTK_SIGNAL_FUNC (connections_change));
            match_count++;
        }
        tmp = tmp->next;
    }

    /* separator */
    if (match_count)
        main_menu->priv->connections_menuitems = g_list_append (
            main_menu->priv->connections_menuitems,
            add_separator (main_menu, connections_menu));

    /* Add all open connections that are not permanent */
    tmp = all_cons;
    while (tmp) {
        GnomeCmdCon *con = GNOME_CMD_CON (tmp->data);
        if (gnome_cmd_con_is_closeable (con) && gnome_cmd_con_is_open (con))
            add_connection (
                main_menu, con,
                gnome_cmd_con_get_close_text (con),
                gnome_cmd_con_get_close_pixmap (con),
                GTK_SIGNAL_FUNC (connections_close));
        tmp = tmp->next;
    }
}


void
gnome_cmd_main_menu_update_bookmarks (GnomeCmdMainMenu *main_menu)
{
    GList *cons;

    g_return_if_fail (GNOME_CMD_IS_MAIN_MENU (main_menu));

    /* Remove all old bookmark menu items */
    g_list_foreach (main_menu->priv->bookmark_menuitems, (GFunc)gtk_widget_destroy, NULL);
    g_list_free (main_menu->priv->bookmark_menuitems);
    main_menu->priv->bookmark_menuitems = NULL;

    /* Remove all old group menu items */
    g_list_foreach (main_menu->priv->group_menuitems, (GFunc)gtk_widget_destroy, NULL);
    g_list_free (main_menu->priv->group_menuitems);
    main_menu->priv->group_menuitems = NULL;

    /* Add bookmark groups */
    cons = gnome_cmd_con_list_get_all (gnome_cmd_data_get_con_list ());
    while (cons) {
        GnomeCmdCon *con = GNOME_CMD_CON (cons->data);
        GnomeCmdBookmarkGroup *group = gnome_cmd_con_get_bookmarks (con);
        GtkMenuShell *bookmarks_menu = GTK_MENU_SHELL (GTK_MENU_ITEM (main_menu->priv->bookmarks_menu)->submenu);
        if (group->bookmarks)
            add_bookmark_group (main_menu, bookmarks_menu, group);
        cons = cons->next;
    }
}


void
gnome_cmd_main_menu_update_sens (GnomeCmdMainMenu *main_menu)
{
    GnomeCmdFileSelector *fs;

    g_return_if_fail (GNOME_CMD_IS_MAIN_MENU (main_menu));

    fs = gnome_cmd_main_win_get_active_fs (main_win);

    gtk_widget_set_sensitive (main_menu->priv->menu_view_back, gnome_cmd_file_selector_can_back (fs));
    gtk_widget_set_sensitive (main_menu->priv->menu_view_forward, gnome_cmd_file_selector_can_forward (fs));
}


static void
on_plugin_menu_activate (GtkMenuItem *item, PluginData *data)
{
    GnomeCmdState *state;

    g_return_if_fail (data != NULL);

    state = gnome_cmd_main_win_get_state (main_win);
    gnome_cmd_plugin_update_main_menu_state (data->plugin, state);
}


void
gnome_cmd_main_menu_add_plugin_menu (GnomeCmdMainMenu *main_menu, PluginData *data)
{
    gtk_menu_shell_append (GTK_MENU_SHELL (GTK_MENU_ITEM (main_menu->priv->plugins_menu)->submenu), data->menu);

    gtk_signal_connect (GTK_OBJECT (data->menu), "activate", GTK_SIGNAL_FUNC (on_plugin_menu_activate), data);
}
