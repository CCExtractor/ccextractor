//
// Created by Oleg Kisselef (olegkisselef at gmail dot com) on 6/21/15
//

#include <stdio.h>
#include <stdlib.h>
#include "ccx_share.h"
#include "ccx_decoders_structs.h"
#include "lib_ccx.h"

void ccx_sub_entry_init(ccx_sub_entry *entry)
{
    entry->counter = 0;
    entry->start_time = 0;
    entry->end_time = 0;
    entry->lines = NULL;
    entry->lines_count = 0;
}

void ccx_sub_entry_cleanup(ccx_sub_entry *entry)
{
    for (int i = 0; i < entry->lines_count; i++) {
        free(entry->lines[i]);
    }
    free(entry->lines);
}

void ccx_sub_entry_print(ccx_sub_entry *entry)
{
    if (!entry) {
        dbg_print(CCX_DMT_SHARE, "[share] !entry\n");
        return;
    }

    dbg_print(CCX_DMT_SHARE, "\n[share] sub entry #%lu\n", entry->counter);
    dbg_print(CCX_DMT_SHARE, "[share] start: %llu\n", entry->start_time);
    dbg_print(CCX_DMT_SHARE, "[share] end: %llu\n", entry->end_time);

    dbg_print(CCX_DMT_SHARE, "[share] lines count: %d\n", entry->lines_count);
    if (!entry->lines) {
        dbg_print(CCX_DMT_SHARE, "[share] no lines allocated\n");
        return;
    }

    dbg_print(CCX_DMT_SHARE, "[share] lines:\n");
    for (unsigned int i = 0; i < entry->lines_count; i++) {
        if (!entry->lines[i]) {
            dbg_print(CCX_DMT_SHARE, "[share] line[%d] is not allocated\n", i);
        }
        dbg_print(CCX_DMT_SHARE, "[share] %s\n", entry->lines[i]);
    }
}

void ccx_sub_entries_init(ccx_sub_entries *entries)
{
    entries->count = 0;
    entries->entries = NULL;
}

void ccx_sub_entries_cleanup(ccx_sub_entries *entries)
{
    for (int i = 0; i < entries->count; i++) {
        ccx_sub_entry_cleanup(&(entries->entries[i]));
    }
    free(entries->entries);
    entries->entries = NULL;
    entries->count = 0;
}

void ccx_sub_entries_print(ccx_sub_entries *entries)
{
    dbg_print(CCX_DMT_SHARE, "[share] ccx_sub_entries_print (%u entries)\n", entries->count);
    for (int i = 0; i < entries->count; i++) {
        ccx_sub_entry_print(&(entries->entries[i]));
    }
}


ccx_share_status ccx_share_start();
ccx_share_status ccx_share_stop();
ccx_share_status ccx_share_file_start(ccx_share_file *);
ccx_share_status ccx_share_file_done();
ccx_share_status ccx_share_send(struct cc_subtitle *sub)
{
    ccx_sub_entries entries;
    ccx_sub_entries_init(&entries);
    _ccx_share_sub_to_entry(sub, &entries);
    ccx_sub_entries_print(&entries);
    ccx_sub_entries_cleanup(&entries);
    return CCX_SHARE_OK;
}

///**
//* Raw Subtitle struct used as output of decoder (cc608)
//* and input for encoder (sami, srt, transcript or smptett etc)
//*/
//struct cc_subtitle
//{
//    /**
//    * A generic data which contain data according to decoder
//    * @warn decoder cant output multiple types of data
//    */
//    void *data;
//    /** number of data */
//    unsigned int nb_data;
//    /**  type of subtitle */
//    enum subtype type;
//    /* set only when all the data is to be displayed at same time */
//    LLONG start_time;
//    LLONG end_time;
//    /* flags */
//    int flags;
//    /* index of language table */
//    int lang_index;
//    /** flag to tell that decoder has given output */
//    int got_output;
//};

ccx_share_status _ccx_share_sub_to_entry(struct cc_subtitle *sub, ccx_sub_entries *ents)
{
    dbg_print(CCX_DMT_SHARE, "\n[share] _ccx_share_sub_to_entry\n");
    if (sub->type == CC_608) {
        dbg_print(CCX_DMT_SHARE, "[share] CC_608\n");
        struct eia608_screen *data;
        unsigned int nb_data = sub->nb_data;
        for (data = sub->data; nb_data; nb_data--, data++) {
            dbg_print(CCX_DMT_SHARE, "[share] data item\n");
            if (data->format == SFORMAT_XDS) {
                dbg_print(CCX_DMT_SHARE, "[share] XDS. Skipping\n");
                continue;
            }
            if (!data->start_time) {
                dbg_print(CCX_DMT_SHARE, "[share] No start time. Skipping\n");
                break;
            }
            ents->entries = realloc(ents->entries, ++ents->count * sizeof(ccx_sub_entry));
            if (!ents->entries) {
                fatal(EXIT_NOT_ENOUGH_MEMORY, "_ccx_share_sub_to_entry\n");
            }
            unsigned int entry_index = ents->count - 1;
            ents->entries[entry_index].lines_count = 0;
            for (int i = 0; i < 15; i++) {
                if (data->row_used[i]) {
                    ents->entries[entry_index].lines_count++;
                }
            }
            if (!ents->entries[entry_index].lines_count) {// Prevent writing empty screens. Not needed in .srt
                dbg_print(CCX_DMT_SHARE, "[share] buffer is empty\n");
                dbg_print(CCX_DMT_SHARE, "[share] done\n");
                return CCX_SHARE_OK;
            }
            ents->entries[entry_index].lines = (char **) malloc(ents->entries[entry_index].lines_count * sizeof(char *));
            if (!ents->entries[entry_index].lines) {
                fatal(EXIT_NOT_ENOUGH_MEMORY, "_ccx_share_sub_to_entry: ents->entries[entry_index].lines\n");
            }

            dbg_print(CCX_DMT_SHARE, "[share] Copying %u lines\n", ents->entries[entry_index].lines_count);
            int i = 0, j = 0;
            while (i < 15) {
                if (data->row_used[i]) {
                    ents->entries[entry_index].lines[j] =
                            (char *) malloc((strnlen((char *) data->characters[i], 32) + 1) * sizeof(char));
                    if (!ents->entries[entry_index].lines[j]) {
                        fatal(EXIT_NOT_ENOUGH_MEMORY, "_ccx_share_sub_to_entry: ents->entries[entry_index].lines[j]\n");
                    }
                    strncpy(ents->entries[entry_index].lines[j], (char *) data->characters[i], 32);
                    ents->entries[entry_index].lines[j][strnlen((char *) data->characters[i], 32)] = '\0';
                    dbg_print(CCX_DMT_SHARE, "[share] line (len=%zd): %s\n",
                              strlen(ents->entries[entry_index].lines[j]),
                              ents->entries[entry_index].lines[j]);
                    j++;
                }
                i++;
            }
            ents->entries[entry_index].start_time = (unsigned long long) data->start_time;
            ents->entries[entry_index].end_time = (unsigned long long) data->end_time;
            ents->entries[entry_index].counter = ++ccx_share.counter;
            dbg_print(CCX_DMT_SHARE, "[share] item done\n");
        }
    }
    if (sub->type == CC_BITMAP) {
        dbg_print(CCX_DMT_SHARE, "[share] CC_BITMAP. Skipping\n");
    }
    if (sub->type == CC_RAW) {
        dbg_print(CCX_DMT_SHARE, "[share] CC_RAW. Skipping\n");
    }
    if (sub->type == CC_TEXT) {
        dbg_print(CCX_DMT_SHARE, "[share] CC_TEXT. Skipping\n");
    }
    dbg_print(CCX_DMT_SHARE, "[share] done\n");

    return CCX_SHARE_OK;
}