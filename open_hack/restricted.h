#ifndef RESTRICTED_H_
#define RESTRICTED_H_
#include <linux/hashtable.h>


void *restricted_allocate(void);
void restricted_destroy(void *ctx);
int restricted_init(const char *mafifest_path, void *ctx);
bool is_restricted(const char *file_path, void *ctx);




#endif /* RESTRICTED_H_ */
