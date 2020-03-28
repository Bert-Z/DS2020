/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2010-2016 Intel Corporation. All rights reserved.
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <assert.h>
#ifdef RTE_LIBRTE_VHOST_NUMA
#include <numaif.h>
#endif

#include <rte_common.h>
#include <rte_malloc.h>
#include <rte_log.h>

#include "vhost.h"
#include "vhost_user.h"

static const char *vhost_message_str[VHOST_USER_MAX] = {
	[VHOST_USER_NONE] = "VHOST_USER_NONE",
	[VHOST_USER_GET_FEATURES] = "VHOST_USER_GET_FEATURES",
	[VHOST_USER_SET_FEATURES] = "VHOST_USER_SET_FEATURES",
	[VHOST_USER_SET_OWNER] = "VHOST_USER_SET_OWNER",
	[VHOST_USER_RESET_OWNER] = "VHOST_USER_RESET_OWNER",
	[VHOST_USER_SET_MEM_TABLE] = "VHOST_USER_SET_MEM_TABLE",
	[VHOST_USER_SET_LOG_BASE] = "VHOST_USER_SET_LOG_BASE",
	[VHOST_USER_SET_LOG_FD] = "VHOST_USER_SET_LOG_FD",
	[VHOST_USER_SET_VRING_NUM] = "VHOST_USER_SET_VRING_NUM",
	[VHOST_USER_SET_VRING_ADDR] = "VHOST_USER_SET_VRING_ADDR",
	[VHOST_USER_SET_VRING_BASE] = "VHOST_USER_SET_VRING_BASE",
	[VHOST_USER_GET_VRING_BASE] = "VHOST_USER_GET_VRING_BASE",
	[VHOST_USER_SET_VRING_KICK] = "VHOST_USER_SET_VRING_KICK",
	[VHOST_USER_SET_VRING_CALL] = "VHOST_USER_SET_VRING_CALL",
	[VHOST_USER_SET_VRING_ERR]  = "VHOST_USER_SET_VRING_ERR",
	[VHOST_USER_GET_PROTOCOL_FEATURES]  = "VHOST_USER_GET_PROTOCOL_FEATURES",
	[VHOST_USER_SET_PROTOCOL_FEATURES]  = "VHOST_USER_SET_PROTOCOL_FEATURES",
	[VHOST_USER_GET_QUEUE_NUM]  = "VHOST_USER_GET_QUEUE_NUM",
	[VHOST_USER_SET_VRING_ENABLE]  = "VHOST_USER_SET_VRING_ENABLE",
	[VHOST_USER_SEND_RARP]  = "VHOST_USER_SEND_RARP",
};

static void
close_msg_fds(struct VhostUserMsg *msg)
{
	int i;

	for (i = 0; i < msg->fd_num; i++)
		close(msg->fds[i]);
}

/*
 * Ensure the expected number of FDs is received,
 * close all FDs and return an error if this is not the case.
 */
static int
validate_msg_fds(struct VhostUserMsg *msg, int expected_fds)
{
	if (msg->fd_num == expected_fds)
		return 0;

	RTE_LOG(ERR, VHOST_CONFIG,
		" Expect %d FDs for request %s, received %d\n",
		expected_fds,
		vhost_message_str[msg->request],
		msg->fd_num);

	close_msg_fds(msg);

	return -1;
}

static uint64_t
get_blk_size(int fd)
{
	struct stat stat;
	int ret;

	ret = fstat(fd, &stat);
	return ret == -1 ? (uint64_t)-1 : (uint64_t)stat.st_blksize;
}

static void
free_mem_region(struct virtio_net *dev)
{
	uint32_t i;
	struct virtio_memory_region *reg;

	if (!dev || !dev->mem)
		return;

	for (i = 0; i < dev->mem->nregions; i++) {
		reg = &dev->mem->regions[i];
		if (reg->host_user_addr) {
			munmap(reg->mmap_addr, reg->mmap_size);
			close(reg->fd);
		}
	}
}

void
vhost_backend_cleanup(struct virtio_net *dev)
{
	if (dev->mem) {
		free_mem_region(dev);
		rte_free(dev->mem);
		dev->mem = NULL;
	}

	free(dev->guest_pages);
	dev->guest_pages = NULL;

	if (dev->log_addr) {
		munmap((void *)(uintptr_t)dev->log_addr, dev->log_size);
		dev->log_addr = 0;
	}
}

/*
 * This function just returns success at the moment unless
 * the device hasn't been initialised.
 */
static int
vhost_user_set_owner(void)
{
	return 0;
}

static int
vhost_user_reset_owner(struct virtio_net *dev)
{
	if (dev->flags & VIRTIO_DEV_RUNNING) {
		dev->flags &= ~VIRTIO_DEV_RUNNING;
		notify_ops->destroy_device(dev->vid);
	}

	cleanup_device(dev, 0);
	reset_device(dev);
	return 0;
}

/*
 * The features that we support are requested.
 */
static uint64_t
vhost_user_get_features(void)
{
	return VHOST_FEATURES;
}

/*
 * We receive the negotiated features supported by us and the virtio device.
 */
static int
vhost_user_set_features(struct virtio_net *dev, uint64_t features)
{
	if (features & ~VHOST_FEATURES)
		return -1;

	dev->features = features;
	if (dev->features &
		((1 << VIRTIO_NET_F_MRG_RXBUF) | (1ULL << VIRTIO_F_VERSION_1))) {
		dev->vhost_hlen = sizeof(struct virtio_net_hdr_mrg_rxbuf);
	} else {
		dev->vhost_hlen = sizeof(struct virtio_net_hdr);
	}
	VHOST_LOG_DEBUG(VHOST_CONFIG,
		"(%d) mergeable RX buffers %s, virtio 1 %s\n",
		dev->vid,
		(dev->features & (1 << VIRTIO_NET_F_MRG_RXBUF)) ? "on" : "off",
		(dev->features & (1ULL << VIRTIO_F_VERSION_1)) ? "on" : "off");

	return 0;
}

/*
 * The virtio device sends us the size of the descriptor ring.
 */
static int
vhost_user_set_vring_num(struct virtio_net *dev,
			 struct vhost_vring_state *state)
{
	struct vhost_virtqueue *vq = dev->virtqueue[state->index];

	vq->size = state->num;

	/* VIRTIO 1.0, 2.4 Virtqueues says:
	 *
	 *   Queue Size value is always a power of 2. The maximum Queue Size
	 *   value is 32768.
	 */
	if ((vq->size & (vq->size - 1)) || vq->size > 32768) {
		RTE_LOG(ERR, VHOST_CONFIG,
			"invalid virtqueue size %u\n", vq->size);
		return -1;
	}

	if (dev->dequeue_zero_copy) {
		vq->nr_zmbuf = 0;
		vq->last_zmbuf_idx = 0;
		vq->zmbuf_size = vq->size;
		if (vq->zmbufs)
			rte_free(vq->zmbufs);
		vq->zmbufs = rte_zmalloc(NULL, vq->zmbuf_size *
					 sizeof(struct zcopy_mbuf), 0);
		if (vq->zmbufs == NULL) {
			RTE_LOG(WARNING, VHOST_CONFIG,
				"failed to allocate mem for zero copy; "
				"zero copy is force disabled\n");
			dev->dequeue_zero_copy = 0;
		}
	}
	if (vq->shadow_used_ring)
		rte_free(vq->shadow_used_ring);
	vq->shadow_used_ring = rte_malloc(NULL,
				vq->size * sizeof(struct vring_used_elem),
				RTE_CACHE_LINE_SIZE);
	if (!vq->shadow_used_ring) {
		RTE_LOG(ERR, VHOST_CONFIG,
			"failed to allocate memory for shadow used ring.\n");
		return -1;
	}

	return 0;
}

/*
 * Reallocate virtio_dev and vhost_virtqueue data structure to make them on the
 * same numa node as the memory of vring descriptor.
 */
#ifdef RTE_LIBRTE_VHOST_NUMA
static struct virtio_net*
numa_realloc(struct virtio_net *dev, int index)
{
	int oldnode, newnode;
	struct virtio_net *old_dev;
	struct vhost_virtqueue *old_vq, *vq;
	int ret;

	/*
	 * vq is allocated on pairs, we should try to do realloc
	 * on first queue of one queue pair only.
	 */
	if (index % VIRTIO_QNUM != 0)
		return dev;

	old_dev = dev;
	vq = old_vq = dev->virtqueue[index];

	ret = get_mempolicy(&newnode, NULL, 0, old_vq->desc,
			    MPOL_F_NODE | MPOL_F_ADDR);

	/* check if we need to reallocate vq */
	ret |= get_mempolicy(&oldnode, NULL, 0, old_vq,
			     MPOL_F_NODE | MPOL_F_ADDR);
	if (ret) {
		RTE_LOG(ERR, VHOST_CONFIG,
			"Unable to get vq numa information.\n");
		return dev;
	}
	if (oldnode != newnode) {
		RTE_LOG(INFO, VHOST_CONFIG,
			"reallocate vq from %d to %d node\n", oldnode, newnode);
		vq = rte_malloc_socket(NULL, sizeof(*vq) * VIRTIO_QNUM, 0,
				       newnode);
		if (!vq)
			return dev;

		memcpy(vq, old_vq, sizeof(*vq) * VIRTIO_QNUM);
		rte_free(old_vq);
	}

	/* check if we need to reallocate dev */
	ret = get_mempolicy(&oldnode, NULL, 0, old_dev,
			    MPOL_F_NODE | MPOL_F_ADDR);
	if (ret) {
		RTE_LOG(ERR, VHOST_CONFIG,
			"Unable to get dev numa information.\n");
		goto out;
	}
	if (oldnode != newnode) {
		RTE_LOG(INFO, VHOST_CONFIG,
			"reallocate dev from %d to %d node\n",
			oldnode, newnode);
		dev = rte_malloc_socket(NULL, sizeof(*dev), 0, newnode);
		if (!dev) {
			dev = old_dev;
			goto out;
		}

		memcpy(dev, old_dev, sizeof(*dev));
		rte_free(old_dev);
	}

out:
	dev->virtqueue[index] = vq;
	dev->virtqueue[index + 1] = vq + 1;
	vhost_devices[dev->vid] = dev;

	return dev;
}
#else
static struct virtio_net*
numa_realloc(struct virtio_net *dev, int index __rte_unused)
{
	return dev;
}
#endif

/*
 * Converts QEMU virtual address to Vhost virtual address. This function is
 * used to convert the ring addresses to our address space.
 */
static uint64_t
qva_to_vva(struct virtio_net *dev, uint64_t qva, uint64_t *len)
{
	struct virtio_memory_region *r;
	uint32_t i;

	/* Find the region where the address lives. */
	for (i = 0; i < dev->mem->nregions; i++) {
		r = &dev->mem->regions[i];

		if (qva >= r->guest_user_addr &&
		    qva <  r->guest_user_addr + r->size) {

			if (unlikely(*len > r->guest_user_addr + r->size - qva))
				*len = r->guest_user_addr + r->size - qva;

			return qva - r->guest_user_addr +
			       r->host_user_addr;
		}
	}
	*len = 0;

	return 0;
}

/*
 * The virtio device sends us the desc, used and avail ring addresses.
 * This function then converts these to our address space.
 */
static int
vhost_user_set_vring_addr(struct virtio_net **pdev,
						  struct vhost_vring_addr *addr)
{
	struct vhost_virtqueue *vq;
	struct virtio_net *dev = *pdev;
	uint64_t size, req_size;

	if (dev->mem == NULL)
		return -1;

	/* addr->index refers to the queue index. The txq 1, rxq is 0. */
	vq = dev->virtqueue[addr->index];

	/* The addresses are converted from QEMU virtual to Vhost virtual. */
	req_size = sizeof(struct vring_desc) * vq->size;
	size = req_size;
	vq->desc = (struct vring_desc *)(uintptr_t)qva_to_vva(dev,
			addr->desc_user_addr, &size);
	if (vq->desc == 0 || size != req_size) {
		RTE_LOG(ERR, VHOST_CONFIG,
			"(%d) failed to map desc ring address.\n",
			dev->vid);
		return -1;
	}

	dev = numa_realloc(dev, addr->index);
	*pdev = dev;

	vq = dev->virtqueue[addr->index];

	req_size = sizeof(struct vring_avail) + sizeof(uint16_t) * vq->size;
	size = req_size;
	vq->avail = (struct vring_avail *)(uintptr_t)qva_to_vva(dev,
			addr->avail_user_addr, &size);
	if (vq->avail == 0 || size != req_size) {
		RTE_LOG(ERR, VHOST_CONFIG,
			"(%d) failed to find avail ring address.\n",
			dev->vid);
		return -1;
	}

	req_size = sizeof(struct vring_used);
	req_size += sizeof(struct vring_used_elem) * vq->size;
	size = req_size;
	vq->used = (struct vring_used *)(uintptr_t)qva_to_vva(dev,
			addr->used_user_addr, &size);
	if (vq->used == 0 || size != req_size) {
		RTE_LOG(ERR, VHOST_CONFIG,
			"(%d) failed to find used ring address.\n",
			dev->vid);
		return -1;
	}

	if (vq->last_used_idx != vq->used->idx) {
		RTE_LOG(WARNING, VHOST_CONFIG,
			"last_used_idx (%u) and vq->used->idx (%u) mismatches; "
			"some packets maybe resent for Tx and dropped for Rx\n",
			vq->last_used_idx, vq->used->idx);
		vq->last_used_idx  = vq->used->idx;
		vq->last_avail_idx = vq->used->idx;
	}

	vq->log_guest_addr = addr->log_guest_addr;

	VHOST_LOG_DEBUG(VHOST_CONFIG, "(%d) mapped address desc: %p\n",
			dev->vid, vq->desc);
	VHOST_LOG_DEBUG(VHOST_CONFIG, "(%d) mapped address avail: %p\n",
			dev->vid, vq->avail);
	VHOST_LOG_DEBUG(VHOST_CONFIG, "(%d) mapped address used: %p\n",
			dev->vid, vq->used);
	VHOST_LOG_DEBUG(VHOST_CONFIG, "(%d) log_guest_addr: %" PRIx64 "\n",
			dev->vid, vq->log_guest_addr);

	return 0;
}

/*
 * The virtio device sends us the available ring last used index.
 */
static int
vhost_user_set_vring_base(struct virtio_net *dev,
			  struct vhost_vring_state *state)
{
	dev->virtqueue[state->index]->last_used_idx  = state->num;
	dev->virtqueue[state->index]->last_avail_idx = state->num;

	return 0;
}

static int
add_one_guest_page(struct virtio_net *dev, uint64_t guest_phys_addr,
		   uint64_t host_phys_addr, uint64_t size)
{
	struct guest_page *page, *last_page;

	if (dev->nr_guest_pages == dev->max_guest_pages) {
		dev->max_guest_pages *= 2;
		dev->guest_pages = realloc(dev->guest_pages,
					dev->max_guest_pages * sizeof(*page));
		if (!dev->guest_pages) {
			RTE_LOG(ERR, VHOST_CONFIG, "cannot realloc guest_pages\n");
			return -1;
		}
	}

	if (dev->nr_guest_pages > 0) {
		last_page = &dev->guest_pages[dev->nr_guest_pages - 1];
		/* merge if the two pages are continuous */
		if (host_phys_addr == last_page->host_phys_addr +
				      last_page->size) {
			last_page->size += size;
			return 0;
		}
	}

	page = &dev->guest_pages[dev->nr_guest_pages++];
	page->guest_phys_addr = guest_phys_addr;
	page->host_phys_addr  = host_phys_addr;
	page->size = size;

	return 0;
}

static int
add_guest_pages(struct virtio_net *dev, struct virtio_memory_region *reg,
		uint64_t page_size)
{
	uint64_t reg_size = reg->size;
	uint64_t host_user_addr  = reg->host_user_addr;
	uint64_t guest_phys_addr = reg->guest_phys_addr;
	uint64_t host_phys_addr;
	uint64_t size;

	host_phys_addr = rte_mem_virt2phy((void *)(uintptr_t)host_user_addr);
	size = page_size - (guest_phys_addr & (page_size - 1));
	size = RTE_MIN(size, reg_size);

	if (add_one_guest_page(dev, guest_phys_addr, host_phys_addr, size) < 0)
		return -1;

	host_user_addr  += size;
	guest_phys_addr += size;
	reg_size -= size;

	while (reg_size > 0) {
		size = RTE_MIN(reg_size, page_size);
		host_phys_addr = rte_mem_virt2phy((void *)(uintptr_t)
						  host_user_addr);
		if (add_one_guest_page(dev, guest_phys_addr, host_phys_addr,
				size) < 0)
			return -1;

		host_user_addr  += size;
		guest_phys_addr += size;
		reg_size -= size;
	}

	return 0;
}

#ifdef RTE_LIBRTE_VHOST_DEBUG
/* TODO: enable it only in debug mode? */
static void
dump_guest_pages(struct virtio_net *dev)
{
	uint32_t i;
	struct guest_page *page;

	for (i = 0; i < dev->nr_guest_pages; i++) {
		page = &dev->guest_pages[i];

		RTE_LOG(INFO, VHOST_CONFIG,
			"guest physical page region %u\n"
			"\t guest_phys_addr: %" PRIx64 "\n"
			"\t host_phys_addr : %" PRIx64 "\n"
			"\t size           : %" PRIx64 "\n",
			i,
			page->guest_phys_addr,
			page->host_phys_addr,
			page->size);
	}
}
#else
#define dump_guest_pages(dev)
#endif

static bool
vhost_memory_changed(struct VhostUserMemory *new,
		      struct virtio_memory *old)
{
	uint32_t i;

	if (new->nregions != old->nregions)
		return true;

	for (i = 0; i < new->nregions; ++i) {
		VhostUserMemoryRegion *new_r = &new->regions[i];
		struct virtio_memory_region *old_r = &old->regions[i];

		if (new_r->guest_phys_addr != old_r->guest_phys_addr)
			return true;
		if (new_r->memory_size != old_r->size)
			return true;
		if (new_r->userspace_addr != old_r->guest_user_addr)
			return true;
	}

	return false;
}

static int
vhost_user_set_mem_table(struct virtio_net *dev, struct VhostUserMsg *pmsg)
{
	struct VhostUserMemory memory = pmsg->payload.memory;
	struct virtio_memory_region *reg;
	void *mmap_addr;
	uint64_t mmap_size;
	uint64_t mmap_offset;
	uint64_t alignment;
	uint32_t i;
	int fd;

	if (dev->mem && !vhost_memory_changed(&memory, dev->mem)) {
		RTE_LOG(INFO, VHOST_CONFIG,
			"(%d) memory regions not changed\n", dev->vid);

		for (i = 0; i < memory.nregions; i++)
			close(pmsg->fds[i]);

		return 0;
	}

	/* Remove from the data plane. */
	if (dev->flags & VIRTIO_DEV_RUNNING) {
		dev->flags &= ~VIRTIO_DEV_RUNNING;
		notify_ops->destroy_device(dev->vid);
	}

	if (dev->mem) {
		free_mem_region(dev);
		rte_free(dev->mem);
		dev->mem = NULL;
	}

	dev->nr_guest_pages = 0;
	if (!dev->guest_pages) {
		dev->max_guest_pages = 8;
		dev->guest_pages = malloc(dev->max_guest_pages *
						sizeof(struct guest_page));
	}

	dev->mem = rte_zmalloc("vhost-mem-table", sizeof(struct virtio_memory) +
		sizeof(struct virtio_memory_region) * memory.nregions, 0);
	if (dev->mem == NULL) {
		RTE_LOG(ERR, VHOST_CONFIG,
			"(%d) failed to allocate memory for dev->mem\n",
			dev->vid);
		return -1;
	}
	dev->mem->nregions = memory.nregions;

	for (i = 0; i < memory.nregions; i++) {
		fd  = pmsg->fds[i];
		reg = &dev->mem->regions[i];

		reg->guest_phys_addr = memory.regions[i].guest_phys_addr;
		reg->guest_user_addr = memory.regions[i].userspace_addr;
		reg->size            = memory.regions[i].memory_size;
		reg->fd              = fd;

		mmap_offset = memory.regions[i].mmap_offset;
		mmap_size   = reg->size + mmap_offset;

		/* mmap() without flag of MAP_ANONYMOUS, should be called
		 * with length argument aligned with hugepagesz at older
		 * longterm version Linux, like 2.6.32 and 3.2.72, or
		 * mmap() will fail with EINVAL.
		 *
		 * to avoid failure, make sure in caller to keep length
		 * aligned.
		 */
		alignment = get_blk_size(fd);
		if (alignment == (uint64_t)-1) {
			RTE_LOG(ERR, VHOST_CONFIG,
				"couldn't get hugepage size through fstat\n");
			goto err_mmap;
		}
		mmap_size = RTE_ALIGN_CEIL(mmap_size, alignment);

		mmap_addr = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE,
				 MAP_SHARED | MAP_POPULATE, fd, 0);

		if (mmap_addr == MAP_FAILED) {
			RTE_LOG(ERR, VHOST_CONFIG,
				"mmap region %u failed.\n", i);
			goto err_mmap;
		}

		reg->mmap_addr = mmap_addr;
		reg->mmap_size = mmap_size;
		reg->host_user_addr = (uint64_t)(uintptr_t)mmap_addr +
				      mmap_offset;

		if (dev->dequeue_zero_copy)
			if (add_guest_pages(dev, reg, alignment) < 0) {
				RTE_LOG(ERR, VHOST_CONFIG,
					"adding guest pages to region %u failed.\n",
					i);
				goto err_mmap;
			}

		RTE_LOG(INFO, VHOST_CONFIG,
			"guest memory region %u, size: 0x%" PRIx64 "\n"
			"\t guest physical addr: 0x%" PRIx64 "\n"
			"\t guest virtual  addr: 0x%" PRIx64 "\n"
			"\t host  virtual  addr: 0x%" PRIx64 "\n"
			"\t mmap addr : 0x%" PRIx64 "\n"
			"\t mmap size : 0x%" PRIx64 "\n"
			"\t mmap align: 0x%" PRIx64 "\n"
			"\t mmap off  : 0x%" PRIx64 "\n",
			i, reg->size,
			reg->guest_phys_addr,
			reg->guest_user_addr,
			reg->host_user_addr,
			(uint64_t)(uintptr_t)mmap_addr,
			mmap_size,
			alignment,
			mmap_offset);
	}

	dump_guest_pages(dev);

	return 0;

err_mmap:
	free_mem_region(dev);
	rte_free(dev->mem);
	dev->mem = NULL;
	return -1;
}

static int
vq_is_ready(struct vhost_virtqueue *vq)
{
	return vq && vq->desc   &&
	       vq->kickfd != VIRTIO_UNINITIALIZED_EVENTFD &&
	       vq->callfd != VIRTIO_UNINITIALIZED_EVENTFD;
}

static int
virtio_is_ready(struct virtio_net *dev)
{
	struct vhost_virtqueue *rvq, *tvq;
	uint32_t i;

	for (i = 0; i < dev->virt_qp_nb; i++) {
		rvq = dev->virtqueue[i * VIRTIO_QNUM + VIRTIO_RXQ];
		tvq = dev->virtqueue[i * VIRTIO_QNUM + VIRTIO_TXQ];

		if (!vq_is_ready(rvq) || !vq_is_ready(tvq)) {
			RTE_LOG(INFO, VHOST_CONFIG,
				"virtio is not ready for processing.\n");
			return 0;
		}
	}

	RTE_LOG(INFO, VHOST_CONFIG,
		"virtio is now ready for processing.\n");
	return 1;
}

static void
vhost_user_set_vring_call(struct virtio_net *dev, struct VhostUserMsg *pmsg)
{
	struct vhost_vring_file file;
	struct vhost_virtqueue *vq;
	uint32_t cur_qp_idx;

	file.index = pmsg->payload.u64 & VHOST_USER_VRING_IDX_MASK;
	if (pmsg->payload.u64 & VHOST_USER_VRING_NOFD_MASK)
		file.fd = VIRTIO_INVALID_EVENTFD;
	else
		file.fd = pmsg->fds[0];
	RTE_LOG(INFO, VHOST_CONFIG,
		"vring call idx:%d file:%d\n", file.index, file.fd);

	/*
	 * FIXME: VHOST_SET_VRING_CALL is the first per-vring message
	 * we get, so we do vring queue pair allocation here.
	 */
	cur_qp_idx = file.index / VIRTIO_QNUM;
	if (cur_qp_idx + 1 > dev->virt_qp_nb) {
		if (alloc_vring_queue_pair(dev, cur_qp_idx) < 0)
			return;
	}

	vq = dev->virtqueue[file.index];
	assert(vq != NULL);

	if (vq->callfd >= 0)
		close(vq->callfd);

	vq->callfd = file.fd;
}

/*
 *  In vhost-user, when we receive kick message, will test whether virtio
 *  device is ready for packet processing.
 */
static int
vhost_user_set_vring_kick(struct virtio_net *dev, struct VhostUserMsg *pmsg)
{
	struct vhost_vring_file file;
	struct vhost_virtqueue *vq;

	file.index = pmsg->payload.u64 & VHOST_USER_VRING_IDX_MASK;
	if (pmsg->payload.u64 & VHOST_USER_VRING_NOFD_MASK)
		file.fd = VIRTIO_INVALID_EVENTFD;
	else
		file.fd = pmsg->fds[0];
	RTE_LOG(INFO, VHOST_CONFIG,
		"vring kick idx:%d file:%d\n", file.index, file.fd);

	vq = dev->virtqueue[file.index];
	if (vq->kickfd >= 0)
		close(vq->kickfd);
	vq->kickfd = file.fd;

	if (virtio_is_ready(dev) && !(dev->flags & VIRTIO_DEV_RUNNING)) {
		if (dev->dequeue_zero_copy) {
			RTE_LOG(INFO, VHOST_CONFIG,
				"dequeue zero copy is enabled\n");
		}

		if (notify_ops->new_device(dev->vid) == 0)
			dev->flags |= VIRTIO_DEV_RUNNING;
	}

	return 0;
}

static void
free_zmbufs(struct vhost_virtqueue *vq)
{
	struct zcopy_mbuf *zmbuf, *next;

	for (zmbuf = TAILQ_FIRST(&vq->zmbuf_list);
	     zmbuf != NULL; zmbuf = next) {
		next = TAILQ_NEXT(zmbuf, next);

		rte_pktmbuf_free(zmbuf->mbuf);
		TAILQ_REMOVE(&vq->zmbuf_list, zmbuf, next);
	}

	rte_free(vq->zmbufs);
}

/*
 * when virtio is stopped, qemu will send us the GET_VRING_BASE message.
 */
static int
vhost_user_get_vring_base(struct virtio_net *dev,
			  struct vhost_vring_state *state)
{
	struct vhost_virtqueue *vq = dev->virtqueue[state->index];

	/* We have to stop the queue (virtio) if it is running. */
	if (dev->flags & VIRTIO_DEV_RUNNING) {
		dev->flags &= ~VIRTIO_DEV_RUNNING;
		notify_ops->destroy_device(dev->vid);
	}

	/* Here we are safe to get the last used index */
	state->num = vq->last_used_idx;

	RTE_LOG(INFO, VHOST_CONFIG,
		"vring base idx:%d file:%d\n", state->index, state->num);
	/*
	 * Based on current qemu vhost-user implementation, this message is
	 * sent and only sent in vhost_vring_stop.
	 * TODO: cleanup the vring, it isn't usable since here.
	 */
	if (vq->kickfd >= 0)
		close(vq->kickfd);

	vq->kickfd = VIRTIO_UNINITIALIZED_EVENTFD;

	if (dev->dequeue_zero_copy)
		free_zmbufs(vq);
	rte_free(vq->shadow_used_ring);
	vq->shadow_used_ring = NULL;

	return 0;
}

/*
 * when virtio queues are ready to work, qemu will send us to
 * enable the virtio queue pair.
 */
static int
vhost_user_set_vring_enable(struct virtio_net *dev,
			    struct vhost_vring_state *state)
{
	int enable = (int)state->num;

	RTE_LOG(INFO, VHOST_CONFIG,
		"set queue enable: %d to qp idx: %d\n",
		enable, state->index);

	if (notify_ops->vring_state_changed)
		notify_ops->vring_state_changed(dev->vid, state->index, enable);

	dev->virtqueue[state->index]->enabled = enable;

	return 0;
}

static int
vhost_user_set_protocol_features(struct virtio_net *dev,
				 uint64_t protocol_features)
{
	if (protocol_features & ~VHOST_USER_PROTOCOL_FEATURES) {
		RTE_LOG(ERR, VHOST_CONFIG,
			"(%d) received invalid protocol features.\n",
			dev->vid);
		return -1;
	}

	dev->protocol_features = protocol_features;
	return 0;
}

static int
vhost_user_set_log_base(struct virtio_net *dev, struct VhostUserMsg *msg)
{
	int fd = msg->fds[0];
	uint64_t size, off;
	void *addr;

	if (fd < 0) {
		RTE_LOG(ERR, VHOST_CONFIG, "invalid log fd: %d\n", fd);
		return -1;
	}

	if (msg->size != sizeof(VhostUserLog)) {
		RTE_LOG(ERR, VHOST_CONFIG,
			"invalid log base msg size: %"PRId32" != %d\n",
			msg->size, (int)sizeof(VhostUserLog));
		return -1;
	}

	size = msg->payload.log.mmap_size;
	off  = msg->payload.log.mmap_offset;
	RTE_LOG(INFO, VHOST_CONFIG,
		"log mmap size: %"PRId64", offset: %"PRId64"\n",
		size, off);

	/*
	 * mmap from 0 to workaround a hugepage mmap bug: mmap will
	 * fail when offset is not page size aligned.
	 */
	addr = mmap(0, size + off, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);
	if (addr == MAP_FAILED) {
		RTE_LOG(ERR, VHOST_CONFIG, "mmap log base failed!\n");
		return -1;
	}

	/*
	 * Free previously mapped log memory on occasionally
	 * multiple VHOST_USER_SET_LOG_BASE.
	 */
	if (dev->log_addr) {
		munmap((void *)(uintptr_t)dev->log_addr, dev->log_size);
	}
	dev->log_addr = (uint64_t)(uintptr_t)addr;
	dev->log_base = dev->log_addr + off;
	dev->log_size = size;

	return 0;
}

/*
 * An rarp packet is constructed and broadcasted to notify switches about
 * the new location of the migrated VM, so that packets from outside will
 * not be lost after migration.
 *
 * However, we don't actually "send" a rarp packet here, instead, we set
 * a flag 'broadcast_rarp' to let rte_vhost_dequeue_burst() inject it.
 */
static int
vhost_user_send_rarp(struct virtio_net *dev, struct VhostUserMsg *msg)
{
	uint8_t *mac = (uint8_t *)&msg->payload.u64;

	RTE_LOG(DEBUG, VHOST_CONFIG,
		":: mac: %02x:%02x:%02x:%02x:%02x:%02x\n",
		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	memcpy(dev->mac.addr_bytes, mac, 6);

	/*
	 * Set the flag to inject a RARP broadcast packet at
	 * rte_vhost_dequeue_burst().
	 *
	 * rte_smp_wmb() is for making sure the mac is copied
	 * before the flag is set.
	 */
	rte_smp_wmb();
	rte_atomic16_set(&dev->broadcast_rarp, 1);

	return 0;
}

/* return bytes# of read on success or negative val on failure. */
static int
read_vhost_message(int sockfd, struct VhostUserMsg *msg)
{
	int ret;

	ret = read_fd_message(sockfd, (char *)msg, VHOST_USER_HDR_SIZE,
		msg->fds, VHOST_MEMORY_MAX_NREGIONS, &msg->fd_num);
	if (ret <= 0)
		return ret;

	if (msg->size) {
		if (msg->size > sizeof(msg->payload)) {
			RTE_LOG(ERR, VHOST_CONFIG,
				"invalid msg size: %d\n", msg->size);
			return -1;
		}
		ret = read(sockfd, &msg->payload, msg->size);
		if (ret <= 0)
			return ret;
		if (ret != (int)msg->size) {
			RTE_LOG(ERR, VHOST_CONFIG,
				"read control message failed\n");
			return -1;
		}
	}

	return ret;
}

static int
send_vhost_message(int sockfd, struct VhostUserMsg *msg)
{
	int ret;

	if (!msg)
		return 0;

	msg->flags &= ~VHOST_USER_VERSION_MASK;
	msg->flags |= VHOST_USER_VERSION;
	msg->flags |= VHOST_USER_REPLY_MASK;

	ret = send_fd_message(sockfd, (char *)msg,
		VHOST_USER_HDR_SIZE + msg->size, NULL, 0);

	return ret;
}

static void
vhost_user_lock_all_queue_pairs(struct virtio_net *dev)
{
	unsigned int i = 0;
	unsigned int vq_num = 0;

	while (vq_num < dev->virt_qp_nb * 2) {
		struct vhost_virtqueue *vq = dev->virtqueue[i];

		if (vq) {
			rte_spinlock_lock(&vq->access_lock);
			vq_num++;
		}
		i++;
	}
}

static void
vhost_user_unlock_all_queue_pairs(struct virtio_net *dev)
{
	unsigned int i = 0;
	unsigned int vq_num = 0;

	while (vq_num < dev->virt_qp_nb * 2) {
		struct vhost_virtqueue *vq = dev->virtqueue[i];

		if (vq) {
			rte_spinlock_unlock(&vq->access_lock);
			vq_num++;
		}
		i++;
	}
}

int
vhost_user_msg_handler(int vid, int fd)
{
	struct virtio_net *dev;
	struct VhostUserMsg msg;
	int ret;
	int unlock_required = 0;
	int expected_fds;

	dev = get_device(vid);
	if (dev == NULL)
		return -1;

	ret = read_vhost_message(fd, &msg);
	if (ret <= 0 || msg.request >= VHOST_USER_MAX) {
		if (ret < 0)
			RTE_LOG(ERR, VHOST_CONFIG,
				"vhost read message failed\n");
		else if (ret == 0)
			RTE_LOG(INFO, VHOST_CONFIG,
				"vhost peer closed\n");
		else
			RTE_LOG(ERR, VHOST_CONFIG,
				"vhost read incorrect message\n");

		return -1;
	}

	RTE_LOG(INFO, VHOST_CONFIG, "read message %s\n",
		vhost_message_str[msg.request]);

	/*
	 * Note: we don't lock all queues on VHOST_USER_GET_VRING_BASE
	 * and VHOST_USER_RESET_OWNER, since it is sent when virtio stops
	 * and device is destroyed. destroy_device waits for queues to be
	 * inactive, so it is safe. Otherwise taking the access_lock
	 * would cause a dead lock.
	 */
	switch (msg.request) {
	case VHOST_USER_SET_FEATURES:
	case VHOST_USER_SET_PROTOCOL_FEATURES:
	case VHOST_USER_SET_OWNER:
	case VHOST_USER_SET_MEM_TABLE:
	case VHOST_USER_SET_LOG_BASE:
	case VHOST_USER_SET_LOG_FD:
	case VHOST_USER_SET_VRING_NUM:
	case VHOST_USER_SET_VRING_ADDR:
	case VHOST_USER_SET_VRING_BASE:
	case VHOST_USER_SET_VRING_KICK:
	case VHOST_USER_SET_VRING_CALL:
	case VHOST_USER_SET_VRING_ERR:
	case VHOST_USER_SET_VRING_ENABLE:
	case VHOST_USER_SEND_RARP:
		vhost_user_lock_all_queue_pairs(dev);
		unlock_required = 1;
		break;
	default:
		break;

	}

	ret = 0;
	switch (msg.request) {
	case VHOST_USER_GET_FEATURES:
		if (validate_msg_fds(&msg, 0) != 0)
			return -1;

		msg.payload.u64 = vhost_user_get_features();
		msg.size = sizeof(msg.payload.u64);
		send_vhost_message(fd, &msg);
		break;
	case VHOST_USER_SET_FEATURES:
		if (validate_msg_fds(&msg, 0) != 0)
			return -1;

		ret = vhost_user_set_features(dev, msg.payload.u64);
		break;

	case VHOST_USER_GET_PROTOCOL_FEATURES:
		if (validate_msg_fds(&msg, 0) != 0)
			return -1;

		msg.payload.u64 = VHOST_USER_PROTOCOL_FEATURES;
		msg.size = sizeof(msg.payload.u64);
		send_vhost_message(fd, &msg);
		break;
	case VHOST_USER_SET_PROTOCOL_FEATURES:
		if (validate_msg_fds(&msg, 0) != 0)
			return -1;

		ret = vhost_user_set_protocol_features(dev, msg.payload.u64);
		break;

	case VHOST_USER_SET_OWNER:
		if (validate_msg_fds(&msg, 0) != 0)
			return -1;

		ret = vhost_user_set_owner();
		break;
	case VHOST_USER_RESET_OWNER:
		if (validate_msg_fds(&msg, 0) != 0)
			return -1;

		ret = vhost_user_reset_owner(dev);
		break;

	case VHOST_USER_SET_MEM_TABLE:
		if (validate_msg_fds(&msg, msg.payload.memory.nregions) != 0)
			return -1;

		vhost_user_set_mem_table(dev, &msg);
		break;

	case VHOST_USER_SET_LOG_BASE:
		if (validate_msg_fds(&msg, 1) != 0)
			return -1;

		ret = vhost_user_set_log_base(dev, &msg);
		if (ret)
			break;
		/*
		 * The spec is not clear about it (yet), but QEMU doesn't expect
		 * any payload in the reply. But a reply is required.
		 */
		msg.size = 0;
		send_vhost_message(fd, &msg);
		break;
	case VHOST_USER_SET_LOG_FD:
		if (validate_msg_fds(&msg, 1) != 0)
			return -1;

		close(msg.fds[0]);
		RTE_LOG(INFO, VHOST_CONFIG, "not implemented.\n");
		break;

	case VHOST_USER_SET_VRING_NUM:
		if (validate_msg_fds(&msg, 0) != 0)
			return -1;

		ret = vhost_user_set_vring_num(dev, &msg.payload.state);
		break;
	case VHOST_USER_SET_VRING_ADDR:
		if (validate_msg_fds(&msg, 0) != 0)
			return -1;

		ret = vhost_user_set_vring_addr(&dev, &msg.payload.addr);
		break;
	case VHOST_USER_SET_VRING_BASE:
		if (validate_msg_fds(&msg, 0) != 0)
			return -1;

		ret = vhost_user_set_vring_base(dev, &msg.payload.state);
		break;

	case VHOST_USER_GET_VRING_BASE:
		if (validate_msg_fds(&msg, 0) != 0)
			return -1;

		ret = vhost_user_get_vring_base(dev, &msg.payload.state);
		if (ret)
			break;
		msg.size = sizeof(msg.payload.state);
		send_vhost_message(fd, &msg);
		break;

	case VHOST_USER_SET_VRING_KICK:
		expected_fds =
			(msg.payload.u64 & VHOST_USER_VRING_NOFD_MASK) ? 0 : 1;
		if (validate_msg_fds(&msg, expected_fds) != 0)
			return -1;

		ret = vhost_user_set_vring_kick(dev, &msg);
		break;
	case VHOST_USER_SET_VRING_CALL:
		expected_fds =
			(msg.payload.u64 & VHOST_USER_VRING_NOFD_MASK) ? 0 : 1;
		if (validate_msg_fds(&msg, expected_fds) != 0)
			return -1;

		vhost_user_set_vring_call(dev, &msg);
		break;

	case VHOST_USER_SET_VRING_ERR:
		expected_fds =
			(msg.payload.u64 & VHOST_USER_VRING_NOFD_MASK) ? 0 : 1;
		if (validate_msg_fds(&msg, expected_fds) != 0)
			return -1;

		if (!(msg.payload.u64 & VHOST_USER_VRING_NOFD_MASK))
			close(msg.fds[0]);
		RTE_LOG(INFO, VHOST_CONFIG, "not implemented\n");
		break;

	case VHOST_USER_GET_QUEUE_NUM:
		if (validate_msg_fds(&msg, 0) != 0)
			return -1;

		msg.payload.u64 = VHOST_MAX_QUEUE_PAIRS;
		msg.size = sizeof(msg.payload.u64);
		send_vhost_message(fd, &msg);
		break;

	case VHOST_USER_SET_VRING_ENABLE:
		if (validate_msg_fds(&msg, 0) != 0)
			return -1;

		ret = vhost_user_set_vring_enable(dev, &msg.payload.state);
		break;
	case VHOST_USER_SEND_RARP:
		if (validate_msg_fds(&msg, 0) != 0)
			return -1;

		ret = vhost_user_send_rarp(dev, &msg);
		break;

	default:
		break;

	}

	if (unlock_required)
		vhost_user_unlock_all_queue_pairs(dev);

	if (ret) {
		RTE_LOG(ERR, VHOST_CONFIG,
			"vhost message handling failed.\n");
		return -1;
	}

	return 0;
}
