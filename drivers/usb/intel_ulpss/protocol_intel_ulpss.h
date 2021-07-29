/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Intel LPSS USB driver
 *
 * Copyright (c) 2020, Intel Corporation.
 */

#ifndef __PROTOCOL_INTEL_ULPSS_H__
#define __PROTOCOL_INTEL_ULPSS_H__

#include <linux/types.h>

/*
* Define FW Communication protocol
*/
#define MAX_GPIO_NUM 20
#define MAX_GPIO_BANK_NUM 5

#define MAX_I2C_CONTROLLER_NUM 2

/* command types */
#define MNG_CMD_TYPE 1
#define DIAG_CMD_TYPE 2
#define GPIO_CMD_TYPE 3
#define I2C_CMD_TYPE 4
#define SPI_CMD_TYPE 5

/* command Flags */
#define ACK_FLAG BIT(0)
#define RESP_FLAG BIT(1)
#define CMPL_FLAG BIT(2)

/* MNG commands */
#define MNG_GET_VERSION 1
#define MNG_RESET_NOTIFY 2
#define MNG_RESET 3
#define MNG_ENUM_GPIO 4
#define MNG_ENUM_I2C 5
#define MNG_POWER_STATE_CHANGE 6
#define MNG_SET_DFU_MODE 7

/* DIAG commands */
#define DIAG_GET_STATE 0x01
#define DIAG_GET_STATISTIC 0x02
#define DIAG_SET_TRACE_LEVEL 0x03
#define DIAG_SET_ECHO_MODE 0x04
#define DIAG_GET_FW_LOG 0x05
#define DIAG_GET_FW_COREDUMP 0x06
#define DIAG_TRIGGER_WDT 0x07
#define DIAG_TRIGGER_FAULT 0x08
#define DIAG_FEED_WDT 0x09
#define DIAG_GET_SECURE_STATE 0x0A

/* GPIO commands */
#define GPIO_CONFIG 1
#define GPIO_READ 2
#define GPIO_WRITE 3
#define GPIO_INTR_NOTIFY 4

/* I2C commands */
#define I2C_INIT 1
#define I2C_XFER 2
#define I2C_START 3
#define I2C_STOP 4
#define I2C_READ 5
#define I2C_WRITE 6

#define GPIO_CONF_DISABLE BIT(0)
#define GPIO_CONF_INPUT BIT(1)
#define GPIO_CONF_OUTPUT BIT(2)
#define GPIO_CONF_PULLUP BIT(3)
#define GPIO_CONF_PULLDOWN BIT(4)

/* Intentional overlap with PULLUP / PULLDOWN */
#define GPIO_CONF_SET BIT(3)
#define GPIO_CONF_CLR BIT(4)

struct cmd_header {
	u8 type;
	u8 cmd;
	u8 flags;
	u8 len;
	u8 data[];
} __attribute__((packed));

struct fw_version {
	u8 major;
	u8 minor;
	u16 patch;
	u16 build;
} __attribute__((packed));

struct bank_descriptor {
	u8 bank_id;
	u8 pin_num;

	/* 1 bit for each gpio, 1 means valid */
	u32 bitmap;
} __attribute__((packed));

struct gpio_descriptor {
	u8 pins_per_bank;
	u8 banks;
	struct bank_descriptor bank_table[MAX_GPIO_BANK_NUM];
} __attribute__((packed));

struct i2c_controller_info {
	u8 id;
	u8 capacity;
	u8 intr_pin;
} __attribute__((packed));

struct i2c_descriptor {
	u8 num;
	struct i2c_controller_info info[MAX_I2C_CONTROLLER_NUM];
} __attribute__((packed));

struct gpio_op {
	u8 index;
	u8 value;
} __attribute__((packed));

struct gpio_packet {
	u8 num;
	struct gpio_op item[0];
} __attribute__((packed));

/* I2C Transfer */
struct i2c_xfer {
	u8 id;
	u8 slave;
	u16 flag; /* speed, 8/16bit addr, addr increase, etc */
	u16 addr;
	u16 len;
	u8 data[0];
} __attribute__((packed));

/* I2C raw commands: Init/Start/Read/Write/Stop */
struct i2c_raw_io {
	u8 id;
	s16 len;
	u8 data[0];
} __attribute__((packed));

#define MAX_PACKET_SIZE 64
#define MAX_PAYLOAD_SIZE (MAX_PACKET_SIZE - sizeof(struct cmd_header))

#define USB_WRITE_TIMEOUT 20
#define USB_WRITE_ACK_TIMEOUT 100

#define DEFAULT_GPIO_CONTROLLER_ID 1
#define DEFAULT_GPIO_PIN_COUNT_PER_BANK 32

#define DEFAULT_I2C_CONTROLLER_ID 1
#define DEFAULT_I2C_CAPACITY 0
#define DEFAULT_I2C_INTR_PIN 0

/* I2C r/w Flags */
#define I2C_SLAVE_TRANSFER_WRITE (0)
#define I2C_SLAVE_TRANSFER_READ (1)

/* i2c init flags */
#define I2C_INIT_FLAG_MODE_MASK (1 << 0)
#define I2C_INIT_FLAG_MODE_POLLING (0 << 0)
#define I2C_INIT_FLAG_MODE_INTERRUPT (1 << 0)

#define I2C_FLAG_ADDR_16BIT (1 << 0)
#define I2C_INIT_FLAG_FREQ_MASK (3 << 1)
#define I2C_FLAG_FREQ_100K (0 << 1)
#define I2C_FLAG_FREQ_400K (1 << 1)
#define I2C_FLAG_FREQ_1M (2 << 1)

#endif
