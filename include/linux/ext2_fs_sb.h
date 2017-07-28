/*
 *  linux/include/linux/ext2_fs_sb.h
 *
 * Copyright (C) 1992, 1993, 1994, 1995
 * Remy Card (card@masi.ibp.fr)
 * Laboratoire MASI - Institut Blaise Pascal
 * Universite Pierre et Marie Curie (Paris VI)
 *
 *  from
 *
 *  linux/include/linux/minix_fs_sb.h
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#ifndef _LINUX_EXT2_FS_SB
#define _LINUX_EXT2_FS_SB

#include <linux/blockgroup_lock.h>
#include <linux/percpu_counter.h>

/*
 * second extended-fs super-block data in memory
 */
/**
 * �ڴ��е�ext2������
 */
struct ext2_sb_info {
	unsigned long s_frag_size;	/* Size of a fragment in bytes */
	unsigned long s_frags_per_block;/* Number of fragments per block */
	unsigned long s_inodes_per_block;/* Number of inodes per block */
	unsigned long s_frags_per_group;/* Number of fragments in a group */
	unsigned long s_blocks_per_group;/* Number of blocks in a group */
	unsigned long s_inodes_per_group;/* Number of inodes in a group */
	unsigned long s_itb_per_group;	/* Number of inode table blocks per group */
	unsigned long s_gdb_count;	/* Number of group descriptor blocks */
	/**
	 * ���Է���һ�����е���������������
	 */
	unsigned long s_desc_per_block;	/* Number of group descriptors per block */
	unsigned long s_groups_count;	/* Number of groups in the fs */
	/**
	 * ָ��������̳�����Ļ������Ļ������ײ���
	 */
	struct buffer_head * s_sbh;	/* Buffer containing the super block */
	/**
	 * ָ����̳����顣
	 */
	struct ext2_super_block * s_es;	/* Pointer to the super block in the buffer */
	/**
	 * ָ��һ���������������Ļ������ײ�����
	 */
	struct buffer_head ** s_group_desc;
	unsigned long  s_mount_opt;
	uid_t s_resuid;
	gid_t s_resgid;
	unsigned short s_mount_state;
	unsigned short s_pad;
	int s_addr_per_block_bits;	// һ���������������� λ��������СΪ4096������������Ϊ1024, λ��Ϊ10
	int s_desc_per_block_bits;
	int s_inode_size;
	int s_first_ino;
	spinlock_t s_next_gen_lock;
	u32 s_next_generation;
	unsigned long s_dir_count;
	u8 *s_debts;
	struct percpu_counter s_freeblocks_counter;
	struct percpu_counter s_freeinodes_counter;
	struct percpu_counter s_dirs_counter;
	struct blockgroup_lock s_blockgroup_lock;
};

#endif	/* _LINUX_EXT2_FS_SB */