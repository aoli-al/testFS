#ifndef _CSUM_H
#define _CSUM_H

#include "testfs.h"
#include "async.h"

#define MAX_NR_CSUMS (CSUM_TABLE_SIZE * BLOCK_SIZE / sizeof(int))

struct super_block;

// TODO: add your code here

int testfs_get_csum(struct super_block *sb, int block_nr);
void testfs_put_csum(struct super_block *sb, int block_nr, int csum);
int testfs_calculate_csum(const char *buf, const int size);
int testfs_verify_csum(struct super_block *sb, int block_nr);
void testfs_put_csum_async(
  struct super_block *sb, struct future *f, int phy_block_nr, int csum);
#endif /* _CSUM_H */
