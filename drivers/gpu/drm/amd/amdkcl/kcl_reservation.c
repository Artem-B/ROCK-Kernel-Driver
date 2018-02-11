#include <kcl/kcl_fence.h>
#include <kcl/kcl_reservation.h>

#if defined(BUILD_AS_DKMS) && LINUX_VERSION_CODE < KERNEL_VERSION(4, 10, 0)
long _kcl_reservation_object_wait_timeout_rcu(struct reservation_object *obj,
					 bool wait_all, bool intr,
					 unsigned long timeout)
{
	struct fence *fence;
	unsigned seq, shared_count, i = 0;
	long ret = timeout ? timeout : 1;

retry:
	shared_count = 0;
	seq = read_seqcount_begin(&obj->seq);
	rcu_read_lock();

	fence = rcu_dereference(obj->fence_excl);
	if (fence && !test_bit(FENCE_FLAG_SIGNALED_BIT, &fence->flags)) {
		if (!dma_fence_get_rcu(fence))
			goto unlock_retry;

		if (dma_fence_is_signaled(fence)) {
			dma_fence_put(fence);
			fence = NULL;
		}

	} else {
		fence = NULL;
	}

	if (!fence && wait_all) {
		struct reservation_object_list *fobj =
						rcu_dereference(obj->fence);

		if (fobj)
			shared_count = fobj->shared_count;

		if (read_seqcount_retry(&obj->seq, seq))
			goto unlock_retry;

		for (i = 0; i < shared_count; ++i) {
			struct fence *lfence = rcu_dereference(fobj->shared[i]);

			if (test_bit(FENCE_FLAG_SIGNALED_BIT, &lfence->flags))
				continue;

			if (!fence_get_rcu(lfence))
				goto unlock_retry;

			if (fence_is_signaled(lfence)) {
				fence_put(lfence);
				continue;
			}

			fence = lfence;
			break;
		}
	}

	rcu_read_unlock();
	if (fence) {
		ret = kcl_fence_wait_timeout(fence, intr, ret);
		fence_put(fence);
		if (ret > 0 && wait_all && (i + 1 < shared_count))
			goto retry;
	}
	return ret;

unlock_retry:
	rcu_read_unlock();
	goto retry;
}
EXPORT_SYMBOL(_kcl_reservation_object_wait_timeout_rcu);
#endif

#if defined(BUILD_AS_DKMS)
int _kcl_reservation_object_copy_fences(struct reservation_object *dst,
					struct reservation_object *src)
{
	struct reservation_object_list *src_list, *dst_list;
	struct dma_fence *old, *new;
	size_t size;
	unsigned i;

	rcu_read_lock();
	src_list = rcu_dereference(src->fence);

retry:
	if (src_list) {
		unsigned shared_count = src_list->shared_count;

		size = offsetof(typeof(*src_list), shared[shared_count]);
		rcu_read_unlock();

		dst_list = kmalloc(size, GFP_KERNEL);
		if (!dst_list)
			return -ENOMEM;

		rcu_read_lock();
		src_list = rcu_dereference(src->fence);
		if (!src_list || src_list->shared_count > shared_count) {
			kfree(dst_list);
			goto retry;
		}

		dst_list->shared_count = 0;
		dst_list->shared_max = shared_count;
		for (i = 0; i < src_list->shared_count; ++i) {
			struct dma_fence *fence;

			fence = rcu_dereference(src_list->shared[i]);
			if (test_bit(FENCE_FLAG_SIGNALED_BIT,
				     &fence->flags))
				continue;

			if (!dma_fence_get_rcu(fence)) {
				kfree(dst_list);
				src_list = rcu_dereference(src->fence);
				goto retry;
			}

			if (dma_fence_is_signaled(fence)) {
				dma_fence_put(fence);
				continue;
			}

			dst_list->shared[dst_list->shared_count++] = fence;
		}
	} else {
		dst_list = NULL;
	}

	new = dma_fence_get_rcu_safe(&src->fence_excl);
	rcu_read_unlock();

	kfree(dst->staged);
	dst->staged = NULL;

	src_list = reservation_object_get_list(dst);
	old = reservation_object_get_excl(dst);

	preempt_disable();
	write_seqcount_begin(&dst->seq);
	/* write_seqcount_begin provides the necessary memory barrier */
	RCU_INIT_POINTER(dst->fence_excl, new);
	RCU_INIT_POINTER(dst->fence, dst_list);
	write_seqcount_end(&dst->seq);
	preempt_enable();

	if (src_list)
		kfree_rcu(src_list, rcu);
	dma_fence_put(old);

	return 0;
}
EXPORT_SYMBOL(_kcl_reservation_object_copy_fences);
#endif
