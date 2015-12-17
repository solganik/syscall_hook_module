#include "restricted.h"
#include "debug.h"
#include <linux/stat.h>
#include <linux/file.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/limits.h>
#include <linux/mm.h>
#include <linux/slab.h>


#define RESTRICTED_HASH_BITS 6
struct restricted_files {
		DECLARE_HASHTABLE(hash, RESTRICTED_HASH_BITS);
};

struct restricted_file_entry {
	struct hlist_node hash;
	u8	buffer[0];
} __packed;


void *restricted_allocate(void)
{
	struct restricted_files* result = kzalloc(sizeof (struct restricted_files), GFP_KERNEL);
	if (!result){
		return result;
	}
	TRACE_DEBUG("Allocated at %p..initilizing",result);
	hash_init(result->hash);

	/* structure init goes here*/
	return result;
}

void restricted_destroy(void *ctx)
{
	int loop_cursor;
	struct restricted_file_entry *entry;
	struct hlist_node *tmp;
	struct restricted_files *restricted_ctx = (struct restricted_files *)ctx;

	TRACE_DEBUG("Destroying hash at %p", ctx);

	hash_for_each_safe(restricted_ctx->hash, loop_cursor, tmp, entry, hash) {
		TRACE_DEBUG("Remove entry %p - %s", entry, entry->buffer);
		hash_del(&entry->hash);
		kfree(entry);
	}

	TRACE_DEBUG("Freeing hash at %p", ctx);
	kfree(ctx);
}

static inline unsigned long hash_str(const char *name, int bits)
{
	unsigned long hash = 0;
	unsigned long l = 0;
	int len = 0;
	unsigned char c;
	do {
		if (unlikely(!(c = *name++))) {
			c = (char)len; len = -1;
		}
		l = (l << 8) | c;
		len++;
		if ((len & (BITS_PER_LONG/8-1))==0)
			hash = hash_long(hash^l, BITS_PER_LONG);
	} while (len);
	return hash >> (BITS_PER_LONG - bits);
}

static size_t _get_size_for_entry_alloc_bytes(int file_path_len)
{
	return sizeof(struct restricted_file_entry) + file_path_len + 1;
}


/* Don't inline this: 'struct kstat' is biggish */
static noinline_for_stack long _manifest_file_size_bytes(struct file *file)
{
	struct kstat st;
	if (vfs_getattr(&file->f_path, &st))
		return -1;
	if (!S_ISREG(st.mode))
		return -1;
	if (st.size != (long)st.size)
		return -1;
	return st.size;
}


static int _read_parse_file(struct file *file, struct restricted_files *ctx)
{
	char *path = __getname();
	int ret;

=
	if (!path) {
		return -ENOMEM;
	}

/*	vfs_read(file, (void __user *)addr, count, &pos);*/

	ret = kernel_read(file, 0, path, PATH_MAX);
	TRACE_INFO("Reply %d", ret);
	if (ret <= 0) {
		ret = -EINVAL;
		goto release_buffer;
	}
	ret = 0;

release_buffer:
	__putname(path);
	return ret;
}

int restricted_init(const char *mafifest_path, void *ctx)
{
	int ret = 0;
	struct file *file;
	struct restricted_files *restricted_ctx = (struct restricted_files *)ctx;

	TRACE_DEBUG("Opening file %s", mafifest_path);
	file = filp_open(mafifest_path, O_RDONLY, 0);
	if (IS_ERR(file)) {
		ret = PTR_ERR(file);
		TRACE_ERROR("Failed to open file err=%d", ret);
		goto failed_open_file;
	}

	ret = _read_parse_file(file, restricted_ctx);
	if (ret != 0) {
		TRACE_ERROR("Failed to read rules file err=%d", ret);
		goto close_file;
	}

	TRACE_DEBUG("Done reading file");
close_file:
	filp_close(file, NULL);
failed_open_file:
	return ret;
}

bool is_restricted(void *ctx, const char *file_path)
{
	return false;
}



