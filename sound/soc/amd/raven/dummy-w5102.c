/*
 * dummy audio codec driver
 *
 * Copyright 2016 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 */

#include <linux/module.h>
#include <sound/soc.h>

#define W5102_RATES	SNDRV_PCM_RATE_8000_96000
#define W5102_FORMATS	(SNDRV_PCM_FMTBIT_S16_LE | \
			SNDRV_PCM_FMTBIT_S32_LE | \
			SNDRV_PCM_FMTBIT_S24_LE)

static const struct snd_soc_dapm_widget w5102_widgets[] = {
	SND_SOC_DAPM_OUTPUT("dummy-w5102-out"),
	SND_SOC_DAPM_INPUT("dummy-w5102-in"),
};

static const struct snd_soc_dapm_route w5102_routes[] = {
	{ "dummy-w5102-out", NULL, "Playback" },
	{ "Capture", NULL, "dummy-w5102-in" },
};

static struct snd_soc_codec_driver soc_codec_w5102_dummy = {
	.component_driver = {
		.dapm_widgets = w5102_widgets,
		.num_dapm_widgets = ARRAY_SIZE(w5102_widgets),
		.dapm_routes = w5102_routes,
		.num_dapm_routes = ARRAY_SIZE(w5102_routes),
	},
};

static struct snd_soc_dai_driver w5102_stub_dai = {
	.name		= "dummy_w5102_dai",
	.playback	= {
		.stream_name	= "Playback",
		.channels_min	= 2,
		.channels_max	= 6,
		.rates		= W5102_RATES,
		.formats	= W5102_FORMATS,
	},
	.capture	= {
		.stream_name	= "Capture",
		.channels_min	= 2,
		.channels_max	= 2,
		.rates		= W5102_RATES,
		.formats	= W5102_FORMATS,
	},

};

static int dummy_w5102_probe(struct platform_device *pdev)
{
	int ret;

	ret = snd_soc_register_codec(&pdev->dev, &soc_codec_w5102_dummy,
			&w5102_stub_dai, 1);
	return ret;
}

static int dummy_w5102_remove(struct platform_device *pdev)
{
	snd_soc_unregister_codec(&pdev->dev);
	return 0;
}


static struct platform_driver dummy_w5102_driver = {
	.probe		= dummy_w5102_probe,
	.remove		= dummy_w5102_remove,
	.driver		= {
		.name	= "dummy_w5102",
		.owner	= THIS_MODULE,
	},
};

module_platform_driver(dummy_w5102_driver);

MODULE_DESCRIPTION("dummy-w5102 dummy codec driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform: dummy_w5102");
