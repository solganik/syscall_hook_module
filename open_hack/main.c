#include "debug.h"
#include "hook.h"
#include "restricted.h"
#include <linux/namei.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>

#define RESTRICTED_FILE_PATH  "/tmp/protected.txt"

static void *restricted_ctx = NULL;
open_syscall_type original_call;



static char *path_utils_find_path(const char *filename, char *file_path, size_t max_len, int flags)
{
	int ret;
	struct path path;
	char *full_path;

	ret = user_path_at(AT_FDCWD, filename, flags | LOOKUP_FOLLOW, &path);
	if (ret != 0) {
		full_path = ERR_PTR(ret);
		goto done;
	}

	full_path = dentry_path_raw(path.dentry, file_path, max_len);
	path_put(&path);
done:
	return full_path;
}


long our_sys_open(const char __user *filename, int flags, umode_t mode)
{
	char *buffer = __getname();
	char *file_path_full;
	int ret;

	/* Get absolute path of the file note that this does not yet deal with symbolic links*/
	file_path_full = path_utils_find_path(filename, buffer, PATH_MAX, flags);
	if (IS_ERR(file_path_full)) {
		ret = PTR_ERR(file_path_full);
		__putname(buffer);
		return ret;
	}

	if (is_restricted(restricted_ctx, file_path_full)){
		TRACE_WARNING("File '%s'  absolute path '%s' is restricted .. deny", filename, file_path_full);
		__putname(buffer);
		return -EPERM;
	}
	__putname(buffer);

	return original_call(filename, flags, mode);
}

static int __init hook_init(void)
{
	int ret = 0;
	TRACE_INFO("Loading module");

	restricted_ctx = restricted_allocate();
	if (!restricted_ctx) {
		TRACE_ERROR("Failed to allocated restricted context");
		ret = -ENOMEM;
		goto done;
	}

	ret = restricted_init(RESTRICTED_FILE_PATH, restricted_ctx);
	if (ret != 0) {
		TRACE_ERROR("Failed to initialize restricted ctx %p err=%d", restricted_ctx, ret);
		goto free_ctx;
	}

	original_call = hook_open(our_sys_open);
	if (!original_call) {
		TRACE_ERROR("Failed hook open syscall");
		goto free_ctx;
	}
	goto done;

free_ctx:
	restricted_destroy(restricted_ctx);
done:
	return ret;
}

static void __exit hook_exit(void){
	TRACE_DEBUG("Exiting");
	unhook_open(original_call);
	restricted_destroy(restricted_ctx);
	TRACE_INFO("Exit Done");
}



module_init(hook_init);
module_exit(hook_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sasha ");
MODULE_DESCRIPTION("Targil module");
