// SPDX-License-Identifier: GPL-2.0-only
/*
 * AD5706R 16-bit Current Output Digital to Analog Converter
 *
 * Copyright 2026 Analog Devices Inc.
 */
 
#include <linux/gpio/consumer.h>
#include <linux/iio/iio.h>
#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/regmap.h>

#define AD5706R_REG_DAC_DATA_READBACK_CH	0x69

struct ad5706r_state {
	struct regmap *regmap;
	struct gpio_desc *en_gpio;

	bool enable;
};

static int ad5706r_read_raw(struct iio_dev *indio_dev,
			    struct iio_chan_spec const *chan,
			    int *val, int *val2, long mask)
{
	struct ad5706r_state *st = iio_priv(indio_dev);
	int reg, reg_val, ret;

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		reg = AD5706R_REG_DAC_DATA_READBACK_CH + (chan->channel * 2);
		ret = regmap_read(st->regmap, reg, &reg_val);
		if (ret)
			return ret;
		*val = reg_val;
		return IIO_VAL_INT;
	case IIO_CHAN_INFO_ENABLE:
		*val = gpiod_get_value_cansleep(st->en_gpio);
		return IIO_VAL_INT;
	default:
		return -EINVAL;
	}
}

static int ad5706r_write_raw(struct iio_dev *indio_dev,
			     struct iio_chan_spec const *chan,
			     int val, int val2, long mask)
{
	struct ad5706r_state *st = iio_priv(indio_dev);

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		return 0;
	case IIO_CHAN_INFO_ENABLE:
		gpiod_set_value_cansleep(st->en_gpio, val);
		st->enable = val;
		return 0;
	default:
		return -EINVAL;
	}
}

static const struct regmap_config ad5706r_regmap_config = {
	.reg_bits = 16,
	.val_bits = 16,
};

static int ad5706r_reg_access(struct iio_dev *indio_dev,
			      unsigned int reg, unsigned int writeval,
			      unsigned int *readval)
{
	struct ad5706r_state *st = iio_priv(indio_dev);

	if (readval)
		return regmap_read(st->regmap, reg, readval);
	
	return regmap_write(st->regmap, reg, writeval);
}

static const struct iio_info ad5706r_info = {
	.read_raw = ad5706r_read_raw,
	.write_raw = ad5706r_write_raw,
	.debugfs_reg_access = ad5706r_reg_access,
};

#define AD5706R_CHAN(_channel) {				\
	.type = IIO_CURRENT,					\
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),		\
	.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE),	\
	.output = 1,						\
	.indexed = 1,						\
	.channel = _channel,					\
}

static const struct iio_chan_spec ad5706r_channels[] = {
	AD5706R_CHAN(0),
	AD5706R_CHAN(1),
	AD5706R_CHAN(2),
	AD5706R_CHAN(3),
};

static int ad5706r_probe(struct spi_device *spi)
{
	struct iio_dev *indio_dev;
	struct ad5706r_state *st;

	indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*st));
	if (!indio_dev)
		return -ENOMEM;

	st = iio_priv(indio_dev);
	st->enable = false;

	st->regmap = devm_regmap_init_spi(spi, &ad5706r_regmap_config);
	if (IS_ERR(st->regmap))
		return dev_err_probe(&spi->dev, PTR_ERR(st->regmap),
				     "Failed to init regmap\n");

	st->en_gpio = devm_gpiod_get_optional(&spi->dev, "enable", GPIOD_OUT_HIGH);
	if (IS_ERR(st->en_gpio))
		return dev_err_probe(&spi->dev, PTR_ERR(st->en_gpio),
				     "Failed to get enable GPIO\n");

	indio_dev->name = "ad5706r";
	indio_dev->info = &ad5706r_info;
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->channels = ad5706r_channels;
	indio_dev->num_channels = ARRAY_SIZE(ad5706r_channels);

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
