#include "debug.h"
#include "hook.h"
#include "restricted.h"

#define RESTRICTED_FILE_PATH  "/tmp/protected.txt"

static void *restricted_ctx = NULL;
open_syscall_type original_call;



static char *path_utils_find_path(const char *filename, char *file_path, size_t max_len, int flags)
{
	int ret;
	struct path path;
	char *full_path;

	ret = user_path_at(AT_FDCWD, filename, flags, &path);
	if (ret != 0){
		TRACE_DEBUG("Failed to lookup path for file %s err=%d", filename, ret);
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
	TRACE_INFO("Redirecting to original %s", filename);
	if (is_restricted(restricted_ctx,filename)){
		TRACE_WARNING("File %s is restricted .. deny", filename);
		return -EPERM;
	}
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
