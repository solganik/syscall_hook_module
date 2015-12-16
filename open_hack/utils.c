#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include "utils.h"


void *map_writable(void *addr, size_t len){
	void *vaddr;
	int i, nr_pages = DIV_ROUND_UP(offset_in_page(addr) + len, PAGE_SIZE);
	struct page **pages = kmalloc(nr_pages * sizeof(*pages), GFP_KERNEL);
	void *page_addr = (void *)((unsigned long)addr & PAGE_MASK);

	if(pages == NULL)
		return NULL;

	for(i = 0; i < nr_pages; i++){
		if(__module_address((unsigned long)page_addr) == NULL){
			pages[i] = virt_to_page(page_addr);
			WARN_ON(!PageReserved(pages[i]));
		}else{
			pages[i] = vmalloc_to_page(page_addr);
		}
		if(pages[i] == NULL){
			vaddr = NULL;
			goto out;
		}
		page_addr += PAGE_SIZE;
	}
	vaddr = vmap(pages, nr_pages, VM_MAP, PAGE_KERNEL);

out:
	kfree(pages);
	return vaddr == NULL ? NULL : vaddr + offset_in_page(addr);
}
