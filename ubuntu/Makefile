#
# Makefile for the Linux kernel ubuntu supplied third-party device drivers.
#

#
# NOTE: to allow drivers to be added and removed without causing merge
# collisions you should add new entries in the middle of the six lines
# of ## at the bottom of the list.  Always add three lines of ## above
# your new entry and maintain the six lines below.
#

##
##
##
obj-$(CONFIG_DM_RAID45)		+= dm-raid4-5/
##
##
##
ifneq ($(filter $(ARCH), i386 x86_64),)
obj-y                         += vbox/
endif
##
##
##
obj-$(CONFIG_DRM_I915_BPO)	+= i915/
##
##
##
##
##
##

# This is a stupid trick to get kbuild to create ubuntu/built-in.o
obj- += foo.o
