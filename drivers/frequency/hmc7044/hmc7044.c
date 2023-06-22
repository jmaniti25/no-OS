/***************************************************************************//**
 *   @file   hmc7044.c
 *   @brief  Implementation of HMC7044, HMC7043 Driver.
 *   @author DBogdan (dragos.bogdan@analog.com)
********************************************************************************
 * Copyright 2018-2020(c) Analog Devices, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  - Neither the name of Analog Devices, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *  - The use of this software may or may not infringe the patent rights
 *    of one or more patent holders.  This license does not release you
 *    from the requirement that you obtain separate licenses from these
 *    patent holders to use this software.
 *  - Use of the software either in source or binary form, must be run
 *    on or directly connected to an Analog Devices Inc. component.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ANALOG DEVICES BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, INTELLECTUAL PROPERTY RIGHTS, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include "no_os_error.h"
#include "no_os_util.h"
#include "no_os_alloc.h"
#include "no_os_clk.h"
#include "hmc7044.h"

/******************************************************************************/
/********************** Macros and Constants Definitions **********************/
/******************************************************************************/
#define HMC7044_WRITE		(0 << 15)
#define HMC7044_READ		(1 << 15)
#define HMC7044_CNT(x)		(((x) - 1) << 13)
#define HMC7044_ADDR(x)		((x) & 0xFFF)

/* Global Control */
#define HMC7044_REG_SOFT_RESET		0x0000
#define HMC7044_SOFT_RESET		NO_OS_BIT(0)

#define HMC7044_REG_REQ_MODE_0		0x0001
#define HMC7044_RESEED_REQ		NO_OS_BIT(7)
#define HMC7044_HIGH_PERF_DISTRIB_PATH	NO_OS_BIT(6)
#define HMC7044_HIGH_PERF_PLL_VCO	NO_OS_BIT(5)
#define HMC7044_FORCE_HOLDOVER		NO_OS_BIT(4)
#define HMC7044_MUTE_OUT_DIV		NO_OS_BIT(3)
#define HMC7044_PULSE_GEN_REQ		NO_OS_BIT(2)
#define HMC7044_RESTART_DIV_FSM		NO_OS_BIT(1)
#define HMC7044_SLEEP_MODE		NO_OS_BIT(0)

#define HMC7044_REG_REQ_MODE_1		0x0002
#define HMC7044_PLL2_AUTOTUNE_TRIG	NO_OS_BIT(2)
#define HMC7044_SLIP_REQ		NO_OS_BIT(1)

#define HMC7044_REG_EN_CTRL_0		0x0003
#define HMC7044_RF_RESEEDER_EN		NO_OS_BIT(5)
#define HMC7044_VCO_SEL(x)		(((x) & 0x3) << 3)
#define HMC7044_VCO_EXT			0
#define HMC7044_VCO_HIGH		1
#define HMC7044_VCO_LOW			2
#define HMC7044_SYSREF_TIMER_EN		NO_OS_BIT(2)
#define HMC7044_PLL2_EN			NO_OS_BIT(1)
#define HMC7044_PLL1_EN			NO_OS_BIT(0)

#define HMC7044_REG_EN_CTRL_1		0x0004
#define HMC7044_SEVEN_PAIRS(x)		((x) & 0x7f)

#define HMC7044_REG_GLOB_MODE		0x0005
#define HMC7044_REF_PATH_EN(x)		((x) & 0xf)
#define HMC7044_RFSYNC_EN		NO_OS_BIT(4)
#define HMC7044_VCOIN_MODE_EN		NO_OS_BIT(5)
#define HMC7044_SYNC_PIN_MODE(x)	(((x) & 0x3) << 6)

/* PLL1 */
#define HMC7044_REG_CLKIN0_BUF_CTRL	0x000A
#define HMC7044_REG_CLKIN1_BUF_CTRL	0x000B
#define HMC7044_REG_CLKIN2_BUF_CTRL	0x000C
#define HMC7044_REG_CLKIN3_BUF_CTRL	0x000D
#define HMC7044_REG_OSCIN_BUF_CTRL	0x000E

#define HMC7044_REG_PLL1_REF_PRIO_CTRL	0x0014

#define HMC7044_HIGH_Z_EN		NO_OS_BIT(4)
#define HMC7044_LVPECL_EN		NO_OS_BIT(3)
#define HMC7044_AC_COUPLING_EN		NO_OS_BIT(2)
#define HMC7044_100_OHM_EN		NO_OS_BIT(1)
#define HMC7044_BUF_EN			NO_OS_BIT(0)

#define HMC7044_REG_CLKIN_PRESCALER(x)	(0x001C + (x))
#define HMC7044_REG_OSCIN_PRESCALER	0x0020

#define HMC7044_REG_PLL1_R_LSB		0x0021
#define HMC7044_R1_LSB(x)		((x) & 0xff)

#define HMC7044_REG_PLL1_R_MSB		0x0022
#define HMC7044_R1_MSB(x)		(((x) & 0xff00) >> 8)

#define HMC7044_REG_PLL1_N_LSB		0x0026
#define HMC7044_N1_LSB(x)		((x) & 0xff)

#define HMC7044_REG_PLL1_N_MSB		0x0027
#define HMC7044_N1_MSB(x)		(((x) & 0xff00) >> 8)

#define HMC7044_REG_PLL1_LOCK_DETECT	0x0028
#define HMC7044_LOCK_DETECT_SLIP	NO_OS_BIT(5)
#define HMC7044_LOCK_DETECT_TIMER(x)	((x) & 0x1f)

/* PLL2 */
#define HMC7044_REG_PLL2_FREQ_DOUBLER	0x0032
#define HMC7044_PLL2_FREQ_DOUBLER_DIS	NO_OS_BIT(0)

#define HMC7044_REG_PLL2_R_LSB		0x0033
#define HMC7044_R2_LSB(x)		((x) & 0xff)

#define HMC7044_REG_PLL2_R_MSB		0x0034
#define HMC7044_R2_MSB(x)		(((x) & 0xf00) >> 8)

#define HMC7044_REG_PLL2_N_LSB		0x0035
#define HMC7044_N2_LSB(x)		((x) & 0xff)

#define HMC7044_REG_PLL2_N_MSB		0x0036
#define HMC7044_N2_MSB(x)		(((x) & 0xff00) >> 8)

#define HMC7044_REG_OSCOUT_PATH		0x0039
#define HMC7044_REG_OSCOUT_DRIVER_0	0x003A
#define HMC7044_REG_OSCOUT_DRIVER_1	0x003B

/* GPIO/SDATA Control */
#define HMC7044_REG_GPI_CTRL(x)		(0x0046 + (x))
#define HMC7044_REG_GPI_SEL(x)		((x) & 0xf)

#define HMC7044_REG_GPO_CTRL(x)		(0x0050 + (x))
#define HMC7044_GPO_SEL(x)		(((x) & 0x3f) << 2)
#define HMC7044_GPO_MODE		NO_OS_BIT(1)
#define HMC7044_GPO_EN			NO_OS_BIT(0)

/* SYSREF/SYNC Control */
#define HMC7044_REG_PULSE_GEN		0x005A
#define HMC7044_PULSE_GEN_MODE(x)	((x) & 0x7)

#define HMC7044_REG_SYNC		0x005B
#define HMC7044_SYNC_RETIME		NO_OS_BIT(2)
#define HMC7044_SYNC_THROUGH_PLL2	NO_OS_BIT(1)
#define HMC7044_SYNC_POLARITY		NO_OS_BIT(0)

#define HMC7044_REG_SYSREF_TIMER_LSB	0x005C
#define HMC7044_SYSREF_TIMER_LSB(x)	((x) & 0xff)

#define HMC7044_REG_SYSREF_TIMER_MSB	0x005D
#define HMC7044_SYSREF_TIMER_MSB(x)	(((x) & 0xf00) >> 8)

#define HMC7044_CLK_INPUT_CTRL		0x0064
#define HMC7044_LOW_FREQ_INPUT_MODE	NO_OS_BIT(0)
#define HMC7044_DIV_2_INPUT_MODE	NO_OS_BIT(1)

/* Status and Alarm readback */
#define HMC7044_REG_ALARM_READBACK	0x007D
#define HMC7044_REG_PLL1_STATUS		0x0082

#define HMC7044_PLL1_FSM_STATE(x)	((x) & 0x7)
#define HMC7044_PLL1_ACTIVE_CLKIN(x)	(((x) >> 3) & 0x3)

#define HMC7044_PLL2_LOCK_DETECT(x)	((x) & 0x1)
#define HMC7044_SYSREF_SYNC_STAT(x)	((x) & 0x2)
#define HMC7044_CLK_OUT_PH_STATUS(x)	((x) & 0x4)
#define HMC7044_PLL1_PLL2_LOCK_STAT(x)	((x) & 0x8)
#define HMC7044_SYNC_REQ_STATUS(x)	((x) & 0x10)

/* Other Controls */
#define HMC7044_REG_CLK_OUT_DRV_LOW_PW	0x009F
#define HMC7044_REG_CLK_OUT_DRV_HIGH_PW	0x00A0
#define HMC7044_REG_PLL1_DELAY		0x00A5
#define HMC7044_REG_PLL1_HOLDOVER	0x00A8
#define HMC7044_REG_VTUNE_PRESET	0x00B0

/* Clock Distribution */
#define HMC7044_REG_CH_OUT_CRTL_0(ch)	(0x00C8 + 0xA * (ch))
#define HMC7044_HI_PERF_MODE		NO_OS_BIT(7)
#define HMC7044_SYNC_EN			NO_OS_BIT(6)
#define HMC7044_CH_EN			NO_OS_BIT(0)
#define HMC7044_START_UP_MODE_DYN_EN	(NO_OS_BIT(3) | NO_OS_BIT(2))

#define HMC7044_REG_CH_OUT_CRTL_1(ch)	(0x00C9 + 0xA * (ch))
#define HMC7044_DIV_LSB(x)		((x) & 0xFF)

#define HMC7044_REG_CH_OUT_CRTL_2(ch)	(0x00CA + 0xA * (ch))
#define HMC7044_DIV_MSB(x)		(((x) >> 8) & 0xFF)

#define HMC7044_REG_CH_OUT_CRTL_3(ch)	(0x00CB + 0xA * (ch))
#define HMC7044_REG_CH_OUT_CRTL_4(ch)	(0x00CC + 0xA * (ch))
#define HMC7044_REG_CH_OUT_CRTL_5(ch)	(0x00CD + 0xA * (ch))
#define HMC7044_REG_CH_OUT_CRTL_6(ch)	(0x00CE + 0xA * (ch))
#define HMC7044_REG_CH_OUT_CRTL_7(ch)	(0x00CF + 0xA * (ch))

#define HMC7044_REG_CH_OUT_CRTL_8(ch)	(0x00D0 + 0xA * (ch))
#define HMC7044_DRIVER_MODE(x)		(((x) & 0x3) << 3)
#define HMC7044_DRIVER_Z_MODE(x)	(((x) & 0x3) << 0)
#define HMC7044_DYN_DRIVER_EN		NO_OS_BIT(5)
#define HMC7044_FORCE_MUTE_EN		NO_OS_BIT(7)

#define HMC7044_NUM_CHAN	14

#define HMC7044_LOW_VCO_MIN	2150000
#define HMC7044_LOW_VCO_MAX	2880000
#define HMC7044_HIGH_VCO_MIN	2650000
#define HMC7044_HIGH_VCO_MAX	3200000

#define HMC7044_RECOMM_LCM_MIN	30000
#define HMC7044_RECOMM_LCM_MAX	70000
#define HMC7044_RECOMM_FPD1	10000

#define HMC7044_R1_MAX		65535
#define HMC7044_N1_MAX		65535

#define HMC7044_R2_MIN		1
#define HMC7044_R2_MAX		4095
#define HMC7044_N2_MIN		8
#define HMC7044_N2_MAX		65535

#define HMC7044_OUT_DIV_MIN	1
#define HMC7044_OUT_DIV_MAX	4094

/******************************************************************************/
/************************** Functions Implementation **************************/
/******************************************************************************/

/**
 * SPI register write to device.
 * @param dev - The device structure.
 * @param reg - The register address.
 * @param val - The register data.
 * @return 0 in case of success, negative error code otherwise.
 */
static int hmc7044_write(struct hmc7044_dev *dev,
			 uint16_t reg,
			 uint8_t val)
{
	uint8_t buf[3];
	uint16_t cmd;

	cmd = HMC7044_WRITE | HMC7044_CNT(1) | HMC7044_ADDR(reg);
	buf[0] = cmd >> 8;
	buf[1] = cmd & 0xFF;
	buf[2] = val;

	return no_os_spi_write_and_read(dev->spi_desc, buf, NO_OS_ARRAY_SIZE(buf));
}

/**
 * SPI register read from device.
 * @param dev - The device structure.
 * @param reg - The register address.
 * @param val - The register data.
 * @return 0 in case of success, negative error code otherwise.
 */

int32_t hmc7044_read(struct hmc7044_dev *dev, uint16_t reg, uint8_t *val)
{
	uint8_t buf[3];
	uint16_t cmd;
	int ret;

	cmd = HMC7044_READ | HMC7044_CNT(1) | HMC7044_ADDR(reg);
	buf[0] = cmd >> 8;
	buf[1] = cmd & 0xFF;
	buf[2] = 0;

	ret = no_os_spi_write_and_read(dev->spi_desc, buf, NO_OS_ARRAY_SIZE(buf));
	if (ret < 0)
		return ret;

	*val = buf[2];

	return 0;
}

/**
 * Calculate the output channel divider.
 * @param rate - The desired rate.
 * @param parent_rate - The parent rate.
 * @return The output divider.
 */
uint32_t hmc7044_calc_out_div(uint32_t rate,
			      uint32_t parent_rate)
{
	uint32_t div;

	div = NO_OS_DIV_ROUND_CLOSEST(parent_rate, rate);

	/* Supported odd divide ratios are 1, 3, and 5 */
	if ((div != 1) && (div != 3) && (div != 5) && (div % 2))
		div = NO_OS_DIV_ROUND_CLOSEST(parent_rate, rate * 2) * 2;

	div = no_os_clamp_t(unsigned int,
			    div,
			    HMC7044_OUT_DIV_MIN,
			    HMC7044_OUT_DIV_MAX);

	return div;
}

/**
 * Recalculate rate corresponding to a channel.
 * @param dev - The device structure.
 * @param chan_num - Channel number.
 * @param rate - Channel rate.
 * @return 0 in case of success, negative error code otherwise.
 */
int32_t hmc7044_clk_recalc_rate(struct hmc7044_dev *dev, uint32_t chan_num,
				uint64_t *rate)
{
	int i;
	struct hmc7044_chan_spec *chan = NULL;

	/* Find the reqested channel number */
	for (i = 0; i < dev->num_channels; i++) {
		if (dev->channels[i].num == chan_num) {
			chan = &dev->channels[i];
			break;
		}
	}
	if (chan == NULL )
		return -1;

	*rate = dev->pll2_freq / chan->divider;

	return 0;
}

/**
 * Calculate closest possible rate
 * @param dev - The device structure
 * @param rate - The desired rate.
 * @param rounded_rate - The closest possible rate of desired rate.
 * @return 0 in case of success, negative error code otherwise.
 */
int32_t hmc7044_clk_round_rate(struct hmc7044_dev *dev, uint32_t rate,
			       uint64_t *rounded_rate)
{
	uint32_t div = hmc7044_calc_out_div(rate, dev->pll2_freq);

	*rounded_rate = NO_OS_DIV_ROUND_CLOSEST(dev->pll2_freq, div);

	return 0;
}

/**
 * Set channel rate.
 * @param dev - The device structure.
 * @param chan_num - Channel number.
 * @param rate - Channel rate.
 * @return 0 in case of success, negative error code otherwise.
 */
int32_t hmc7044_clk_set_rate(struct hmc7044_dev *dev, uint32_t chan_num,
			     uint64_t rate)
{
	uint32_t div;
	int32_t ret;
	int i;
	struct hmc7044_chan_spec *chan = NULL;

	/* Find the reqested channel number */
	for (i = 0; i < dev->num_channels; i++) {
		if (dev->channels[i].num == chan_num) {
			chan = &dev->channels[i];
			break;
		}
	}
	if (chan == NULL )
		return -1;

	div = hmc7044_calc_out_div(rate, dev->pll2_freq);
	chan->divider = div;

	ret = hmc7044_write(dev, HMC7044_REG_CH_OUT_CRTL_1(chan->num),
			    HMC7044_DIV_LSB(div));
	if(ret < 0)
		return ret;

	return hmc7044_write(dev, HMC7044_REG_CH_OUT_CRTL_2(chan->num),
			     HMC7044_DIV_MSB(div));
}

/**
 * Setup the device.
 * @param dev - The device structure.
 * @return 0 in case of success, negative error code otherwise.
 */
static int32_t hmc7044_setup(struct hmc7044_dev *dev)
{
	struct hmc7044_chan_spec *chan;
	bool high_vco_en;
	bool pll2_freq_doubler_en;
	uint32_t vcxo_freq, pll2_freq;
	uint32_t clkin_freq[4];
	uint32_t lcm_freq;
	uint32_t in_prescaler[5];
	uint32_t pll1_lock_detect;
	uint32_t n1, r1;
	uint32_t pfd1_freq;
	uint32_t vco_limit;
	uint32_t n2[2], r2[2];
	uint32_t i, ref_en = 0;

	vcxo_freq = dev->vcxo_freq / 1000;
	pll2_freq = dev->pll2_freq / 1000;

	lcm_freq = vcxo_freq;
	for (i = 0; i < NO_OS_ARRAY_SIZE(clkin_freq); i++) {
		if (dev->clkin_freq_ccf[i])
			clkin_freq[i] = dev->clkin_freq_ccf[i] / 1000;
		else
			clkin_freq[i] = dev->clkin_freq[i] / 1000;

		if (clkin_freq[i]) {
			lcm_freq = no_os_greatest_common_divisor(clkin_freq[i], lcm_freq);
			ref_en |= NO_OS_BIT(i);
		}
	}

	while (lcm_freq > HMC7044_RECOMM_LCM_MAX)
		lcm_freq /= 2;

	for (i = 0; i < NO_OS_ARRAY_SIZE(clkin_freq); i++) {
		if (clkin_freq[i])
			in_prescaler[i] = clkin_freq[i] / lcm_freq;
		else
			in_prescaler[i] = 1;
	}
	in_prescaler[4] = vcxo_freq / lcm_freq;

	pll1_lock_detect = no_os_log_base_2((lcm_freq * 4000) / dev->pll1_loop_bw);

	/* fVCXO / N1 = fLCM / R1 */
	no_os_rational_best_approximation(vcxo_freq, lcm_freq,
					  HMC7044_N1_MAX, HMC7044_R1_MAX,
					  &n1, &r1);

	pfd1_freq = vcxo_freq / n1;
	while ((pfd1_freq > HMC7044_RECOMM_FPD1) &&
	       (n1 <= HMC7044_N1_MAX / 2) &&
	       (r1 <= HMC7044_R1_MAX / 2)) {
		pfd1_freq /= 2;
		n1 *= 2;
		r1 *= 2;
	}

	dev->pll1_pfd = pfd1_freq;

	if (pll2_freq < HMC7044_LOW_VCO_MIN  ||
	    pll2_freq > HMC7044_HIGH_VCO_MAX)
		return -1;

	vco_limit = (HMC7044_LOW_VCO_MAX + HMC7044_HIGH_VCO_MIN) / 2;
	if (pll2_freq >= vco_limit)
		high_vco_en = true;
	else
		high_vco_en = false;

	/* fVCO / N2 = fVCXO * doubler / R2 */
	pll2_freq_doubler_en = true;
	no_os_rational_best_approximation(pll2_freq, vcxo_freq * 2,
					  HMC7044_N2_MAX, HMC7044_R2_MAX,
					  &n2[0], &r2[0]);

	if (pll2_freq != vcxo_freq * n2[0] / r2[0]) {
		no_os_rational_best_approximation(pll2_freq, vcxo_freq,
						  HMC7044_N2_MAX, HMC7044_R2_MAX,
						  &n2[1], &r2[1]);

		if (abs((int)pll2_freq - (int)(vcxo_freq * 2 * n2[0] / r2[0])) >
		    abs((int)pll2_freq - (int)(vcxo_freq * n2[1] / r2[1]))) {
			n2[0] = n2[1];
			r2[0] = r2[1];
			pll2_freq_doubler_en = false;
		}
	}

	while ((n2[0] < HMC7044_N2_MIN) && (r2[0] <= HMC7044_R2_MAX / 2)) {
		n2[0] *= 2;
		r2[0] *= 2;
	}
	if (n2[0] < HMC7044_N2_MIN)
		return -1;

	/* Resets all registers to default values */
	hmc7044_write(dev, HMC7044_REG_SOFT_RESET, HMC7044_SOFT_RESET);
	no_os_mdelay(10);
	hmc7044_write(dev, HMC7044_REG_SOFT_RESET, 0);
	no_os_mdelay(10);

	/* Disable all channels */
	for (i = 0; i < HMC7044_NUM_CHAN; i++)
		hmc7044_write(dev, HMC7044_REG_CH_OUT_CRTL_0(i), 0);

	/* Load the configuration updates (provided by Analog Devices) */
	hmc7044_write(dev, HMC7044_REG_CLK_OUT_DRV_LOW_PW, 0x4d);
	hmc7044_write(dev, HMC7044_REG_CLK_OUT_DRV_HIGH_PW, 0xdf);
	hmc7044_write(dev, HMC7044_REG_PLL1_DELAY, 0x06);
	hmc7044_write(dev, HMC7044_REG_PLL1_HOLDOVER, 0x06);
	hmc7044_write(dev, HMC7044_REG_VTUNE_PRESET, 0x04);

	hmc7044_write(dev, HMC7044_REG_GLOB_MODE,
		      HMC7044_SYNC_PIN_MODE(dev->sync_pin_mode) |
		      (dev->clkin0_rfsync_en ? HMC7044_RFSYNC_EN : 0) |
		      (dev->clkin1_vcoin_en ? HMC7044_VCOIN_MODE_EN : 0) |
		      HMC7044_REF_PATH_EN(ref_en));

	/* Program PLL2 */

	/* Select the VCO range */
	hmc7044_write(dev, HMC7044_REG_EN_CTRL_0,
		      (dev->rf_reseeder_en ? HMC7044_RF_RESEEDER_EN : 0) |
		      HMC7044_VCO_SEL(high_vco_en ?
				      HMC7044_VCO_HIGH :
				      HMC7044_VCO_LOW) |
		      HMC7044_SYSREF_TIMER_EN | HMC7044_PLL2_EN |
		      HMC7044_PLL1_EN);

	/* Program the dividers */
	hmc7044_write(dev, HMC7044_REG_PLL2_R_LSB,
		      HMC7044_R2_LSB(r2[0]));
	hmc7044_write(dev, HMC7044_REG_PLL2_R_MSB,
		      HMC7044_R2_MSB(r2[0]));
	hmc7044_write(dev, HMC7044_REG_PLL2_N_LSB,
		      HMC7044_N2_LSB(n2[0]));
	hmc7044_write(dev, HMC7044_REG_PLL2_N_MSB,
		      HMC7044_N2_MSB(n2[0]));

	/* Program the reference doubler */
	hmc7044_write(dev, HMC7044_REG_PLL2_FREQ_DOUBLER,
		      pll2_freq_doubler_en ? 0 : HMC7044_PLL2_FREQ_DOUBLER_DIS);

	/* Program PLL1 */

	/* Set the lock detect timer threshold */
	hmc7044_write(dev, HMC7044_REG_PLL1_LOCK_DETECT,
		      HMC7044_LOCK_DETECT_TIMER(pll1_lock_detect));

	/* Set the LCM */
	for (i = 0; i < NO_OS_ARRAY_SIZE(clkin_freq); i++) {
		hmc7044_write(dev, HMC7044_REG_CLKIN_PRESCALER(i),
			      in_prescaler[i]);
	}
	hmc7044_write(dev, HMC7044_REG_OSCIN_PRESCALER,
		      in_prescaler[4]);

	/* Program the dividers */
	hmc7044_write(dev, HMC7044_REG_PLL1_R_LSB,
		      HMC7044_R2_LSB(r1));
	hmc7044_write(dev, HMC7044_REG_PLL1_R_MSB,
		      HMC7044_R2_MSB(r1));
	hmc7044_write(dev, HMC7044_REG_PLL1_N_LSB,
		      HMC7044_N2_LSB(n1));
	hmc7044_write(dev, HMC7044_REG_PLL1_N_MSB,
		      HMC7044_N2_MSB(n1));

	hmc7044_write(dev, HMC7044_REG_PLL1_REF_PRIO_CTRL,
		      dev->pll1_ref_prio_ctrl);

	/* Program the SYSREF timer */

	/* Set the divide ratio */
	hmc7044_write(dev, HMC7044_REG_SYSREF_TIMER_LSB,
		      HMC7044_SYSREF_TIMER_LSB(dev->sysref_timer_div));
	hmc7044_write(dev, HMC7044_REG_SYSREF_TIMER_MSB,
		      HMC7044_SYSREF_TIMER_MSB(dev->sysref_timer_div));

	/* Set the pulse generator mode configuration */
	hmc7044_write(dev, HMC7044_REG_PULSE_GEN,
		      HMC7044_PULSE_GEN_MODE(dev->pulse_gen_mode));

	/* Enable the input buffers */
	hmc7044_write(dev, HMC7044_REG_CLKIN0_BUF_CTRL,
		      dev->in_buf_mode[0]);
	hmc7044_write(dev, HMC7044_REG_CLKIN1_BUF_CTRL,
		      dev->in_buf_mode[1]);
	hmc7044_write(dev, HMC7044_REG_CLKIN2_BUF_CTRL,
		      dev->in_buf_mode[2]);
	hmc7044_write(dev, HMC7044_REG_CLKIN3_BUF_CTRL,
		      dev->in_buf_mode[3]);
	hmc7044_write(dev, HMC7044_REG_OSCIN_BUF_CTRL,
		      dev->in_buf_mode[4]);

	/* Set GPIOs */
	for (i = 0; i < NO_OS_ARRAY_SIZE(dev->gpi_ctrl); i++) {
		hmc7044_write(dev, HMC7044_REG_GPI_CTRL(i),
			      dev->gpi_ctrl[i]);
	}

	for (i = 0; i < NO_OS_ARRAY_SIZE(dev->gpo_ctrl); i++) {
		hmc7044_write(dev, HMC7044_REG_GPO_CTRL(i),
			      dev->gpo_ctrl[i]);
	}

	no_os_mdelay(10);

	/* Program the output channels */
	for (i = 0; i < dev->num_channels; i++) {
		chan = &dev->channels[i];
		if (chan->num >= HMC7044_NUM_CHAN || chan->disable)
			continue;

		hmc7044_write(dev, HMC7044_REG_CH_OUT_CRTL_1(chan->num),
			      HMC7044_DIV_LSB(chan->divider));
		hmc7044_write(dev, HMC7044_REG_CH_OUT_CRTL_2(chan->num),
			      HMC7044_DIV_MSB(chan->divider));
		hmc7044_write(dev, HMC7044_REG_CH_OUT_CRTL_8(chan->num),
			      HMC7044_DRIVER_MODE(chan->driver_mode) |
			      HMC7044_DRIVER_Z_MODE(chan->driver_impedance) |
			      (chan->dynamic_driver_enable ?
			       HMC7044_DYN_DRIVER_EN : 0) |
			      (chan->force_mute_enable ?
			       HMC7044_FORCE_MUTE_EN : 0));

		hmc7044_write(dev, HMC7044_REG_CH_OUT_CRTL_3(chan->num),
			      chan->fine_delay & 0x1F);
		hmc7044_write(dev, HMC7044_REG_CH_OUT_CRTL_4(chan->num),
			      chan->coarse_delay & 0x1F);
		hmc7044_write(dev, HMC7044_REG_CH_OUT_CRTL_7(chan->num),
			      chan->out_mux_mode & 0x3);

		hmc7044_write(dev, HMC7044_REG_CH_OUT_CRTL_0(chan->num),
			      (chan->start_up_mode_dynamic_enable ?
			       HMC7044_START_UP_MODE_DYN_EN : 0) |
			      (chan->output_control0_rb4_enable ? NO_OS_BIT(4) : 0) |
			      (chan->high_performance_mode_dis ?
			       0 : HMC7044_HI_PERF_MODE) | HMC7044_SYNC_EN |
			      HMC7044_CH_EN);
	}
	no_os_mdelay(10);

	/* Do a restart to reset the system and initiate calibration */
	hmc7044_write(dev, HMC7044_REG_REQ_MODE_0,
		      HMC7044_RESTART_DIV_FSM);
	no_os_mdelay(1);
	hmc7044_write(dev, HMC7044_REG_REQ_MODE_0,
		      (dev->high_performance_mode_clock_dist_en ?
		       HMC7044_HIGH_PERF_DISTRIB_PATH : 0));
	no_os_mdelay(1);

	return 0;
}

/**
 * Setup the device.
 * @param dev - The device structure.
 * @return 0 in case of success, negative error code otherwise.
 */
static int32_t hmc7043_setup(struct hmc7044_dev *dev)
{
	struct hmc7044_chan_spec *chan;
	uint32_t i;

	if (dev->clkin_freq_ccf[0])
		dev->pll2_freq = dev->clkin_freq_ccf[0];
	else
		dev->pll2_freq  = dev->clkin_freq[0];

	if (!dev->pll2_freq) {
		printf("%s: Failed to get valid parent rate\n", __func__);
		return -1;
	}

	/* Resets all registers to default values */
	hmc7044_write(dev, HMC7044_REG_SOFT_RESET, HMC7044_SOFT_RESET);
	no_os_mdelay(10);
	hmc7044_write(dev, HMC7044_REG_SOFT_RESET, 0);
	no_os_mdelay(10);

	/* Load the configuration updates (provided by Analog Devices) */
	hmc7044_write(dev, HMC7044_REG_CLK_OUT_DRV_LOW_PW, 0x4d);
	hmc7044_write(dev, HMC7044_REG_CLK_OUT_DRV_HIGH_PW, 0xdf);

	/* Disable all channels */
	for (i = 0; i < HMC7044_NUM_CHAN; i++)
		hmc7044_write(dev, HMC7044_REG_CH_OUT_CRTL_0(i), 0);

	if (dev->pll2_freq < 1000000000U)
		hmc7044_write(dev, HMC7044_CLK_INPUT_CTRL,
			      HMC7044_LOW_FREQ_INPUT_MODE);

	hmc7044_write(dev, HMC7044_REG_EN_CTRL_0,
		      (dev->rf_reseeder_en ? HMC7044_RF_RESEEDER_EN : 0) |
		      HMC7044_SYSREF_TIMER_EN);

	/* Program the SYSREF timer */

	/* Set the divide ratio */
	hmc7044_write(dev, HMC7044_REG_SYSREF_TIMER_LSB,
		      HMC7044_SYSREF_TIMER_LSB(dev->sysref_timer_div));
	hmc7044_write(dev, HMC7044_REG_SYSREF_TIMER_MSB,
		      HMC7044_SYSREF_TIMER_MSB(dev->sysref_timer_div));

	/* Set the pulse generator mode configuration */
	hmc7044_write(dev, HMC7044_REG_PULSE_GEN,
		      HMC7044_PULSE_GEN_MODE(dev->pulse_gen_mode));

	/* Enable the input buffers */
	hmc7044_write(dev, HMC7044_REG_CLKIN0_BUF_CTRL,
		      dev->in_buf_mode[0]);
	hmc7044_write(dev, HMC7044_REG_CLKIN1_BUF_CTRL,
		      dev->in_buf_mode[1]);

	/* Set GPIOs */
	hmc7044_write(dev, HMC7044_REG_GPI_CTRL(0),
		      dev->gpi_ctrl[0]);

	hmc7044_write(dev, HMC7044_REG_GPO_CTRL(0),
		      dev->gpo_ctrl[0]);

	/* Program the output channels */
	for (i = 0; i < dev->num_channels; i++) {
		chan = &dev->channels[i];

		if (chan->num >= HMC7044_NUM_CHAN || chan->disable)
			continue;

		hmc7044_write(dev, HMC7044_REG_CH_OUT_CRTL_1(chan->num),
			      HMC7044_DIV_LSB(chan->divider));
		hmc7044_write(dev, HMC7044_REG_CH_OUT_CRTL_2(chan->num),
			      HMC7044_DIV_MSB(chan->divider));
		hmc7044_write(dev, HMC7044_REG_CH_OUT_CRTL_8(chan->num),
			      HMC7044_DRIVER_MODE(chan->driver_mode) |
			      HMC7044_DRIVER_Z_MODE(chan->driver_impedance) |
			      (chan->dynamic_driver_enable ?
			       HMC7044_DYN_DRIVER_EN : 0) |
			      (chan->force_mute_enable ?
			       HMC7044_FORCE_MUTE_EN : 0));

		hmc7044_write(dev, HMC7044_REG_CH_OUT_CRTL_3(chan->num),
			      chan->fine_delay & 0x1F);
		hmc7044_write(dev, HMC7044_REG_CH_OUT_CRTL_4(chan->num),
			      chan->coarse_delay & 0x1F);
		hmc7044_write(dev, HMC7044_REG_CH_OUT_CRTL_7(chan->num),
			      chan->out_mux_mode & 0x3);

		hmc7044_write(dev, HMC7044_REG_CH_OUT_CRTL_0(chan->num),
			      (chan->start_up_mode_dynamic_enable ?
			       HMC7044_START_UP_MODE_DYN_EN : 0) |
			      (chan->output_control0_rb4_enable ? NO_OS_BIT(4) : 0) |
			      (chan->high_performance_mode_dis ?
			       0 : HMC7044_HI_PERF_MODE) | HMC7044_SYNC_EN |
			      HMC7044_CH_EN);
	}
	no_os_mdelay(10);


	/* Do a restart to reset the system and initiate calibration */
	hmc7044_write(dev, HMC7044_REG_REQ_MODE_0,
		      HMC7044_RESTART_DIV_FSM);
	no_os_mdelay(1);
	hmc7044_write(dev, HMC7044_REG_REQ_MODE_0,
		      (dev->high_performance_mode_clock_dist_en ?
		       HMC7044_HIGH_PERF_DISTRIB_PATH : 0));
	no_os_mdelay(1);

	return 0;
}

/**
 * Initialize the device.
 * @param device - The device structure.
 * @param init_param - The structure that contains the device initial
 * 		       parameters.
 * @return 0 in case of success, negative error code otherwise.
 */
int32_t hmc7044_init(struct hmc7044_dev **device,
		     const struct hmc7044_init_param *init_param)
{
	struct hmc7044_dev *dev;
	int32_t ret;
	unsigned int i;
	struct no_os_clk_desc **clocks;
	struct no_os_clk_init_param clk_init;
	const char *names[HMC7044_NUM_CHAN] = {
		"clock_0", "clock_1", "clock_2", "clock_3", "clock_4",
		"clock_5", "clock_6", "clock_7", "clock_8", "clock_9",
		"clock_10", "clock_11", "clock_12", "clock_13"
	};

	dev = (struct hmc7044_dev *)no_os_malloc(sizeof(*dev));
	if (!dev)
		return -1;

	ret = no_os_spi_init(&dev->spi_desc, init_param->spi_init);
	if (ret < 0)
		return ret;

	if (init_param->export_no_os_clk) {
		clocks = malloc(HMC7044_NUM_CHAN * sizeof(struct no_os_clk_desc *));
		if (!clocks)
			return -1;
		for (i = 0; i < HMC7044_NUM_CHAN; i++) {
			clocks[i] = malloc(sizeof(struct no_os_clk_desc));
			if (!clocks[i])
				return -1;
			/* Initialize clk component */
			clk_init.name = names[i];
			clk_init.hw_ch_num = i;
			clk_init.platform_ops = &hmc7044_clk_ops;
			clk_init.dev_desc = dev;

			ret = no_os_clk_init(&clocks[i], &clk_init);
			if (ret)
				return ret;
		}
	}

	dev->clk_desc = clocks;

	dev->is_hmc7043 = init_param->is_hmc7043;

	dev->clkin_freq[0] = init_param->clkin_freq[0];
	dev->clkin_freq[1] = init_param->clkin_freq[1];
	dev->clkin_freq[2] = init_param->clkin_freq[2];
	dev->clkin_freq[3] = init_param->clkin_freq[3];

	dev->clkin_freq_ccf[0] = init_param->clkin_freq_ccf[0];
	dev->clkin_freq_ccf[1] = init_param->clkin_freq_ccf[1];
	dev->clkin_freq_ccf[2] = init_param->clkin_freq_ccf[2];
	dev->clkin_freq_ccf[3] = init_param->clkin_freq_ccf[3];

	dev->vcxo_freq = init_param->vcxo_freq;
	dev->pll1_pfd = init_param->pll1_pfd;
	dev->pll2_freq = init_param->pll2_freq;
	dev->pll1_loop_bw = init_param->pll1_loop_bw;

	dev->sysref_timer_div = init_param->sysref_timer_div;
	dev->pll1_ref_prio_ctrl = init_param->pll1_ref_prio_ctrl;
	dev->clkin0_rfsync_en = init_param->clkin0_rfsync_en;
	dev->clkin1_vcoin_en = init_param->clkin1_vcoin_en;
	dev->high_performance_mode_clock_dist_en =
		init_param->high_performance_mode_clock_dist_en;
	dev->rf_reseeder_en = !init_param->rf_reseeder_disable;
	dev->sync_pin_mode = init_param->sync_pin_mode;
	dev->pulse_gen_mode = init_param->pulse_gen_mode;

	dev->in_buf_mode[0] = init_param->in_buf_mode[0];
	dev->in_buf_mode[1] = init_param->in_buf_mode[1];
	dev->in_buf_mode[2] = init_param->in_buf_mode[2];
	dev->in_buf_mode[3] = init_param->in_buf_mode[3];
	dev->in_buf_mode[4] = init_param->in_buf_mode[4];

	dev->gpi_ctrl[0] = init_param->gpi_ctrl[0];
	dev->gpi_ctrl[1] = init_param->gpi_ctrl[1];
	dev->gpi_ctrl[2] = init_param->gpi_ctrl[2];
	dev->gpi_ctrl[3] = init_param->gpi_ctrl[3];

	dev->gpo_ctrl[0] = init_param->gpo_ctrl[0];
	dev->gpo_ctrl[1] = init_param->gpo_ctrl[1];
	dev->gpo_ctrl[2] = init_param->gpo_ctrl[2];
	dev->gpo_ctrl[3] = init_param->gpo_ctrl[3];

	dev->num_channels = init_param->num_channels;
	dev->channels = (struct hmc7044_chan_spec *)
			no_os_malloc(sizeof(*dev->channels) * dev->num_channels);

	for (i = 0; i < dev->num_channels; i++) {
		dev->channels[i].num = init_param->channels[i].num;
		dev->channels[i].disable = init_param->channels[i].disable;
		dev->channels[i].divider = init_param->channels[i].divider;
		dev->channels[i].driver_mode =
			init_param->channels[i].driver_mode;
		dev->channels[i].high_performance_mode_dis =
			init_param->channels[i].high_performance_mode_dis;
		dev->channels[i].start_up_mode_dynamic_enable =
			init_param->channels[i].start_up_mode_dynamic_enable;
		dev->channels[i].dynamic_driver_enable =
			init_param->channels[i].dynamic_driver_enable;
		dev->channels[i].output_control0_rb4_enable =
			init_param->channels[i].output_control0_rb4_enable;
		dev->channels[i].force_mute_enable =
			init_param->channels[i].force_mute_enable;
		dev->channels[i].driver_impedance =
			init_param->channels[i].driver_impedance;
		dev->channels[i].coarse_delay =
			init_param->channels[i].coarse_delay;
		dev->channels[i].fine_delay =
			init_param->channels[i].fine_delay;
		dev->channels[i].out_mux_mode =
			init_param->channels[i].out_mux_mode;
	}

	*device = dev;

	if (!dev->is_hmc7043)
		return hmc7044_setup(dev);
	else
		return hmc7043_setup(dev);
}

/**
 * Remove the device - release resources.
 * @param device - The device structure.
 * @return 0 in case of success, negative error code otherwise.
 */
int32_t hmc7044_remove(struct hmc7044_dev *device)
{
	int32_t ret;

	ret = no_os_spi_remove(device->spi_desc);
	no_os_free(device->channels);
	no_os_free(device);

	return ret;
}

/**
 * @brief Initialize the CLK structure.
 *
 * @param desc - The CLK descriptor.
 * @param init_param - The structure holding the device initial parameters.
 *
 * @return 0 in case of success, negative error code otherwise.
 */
static int hmc7044_clk_init(struct no_os_clk_desc **desc,
			    const struct no_os_clk_init_param *init_param)
{
	struct hmc7044_dev *hmc7044_d;

	/* Exit if we have no init_params */
	if (!init_param) {
		return -EINVAL;
	}

	*desc = no_os_calloc(1, sizeof(**desc));
	/* Exit if memory cannot be allocated */
	if(!*desc) {
		free(*desc);
		return -ENOMEM;
	}

	hmc7044_d = init_param->dev_desc;
	/* Exit if no hardware device specified in init_param */
	if(!hmc7044_d) {
		free(*desc);
		return -ENOMEM;
	}

	(*desc)->name = init_param->name;
	(*desc)->hw_ch_num = init_param->hw_ch_num;

	(*desc)->dev_desc = (void *)no_os_calloc(1, sizeof(struct hmc7044_dev));

	(*desc)->dev_desc = init_param->dev_desc;

	return 0;
}


/**
 * @brief Remove the CLK structure.
 *
 * @param desc - The CLK descriptor.
 *
 * @return 0 in case of success, negative error code otherwise.
 */
static int hmc7044_clk_remove(struct no_os_clk_desc *desc)
{
	struct hmc7044_dev *hmc7044_dev;
	int ret;

	hmc7044_dev = desc->dev_desc;

	if(!hmc7044_dev)
		return -ENODEV;

	free(desc);

	ret = hmc7044_remove(desc->dev_desc);
	if (ret) {
		free(desc->dev_desc);
		return ret;
	}

	return 0;
}

/**
 * @brief Start the clock.
 *
 * @param desc - The CLK descriptor.
 * @param rate - The desiered rate.
 *
 * @return 0 in case of success, negative error code otherwise.
 */
static int hmc7044_recalc_rate(struct no_os_clk_desc *desc, uint64_t *rate)
{
	return hmc7044_clk_recalc_rate(desc->dev_desc, desc->hw_ch_num,
				       rate);
}

/**
 * @brief ad9523 platform specific CLK platform ops structure
 */
const struct no_os_clk_platform_ops hmc7044_clk_ops = {
	.init = &hmc7044_clk_init,
	.clk_recalc_rate =&hmc7044_recalc_rate,
	.remove = &hmc7044_clk_remove
};
