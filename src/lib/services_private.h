/*
 * Copyright (C) 2010 - 2011, Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __MH_SERVICES_PRIVATE_H__
#define __MH_SERVICES_PRIVATE_H__

struct svc_action_private_s {
    char *exec;
    char *args[7];

    guint repeat_timer;
    void (*callback)(svc_action_t *op);

    int            stderr_fd;
    mainloop_fd_t *stderr_gsource;

    int            stdout_fd;
    mainloop_fd_t *stdout_gsource;
};

GList *
services_os_get_directory_list(const char *root, gboolean files);

gboolean
services_os_action_execute(svc_action_t *op, gboolean synchronous);

void
services_os_set_exec(svc_action_t *op);

GList *
services_os_list(void);

GList *
resources_os_list_ocf_providers(void);

GList *
resources_os_list_ocf_agents(const char *provider);

GList *
resources_os_list_systemd_services(void);

#endif /* __MH_SERVICES_PRIVATE_H__ */
