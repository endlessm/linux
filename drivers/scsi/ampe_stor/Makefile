# Driver for Alcor Micro PCI-Express card reader
#
# Copyright(c) 2012 Alcor Micro Corp. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2, or (at your option) any
# later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, see <http://www.gnu.org/licenses/>.
#
# Author:
#
#
# Makefile for the PCI-Express Card Reader drivers.
#


TARGET_MODULE := ampe_stor

EXTRA_CFLAGS := -Idrivers/scsi

obj-m += $(TARGET_MODULE).o

$(TARGET_MODULE)-objs := amcr_stor.o  amcr_scsi.o  general.o sd.o ms.o


default:
	make -C /lib/modules/$(shell uname -r)/build/ SUBDIRS=$(PWD) modules
debug:
	make -C /lib/modules/$(shell uname -r)/build/ SUBDIRS=$(PWD) modules
install:
	cp $(TARGET_MODULE).ko /lib/modules/$(shell uname -r)/kernel/drivers/scsi -f
clean:
	rm -f *.o *.ko
	rm -f $(TARGET_MODULE).mod.c


