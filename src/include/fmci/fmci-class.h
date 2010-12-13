/*
 * fmci-class.h
 *
 * Copyright (C) 2010 Red Hat, Inc.
 * Written by Roman Rakus <rrakus@redhat.com>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef FMCICLASS_H_
#define FMCICLASS_H_

#define FMCI_TYPE              (fmci_get_type())
#define FMCI(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), FMCI_TYPE, Fmci))
#define FMCI_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), FMCI_TYPE, FmciClass))
#define IS_FMCI(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), FMCI_TYPE))
#define IS_FMCI_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), FMCI_TYPE))
#define FMCI_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), FMCI_TYPE, FmciClass))

typedef struct _Fmci Fmci;
typedef struct _FmciClass FmciClass;

GType
fmci_get_type(void);

struct _Fmci
{
  GObject parent;
};

struct _FmciClass
{
  GObjectClass parent;
};

gboolean
fmci_version(Fmci *fmci, char** version, GError** error);

#endif /* FMCICLASS_H_ */
