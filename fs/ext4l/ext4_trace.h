/* SPDX-License-Identifier: GPL-2.0 */
/*
 * ext4 and jbd2 trace stubs for U-Boot
 *
 * In Linux, these are defined as trace events via include/trace/events/ext4.h
 * and include/trace/events/jbd2.h. In U-Boot, we stub them out completely.
 */

#ifndef _EXT4_TRACE_H
#define _EXT4_TRACE_H

/* ext4 journal traces */
#define trace_ext4_journal_start_inode(...)		do { } while (0)
#define trace_ext4_journal_start_sb(...)		do { } while (0)
#define trace_ext4_journal_start_reserved(...)		do { } while (0)
#define trace_ext4_forget(...)				do { } while (0)

/* ext4 bitmap traces */
#define trace_ext4_read_block_bitmap_load(...)		do { } while (0)
#define trace_ext4_load_inode_bitmap(...)		do { } while (0)

/* ext4 inode allocation traces */
#define trace_ext4_free_inode(...)			do { } while (0)
#define trace_ext4_allocate_inode(...)			do { } while (0)
#define trace_ext4_request_inode(...)			do { } while (0)

/* ext4 extent traces */
#define trace_ext4_ext_load_extent(...)			do { } while (0)
#define trace_ext4_ext_rm_idx(...)			do { } while (0)
#define trace_ext4_remove_blocks(...)			do { } while (0)
#define trace_ext4_ext_rm_leaf(...)			do { } while (0)
#define trace_ext4_ext_remove_space(...)		do { } while (0)
#define trace_ext4_ext_remove_space_done(...)		do { } while (0)
#define trace_ext4_ext_convert_to_initialized_enter(...)	do { } while (0)
#define trace_ext4_ext_convert_to_initialized_fastpath(...)	do { } while (0)
#define trace_ext4_ext_handle_unwritten_extents(...)	do { } while (0)
#define trace_ext4_get_implied_cluster_alloc_exit(...)	do { } while (0)
#define trace_ext4_ext_map_blocks_enter(...)		do { } while (0)
#define trace_ext4_ext_map_blocks_exit(...)		do { } while (0)
#define trace_ext4_ext_show_extent(...)			do { } while (0)

/* ext4 fallocate traces */
#define trace_ext4_collapse_range(...)			do { } while (0)
#define trace_ext4_insert_range(...)			do { } while (0)
#define trace_ext4_zero_range(...)			do { } while (0)
#define trace_ext4_fallocate_enter(...)			do { } while (0)
#define trace_ext4_fallocate_exit(...)			do { } while (0)

/* ext4 indirect block traces */
#define trace_ext4_ind_map_blocks_enter(...)		do { } while (0)
#define trace_ext4_ind_map_blocks_exit(...)		do { } while (0)

/* ext4 inode traces */
#define trace_ext4_begin_ordered_truncate(...)		do { } while (0)
#define trace_ext4_evict_inode(...)			do { } while (0)
#define trace_ext4_load_inode(...)			do { } while (0)
#define trace_ext4_other_inode_update_time(...)		do { } while (0)
#define trace_ext4_mark_inode_dirty(...)		do { } while (0)
#define trace_ext4_drop_inode(...)			do { } while (0)
#define trace_ext4_nfs_commit_metadata(...)		do { } while (0)

/* ext4 delayed allocation traces */
#define trace_ext4_da_update_reserve_space(...)		do { } while (0)
#define trace_ext4_da_reserve_space(...)		do { } while (0)
#define trace_ext4_da_release_space(...)		do { } while (0)
#define trace_ext4_da_write_pages_extent(...)		do { } while (0)
#define trace_ext4_alloc_da_blocks(...)			do { } while (0)

/* ext4 writeback traces */
#define trace_ext4_writepages(...)			do { } while (0)
#define trace_ext4_da_write_folios_start(...)		do { } while (0)
#define trace_ext4_da_write_folios_end(...)		do { } while (0)
#define trace_ext4_writepages_result(...)		do { } while (0)
#define trace_ext4_da_write_begin(...)			do { } while (0)
#define trace_ext4_da_write_end(...)			do { } while (0)
#define trace_ext4_write_begin(...)			do { } while (0)
#define trace_ext4_write_end(...)			do { } while (0)
#define trace_ext4_journalled_write_end(...)		do { } while (0)

/* ext4 folio traces */
#define trace_ext4_read_folio(...)			do { } while (0)
#define trace_ext4_invalidate_folio(...)		do { } while (0)
#define trace_ext4_journalled_invalidate_folio(...)	do { } while (0)
#define trace_ext4_release_folio(...)			do { } while (0)

/* ext4 truncate traces */
#define trace_ext4_punch_hole(...)			do { } while (0)
#define trace_ext4_truncate_enter(...)			do { } while (0)
#define trace_ext4_truncate_exit(...)			do { } while (0)

/* ext4 sync traces */
#define trace_ext4_sync_file_enter(...)			do { } while (0)
#define trace_ext4_sync_file_exit(...)			do { } while (0)
#define trace_ext4_sync_fs(...)				do { } while (0)

/* ext4 unlink traces */
#define trace_ext4_unlink_enter(...)			do { } while (0)
#define trace_ext4_unlink_exit(...)			do { } while (0)

/* ext4 super traces */
#define trace_ext4_prefetch_bitmaps(...)		do { } while (0)
#define trace_ext4_lazy_itable_init(...)		do { } while (0)
/* trace_ext4_error is a function implemented in stub.c, not a trace stub */

/* ext4 mballoc traces */
#define trace_ext4_mb_bitmap_load(...)			do { } while (0)
#define trace_ext4_mb_buddy_bitmap_load(...)		do { } while (0)
#define trace_ext4_mballoc_alloc(...)			do { } while (0)
#define trace_ext4_mballoc_prealloc(...)		do { } while (0)
#define trace_ext4_mballoc_discard(...)			do { } while (0)
#define trace_ext4_mballoc_free(...)			do { } while (0)
#define trace_ext4_mb_release_inode_pa(...)		do { } while (0)
#define trace_ext4_mb_release_group_pa(...)		do { } while (0)
#define trace_ext4_mb_new_inode_pa(...)			do { } while (0)
#define trace_ext4_mb_new_group_pa(...)			do { } while (0)
#define trace_ext4_discard_blocks(...)			do { } while (0)
#define trace_ext4_discard_preallocations(...)		do { } while (0)
#define trace_ext4_mb_discard_preallocations(...)	do { } while (0)
#define trace_ext4_request_blocks(...)			do { } while (0)
#define trace_ext4_allocate_blocks(...)			do { } while (0)
#define trace_ext4_free_blocks(...)			do { } while (0)
#define trace_ext4_trim_extent(...)			do { } while (0)
#define trace_ext4_trim_all_free(...)			do { } while (0)

/* ext4 fast commit traces */
#define trace_ext4_fc_track_unlink(...)			do { } while (0)
#define trace_ext4_fc_track_link(...)			do { } while (0)
#define trace_ext4_fc_track_create(...)			do { } while (0)
#define trace_ext4_fc_track_inode(...)			do { } while (0)
#define trace_ext4_fc_track_range(...)			do { } while (0)
#define trace_ext4_fc_cleanup(...)			do { } while (0)
#define trace_ext4_fc_stats(...)			do { } while (0)
#define trace_ext4_fc_commit_start(...)			do { } while (0)
#define trace_ext4_fc_commit_stop(...)			do { } while (0)
#define trace_ext4_fc_replay_scan(...)			do { } while (0)
#define trace_ext4_fc_replay(...)			do { } while (0)

/* ext4 fsmap traces */
#define trace_ext4_fsmap_mapping(...)			do { } while (0)
#define trace_ext4_fsmap_low_key(...)			do { } while (0)
#define trace_ext4_fsmap_high_key(...)			do { } while (0)

/* jbd2 checkpoint traces */
#define trace_jbd2_checkpoint(...)			do { } while (0)
#define trace_jbd2_shrink_checkpoint_list(...)		do { } while (0)
#define trace_jbd2_checkpoint_stats(...)		do { } while (0)
#define trace_jbd2_drop_transaction(...)		do { } while (0)

/* jbd2 commit traces */
#define trace_jbd2_submit_inode_data(...)		do { } while (0)
#define trace_jbd2_start_commit(...)			do { } while (0)
#define trace_jbd2_commit_locking(...)			do { } while (0)
#define trace_jbd2_commit_flushing(...)			do { } while (0)
#define trace_jbd2_commit_logging(...)			do { } while (0)
#define trace_jbd2_run_stats(...)			do { } while (0)
#define trace_jbd2_end_commit(...)			do { } while (0)

/* jbd2 handle traces */
#define trace_jbd2_handle_start(...)			do { } while (0)
#define trace_jbd2_handle_extend(...)			do { } while (0)
#define trace_jbd2_handle_restart(...)			do { } while (0)
#define trace_jbd2_handle_stats(...)			do { } while (0)
#define trace_jbd2_lock_buffer_stall(...)		do { } while (0)

/* jbd2 journal traces */
#define trace_jbd2_update_log_tail(...)			do { } while (0)
#define trace_jbd2_shrink_scan_enter(...)		do { } while (0)
#define trace_jbd2_shrink_scan_exit(...)		do { } while (0)
#define trace_jbd2_shrink_count(...)			do { } while (0)
#define trace_jbd2_write_superblock(...)		do { } while (0)

#endif /* _EXT4_TRACE_H */
