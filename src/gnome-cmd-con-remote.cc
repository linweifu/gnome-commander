/** 
 * @file gnome-cmd-con-remote.cc
 * @copyright (C) 2001-2006 Marcus Bjurman\n
 * @copyright (C) 2007-2012 Piotr Eljasiak\n
 * @copyright (C) 2013-2018 Uwe Scholz\n
 *
 * @copyright This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * @copyright This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * @copyright You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <config.h>

#include "gnome-cmd-includes.h"
#include "gnome-cmd-data.h"
#include "gnome-cmd-con-remote.h"
#include "gnome-cmd-plain-path.h"
#include "imageloader.h"
#include "utils.h"

using namespace std;


static GnomeCmdConClass *parent_class = NULL;


static void get_file_info_func (GnomeCmdCon *con)
{
    GnomeVFSURI *uri = gnome_cmd_con_create_uri (con, con->base_path);

    GnomeVFSFileInfoOptions infoOpts = (GnomeVFSFileInfoOptions) (GNOME_VFS_FILE_INFO_FOLLOW_LINKS | GNOME_VFS_FILE_INFO_GET_MIME_TYPE | GNOME_VFS_FILE_INFO_FORCE_FAST_MIME_TYPE);

    DEBUG('m', "Connecting to %s\n", gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_PASSWORD));

    // Get basic file info - opens gnome-keyring dialog via libgnome-keyring for password input if needed
    con->base_info = gnome_vfs_file_info_new ();
    GnomeVFSResult res = gnome_vfs_get_file_info_uri (uri, con->base_info, infoOpts);

    gnome_vfs_uri_unref (uri);

    if (con->state == GnomeCmdCon::STATE_OPENING)
    {
        DEBUG('m', "State was OPENING, setting flags\n");
        if (res == GNOME_VFS_OK)
        {
            con->state = GnomeCmdCon::STATE_OPEN;
            con->open_result = GnomeCmdCon::OPEN_OK;
        }
        else
        {
            con->state = GnomeCmdCon::STATE_CLOSED;
            con->open_failed_reason = res;
            con->open_result = GnomeCmdCon::OPEN_FAILED;
        }
    }
    else
        if (con->state == GnomeCmdCon::STATE_CANCELLING)
            DEBUG('m', "The open operation was cancelled, doing nothing\n");
        else
            DEBUG('m', "Strange ConState %d\n", con->state);
}


static gboolean start_get_file_info (GnomeCmdCon *con)
{
    g_thread_new (NULL, (GThreadFunc) get_file_info_func, con);

    return FALSE;
}


static void remote_open (GnomeCmdCon *con)
{
    DEBUG('m', "Opening remote connection\n");

    g_return_if_fail (con->uri!=NULL);

    con->state = GnomeCmdCon::STATE_OPENING;
    con->open_result = GnomeCmdCon::OPEN_IN_PROGRESS;

    if (!con->base_path)
        con->base_path = new GnomeCmdPlainPath(G_DIR_SEPARATOR_S);

    g_timeout_add (1, (GSourceFunc) start_get_file_info, con);
}


static gboolean remote_close (GnomeCmdCon *con)
{
    gnome_cmd_con_set_default_dir (con, NULL);
    delete con->base_path;
    con->base_path = NULL;
    con->state = GnomeCmdCon::STATE_CLOSED;
    con->open_result = GnomeCmdCon::OPEN_NOT_STARTED;

    return TRUE;
}


static void remote_cancel_open (GnomeCmdCon *con)
{
    DEBUG('m', "Setting state CANCELLING\n");
    con->state = GnomeCmdCon::STATE_CANCELLING;
}


static gboolean remote_open_is_needed (GnomeCmdCon *con)
{
    return TRUE;
}


static GnomeVFSURI *remote_create_uri (GnomeCmdCon *con, GnomeCmdPath *path)
{
    g_return_val_if_fail (con->uri != NULL, NULL);

    GnomeVFSURI *u0 = gnome_vfs_uri_new (con->uri);
    GnomeVFSURI *u1 = gnome_vfs_uri_append_path (u0, path->get_path());

    gnome_vfs_uri_unref (u0);

    return u1;
}


static GnomeCmdPath *remote_create_path (GnomeCmdCon *con, const gchar *path_str)
{
    return new GnomeCmdPlainPath(path_str);
}


/*******************************
 * Gtk class implementation
 *******************************/

static void destroy (GtkObject *object)
{
    if (GTK_OBJECT_CLASS (parent_class)->destroy)
        (*GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}


static void class_init (GnomeCmdConRemoteClass *klass)
{
    GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);
    GnomeCmdConClass *con_class = GNOME_CMD_CON_CLASS (klass);

    parent_class = (GnomeCmdConClass *) gtk_type_class (GNOME_CMD_TYPE_CON);

    object_class->destroy = destroy;

    con_class->open = remote_open;
    con_class->close = remote_close;
    con_class->cancel_open = remote_cancel_open;
    con_class->open_is_needed = remote_open_is_needed;
    con_class->create_uri = remote_create_uri;
    con_class->create_path = remote_create_path;
}


static void init (GnomeCmdConRemote *remote_con)
{
    guint dev_icon_size = gnome_cmd_data.dev_icon_size;
    gint icon_size;

    g_assert (gtk_icon_size_lookup (GTK_ICON_SIZE_LARGE_TOOLBAR, &icon_size, NULL));

    GnomeCmdCon *con = GNOME_CMD_CON (remote_con);

    con->method = CON_FTP;
    con->should_remember_dir = TRUE;
    con->needs_open_visprog = TRUE;
    con->needs_list_visprog = TRUE;
    con->can_show_free_space = FALSE;
    con->is_local = FALSE;
    con->is_closeable = TRUE;
    con->go_pixmap = gnome_cmd_pixmap_new_from_icon (gnome_cmd_con_get_icon_name (con), dev_icon_size);
    con->open_pixmap = gnome_cmd_pixmap_new_from_icon (gnome_cmd_con_get_icon_name (con), dev_icon_size);
    con->close_pixmap = gnome_cmd_pixmap_new_from_icon (gnome_cmd_con_get_icon_name (con), icon_size);

    if (con->close_pixmap)
    {
        GdkPixbuf *overlay = gdk_pixbuf_copy (con->close_pixmap->pixbuf);

        if (overlay)
        {
            GdkPixbuf *umount = IMAGE_get_pixbuf (PIXMAP_OVERLAY_UMOUNT);

            if (umount)
            {
                gdk_pixbuf_copy_area (umount, 0, 0,
                                      MIN (gdk_pixbuf_get_width (umount), icon_size),
                                      MIN (gdk_pixbuf_get_height (umount), icon_size),
                                      overlay, 0, 0);
                // FIXME: free con->close_pixmap here
                con->close_pixmap = gnome_cmd_pixmap_new_from_pixbuf (overlay);
            }

            g_object_unref (overlay);
        }
    }
}



/***********************************
 * Public functions
 ***********************************/

GtkType gnome_cmd_con_remote_get_type ()
{
    static GtkType type = 0;

    if (type == 0)
    {
        GtkTypeInfo info =
        {
            (gchar*) "GnomeCmdConRemote",
            sizeof (GnomeCmdConRemote),
            sizeof (GnomeCmdConRemoteClass),
            (GtkClassInitFunc) class_init,
            (GtkObjectInitFunc) init,
            /* reserved_1 */ NULL,
            /* reserved_2 */ NULL,
            (GtkClassInitFunc) NULL
        };

        type = gtk_type_unique (GNOME_CMD_TYPE_CON, &info);
    }
    return type;
}

/**
 * Logic for setting up a new remote connection accordingly to the given uri_str.
 */
GnomeCmdConRemote *gnome_cmd_con_remote_new (const gchar *alias, const string &uri_str, GnomeCmdCon::Authentication auth)
{
    gchar *canonical_uri = gnome_vfs_make_uri_canonical (uri_str.c_str());

    GnomeVFSURI *uri = gnome_vfs_uri_new (canonical_uri);

    g_return_val_if_fail (uri != NULL, NULL);

    GnomeCmdConRemote *server = (GnomeCmdConRemote *) g_object_new (GNOME_CMD_TYPE_CON_REMOTE, NULL);

    g_return_val_if_fail (server != NULL, NULL);

    const gchar *host = gnome_vfs_uri_get_host_name (uri);      // do not g_free
    const gchar *password = gnome_vfs_uri_get_password (uri);   // do not g_free
    gchar *path = gnome_vfs_unescape_string (gnome_vfs_uri_get_path (uri), NULL);

    GnomeCmdCon *con = GNOME_CMD_CON (server);

    gnome_cmd_con_set_alias (con, alias);
    gnome_cmd_con_set_uri (con, canonical_uri);
    gnome_cmd_con_set_host_name (con, host);
    gnome_cmd_con_set_root_path (con, path);

    gnome_cmd_con_remote_set_host_name (server, host);

    con->method = gnome_cmd_con_get_scheme (uri);
    con->auth = con->method==CON_ANON_FTP ? GnomeCmdCon::NOT_REQUIRED : password ? GnomeCmdCon::SAVE_FOR_SESSION : GnomeCmdCon::SAVE_PERMANENTLY;

    g_free (path);
    gnome_vfs_uri_unref (uri);

    return server;
}


void gnome_cmd_con_remote_set_host_name (GnomeCmdConRemote *con, const gchar *host_name)
{
    g_return_if_fail (con != NULL);
    g_return_if_fail (host_name != NULL);

    GNOME_CMD_CON (con)->open_tooltip = g_strdup_printf (_("Opens remote connection to %s"), host_name);
    GNOME_CMD_CON (con)->close_tooltip = g_strdup_printf (_("Closes remote connection to %s"), host_name);
}
