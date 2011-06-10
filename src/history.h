/*
    GNOME Commander - A GNOME based file manager
    Copyright (C) 2001-2006 Marcus Bjurman
    Copyright (C) 2007-2011 Piotr Eljasiak

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

#ifndef __HISTORY_H__
#define __HISTORY_H__

class History
{
    gint max;
    gboolean is_locked;

  public:

    GList *ents;
    GList *pos;

    History(gint max): is_locked(FALSE), ents(NULL), pos(NULL)       {  this->max = max;  }
    ~History();

    History &operator = (GList *list);

    guint size()                                         {  return g_list_length (ents);  }
    gboolean empty()                                     {  return ents==NULL;            }

    const gchar *front()          {  return empty() ? NULL : (const gchar *) ents->data;  }

    void add(const gchar *text);
    void reverse()                               {  pos = ents = g_list_reverse (ents);  }

    gboolean can_back()                                     {  return pos && pos->next;  }
    gboolean can_forward()                                  {  return pos && pos->prev;  }

    const gchar *first();
    const gchar *back();
    const gchar *forward();
    const gchar *last();

    void lock()                                             {  is_locked = TRUE;         }
    void unlock()                                           {  is_locked = FALSE;        }

    gboolean locked() const                                 {  return is_locked;         }
};

inline History &History::operator = (GList *list)
{
    if (ents)
    {
        g_list_foreach (ents, (GFunc) g_free, NULL);
        g_list_free (ents);
    }

    pos = ents = list;      //  FIXME: no check for max and dups...

    return *this;
}

#endif // __HISTORY_H__
