// SPDX-License-Identifier: GPL-2.0
/*
 * FU740 DesignWare PCIe Controller integration
 * Copyright (C) 2019-2021 SiFive, Inc.
 * Paul Walmsley
 * Greentime Hu
 *
 * Based in part on the i.MX6 PCIe host controller shim which is:
 *
 * Copyright (C) 2013 Kosagi
 *		https://www.kosagi.com
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/resource.h>
#include <linux/signal.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/reset.h>
#include <linux/pm_domain.h>
#include <linux/pm_runtime.h>

#include "pcie-designware.h"

#define to_fu740_pcie(x)	dev_get_drvdata((x)->dev)

struct fu740_pcie {
	struct dw_pcie *pci;
	void __iomem *mgmt_base;
	int perstn_gpio;
	int pwren_gpio;
	struct clk *pcie_aux;
	struct reset_control *rst;
};

#define SIFIVE_DEVICESRESETREG			0x28

#define PCIEX8MGMT_PERST_N			0x0
#define PCIEX8MGMT_APP_LTSSM_ENABLE		0x10
#define PCIEX8MGMT_APP_HOLD_PHY_RST		0x18
#define PCIEX8MGMT_DEVICE_TYPE			0x708
#define PCIEX8MGMT_PHY0_CR_PARA_ADDR		0x860
#define PCIEX8MGMT_PHY0_CR_PARA_RD_EN		0x870
#define PCIEX8MGMT_PHY0_CR_PARA_RD_DATA		0x878
#define PCIEX8MGMT_PHY0_CR_PARA_SEL		0x880
#define PCIEX8MGMT_PHY0_CR_PARA_WR_DATA		0x888
#define PCIEX8MGMT_PHY0_CR_PARA_WR_EN		0x890
#define PCIEX8MGMT_PHY0_CR_PARA_ACK		0x898
#define PCIEX8MGMT_PHY1_CR_PARA_ADDR		0x8a0
#define PCIEX8MGMT_PHY1_CR_PARA_RD_EN		0x8b0
#define PCIEX8MGMT_PHY1_CR_PARA_RD_DATA		0x8b8
#define PCIEX8MGMT_PHY1_CR_PARA_SEL		0x8c0
#define PCIEX8MGMT_PHY1_CR_PARA_WR_DATA		0x8c8
#define PCIEX8MGMT_PHY1_CR_PARA_WR_EN		0x8d0
#define PCIEX8MGMT_PHY1_CR_PARA_ACK		0x8d8

/* PCIe Port Logic registers (memory-mapped) */
#define PL_OFFSET				0x700
#define PCIE_PL_GEN2_CTRL_OFF			(PL_OFFSET + 0x10c)
#define PCIE_PL_DIRECTED_SPEED_CHANGE_OFF	0x20000

#define PCIE_PHY_MAX_RETRY_CNT			1000

static void fu740_pcie_assert_perstn(struct fu740_pcie *afp)
{
	/* PERST_N GPIO */
	if (gpio_is_valid(afp->perstn_gpio))
		gpio_direction_output(afp->perstn_gpio, 0);

	/* Controller PERST_N */
	__raw_writel(0x0, afp->mgmt_base + PCIEX8MGMT_PERST_N);
}

static void fu740_pcie_deassert_perstn(struct fu740_pcie *afp)
{
	/* Controller PERST_N */
	__raw_writel(0x1, afp->mgmt_base + PCIEX8MGMT_PERST_N);
	/* PERST_N GPIO */
	if (gpio_is_valid(afp->perstn_gpio))
		gpio_direction_output(afp->perstn_gpio, 1);
}

static void fu740_pcie_power_on(struct fu740_pcie *afp)
{
	if (gpio_is_valid(afp->pwren_gpio)) {
		gpio_direction_output(afp->pwren_gpio, 1);
		mdelay(100);
	}
}

static void fu740_pcie_drive_perstn(struct fu740_pcie *afp)
{
	fu740_pcie_assert_perstn(afp);
	fu740_pcie_power_on(afp);
	fu740_pcie_deassert_perstn(afp);
}

static void fu740_phyregreadwrite(const uint8_t phy, const uint8_t write,
				const uint16_t addr,
				const uint16_t wrdata, uint16_t *rddata,
				struct fu740_pcie *afp)
{
	unsigned char ack = 0;
	unsigned int cnt = 0;
	struct device *dev = afp->pci->dev;

	/* setup */
	__raw_writel(addr,
		     afp->mgmt_base +
		     (phy ? PCIEX8MGMT_PHY1_CR_PARA_ADDR :
		      PCIEX8MGMT_PHY0_CR_PARA_ADDR));
	if (write)
		__raw_writel(wrdata,
			     afp->mgmt_base +
			     (phy ? PCIEX8MGMT_PHY1_CR_PARA_WR_DATA :
			      PCIEX8MGMT_PHY0_CR_PARA_WR_DATA));
	if (write)
		__raw_writel(1,
			     afp->mgmt_base +
			     (phy ? PCIEX8MGMT_PHY1_CR_PARA_WR_EN :
			      PCIEX8MGMT_PHY0_CR_PARA_WR_EN));
	else
		__raw_writel(1,
			     afp->mgmt_base +
			     (phy ? PCIEX8MGMT_PHY1_CR_PARA_RD_EN :
			      PCIEX8MGMT_PHY0_CR_PARA_RD_EN));

	/* wait for wait_idle */
	do {
		if (__raw_readl
		    (afp->mgmt_base +
		     (phy ? PCIEX8MGMT_PHY1_CR_PARA_ACK :
		      PCIEX8MGMT_PHY0_CR_PARA_ACK))) {
			ack = 1;
			if (!write)
				__raw_readl(afp->mgmt_base +
					    (phy ?
					     PCIEX8MGMT_PHY1_CR_PARA_RD_DATA :
					     PCIEX8MGMT_PHY0_CR_PARA_RD_DATA));
		}
	} while (!ack);

	/* clear */
	if (write)
		__raw_writel(0,
			     afp->mgmt_base +
			     (phy ? PCIEX8MGMT_PHY1_CR_PARA_WR_EN :
			      PCIEX8MGMT_PHY0_CR_PARA_WR_EN));
	else
		__raw_writel(0,
			     afp->mgmt_base +
			     (phy ? PCIEX8MGMT_PHY1_CR_PARA_RD_EN :
			      PCIEX8MGMT_PHY0_CR_PARA_RD_EN));

	/* wait for ~wait_idle */
	while (__raw_readl
	       (afp->mgmt_base +
		(phy ? PCIEX8MGMT_PHY1_CR_PARA_ACK :
		 PCIEX8MGMT_PHY0_CR_PARA_ACK))) {
		cpu_relax();
		cnt++;
		if (cnt > PCIE_PHY_MAX_RETRY_CNT) {
			dev_err(dev, "PCIE phy doesn't enter idle state.\n");
			break;
		}
	}
}

static void fu740_pcie_init_phy(struct fu740_pcie *afp)
{
	int lane;

	/* enable phy cr_para_sel interfaces */
	__raw_writel(0x1, afp->mgmt_base + PCIEX8MGMT_PHY0_CR_PARA_SEL);
	__raw_writel(0x1, afp->mgmt_base + PCIEX8MGMT_PHY1_CR_PARA_SEL);

	/* wait 10 cr_para cycles */
	msleep(1);

	/* set PHY AC termination mode */
	for (lane = 0; lane < 4; lane++) {
		fu740_phyregreadwrite(0, 1,
				     0x1008 + (0x100 * lane),
				     0x0e21, NULL, afp);
		fu740_phyregreadwrite(1, 1,
				     0x1008 + (0x100 * lane),
				     0x0e21, NULL, afp);
	}

}

static void fu740_pcie_ltssm_enable(struct device *dev)
{
	struct fu740_pcie *afp = dev_get_drvdata(dev);

	/* Enable LTSSM */
	__raw_writel(0x1, afp->mgmt_base + PCIEX8MGMT_APP_LTSSM_ENABLE);
}

static int fu740_pcie_start_link(struct dw_pcie *pci)
{
	struct device *dev = pci->dev;
	u8 offset = dw_pcie_find_capability(pci, PCI_CAP_ID_EXP);
	u32 tmp;
	int ret;

	/*
	 * Force Gen1 operation when starting the link.  In case the link is
	 * started in Gen2 mode, there is a possibility the devices on the
	 * bus will not be detected at all.  This happens with PCIe switches.
	 */
	dw_pcie_dbi_ro_wr_en(pci);
	tmp = dw_pcie_readl_dbi(pci, offset + PCI_EXP_LNKCAP);
	tmp &= ~PCI_EXP_LNKCAP_SLS;
	tmp |= PCI_EXP_LNKCAP_SLS_2_5GB;
	dw_pcie_writel_dbi(pci, offset + PCI_EXP_LNKCAP, tmp);
	dw_pcie_dbi_ro_wr_dis(pci);

	/* Start LTSSM. */
	fu740_pcie_ltssm_enable(dev);

	ret = dw_pcie_wait_for_link(pci);
	if (ret)
		goto err_reset_phy;

	/* Now set it to operate in Gen3 */
	dw_pcie_dbi_ro_wr_en(pci);
	tmp = dw_pcie_readl_dbi(pci, offset + PCI_EXP_LNKCAP);
	tmp &= ~PCI_EXP_LNKCAP_SLS;
	tmp |= PCI_EXP_LNKCAP_SLS_8_0GB;
	dw_pcie_writel_dbi(pci, offset + PCI_EXP_LNKCAP, tmp);
	/* Enable DIRECTED SPEED CHANGE bit of GEN2_CTRL_OFF register */
	tmp = dw_pcie_readl_dbi(pci, PCIE_PL_GEN2_CTRL_OFF);
	tmp |= PCIE_PL_DIRECTED_SPEED_CHANGE_OFF;
	dw_pcie_writel_dbi(pci, PCIE_PL_GEN2_CTRL_OFF, tmp);
	dw_pcie_dbi_ro_wr_dis(pci);

	ret = dw_pcie_wait_for_link(pci);
	if (ret)
		goto err_reset_phy;

	/*
	 * Reenable DIRECTED SPEED CHANGE.
	 *
	 * You need to set this bit after each speed change, but after
	 * reaching G1, setting it once doesn't seem to work (it reaches G3
	 * equalization states and then times out, falls back to G1). But
	 * If after that, you set it again, it then reaches G3 perfectly
	 * fine.
	 */
	dw_pcie_dbi_ro_wr_en(pci);
	tmp = dw_pcie_readl_dbi(pci, PCIE_PL_GEN2_CTRL_OFF);
	tmp |= PCIE_PL_DIRECTED_SPEED_CHANGE_OFF;
	dw_pcie_writel_dbi(pci, PCIE_PL_GEN2_CTRL_OFF, tmp);
	dw_pcie_dbi_ro_wr_dis(pci);

	ret = dw_pcie_wait_for_link(pci);
	if (ret)
		goto err_reset_phy;

	tmp = dw_pcie_readw_dbi(pci, offset + PCI_EXP_LNKSTA);
	dev_info(dev, "Link up, Gen%i\n", tmp & PCI_EXP_LNKSTA_CLS);
	return 0;

 err_reset_phy:
	dev_err(dev, "Failed to bring link up!\n"
		"PHY DEBUG_R0=0x%08x DEBUG_R1=0x%08x\n",
		dw_pcie_readl_dbi(pci, PCIE_PORT_DEBUG0),
		dw_pcie_readl_dbi(pci, PCIE_PORT_DEBUG1));
	return ret;
}

static int fu740_pcie_host_init(struct pcie_port *pp)
{
	struct dw_pcie *pci = to_dw_pcie_from_pp(pp);
	struct fu740_pcie *afp = to_fu740_pcie(pci);
	struct device *dev = pci->dev;
	int ret = 0;

	/* power on reset */
	fu740_pcie_drive_perstn(afp);

	/* enable pcieauxclk */
	ret = clk_prepare_enable(afp->pcie_aux);
	if (ret)
		dev_err(dev, "unable to enable pcie_aux clock\n");

	/*
	 * assert hold_phy_rst (hold the controller LTSSM in reset after
	 * power_up_rst_n
	 * for register programming with cr_para)
	 */
	__raw_writel(0x1, afp->mgmt_base + PCIEX8MGMT_APP_HOLD_PHY_RST);

	/* deassert power_up_rst_n */
	ret = reset_control_deassert(afp->rst);
	if (ret)
		dev_err(dev, "unable to deassert pcie_power_up_rst_n\n");

	fu740_pcie_init_phy(afp);

	/* disable pcieauxclk */
	clk_disable_unprepare(afp->pcie_aux);
	/* clear hold_phy_rst */
	__raw_writel(0x0, afp->mgmt_base + PCIEX8MGMT_APP_HOLD_PHY_RST);
	/* enable pcieauxclk */
	ret = clk_prepare_enable(afp->pcie_aux);
	/* set RC mode */
	__raw_writel(0x4, afp->mgmt_base + PCIEX8MGMT_DEVICE_TYPE);

	return 0;
}

static const struct dw_pcie_host_ops fu740_pcie_host_ops = {
	.host_init = fu740_pcie_host_init,
};

static const struct dw_pcie_ops dw_pcie_ops = {
	.start_link = fu740_pcie_start_link,
};

static const struct dev_pm_ops fu740_pcie_pm_ops = {
	SET_NOIRQ_SYSTEM_SLEEP_PM_OPS(fu740_pcie_suspend_noirq,
				      fu740_pcie_resume_noirq)
};

static int fu740_pcie_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct dw_pcie *pci;
	struct fu740_pcie *afp;
	struct resource *mgmt_res;
	struct device_node *node = dev->of_node;
	int ret;
	u16 val;

	afp = devm_kzalloc(dev, sizeof(*afp), GFP_KERNEL);
	if (!afp)
		return -ENOMEM;

	pci = devm_kzalloc(dev, sizeof(*pci), GFP_KERNEL);
	if (!pci)
		return -ENOMEM;

	pci->dev = dev;
	pci->ops = &dw_pcie_ops;
	pci->pp.ops = &fu740_pcie_host_ops;

	afp->pci = pci;

	mgmt_res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "mgmt");
	if (!mgmt_res) {
		dev_warn(dev, "missing required mgmt address range");
		return -ENOENT;
	}
	afp->mgmt_base = devm_ioremap_resource(dev, mgmt_res);
	if (IS_ERR(afp->mgmt_base))
		return PTR_ERR(afp->mgmt_base);

	/* Fetch GPIOs */
	afp->perstn_gpio = of_get_named_gpio(node, "perstn-gpios", 0);
	if (gpio_is_valid(afp->perstn_gpio)) {
		ret = devm_gpio_request_one(dev, afp->perstn_gpio,
					    GPIOF_OUT_INIT_LOW, "perstn-gpios");
		if (ret) {
			dev_err(dev, "unable to get perstn gpio\n");
			return ret;
		}
	} else if (afp->perstn_gpio == -EPROBE_DEFER) {
		dev_err(dev, "perst-gpios EPROBE_DEFER\n");
		return afp->perstn_gpio;
	}

	afp->pwren_gpio = of_get_named_gpio(node, "pwren-gpios", 0);
	if (gpio_is_valid(afp->pwren_gpio)) {
		ret = devm_gpio_request_one(dev, afp->pwren_gpio,
					    GPIOF_OUT_INIT_LOW, "pwren-gpios");
		if (ret) {
			dev_err(dev, "unable to get pwren gpio\n");
			return ret;
		}
	} else if (afp->pwren_gpio == -EPROBE_DEFER) {
		dev_err(dev, "pwren-gpios EPROBE_DEFER\n");
		return afp->pwren_gpio;
	}

	/* Fetch clocks */
	afp->pcie_aux = devm_clk_get(dev, "pcie_aux");
	if (IS_ERR(afp->pcie_aux))
		return dev_err_probe(dev, PTR_ERR(afp->pcie_aux),
					     "pcie_aux clock source missing or invalid\n");

	/* Fetch reset */
	afp->rst = devm_reset_control_get(dev, NULL);
	if (IS_ERR(afp->rst))
		return dev_err_probe(dev, PTR_ERR(afp->rst), "unable to get reset\n");

	platform_set_drvdata(pdev, afp);

	ret = dw_pcie_host_init(&pci->pp);
	if (ret < 0)
		return ret;

	if (pci_msi_enabled()) {
		u8 offset = dw_pcie_find_capability(pci, PCI_CAP_ID_MSI);

		val = dw_pcie_readw_dbi(pci, offset + PCI_MSI_FLAGS);
		val |= PCI_MSI_FLAGS_ENABLE;
		dw_pcie_writew_dbi(pci, offset + PCI_MSI_FLAGS, val);
	}

	return 0;
}

static void fu740_pcie_shutdown(struct platform_device *pdev)
{
	struct fu740_pcie *afp = platform_get_drvdata(pdev);

	/* bring down link, so bootloader gets clean state in case of reboot */
	fu740_pcie_assert_perstn(afp);
}

static const struct of_device_id fu740_pcie_of_match[] = {
	{.compatible = "sifive,fu740-pcie"},
	{},
};

static struct platform_driver fu740_pcie_driver = {
	.driver = {
		   .name = "fu740-pcie",
		   .of_match_table = fu740_pcie_of_match,
		   .suppress_bind_attrs = true,
		   .pm = &fu740_pcie_pm_ops,
		   },
	.probe = fu740_pcie_probe,
	.shutdown = fu740_pcie_shutdown,
};

static int __init fu740_pcie_init(void)
{
	return platform_driver_register(&fu740_pcie_driver);
}

device_initcall(fu740_pcie_init);
