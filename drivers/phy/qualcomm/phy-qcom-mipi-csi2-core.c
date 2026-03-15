// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2025, Linaro Ltd.
 */
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/pm_opp.h>
#include <linux/phy/phy.h>
#include <linux/phy/phy-mipi-dphy.h>
#include <linux/platform_device.h>
#include <linux/pm_domain.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/reset.h>
#include <linux/slab.h>

#include "phy-qcom-mipi-csi2.h"

static int
phy_qcom_mipi_csi2_set_clock_rates(struct mipi_csi2phy_device *csi2phy,
				   s64 link_freq)
{
	struct device *dev = csi2phy->dev;
	unsigned long opp_rate = link_freq / 4;
	struct dev_pm_opp *opp;
	long timer_rate;
	int ret;

	opp = dev_pm_opp_find_freq_ceil(dev, &opp_rate);
	if (IS_ERR(opp)) {
		dev_err(csi2phy->dev, "Couldn't find ceiling for %lld Hz\n",
			link_freq);
		return PTR_ERR(opp);
	}

	for (int i = 0; i < csi2phy->num_pds; i++) {
		unsigned int perf = dev_pm_opp_get_required_pstate(opp, i);

		ret = dev_pm_genpd_set_performance_state(csi2phy->pds[i], perf);
		if (ret) {
			dev_err(csi2phy->dev, "Couldn't set perf state %u\n",
				perf);
			dev_pm_opp_put(opp);
			return ret;
		}
	}
	dev_pm_opp_put(opp);

	ret = dev_pm_opp_set_rate(dev, opp_rate);
	if (ret) {
		dev_err(csi2phy->dev, "dev_pm_opp_set_rate() fail\n");
		return ret;
	}

	timer_rate = clk_round_rate(csi2phy->timer_clk, link_freq / 4);
	if (timer_rate < 0)
		return timer_rate;

	ret = clk_set_rate(csi2phy->timer_clk, timer_rate);
	if (ret)
		return ret;

	csi2phy->timer_clk_rate = timer_rate;

	return 0;
}

static int phy_qcom_mipi_csi2_configure(struct phy *phy,
					union phy_configure_opts *opts)
{
	struct mipi_csi2phy_device *csi2phy = phy_get_drvdata(phy);
	struct phy_configure_opts_mipi_dphy *dphy_cfg_opts = &opts->mipi_dphy;
	struct mipi_csi2phy_stream_cfg *stream_cfg = &csi2phy->stream_cfg;
	int ret;
	int i;

	ret = phy_mipi_dphy_config_validate(dphy_cfg_opts);
	if (ret)
		return ret;

	if (dphy_cfg_opts->lanes < 1 || dphy_cfg_opts->lanes > CSI2_MAX_DATA_LANES)
		return -EINVAL;

	stream_cfg->combo_mode = 0;
	stream_cfg->link_freq = dphy_cfg_opts->hs_clk_rate;
	stream_cfg->num_data_lanes = dphy_cfg_opts->lanes;

	/*
	 * phy_configure_opts_mipi_dphy.lanes starts from zero to
	 * the maximum number of enabled lanes.
	 *
	 * TODO: add support for bitmask of enabled lanes and polarities
	 * of those lanes to the phy_configure_opts_mipi_dphy struct.
	 * For now take the polarities as zero and the position as fixed
	 * this is fine as no current upstream implementation maps otherwise.
	 */
	for (i = 0; i < stream_cfg->num_data_lanes; i++) {
		stream_cfg->lane_cfg.data[i].pol = 0;
		stream_cfg->lane_cfg.data[i].pos = i;
	}

	stream_cfg->lane_cfg.clk.pol = 0;
	stream_cfg->lane_cfg.clk.pos = 7;

	return 0;
}

static int phy_qcom_mipi_csi2_power_on(struct phy *phy)
{
	struct mipi_csi2phy_device *csi2phy = phy_get_drvdata(phy);
	const struct mipi_csi2phy_hw_ops *ops = csi2phy->soc_cfg->ops;
	struct device *dev = &phy->dev;
	int ret;

	ret = regulator_bulk_enable(csi2phy->soc_cfg->num_supplies,
				    csi2phy->supplies);
	if (ret)
		return ret;

	ret = phy_qcom_mipi_csi2_set_clock_rates(csi2phy, csi2phy->stream_cfg.link_freq);
	if (ret)
		goto poweroff_phy;

	ret = clk_bulk_prepare_enable(csi2phy->soc_cfg->num_clk,
				      csi2phy->clks);
	if (ret) {
		dev_err(dev, "failed to enable clocks, %d\n", ret);
		goto poweroff_phy;
	}

	ops->reset(csi2phy);

	ops->hw_version_read(csi2phy);

	return ops->lanes_enable(csi2phy, &csi2phy->stream_cfg);

poweroff_phy:
	regulator_bulk_disable(csi2phy->soc_cfg->num_supplies,
			       csi2phy->supplies);

	return ret;
}

static int phy_qcom_mipi_csi2_power_off(struct phy *phy)
{
	struct mipi_csi2phy_device *csi2phy = phy_get_drvdata(phy);
	int i;

	for (int i = 0; i < csi2phy->num_pds; i++)
		dev_pm_genpd_set_performance_state(csi2phy->pds[i], 0);

	clk_bulk_disable_unprepare(csi2phy->soc_cfg->num_clk,
				   csi2phy->clks);
	regulator_bulk_disable(csi2phy->soc_cfg->num_supplies,
			       csi2phy->supplies);

	return 0;
}

static const struct phy_ops phy_qcom_mipi_csi2_ops = {
	.configure	= phy_qcom_mipi_csi2_configure,
	.power_on	= phy_qcom_mipi_csi2_power_on,
	.power_off	= phy_qcom_mipi_csi2_power_off,
	.owner		= THIS_MODULE,
};

static int phy_qcom_mipi_csi2_probe(struct platform_device *pdev)
{
	unsigned int i, num_clk, num_supplies, num_pds;
	struct mipi_csi2phy_device *csi2phy;
	struct phy_provider *phy_provider;
	struct device *dev = &pdev->dev;
	struct phy *generic_phy;
	int ret;

	csi2phy = devm_kzalloc(dev, sizeof(*csi2phy), GFP_KERNEL);
	if (!csi2phy)
		return -ENOMEM;

	csi2phy->dev = dev;
	csi2phy->soc_cfg = device_get_match_data(&pdev->dev);

	if (!csi2phy->soc_cfg)
		return -EINVAL;

	num_clk = csi2phy->soc_cfg->num_clk;
	csi2phy->clks = devm_kzalloc(dev, sizeof(*csi2phy->clks) * num_clk, GFP_KERNEL);
	if (!csi2phy->clks)
		return -ENOMEM;

	num_pds = csi2phy->soc_cfg->num_genpd_names;
	if (!num_pds)
		return -EINVAL;

	csi2phy->pds = devm_kzalloc(dev, sizeof(*csi2phy->pds) * num_pds, GFP_KERNEL);
	if (!csi2phy->pds)
		return -ENOMEM;

	for (i = 0; i < num_pds; i++) {
		csi2phy->pds[i] = dev_pm_domain_attach_by_name(dev,
							       csi2phy->soc_cfg->genpd_names[i]);
		if (IS_ERR(csi2phy->pds[i])) {
			return dev_err_probe(dev, PTR_ERR(csi2phy->pds[i]),
					     "Failed to attach %s\n",
					     csi2phy->soc_cfg->genpd_names[i]);
		}
	}
	csi2phy->num_pds = num_pds;

	for (i = 0; i < num_clk; i++)
		csi2phy->clks[i].id = csi2phy->soc_cfg->clk_names[i];

	ret = devm_clk_bulk_get(dev, num_clk, csi2phy->clks);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to get clocks\n");

	csi2phy->timer_clk = devm_clk_get(dev, csi2phy->soc_cfg->timer_clk);
	if (IS_ERR(csi2phy->timer_clk)) {
		return dev_err_probe(dev, PTR_ERR(csi2phy->timer_clk),
				     "Failed to get timer clock\n");
	}

	ret = devm_pm_opp_set_clkname(dev, csi2phy->soc_cfg->opp_clk);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to set opp clkname\n");

	ret = devm_pm_opp_of_add_table(dev);
	if (ret && ret != -ENODEV)
		return dev_err_probe(dev, ret, "invalid OPP table in device tree\n");

	num_supplies = csi2phy->soc_cfg->num_supplies;
	csi2phy->supplies = devm_kzalloc(dev, sizeof(*csi2phy->supplies) * num_supplies,
					 GFP_KERNEL);
	if (!csi2phy->supplies)
		return -ENOMEM;

	for (i = 0; i < num_supplies; i++)
		csi2phy->supplies[i].supply = csi2phy->soc_cfg->supply_names[i];

	ret = devm_regulator_bulk_get(dev, num_supplies, csi2phy->supplies);
	if (ret)
		return dev_err_probe(dev, ret,
				     "failed to get regulator supplies\n");

	csi2phy->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(csi2phy->base))
		return PTR_ERR(csi2phy->base);

	generic_phy = devm_phy_create(dev, NULL, &phy_qcom_mipi_csi2_ops);
	if (IS_ERR(generic_phy)) {
		ret = PTR_ERR(generic_phy);
		return dev_err_probe(dev, ret, "failed to create phy\n");
	}
	csi2phy->phy = generic_phy;

	phy_set_drvdata(generic_phy, csi2phy);

	phy_provider = devm_of_phy_provider_register(dev, of_phy_simple_xlate);
	if (!IS_ERR(phy_provider))
		dev_dbg(dev, "Registered MIPI CSI2 PHY device\n");

	return PTR_ERR_OR_ZERO(phy_provider);
}

static const struct of_device_id phy_qcom_mipi_csi2_of_match_table[] = {
	{ .compatible	= "qcom,x1e80100-csi2-phy", .data = &mipi_csi2_dphy_4nm_x1e },
	{ }
};
MODULE_DEVICE_TABLE(of, phy_qcom_mipi_csi2_of_match_table);

static struct platform_driver phy_qcom_mipi_csi2_driver = {
	.probe		= phy_qcom_mipi_csi2_probe,
	.driver = {
		.name	= "qcom-mipi-csi2-phy",
		.of_match_table = phy_qcom_mipi_csi2_of_match_table,
	},
};

module_platform_driver(phy_qcom_mipi_csi2_driver);

MODULE_DESCRIPTION("Qualcomm MIPI CSI2 PHY driver");
MODULE_AUTHOR("Bryan O'Donoghue <bryan.odonoghue@linaro.org>");
MODULE_LICENSE("GPL");
