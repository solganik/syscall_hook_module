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


#define RESTRICTED_HASH_BITS 7
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

static int add_to_context(char *file_path, int len, struct restricted_files *ctx)
{
	unsigned long hash_key;
	int alloc_len = _get_size_for_entry_alloc_bytes(len);
	struct restricted_file_entry *entry;

	TRACE_DEBUG("Allocating %lu	 -  %d bytes", sizeof(struct restricted_file_entry), alloc_len);
	entry = (struct restricted_file_entry *)kmalloc(alloc_len, GFP_KERNEL);

	hash_key = hash_str(file_path,RESTRICTED_HASH_BITS);

	TRACE_DEBUG("Adding '%s' len %d hash %lx", file_path, len, hash_key)
	if (!entry){
		TRACE_ERROR("Failed to allocate entry for path %s len %d", file_path, len);
		return -ENOMEM;
	}
	TRACE_DEBUG("Adding to hash Filepath is '%s' hash %lx len %d", file_path, hash_key, len);
	INIT_HLIST_NODE(&entry->hash);
	memcpy(entry->buffer, file_path, len);
	entry->buffer[len] = '\0';

	hash_add(ctx->hash, &entry->hash, hash_key);
	return 0;
}

static int parse_fill_context_section(char *buffer, int size,  struct restricted_files *ctx)
{
	char *file_name_end;
	char *buffer_start = buffer;
	int parsed_files = 0;
	int consumed;
	int file_path_len = 0;


	while (true) {
		file_name_end = strpbrk(buffer, "\n");

		if (!file_name_end) {
			consumed =  buffer - buffer_start;
			TRACE_DEBUG("Done processing, total=%d filenames, consumed %d bytes", parsed_files, consumed);
			break;
		}
		file_path_len = file_name_end - buffer;
		*file_name_end = '\0';
		++parsed_files;
		add_to_context(buffer, file_path_len, ctx);
		buffer = file_name_end + 1;
	}

	return consumed;
}

static int _read_parse_file(struct file *file, struct restricted_files *ctx)
{
	char *path = __getname();
	loff_t offset = 0;
	int ret;

	if (!path) {
		return -ENOMEM;
	}

	while (true) {
		ret = kernel_read(file, offset, path, PATH_MAX - 1);
		path[ret] = '\0';
		TRACE_DEBUG("read %d characters", ret);
		if (ret <= 0) {
			TRACE_DEBUG("Done reading file");
			break;
		}

		ret = parse_fill_context_section(path, ret, ctx);
		if (ret <= 0) {
			TRACE_WARNING("Failed to prase section %s", path);
			goto release_buffer;
		}

		offset += ret;
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
	struct restricted_file_entry *entry;
	struct restricted_files *restricted_ctx = (struct restricted_files *)ctx;
	unsigned long hash_key = hash_str(file_path,RESTRICTED_HASH_BITS);

	TRACE_DEBUG("Check if %s restricted key 0x%lx", file_path, hash_key);
	hash_for_each_possible(restricted_ctx->hash, entry, hash, hash_key) {
		if (!strcmp(file_path,entry->buffer)){
			return true;
		}
	}

	return false;
}



