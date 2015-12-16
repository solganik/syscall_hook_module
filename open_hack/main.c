#include "debug.h"
#include "hook.h"
#include "restricted.h"

#define RESTRICTED_FILE_PATH  "/tmp/protected.txt"

static void *restricted_ctx = NULL;
asmlinkage int (*original_call)(const char *, int, int);

asmlinkage int our_sys_open(const char *filename, int flags, int mode)
{
	TRACE_INFO("Redirecting to original %s", filename);
	return original_call(filename, flags, mode);
}

static int __init hook_init(void)
{
	int ret = 0;
	TRACE_INFO("Loading module");
	original_call = hook_open(our_sys_open);
	if (!original_call) {
		TRACE_ERROR("Failed hook open syscall");
		return -1; /* Donno what went wrong :) */
	}

	restricted_ctx = restricted_allocate();
	if (!restricted_ctx) {
		TRACE_ERROR("Failed to allocated restricted context");
		ret = -ENOMEM;
		goto unhook;
	}

	ret = restricted_init(RESTRICTED_FILE_PATH, restricted_ctx);
	if (ret != 0) {
		TRACE_ERROR("Failed to initialize restricted ctx %p err=%d", restricted_ctx, ret);
		goto free_ctx;
	}
	goto done;
free_ctx:
	restricted_destroy(restricted_ctx);
unhook:
	unhook_open(original_call);
done:
	return ret;
}

static void __exit hook_exit(void){
	TRACE_DEBUG("Exiting");
	restricted_destroy(restricted_ctx);
	unhook_open(original_call);
	TRACE_INFO("Exit Done");
}



module_init(hook_init);
module_exit(hook_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sasha ");
MODULE_DESCRIPTION("Targil module");
