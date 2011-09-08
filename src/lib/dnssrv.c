/* dnssrv.c - Copyright (C) 2011 Red Hat, Inc.
 * Written by Russell Bryant <rbryant@redhat.com>
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

#ifndef WIN32
#include "config.h"
#endif

#include <assert.h>
#include <glib.h>

#include "matahari/utilities.h"
#include "matahari/dnssrv.h"
#include "dnssrv_private.h"


struct mh_dnssrv_record {
    uint16_t port;
    uint16_t priority;
    uint16_t weight;
    /** Used in the weight sorting algorithm */
    uint32_t weight_running_sum;
    char host[1];
};


const char *
mh_dnssrv_record_get_host(const struct mh_dnssrv_record *record)
{
    return record->host;
}

uint16_t
mh_dnssrv_record_get_port(const struct mh_dnssrv_record *record)
{
    return record->port;
}

uint16_t
mh_dnssrv_record_get_priority(const struct mh_dnssrv_record *record)
{
    return record->priority;
}

uint16_t
mh_dnssrv_record_get_weight(const struct mh_dnssrv_record *record)
{
    return record->weight;
}

static gint
dnssrv_record_priority_cmp(gconstpointer _a, gconstpointer _b)
{
    const struct mh_dnssrv_record *a = _a;
    const struct mh_dnssrv_record *b = _b;

    return (gint) (a->priority - b->priority);
}

static void
add_weight(gpointer data, gpointer user_data)
{
    struct mh_dnssrv_record *record = data;
    uint32_t *sum_weights = user_data;

    *sum_weights += record->weight;

    record->weight_running_sum = *sum_weights;
}

static GList *
apply_weight_sort(GList *records)
{
    GList *processed_records = NULL;

    while (records) {
        uint32_t sum_weights = 0;
        uint32_t magic;
        GList *cur;

        g_list_foreach(records, add_weight, &sum_weights);

        magic = g_random_int_range(0, sum_weights + 1);

        for (cur = records; cur; cur = cur->next) {
            struct mh_dnssrv_record *cur_record = cur->data;

            if (cur_record->weight_running_sum >= magic) {
                processed_records = g_list_append(processed_records, cur_record);
                records = g_list_delete_link(records, cur);
                break;
            }
        }
    }

    return processed_records;
}

GList *
mh_dnssrv_records_sort(GList *records)
{
    GList *processed_records = NULL;

    /*
     * At this point we have a list of records sorted by priority.
     * There is also a weight field.  Oh, so the weight is just a
     * secondary sort key?  No.  That would be too simple.
     *
     * For each priority, we have to sort by weight using the algorithm
     * described in RFC 2782.  Specifically, see the section:
     *     "The format of the SRV RR" -> Weight
     */

    while (records) {
        GList *tmp_list = NULL;
        uint16_t cur_priority;

        /*
         * Move all records at this priority into a temporary list.
         * We will then sort them based on weights before moving them
         * into the list of processed records.
         */

        cur_priority = ((struct mh_dnssrv_record *) g_list_nth_data(records, 0))->priority;
        do {
            struct mh_dnssrv_record *record = g_list_nth_data(records, 0);

            if (record->priority != cur_priority) {
                break;
            }

            records = g_list_remove(records, record);

            /*
             * Records with a weight of 0 must be at the beginning of the list.
             * The rest don't matter.
             */

            if (record->weight == 0) {
                tmp_list = g_list_prepend(tmp_list, record);
            } else {
                tmp_list = g_list_append(tmp_list, record);
            }
        } while (records);

        tmp_list = apply_weight_sort(tmp_list);

        processed_records = g_list_concat(processed_records, tmp_list);
    }

    return processed_records;
}

GList *
mh_dnssrv_lookup(const char *query)
{
    return mh_dnssrv_records_sort(mh_os_dnssrv_lookup(query));
}

int
mh_dnssrv_lookup_single(const char *query, char *host, size_t host_len, uint16_t *port)
{
    GList *records;
    struct mh_dnssrv_record *record;

    records = mh_dnssrv_lookup(query);

    if (!records) {
        return -1;
    }

    record = g_list_nth_data(records, 0);

    if (record) {
        strncpy(host, record->host, host_len);
        host[host_len - 1] = '\0';
        *port = record->port;
    }

    g_list_free_full(records, mh_dnssrv_record_free);

    return record ? 0 : -1;
}

GList *
mh_dnssrv_add_record(GList *records, const char *host, uint16_t port,
                     uint16_t priority, uint16_t weight)
{
    struct mh_dnssrv_record *record;
    size_t host_len;

    host_len = strlen(host);

    record = malloc(sizeof(*record) + host_len);

    assert(record != NULL);

    record->port = port;
    record->priority = priority;
    record->weight = weight;
    memcpy(record->host, host, host_len);
    record->host[host_len] = '\0';

    return g_list_insert_sorted(records, record, dnssrv_record_priority_cmp);
}


void
mh_dnssrv_record_free(gpointer data)
{
    struct dnssrv_record *record = data;

    free(record);
}
