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


struct restricted_files {

};

void *restricted_allocate(void)
{
	struct restricted_files* result = kzalloc(sizeof (struct restricted_files), GFP_KERNEL);
	if (!result){
		return result;
	}
	TRACE_DEBUG("Allocated at %p",result);
	/* structure init goes here*/
	return result;
}

void restricted_destroy(void *ctx)
{
	TRACE_DEBUG("Destroying at %p", ctx);
	kfree(ctx);
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
	/*file_size = _manifest_file_size_bytes(file);
	if (file_size <= 0) {
		TRACE_ERROR("File size is wrong file - %s size %lu", mafifest_path, file_size);
		goto close_file;
	}*/

close_file:
	filp_close(file, NULL);
failed_open_file:
	return ret;
}

bool is_restricted(void *ctx, const char *file_path)
{
	return false;
}



