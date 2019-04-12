#include <unistd.h>
#include "spdk/bdev.h"
#include "spdk/env.h"
#include "spdk/nvme.h"
#include "spdk/stdinc.h"
#include "spdk/thread.h"

#include "common.h"
#include "super.h"
#include "testfs.h"
#include "device.h"
#include "block.h"

#include <semaphore.h>
#include "spdk/event.h"

static char zero[BLOCK_SIZE] = {0};

void free_request(struct request *request) {
  spdk_dma_free(request->buf);
  free(request);
}

static void complete(struct spdk_bdev_io *bdev_io, bool success, void *cb_arg) {
  struct request *request = cb_arg;
  spdk_bdev_free_io(bdev_io);
  sem_post(request->sem);
  if (request->free) {
    free_request(request);
  }
//  if (!request->read) {
//    free(request);
//  }
}

void readwrite(void *arg) {
  struct request *req = arg;
  struct spdk_io_channel *io_channel = req->io_channel;
  if (req->read) {
    spdk_bdev_read_blocks(req->bdev_desc, io_channel,
                          req->buf, req->start, req->nr, complete, req);
  }
  else {
    spdk_bdev_write_blocks(req->bdev_desc, io_channel,
                           req->buf, req->start, req->nr, complete, req);
  }
}

struct request *generate_request(struct bdev_context *context, struct spdk_io_channel *io_channel, bool is_read, size_t start, size_t nr, char* blocks) {
  struct request *request= malloc(sizeof(struct request));
  request->bdev_desc = context->bdev_desc;
  request->read = is_read;
  request->start = start;
  request->nr = nr;
  request->free = false;
  request->buf = spdk_dma_zmalloc(nr * BLOCK_SIZE, context->buf_align, NULL);
  if (!request->buf) {
    printf("????\n");
  }
  request->io_channel = io_channel;
  request->sem = &context->sem;
  if (!is_read) {
    memcpy(request->buf, blocks, nr * BLOCK_SIZE);
  }
  return request;
}


void read_blocks(struct super_block *sb, char *blocks, int start, int nr) {
  struct bdev_context *bdev_ctx = &(sb->fs->bdev_ctx);
  struct spdk_io_channel *io_channel = sb->fs->reactors[MAIN_REACTOR].io_channel;
  struct request *request = generate_request(bdev_ctx, io_channel, true, start, nr, NULL);
  readwrite(request);
  sem_wait(request->sem);
  memcpy(blocks, request->buf, nr * BLOCK_SIZE);
  free_request(request);
}

void read_blocks_async(struct super_block *sb, uint32_t reactor_id, char *blocks, int start, int nr) {
  // TODO: WIP - this needs to be scheduled on a different thread
  struct bdev_context *bdev_ctx = &(sb->fs->bdev_ctx);
  struct spdk_io_channel *io_channel = sb->fs->reactors[reactor_id].io_channel;
  struct request *request = generate_request(bdev_ctx, io_channel, true, start, nr, NULL);
  readwrite(request);
  sem_wait(request->sem);
  memcpy(blocks, request->buf, nr * BLOCK_SIZE);
  free_request(request);
}

void write_blocks(struct super_block *sb, char *blocks, int start, int nr) {
  struct bdev_context *bdev_ctx = &(sb->fs->bdev_ctx);
  struct spdk_io_channel *io_channel = sb->fs->reactors[MAIN_REACTOR].io_channel;
  struct request *request = generate_request(bdev_ctx, io_channel, false, start, nr, blocks);
  readwrite(request);
  sem_wait(request->sem);
  free_request(request);
}

void write_blocks_async(struct super_block *sb, uint32_t reactor_id, char *blocks, int start, int nr) {
  // TODO: WIP - this needs to be scheduled on a different thread
  struct bdev_context *bdev_ctx = &(sb->fs->bdev_ctx);
  struct spdk_io_channel *io_channel = sb->fs->reactors[reactor_id].io_channel;
  bdev_ctx->counter++;
  struct request *request = generate_request(bdev_ctx, io_channel, false, start, nr, blocks);
  request->free = true;
  readwrite(request);
}


void zero_blocks(struct super_block *sb, int start, int nr) {
  int i;

  for (i = 0; i < nr; i++) {
    write_blocks(sb, zero, start + i, 1);
  }
}
