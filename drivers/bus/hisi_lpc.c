/*
 * Copyright (C) 2017 Hisilicon Limited, All Rights Reserved.
 * Author: Zhichang Yuan <yuanzhichang@hisilicon.com>
 * Author: Zou Rongrong <zourongrong@huawei.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/acpi.h>
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/libio.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/pci.h>
#include <linux/serial_8250.h>
#include <linux/slab.h>

/*
 * Setting this bit means each IO operation will target to a
 * different port address:
 * 0 means repeatedly IO operations will stick on the same port,
 * such as BT;
 */
#define FG_INCRADDR_LPC		0x02

struct lpc_cycle_para {
	unsigned int opflags;
	unsigned int csize; /* the data length of each operation */
};

struct hisilpc_dev {
	spinlock_t cycle_lock;
	void __iomem  *membase;
	struct libio_range *io_host;
};

/* bounds of the LPC bus address range */
#define LPC_MIN_BUS_RANGE	0x0

/*
 * The default maximal IO size for Hip06/Hip07 LPC bus.
 * Defining the I/O range size as 0x400 here should be sufficient for
 * all peripherals under the bus.
 */
#define LPC_BUS_IO_SIZE		0x400

/* The maximum continuous cycles per burst */
#define LPC_MAX_BURST	16
/* The IO cycle counts supported is four per operation at maximum */
#define LPC_MAX_DULEN	4
#if LPC_MAX_DULEN > LPC_MAX_BURST
#error "LPC.. MAX_DULEN must be not bigger than MAX_OPCNT!"
#endif

#if LPC_MAX_BURST % LPC_MAX_DULEN
#error "LPC.. LPC_MAX_BURST must be multiple of LPC_MAX_DULEN!"
#endif

#define LPC_REG_START		0x00 /* start a new LPC cycle */
#define LPC_REG_OP_STATUS	0x04 /* the current LPC status */
#define LPC_REG_IRQ_ST		0x08 /* interrupt enable&status */
#define LPC_REG_OP_LEN		0x10 /* how many LPC cycles each start */
#define LPC_REG_CMD		0x14 /* command for the required LPC cycle */
#define LPC_REG_ADDR		0x20 /* LPC target address */
#define LPC_REG_WDATA		0x24 /* data to be written */
#define LPC_REG_RDATA		0x28 /* data coming from peer */


/* The command register fields */
#define LPC_CMD_SAMEADDR	0x08
#define LPC_CMD_TYPE_IO		0x00
#define LPC_CMD_WRITE		0x01
#define LPC_CMD_READ		0x00
/* the bit attribute is W1C. 1 represents OK. */
#define LPC_STAT_BYIRQ		0x02

#define LPC_STATUS_IDLE		0x01
#define LPC_OP_FINISHED		0x02

#define START_WORK		0x01

/*
 * The minimal nanosecond interval for each query on LPC cycle status.
 */
#define LPC_NSEC_PERWAIT	100
/*
 * The maximum waiting time is about 128us.
 * It is specific for stream I/O, such as ins.
 * The fastest IO cycle time is about 390ns, but the worst case will wait
 * for extra 256 lpc clocks, so (256 + 13) * 30ns = 8 us. The maximum
 * burst cycles is 16. So, the maximum waiting time is about 128us under
 * worst case.
 * choose 1300 as the maximum.
 */
#define LPC_MAX_WAITCNT		1300
/* About 10us. This is specific for single IO operation, such as inb. */
#define LPC_PEROP_WAITCNT	100


static inline int wait_lpc_idle(unsigned char *mbase,
				unsigned int waitcnt) {
	u32 opstatus;

	while (waitcnt--) {
		ndelay(LPC_NSEC_PERWAIT);
		opstatus = readl(mbase + LPC_REG_OP_STATUS);
		if (opstatus & LPC_STATUS_IDLE)
			return (opstatus & LPC_OP_FINISHED) ? 0 : (-EIO);
	}
	return -ETIME;
}

/*
 * hisilpc_target_in - trigger a series of lpc cycles to read required data
 *		       from target peripheral.
 * @pdev: pointer to hisi lpc device
 * @para: some parameters used to control the lpc I/O operations
 * @ptaddr: the lpc I/O target port address
 * @buf: where the read back data is stored
 * @opcnt: how many I/O operations required in this calling
 *
 * Only one byte data is read each I/O operation.
 *
 * Returns 0 on success, non-zero on fail.
 *
 */
static int
hisilpc_target_in(struct hisilpc_dev *lpcdev, struct lpc_cycle_para *para,
		  unsigned long ptaddr, unsigned char *buf,
		  unsigned long opcnt)
{
	unsigned long cnt_per_trans;
	unsigned int cmd_word;
	unsigned int waitcnt;
	int ret;

	if (!buf || !opcnt || !para || !para->csize || !lpcdev)
		return -EINVAL;

	cmd_word = LPC_CMD_TYPE_IO | LPC_CMD_READ;
	waitcnt = LPC_PEROP_WAITCNT;
	if (!(para->opflags & FG_INCRADDR_LPC)) {
		cmd_word |= LPC_CMD_SAMEADDR;
		waitcnt = LPC_MAX_WAITCNT;
	}

	ret = 0;
	cnt_per_trans = (para->csize == 1) ? opcnt : para->csize;
	for (; opcnt && !ret; cnt_per_trans = para->csize) {
		unsigned long flags;

		/* whole operation must be atomic */
		spin_lock_irqsave(&lpcdev->cycle_lock, flags);

		writel_relaxed(cnt_per_trans, lpcdev->membase + LPC_REG_OP_LEN);

		writel_relaxed(cmd_word, lpcdev->membase + LPC_REG_CMD);

		writel_relaxed(ptaddr, lpcdev->membase + LPC_REG_ADDR);

		writel(START_WORK, lpcdev->membase + LPC_REG_START);

		/* whether the operation is finished */
		ret = wait_lpc_idle(lpcdev->membase, waitcnt);
		if (!ret) {
			opcnt -= cnt_per_trans;
			for (cnt_per_trans--; cnt_per_trans--; buf++)
				*buf = readb_relaxed(lpcdev->membase +
					LPC_REG_RDATA);
			*buf = readb(lpcdev->membase + LPC_REG_RDATA);
		}

		spin_unlock_irqrestore(&lpcdev->cycle_lock, flags);
	}

	return ret;
}

/*
 * hisilpc_target_out - trigger a series of lpc cycles to write required
 *			data to target peripheral.
 * @pdev: pointer to hisi lpc device
 * @para: some parameters used to control the lpc I/O operations
 * @ptaddr: the lpc I/O target port address
 * @buf: where the data to be written is stored
 * @opcnt: how many I/O operations required
 *
 * Only one byte data is read each I/O operation.
 *
 * Returns 0 on success, non-zero on fail.
 *
 */
static int
hisilpc_target_out(struct hisilpc_dev *lpcdev, struct lpc_cycle_para *para,
		   unsigned long ptaddr, const unsigned char *buf,
		   unsigned long opcnt)
{
	unsigned long cnt_per_trans;
	unsigned int cmd_word;
	unsigned int waitcnt;
	int ret;

	if (!buf || !opcnt || !para || !lpcdev)
		return -EINVAL;

	/* default is increasing address */
	cmd_word = LPC_CMD_TYPE_IO | LPC_CMD_WRITE;
	waitcnt = LPC_PEROP_WAITCNT;
	if (!(para->opflags & FG_INCRADDR_LPC)) {
		cmd_word |= LPC_CMD_SAMEADDR;
		waitcnt = LPC_MAX_WAITCNT;
	}

	ret = 0;
	cnt_per_trans = (para->csize == 1) ? opcnt : para->csize;
	for (; opcnt && !ret; cnt_per_trans = para->csize) {
		unsigned long flags;

		spin_lock_irqsave(&lpcdev->cycle_lock, flags);

		writel_relaxed(cnt_per_trans, lpcdev->membase + LPC_REG_OP_LEN);
		writel_relaxed(cmd_word, lpcdev->membase + LPC_REG_CMD);
		writel_relaxed(ptaddr, lpcdev->membase + LPC_REG_ADDR);

		opcnt -= cnt_per_trans;
		for (; cnt_per_trans--; buf++)
			writeb_relaxed(*buf, lpcdev->membase + LPC_REG_WDATA);

		writel(START_WORK, lpcdev->membase + LPC_REG_START);

		/* whether the operation is finished */
		ret = wait_lpc_idle(lpcdev->membase, waitcnt);

		spin_unlock_irqrestore(&lpcdev->cycle_lock, flags);
	}

	return ret;
}

static inline unsigned long
hisi_lpc_pio_to_addr(struct hisilpc_dev *lpcdev, unsigned long pio)
{
	return pio - lpcdev->io_host->io_start + lpcdev->io_host->hw_start;
}


/**
 * hisilpc_comm_in - read/input the data from the I/O peripheral
 *		     through LPC.
 * @devobj: pointer to the device information relevant to LPC controller.
 * @pio: the target I/O port address.
 * @dlen: the data length required to read from the target I/O port.
 *
 * when succeed, the data read back is stored in buffer pointed by inbuf.
 * For inb, return the data read from I/O or -1 when error occur.
 */
static u32 hisilpc_comm_in(void *devobj, unsigned long pio, size_t dlen)
{
	int ret = 0;
	u32 rd_data;
	unsigned long ptaddr;
	unsigned char *newbuf;
	struct lpc_cycle_para iopara;
	struct hisilpc_dev *lpcdev = devobj;

	if (!lpcdev || !dlen || dlen > LPC_MAX_DULEN ||	(dlen & (dlen - 1)))
		return -1;

	/* the local buffer must be enough for one data unit */
	if (sizeof(rd_data) < dlen)
		return -1;

	newbuf = (unsigned char *)&rd_data;

	ptaddr = hisi_lpc_pio_to_addr(lpcdev, pio);

	iopara.opflags = FG_INCRADDR_LPC;
	iopara.csize = dlen;

	ret = hisilpc_target_in(lpcdev, &iopara, ptaddr, newbuf, dlen);
	if (ret)
		return -1;

	return le32_to_cpu(rd_data);
}

/**
 * hisilpc_comm_out - output the data whose maximum length is four bytes
		      to the I/O peripheral through the LPC host.
 * @devobj: pointer to the device information relevant to LPC controller.
 * @outval: a value to be outputted from caller, maximum is four bytes.
 * @pio: the target I/O port address.
 * @dlen: the data length required writing to the target I/O port.
 *
 * This function is corresponding to out(b,w,l) only
 *
 */
static void hisilpc_comm_out(void *devobj, unsigned long pio,
			     u32 outval, size_t dlen)
{
	unsigned long ptaddr;
	struct hisilpc_dev *lpcdev = devobj;
	struct lpc_cycle_para iopara;
	const unsigned char *newbuf;

	if (!lpcdev || !dlen || dlen > LPC_MAX_DULEN)
		return;

	if (sizeof(outval) < dlen)
		return;

	outval = cpu_to_le32(outval);

	newbuf = (const unsigned char *)&outval;
	ptaddr = hisi_lpc_pio_to_addr(lpcdev, pio);

	iopara.opflags = FG_INCRADDR_LPC;
	iopara.csize = dlen;

	hisilpc_target_out(lpcdev, &iopara, ptaddr, newbuf, dlen);
}

/*
 * hisilpc_comm_ins - read/input the data in buffer to the I/O
 *		peripheral through LPC, it corresponds to ins(b,w,l)
 * @devobj: pointer to the device information relevant to LPC controller.
 * @pio: the target I/O port address.
 * @inbuf: a buffer where read/input data bytes are stored.
 * @dlen: the data length required writing to the target I/O port.
 * @count: how many data units whose length is dlen will be read.
 *
 */
static u32
hisilpc_comm_ins(void *devobj, unsigned long pio, void *inbuf,
		 size_t dlen, unsigned int count)
{
	struct hisilpc_dev *lpcdev = devobj;
	struct lpc_cycle_para iopara;
	unsigned char *newbuf;
	unsigned int loopcnt, cntleft;
	unsigned long ptaddr;

	if (!lpcdev || !inbuf || !count || !dlen ||
		dlen > LPC_MAX_DULEN || (dlen & (dlen - 1)) || count % dlen)
		return -EINVAL;

	iopara.opflags = 0;
	if (dlen > 1)
		iopara.opflags |= FG_INCRADDR_LPC;
	iopara.csize = dlen;

	ptaddr = hisi_lpc_pio_to_addr(lpcdev, pio);
	newbuf = (unsigned char *)inbuf;
	/*
	 * ensure data stream whose length is multiple of dlen to be processed
	 * each IO input
	 */
	cntleft = count * dlen;
	do {
		int ret;

		loopcnt = (cntleft >= LPC_MAX_BURST) ? LPC_MAX_BURST : cntleft;
		ret = hisilpc_target_in(lpcdev, &iopara, ptaddr,
					newbuf, loopcnt);
		if (ret)
			return ret;
		newbuf += loopcnt;
		cntleft -= loopcnt;
	} while (cntleft);

	return 0;
}

/*
 * hisilpc_comm_outs - write/output the data in buffer to the I/O
 *		peripheral through LPC, it corresponds to outs(b,w,l)
 * @devobj: pointer to the device information relevant to LPC controller.
 * @pio: the target I/O port address.
 * @outbuf: a buffer where write/output data bytes are stored.
 * @dlen: the data length required writing to the target I/O port .
 * @count: how many data units whose length is dlen will be written.
 *
 */
static void
hisilpc_comm_outs(void *devobj, unsigned long pio, const void *outbuf,
		  size_t dlen, unsigned int count)
{
	struct hisilpc_dev *lpcdev = devobj;
	struct lpc_cycle_para iopara;
	const unsigned char *newbuf;
	unsigned int loopcnt, cntleft;
	unsigned long ptaddr;

	if (!lpcdev || !outbuf || !count || !dlen ||
		dlen > LPC_MAX_DULEN || (dlen & (dlen - 1)) || count % dlen)
		return;

	iopara.opflags = 0;
	if (dlen > 1)
		iopara.opflags |= FG_INCRADDR_LPC;
	iopara.csize = dlen;

	ptaddr = hisi_lpc_pio_to_addr(lpcdev, pio);
	newbuf = (unsigned char *)outbuf;
	/*
	 * ensure data stream whose length is multiple of dlen to be processed
	 * each IO input
	 */
	cntleft = count * dlen;
	do {
		loopcnt = (cntleft >= LPC_MAX_BURST) ? LPC_MAX_BURST : cntleft;
		if (hisilpc_target_out(lpcdev, &iopara, ptaddr, newbuf,
						loopcnt))
			break;
		newbuf += loopcnt;
		cntleft -= loopcnt;
	} while (cntleft);
}

static struct libio_ops hisi_lpc_ops = {
	.pfin = hisilpc_comm_in,
	.pfout = hisilpc_comm_out,
	.pfins = hisilpc_comm_ins,
	.pfouts = hisilpc_comm_outs,
};


static int hisilpc_host_io_register(struct device *dev)
{
	struct libio_range *range, *tmprange;

	/*
	 * indirectIO bus was detected, time to request the linux virtual
	 * IO.
	 */
	range = kzalloc(sizeof(*range), GFP_KERNEL);
	if (!range)
		return -ENOMEM;
	range->node = dev->fwnode;
	range->flags = IO_HOST_INDIRECT;
	range->size = LPC_BUS_IO_SIZE;
	range->hw_start = LPC_MIN_BUS_RANGE;

	tmprange = register_libio_range(range);
	if (tmprange != range) {
		kfree(range);
		if (IS_ERR(tmprange))
			return -EFAULT;
	}

	/*
	 * For ACPI children, translate the bus-local I/O range to logical
	 * I/O range and set it as the current resource before the children
	 * are enumerated.
	 */
	if (has_acpi_companion(dev)) {
		struct acpi_device *root, *child;

		root = to_acpi_device_node(dev->fwnode);
		/* For hisilpc, only care about the sons of host. */
		list_for_each_entry(child, &root->children, node) {
			int ret;

			ret = acpi_set_libio_resource(&child->dev, &root->dev);
			if (ret) {
				dev_err(&child->dev, "set resource failed..\n");
				return ret;
			}
		}
	}

	return 0;
}

/**
 * hisilpc_probe - the probe callback function for hisi lpc device,
 *		   will finish all the initialization.
 * @pdev: the platform device corresponding to hisi lpc
 *
 * Returns 0 on success, non-zero on fail.
 *
 */
static int hisilpc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct resource *res;
	struct hisilpc_dev *lpcdev;
	int ret = 0;

	dev_info(dev, "probing...\n");

	lpcdev = devm_kzalloc(dev, sizeof(struct hisilpc_dev), GFP_KERNEL);
	if (!lpcdev)
		return -ENOMEM;

	spin_lock_init(&lpcdev->cycle_lock);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "no MEM resource\n");
		return -ENODEV;
	}

	lpcdev->membase = devm_ioremap_resource(dev, res);
	if (IS_ERR(lpcdev->membase)) {
		dev_err(dev, "remap failed\n");
		return PTR_ERR(lpcdev->membase);
	}

	lpcdev->io_host = find_io_range_from_fwnode(dev->fwnode);
	if (!lpcdev->io_host) {
		dev_err(dev, "Hisilpc IO hasn't registered!\n");
		return -EFAULT;
	}

	lpcdev->io_host->devpara = lpcdev;
	lpcdev->io_host->ops = &hisi_lpc_ops;

	platform_set_drvdata(pdev, lpcdev);

	/*
	 * It is time to start the children scannings....
	 */
	if (!has_acpi_companion(dev)) {
		ret = of_platform_populate(dev->of_node, NULL, NULL, dev);
		if (ret)
			dev_err(dev, "OF: enumerate LPC bus fail(%d)\n", ret);
	}

	if (!ret) {
		dev_info(dev, "hslpc end probing. range[%pa - sz:%pa]\n",
			 &lpcdev->io_host->io_start,
			 &lpcdev->io_host->size);
	} else {
		dev_info(dev, "hslpc probing is fail(%d)\n", ret);
		/*
		 * When LPC probing is not completely successful, set 'devpara'
		 * as NULL. This will make all the LPC I/O return failure
		 * directly without any hardware operations. It will block
		 * some peripherals which had not finished the initialization
		 * manipulate I/O for safety.
		 */
		lpcdev->io_host->devpara = NULL;
	}

	return ret;
}

static const struct of_device_id hisilpc_of_match[] = {
	{ .compatible = "hisilicon,hip06-lpc", },
	{ .compatible = "hisilicon,hip07-lpc", },
	{},
};

static const struct acpi_device_id hisilpc_acpi_match[] = {
	{"HISI0191", },
	{},
};

static struct platform_driver hisilpc_driver = {
	.driver = {
		.name           = "hisi_lpc",
		.of_match_table = hisilpc_of_match,
		.acpi_match_table = ACPI_PTR(hisilpc_acpi_match),
	},
	.probe = hisilpc_probe,
};

/*
 * hisilpc_bus_platform_notify -- notify callback function specific for
 *			hisi-lpc bus. Here, will register linux virtual
 *			PIO for the bus detected, then the bus children
 *			can translate their bus-local IO to linux PIO.
 */
static int hisilpc_bus_platform_notify(struct notifier_block *nb,
			unsigned long action, void *data)
{
	struct device *dev = data;
	int ret;

	if (!is_of_node(dev->fwnode) && !is_acpi_node(dev->fwnode))
		return NOTIFY_DONE;

	if (action != BUS_NOTIFY_ADD_DEVICE)
		return NOTIFY_DONE;

	/* whether the device notified is hisi-lpc? */
	if (has_acpi_companion(dev)) {
		if (!acpi_match_device(hisilpc_acpi_match, dev))
			return NOTIFY_DONE;
	} else {
		if (!of_match_node(hisilpc_of_match, dev->of_node))
			return NOTIFY_DONE;
	}

	/* register the LPC host PIO resources */
	ret = hisilpc_host_io_register(dev);
	if (ret) {
		dev_err(dev, "host PIO registration failed!\n");
		return NOTIFY_DONE;
	}

	dev_info(dev, "hisilpc notifier processes OK!\n");
	return NOTIFY_OK;
}

static struct notifier_block hisilpc_preinit_nb = {
	.notifier_call = hisilpc_bus_platform_notify,
};

static int __init hisilpc_init(void)
{
	int ret;

	ret = bus_register_notifier(&platform_bus_type, &hisilpc_preinit_nb);
	if (!ret)
		ret = platform_driver_register(&hisilpc_driver);

	return ret;
}

/*
 * This initial funtion must be called before the platform bus scanning to make
 * the lpc-relevant I/O resource is ready for the device enumeration.
 */
arch_initcall(hisilpc_init);
