/*
 * AppliedMicro X-Gene SoC Real Time Clock Driver
 *
 * Copyright (c) 2013, Applied Micro Circuits Corporation
 * Author: Rameshwar Prasad Sahu <rsahu@apm.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/rtc.h>

#undef XGENE_RTC_DEBUG
#define DRV_NAME	"xgene-rtc"

/* RTC Register Offset */

#define RTC_CCVR   	0x00
#define RTC_CMR		0x04
#define RTC_CLR		0x08
#define RTC_CCR		0x0C
#define RTC_STAT	0x10
#define RTC_RSTAT	0x14
#define RTC_EOI		0x18
#define RTC_VER		0x1C

#define RTC_CCR_WEN	0x08
#define RTC_CCR_EN	0x04
#define RTC_CCR_MASK	0x02
#define RTC_CCR_IE	0x01

#define RTC_STAT_BIT	0x01

#if defined(XGENE_RTC_DEBUG)
#define XGENE_RTC_LOG(fmt...) do { printk(KERN_INFO "X-Gene RTC CSR: " fmt); } while (0)
#else
#define XGENE_RTC_LOG(fmt...)
#endif

struct xgene_rtc_pdata {
	struct rtc_device *rtc;
	struct device *dev;
	unsigned long alarm_time;
	void __iomem *baseaddr;
	int irq;	
	spinlock_t lock;
};

static u32 xgene_rtc_readl(struct xgene_rtc_pdata *pdata, u32 reg)
{
        u32 val = readl(pdata->baseaddr + reg);
	XGENE_RTC_LOG("RD: 0x%p  VAL: 0x%08X\n", pdata->baseaddr + reg, val);

	return val;
}

static void xgene_rtc_writel(struct xgene_rtc_pdata *pdata, u32 reg, u32 val)
{
        writel(val, pdata->baseaddr + reg);
	XGENE_RTC_LOG("WR: 0x%p  VAL: 0x%08X\n", pdata->baseaddr + reg, val);
}

static int xgene_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	struct xgene_rtc_pdata *pdata = dev_get_drvdata(dev);
	unsigned long rtc_time;
	unsigned long flags;

	spin_lock_irqsave(&pdata->lock, flags);
	rtc_time = (unsigned long) xgene_rtc_readl(pdata, RTC_CCVR);
	spin_unlock_irqrestore(&pdata->lock, flags);

	rtc_time_to_tm(rtc_time, tm);

	pr_debug(DRV_NAME "tm is secs=%d, mins=%d, hours=%d, "
		"mday=%d, mon=%d, year=%d, wday=%d\n",
		tm->tm_sec, tm->tm_min, tm->tm_hour,tm->tm_mday,
		tm->tm_mon, tm->tm_year, tm->tm_wday);

	return 0;
}

static int xgene_rtc_set_mmss(struct device *dev, unsigned long secs)
{
	struct xgene_rtc_pdata *pdata = dev_get_drvdata(dev);
	volatile u32 rtc_time;
	volatile u32 loop_cnt = 0;
	u32 cnt = (u32) secs;
	unsigned long flags;
	
	spin_lock_irqsave(&pdata->lock, flags);
	rtc_time = xgene_rtc_readl(pdata, RTC_CCVR);
	xgene_rtc_writel(pdata, RTC_CLR, cnt);

	do {
		rtc_time = xgene_rtc_readl(pdata, RTC_CCVR);
		loop_cnt++;
		udelay(100);
	} while ((rtc_time != cnt) && (loop_cnt != 50000));

	spin_unlock_irqrestore(&pdata->lock, flags);

	if (loop_cnt >= 50000)
		printk("%s: Failed to load RTC counter 0x%x\n", __func__, cnt);

	return 0;
}

static int xgene_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct xgene_rtc_pdata *pdata = dev_get_drvdata(dev);
	unsigned long flags;

	spin_lock_irqsave(&pdata->lock, flags);
	rtc_time_to_tm(pdata->alarm_time, &alrm->time);
	alrm->enabled = xgene_rtc_readl(pdata, RTC_CCR) & RTC_CCR_IE;
	spin_unlock_irqrestore(&pdata->lock, flags);

	return 0;
}

static int xgene_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct xgene_rtc_pdata *pdata = dev_get_drvdata(dev);
	unsigned long rtc_time;
	unsigned long alarm_time;
	u32 ccr;
	int ret;
	unsigned long flags;

	if (pdata->irq <= 0)
		return -EINVAL;

	spin_lock_irqsave(&pdata->lock, flags);
	rtc_time = (unsigned long) xgene_rtc_readl(pdata, RTC_CCVR);

	ret = rtc_tm_to_time(&alrm->time, &alarm_time);
	if (ret) {
		spin_unlock_irqrestore(&pdata->lock, flags);
		return ret;
	}

	if (alarm_time < rtc_time) {
		spin_unlock_irqrestore(&pdata->lock, flags);
		return -EINVAL;
	}

	pdata->alarm_time = alarm_time;
	xgene_rtc_writel(pdata, RTC_CMR, (u32)pdata->alarm_time);

	ccr = xgene_rtc_readl(pdata, RTC_CCR);

	if (alrm->enabled) {
		ccr &= ~RTC_CCR_MASK;
		ccr |= RTC_CCR_IE;
	} else {
		ccr &= ~RTC_CCR_IE;
		ccr |= RTC_CCR_MASK;
	}

	xgene_rtc_writel(pdata, RTC_CCR, ccr);

	spin_unlock_irqrestore(&pdata->lock, flags);

	return 0;
}

static int xgene_rtc_proc(struct device *dev, struct seq_file *seq)
{
	struct xgene_rtc_pdata *pdata = dev_get_drvdata(dev);
	u32 reg;

	reg= xgene_rtc_readl(pdata, RTC_CCVR);
	seq_printf(seq, "Current Counter Value : %u\n", reg);

	reg = xgene_rtc_readl(pdata, RTC_CMR);
	seq_printf(seq, "Counter Match Value   : %u\n", reg);

	reg = xgene_rtc_readl(pdata, RTC_CLR);
	seq_printf(seq, "Counter Load Value    : %u\n", reg);

	reg = xgene_rtc_readl(pdata, RTC_CCR);
	seq_printf(seq, "Counter Control       : 0x%X\n", reg);	

	reg= xgene_rtc_readl(pdata, RTC_STAT);
	seq_printf(seq, "Interrupt Status      : 0x%X\n", reg);

	reg = xgene_rtc_readl(pdata, RTC_RSTAT);
	seq_printf(seq, "Raw Interrupt Status  : 0x%X\n", reg);	

	reg = xgene_rtc_readl(pdata, RTC_VER);
	seq_printf(seq, "RTC Component Version : 0x%X\n", reg);

	return 0;
}

static int xgene_rtc_alarm_irq_enable(struct device *dev, u32 enabled)
{
	struct xgene_rtc_pdata *pdata = dev_get_drvdata(dev);
	u32 ccr;
	unsigned long flags;

	spin_lock_irqsave(&pdata->lock, flags);

	ccr = xgene_rtc_readl(pdata, RTC_CCR);

	if (enabled) {
		ccr &= ~RTC_CCR_MASK;
		ccr |= RTC_CCR_IE;
	} else {
		ccr &= ~RTC_CCR_IE;
		ccr |= RTC_CCR_MASK;
	}

	xgene_rtc_writel(pdata, RTC_CCR, ccr);

	spin_unlock_irqrestore(&pdata->lock, flags);

	return 0;
}

static const struct rtc_class_ops xgene_rtc_ops = {
	.read_time	  = xgene_rtc_read_time,
	.set_mmss 	  = xgene_rtc_set_mmss,
	.read_alarm	  = xgene_rtc_read_alarm,
	.set_alarm	  = xgene_rtc_set_alarm,
	.proc		  = xgene_rtc_proc,
        .alarm_irq_enable = xgene_rtc_alarm_irq_enable,
};

static irqreturn_t xgene_rtc_interrupt(int irq, void *dev_id)
{
	struct xgene_rtc_pdata *pdata = (struct xgene_rtc_pdata *) dev_id;
	unsigned long events = RTC_IRQF;
	unsigned long flags;
	u32 rtc_stat;
	u32 rtc_eoi;

	spin_lock_irqsave(&pdata->lock, flags);

	/* read interrupt*/
	rtc_stat = xgene_rtc_readl(pdata, RTC_STAT);
	if (!(rtc_stat & RTC_STAT_BIT)){
	 	spin_unlock_irqrestore(&pdata->lock, flags);
		return IRQ_NONE;
	}

	/* clear interrupt  */
        rtc_eoi = xgene_rtc_readl(pdata, RTC_EOI);

	events |= RTC_AF;
	if (pdata->rtc)
		rtc_update_irq(pdata->rtc, 1, events);
	spin_unlock_irqrestore(&pdata->lock, flags);

	return IRQ_HANDLED;
}

static int xgene_rtc_probe(struct platform_device *pdev)
{
	struct rtc_device *rtc;
	struct xgene_rtc_pdata *pdata;
	struct clk *clk = NULL;
	int ret;

	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	pdata->dev = &pdev->dev;

	pdata->baseaddr = of_iomap(pdev->dev.of_node, 0);
	if (!pdata->baseaddr) {
		dev_err(pdata->dev, "No RTC IO register entry in DTS\n");
		ret = -ENOMEM;
		goto fail_map;
	}

	pdata->irq = irq_of_parse_and_map(pdev->dev.of_node, 0);
	if (pdata->irq > 0){
		ret = request_irq(pdata->irq, xgene_rtc_interrupt,
				     0, DRV_NAME, pdata);
		if (ret) {
			dev_err(pdata->dev, "Failed to Register Interrupt\n");
			goto fail_request;
		}
	}

        clk = clk_get(pdata->dev, NULL);
	if (IS_ERR(clk)) {
               dev_err(pdata->dev, "Couldn't get the clock for RTC\n");
	       return -ENODEV;
	}
        clk_prepare_enable(clk);

	/*
	 * turn on the clock and the crystal, etc.
 	 */
	xgene_rtc_writel(pdata, RTC_CCR, RTC_CCR_EN);

	device_init_wakeup(&pdev->dev, 1);

	dev_set_drvdata(&pdev->dev, pdata);

	rtc = rtc_device_register(DRV_NAME, &pdev->dev, &xgene_rtc_ops, 
				  	THIS_MODULE);
	if (IS_ERR(rtc)) {
		ret = PTR_ERR(rtc);
		goto out;
	}

	rtc->uie_unsupported =1;
	pdata->rtc = rtc;	

	spin_lock_init(&pdata->lock);

	return 0;

out:
	if (pdata->rtc)
		rtc_device_unregister(pdata->rtc);

	dev_set_drvdata(&pdev->dev, NULL);
	free_irq(pdata->irq, pdata);

fail_request:
	irq_dispose_mapping(pdata->irq);
	iounmap(pdata->baseaddr);

fail_map:
	kfree(pdata);
	return ret;
}

static int xgene_rtc_remove(struct platform_device *pdev)
{
	struct xgene_rtc_pdata *pdata = dev_get_drvdata(&pdev->dev);

	rtc_device_unregister(pdata->rtc);
	pdata->rtc = NULL;
	dev_set_drvdata(&pdev->dev, NULL);

	if (pdata->irq > 0)
		free_irq(pdata->irq, pdata);

	irq_dispose_mapping(pdata->irq);
	iounmap(pdata->baseaddr);

	return 0;
}

static const struct of_device_id xgene_rtc_of_match[] = {
	{.compatible = "apm,xgene-rtc",} ,
	{},
};

/* Structure for a device driver */
static struct platform_driver xgene_rtc_driver = {
	.probe		= xgene_rtc_probe,
	.remove		= xgene_rtc_remove,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= DRV_NAME,
		.of_match_table	= xgene_rtc_of_match,
	},
};

module_platform_driver(xgene_rtc_driver);

MODULE_DESCRIPTION("APM X-Gene RTC driver");
MODULE_AUTHOR("Rameshwar Sahu <rsahu@apm.com>");
MODULE_LICENSE("GPL");
