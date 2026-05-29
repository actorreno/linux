// SPDX-License-Identifier: GPL-2.0-only
/*
 * AD5706R 16-bit Current Output Digital to Analog Converter
 *
 * Copyright 2026 Analog Devices Inc.
 */

#include <linux/iio/iio.h>
#include <linux/module.h>
#include <linux/spi/spi.h>

static const struct iio_info ad5706r_info = {
};

static int ad5706r_probe(struct spi_device *spi)
{
	struct iio_dev *indio_dev;

	indio_dev = devm_iio_device_alloc(&spi->dev, 0);
	if (!indio_dev)
		return -ENOMEM;

	indio_dev->name = "ad5706r";
	indio_dev->info = &ad5706r_info;

	return devm_iio_device_register(&spi->dev, indio_dev);
}

static const struct of_device_id ad5706r_of_match[] = {
	{ .compatible = "adi,ad5706r" },
	{ }
};
MODULE_DEVICE_TABLE(of, ad5706r_of_match);

static const struct spi_device_id ad5706r_id[] = {
	{ "ad5706r" },
	{ }
};
MODULE_DEVICE_TABLE(spi, ad5706r_id);

static struct spi_driver ad5706r_driver = {
	.driver = {
		.name = "ad5706r",
		.of_match_table = ad5706r_of_match,
	},
	.probe = ad5706r_probe,
	.id_table = ad5706r_id,
};
module_spi_driver(ad5706r_driver);
