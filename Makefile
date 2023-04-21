CONTIKI_PROJECT = PROJECT
all: $(CONTIKI_PROJECT)

TARGET_ARCH = z1

#use this to enable TSCH: MAKE_MAC = MAKE_MAC_TSCH
MAKE_MAC ?= MAKE_MAC_CSMA
MAKE_NET = MAKE_NET_NULLNET

# Include the utils.c file here
PROJECT_SOURCEFILES += utils.c

CONTIKI = /home/user/contiki-ng
include $(CONTIKI)/Makefile.include
