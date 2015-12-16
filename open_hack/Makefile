NAME		:= open-hack

KVERSION ?= $(shell rpm -q --queryformat '%{version}-%{release}'  kernel-devel-3.10.0)
KDIR ?= /usr/src/kernels/$(KVERSION).x86_64

#KDIR		?= /lib/modules/$(shell uname -r)/build

ccflags-y	:= -g -Wall -nostdinc -DDEBUG
obj-m		:= $(NAME).o
obj-y		:= ../udis86/libudis86/



$(NAME)-y := main.o  sct_hook.o  utils.o \
             udis_utils.o restricted.o
$(NAME)-y += ../udis86/libudis86/built-in.o


new: clean all

clean:
	make -C $(KDIR) M=$(shell pwd) clean
	@rm -rf ../common/*.o ../udis86/libudis86/*.o
	@rm -rf ../$(NAME).ko

all: 
	make -C $(KDIR) M=$(shell pwd)

