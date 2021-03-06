/*
 *  Copyright (C) 2012, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */
#include "../ssp.h"

/*************************************************************************/
/* factory Sysfs                                                         */
/*************************************************************************/

#define VENDOR		"INVENSENSE"
#define CHIP_ID		"MPU9250"

#define GM_DATA_SPEC_MIN	-6500
#define GM_DATA_SPEC_MAX	6500

#define GM_SELFTEST_X_SPEC_MIN	-200
#define GM_SELFTEST_X_SPEC_MAX	200
#define GM_SELFTEST_Y_SPEC_MIN	-200
#define GM_SELFTEST_Y_SPEC_MAX	200
#define GM_SELFTEST_Z_SPEC_MIN	-3200
#define GM_SELFTEST_Z_SPEC_MAX	-800

int set_magnetic_static_matrix(struct ssp_data *data)
{
	int iRet = 0;

	struct ssp_msg *msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	if (msg == NULL) {
		ssp_errf("failed to alloc memory for ssp_msg");
		return -ENOMEM;
	}
	msg->cmd = MSG2SSP_AP_SET_MAGNETIC_STATIC_MATRIX;
	msg->length = data->mag_matrix_size;
	msg->options = AP2HUB_WRITE;
	msg->buffer = (char*) kzalloc(data->mag_matrix_size, GFP_KERNEL);
	msg->free_buffer = 1;

	memcpy(msg->buffer, data->mag_matrix, data->mag_matrix_size);

	iRet = ssp_spi_async(data, msg);
	if (iRet != SUCCESS) {
		ssp_errf("fail to set_magnetic_static_matrix %d", iRet);
		iRet = ERROR;
	}

	return iRet;
}

static ssize_t magnetic_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR);
}

static ssize_t magnetic_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", CHIP_ID);
}

static int check_data_spec(struct ssp_data *data)
{
	if ((data->buf[GEOMAGNETIC_RAW].x == 0) &&
		(data->buf[GEOMAGNETIC_RAW].y == 0) &&
		(data->buf[GEOMAGNETIC_RAW].z == 0))
		return FAIL;
	else if ((data->buf[GEOMAGNETIC_RAW].x > GM_DATA_SPEC_MAX)
		|| (data->buf[GEOMAGNETIC_RAW].x < GM_DATA_SPEC_MIN)
		|| (data->buf[GEOMAGNETIC_RAW].y > GM_DATA_SPEC_MAX)
		|| (data->buf[GEOMAGNETIC_RAW].y < GM_DATA_SPEC_MIN)
		|| (data->buf[GEOMAGNETIC_RAW].z > GM_DATA_SPEC_MAX)
		|| (data->buf[GEOMAGNETIC_RAW].z < GM_DATA_SPEC_MIN))
		return FAIL;
	else
		return SUCCESS;
}

static ssize_t raw_data_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	ssp_infof("x=%d, y=%d, z=%d",
		data->buf[GEOMAGNETIC_RAW].x,
		data->buf[GEOMAGNETIC_RAW].y,
		data->buf[GEOMAGNETIC_RAW].z);

	if (data->bGeomagneticRawEnabled == false) {
		data->buf[GEOMAGNETIC_RAW].x = -1;
		data->buf[GEOMAGNETIC_RAW].y = -1;
		data->buf[GEOMAGNETIC_RAW].z = -1;
	} else {
		if (data->buf[GEOMAGNETIC_RAW].x > 32766)
			data->buf[GEOMAGNETIC_RAW].x = 32766;
		else if (data->buf[GEOMAGNETIC_RAW].x < -32767)
			data->buf[GEOMAGNETIC_RAW].x = -32767;
		if (data->buf[GEOMAGNETIC_RAW].y > 32766)
			data->buf[GEOMAGNETIC_RAW].y = 32766;
		else if (data->buf[GEOMAGNETIC_RAW].y < -32767)
			data->buf[GEOMAGNETIC_RAW].y = -32767;
		if (data->buf[GEOMAGNETIC_RAW].z > 32766)
			data->buf[GEOMAGNETIC_RAW].z = 32766;
		else if (data->buf[GEOMAGNETIC_RAW].z < -32767)
			data->buf[GEOMAGNETIC_RAW].z = -32767;
	}

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n",
		data->buf[GEOMAGNETIC_RAW].x,
		data->buf[GEOMAGNETIC_RAW].y,
		data->buf[GEOMAGNETIC_RAW].z);
}

static ssize_t raw_data_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	char chTempbuf[9] = { 0 };
	int iRet;
	int64_t dEnable;
	int iRetries = 50;
	struct ssp_data *data = dev_get_drvdata(dev);
	s32 dMsDelay = 20;
	memcpy(&chTempbuf[0], &dMsDelay, 4);
	memcpy(&chTempbuf[4], &data->batchLatencyBuf[GEOMAGNETIC_RAW], 4);
	chTempbuf[8] = data->batchOptBuf[GEOMAGNETIC_RAW];

	iRet = kstrtoll(buf, 10, &dEnable);
	if (iRet < 0)
		return iRet;

	if (dEnable) {
		data->buf[GEOMAGNETIC_RAW].x = 0;
		data->buf[GEOMAGNETIC_RAW].y = 0;
		data->buf[GEOMAGNETIC_RAW].z = 0;

		send_instruction(data, ADD_SENSOR, GEOMAGNETIC_RAW,
			chTempbuf, 9);

#if 0
		do {
			msleep(20);
			if (check_data_spec(data) == SUCCESS)
				break;
		} while (--iRetries);
#endif

		if (iRetries > 0) {
			ssp_infof("success, %d", retries);
			data->bGeomagneticRawEnabled = true;
		} else {
			ssp_errf("wait timeout, %d", retries);
			data->bGeomagneticRawEnabled = false;
		}
	} else {
		send_instruction(data, REMOVE_SENSOR, GEOMAGNETIC_RAW,
			chTempbuf, 4);
		data->bGeomagneticRawEnabled = false;
	}

	return size;
}

static ssize_t adc_data_read(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	bool bSuccess = false;
	u8 chTempbuf[9] = { 0 };
	s16 iSensorBuf[3] = {0, };
	int iRetries = 10;
	struct ssp_data *data = dev_get_drvdata(dev);
	s32 dMsDelay = 20;
	memcpy(&chTempbuf[0], &dMsDelay, 4);
	memcpy(&chTempbuf[4], &data->batchLatencyBuf[GEOMAGNETIC_RAW], 4);
	chTempbuf[8] = data->batchOptBuf[GEOMAGNETIC_RAW];

	data->buf[GEOMAGNETIC_SENSOR].x = 0;
	data->buf[GEOMAGNETIC_SENSOR].y = 0;
	data->buf[GEOMAGNETIC_SENSOR].z = 0;

	if (!(atomic_read(&data->aSensorEnable) & (1 << GEOMAGNETIC_SENSOR)))
		send_instruction(data, ADD_SENSOR, GEOMAGNETIC_SENSOR,
			chTempbuf, 9);

	do {
		msleep(60);
		if (check_data_spec(data) == SUCCESS)
			break;
	} while (--iRetries);

	if (iRetries > 0)
		bSuccess = true;

	iSensorBuf[0] = data->buf[GEOMAGNETIC_SENSOR].x;
	iSensorBuf[1] = data->buf[GEOMAGNETIC_SENSOR].y;
	iSensorBuf[2] = data->buf[GEOMAGNETIC_SENSOR].z;

	if (!(atomic_read(&data->aSensorEnable) & (1 << GEOMAGNETIC_SENSOR)))
		send_instruction(data, REMOVE_SENSOR, GEOMAGNETIC_SENSOR,
			chTempbuf, 4);

	ssp_infof("x = %d, y = %d, z = %d",
		iSensorBuf[0], iSensorBuf[1], iSensorBuf[2]);

	return snprintf(buf, PAGE_SIZE, "%s,%d,%d,%d\n",
		(bSuccess ? "OK" : "NG"),
		iSensorBuf[0], iSensorBuf[1], iSensorBuf[2]);
}

static ssize_t magnetic_get_asa(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n",
		(s16)data->uFuseRomData[0],
		(s16)data->uFuseRomData[1],
		(s16)data->uFuseRomData[2]);
}

static ssize_t magnetic_get_status(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	bool bSuccess;
	struct ssp_data *data = dev_get_drvdata(dev);

	if ((data->uFuseRomData[0] == 0) ||
		(data->uFuseRomData[0] == 0xff) ||
		(data->uFuseRomData[1] == 0) ||
		(data->uFuseRomData[1] == 0xff) ||
		(data->uFuseRomData[2] == 0) ||
		(data->uFuseRomData[2] == 0xff))
		bSuccess = false;
	else
		bSuccess = true;

	return snprintf(buf, PAGE_SIZE, "%s,%u\n",
		(bSuccess ? "OK" : "NG"), bSuccess);
}

static ssize_t magnetic_check_cntl(struct device *dev,
		struct device_attribute *attr, char *strbuf)
{
	bool bSuccess = false;
	int iRet;
	char chTempBuf[7] = { 0,  };
	struct ssp_data *data = dev_get_drvdata(dev);
	struct ssp_msg *msg;

	if (!data->uMagCntlRegData) {
		bSuccess = true;
	} else {
		ssp_infof("check cntl register before selftest");
		msg = kzalloc(sizeof(*msg), GFP_KERNEL);
		if (msg == NULL) {
			ssp_errf("failed to alloc memory for ssp_msg");
			return -ENOMEM;
		}
		msg->cmd = GEOMAGNETIC_FACTORY;
		msg->length = 7;
		msg->options = AP2HUB_READ;
		msg->buffer = chTempBuf;
		msg->free_buffer = 0;

		iRet = ssp_spi_sync(data, msg, 1000);

		if (iRet != SUCCESS) {
			ssp_errf("spi sync failed due to Timeout!! %d", ret);
		}

		data->uMagCntlRegData = (chTempBuf[0] & 0x0f);
		bSuccess = !data->uMagCntlRegData;
	}

	ssp_infof("CTRL : 0x%x", data->uMagCntlRegData);

	data->uMagCntlRegData = 1;	/* reset the value */

	return snprintf(strbuf, PAGE_SIZE, "%s,%d,%d,%d\n",
		(bSuccess ? "OK" : "NG"), (bSuccess ? 1 : 0), 0, 0);
}

static ssize_t magnetic_get_selftest(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	s8 iResult[4] = {-1, -1, -1, -1};
	char bufSelftset[7] = {0, };
	char bufAdc[8] = {0, };
	s16 iSF_X = 0, iSF_Y = 0, iSF_Z = 0;
	s16 iADC_X = 0, iADC_Y = 0, iADC_Z = 0;
	s32 dMsDelay = 20;
	int iRet = 0, iSpecOutRetries = 0;
	struct ssp_data *data = dev_get_drvdata(dev);
	struct ssp_msg *msg;

	ssp_infof("in");

	/* STATUS */
	if ((data->uFuseRomData[0] == 0) ||
		(data->uFuseRomData[0] == 0xff) ||
		(data->uFuseRomData[1] == 0) ||
		(data->uFuseRomData[1] == 0xff) ||
		(data->uFuseRomData[2] == 0) ||
		(data->uFuseRomData[2] == 0xff))
		iResult[0] = -1;
	else
		iResult[0] = 0;

Retry_selftest:
	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	if (msg == NULL) {
		ssp_errf("failed to alloc memory for ssp_msg");
		goto exit;
	}
	msg->cmd = GEOMAGNETIC_FACTORY;
	msg->length = 7;
	msg->options = AP2HUB_READ;
	msg->buffer = bufSelftset;
	msg->free_buffer = 0;

	iRet = ssp_spi_sync(data, msg, 1000);
	if (iRet != SUCCESS) {
		ssp_errf("Magnetic Selftest Timeout!! %d", ret);
		goto exit;
	}

	/* read 6bytes data registers */
	iSF_X = (s16)((bufSelftset[2] << 8) + bufSelftset[1]);
	iSF_Y = (s16)((bufSelftset[4] << 8) + bufSelftset[3]);
	iSF_Z = (s16)((bufSelftset[6] << 8) + bufSelftset[5]);

	/* DAC (store Cntl Register value to check power down) */
	iResult[2] = (bufSelftset[0] & 0x0f);

	iSF_X = (s16)(((int)iSF_X * ((int)data->uFuseRomData[0] + 128)) >> 8);
	iSF_Y = (s16)(((int)iSF_Y * ((int)data->uFuseRomData[1] + 128)) >> 8);
	iSF_Z = (s16)(((int)iSF_Z * ((int)data->uFuseRomData[2] + 128))>> 8);

	ssp_infof("self test x = %d, y = %d, z = %d", iSF_X, iSF_Y, iSF_Z);

	if ((iSF_X >= GM_SELFTEST_X_SPEC_MIN)
		&& (iSF_X <= GM_SELFTEST_X_SPEC_MAX))
		ssp_infof("x passed self test, expect -200<=x<=200");
	else
		ssp_infof("x failed self test, expect -200<=x<=200");
	if ((iSF_Y >= GM_SELFTEST_Y_SPEC_MIN)
		&& (iSF_Y <= GM_SELFTEST_Y_SPEC_MAX))
		ssp_infof("y passed self test, expect -200<=y<=200");
	else
		ssp_infof("y failed self test, expect -200<=y<=200");
	if ((iSF_Z >= GM_SELFTEST_Z_SPEC_MIN)
		&& (iSF_Z <= GM_SELFTEST_Z_SPEC_MAX))
		ssp_infof("z passed self test, expect -3200<=z<=-800");
	else
		ssp_infof("z failed self test, expect -3200<=z<=-800");

	/* SELFTEST */
	if ((iSF_X >= GM_SELFTEST_X_SPEC_MIN)
		&& (iSF_X <= GM_SELFTEST_X_SPEC_MAX)
		&& (iSF_Y >= GM_SELFTEST_Y_SPEC_MIN)
		&& (iSF_Y <= GM_SELFTEST_Y_SPEC_MAX)
		&& (iSF_Z >= GM_SELFTEST_Z_SPEC_MIN)
		&& (iSF_Z <= GM_SELFTEST_Z_SPEC_MAX))
		iResult[1] = 0;

	if ((iResult[1] == -1) && (iSpecOutRetries++ < 5)) {
		ssp_errf("selftest spec out. Retry = %d", iSpecOutRetries);
		goto Retry_selftest;
	}

	iSpecOutRetries = 10;

	/* ADC */
	memcpy(&bufAdc[0], &dMsDelay, 8);

	data->buf[GEOMAGNETIC_RAW].x = 0;
	data->buf[GEOMAGNETIC_RAW].y = 0;
	data->buf[GEOMAGNETIC_RAW].z = 0;

	if (!(atomic_read(&data->aSensorEnable) & (1 << GEOMAGNETIC_RAW)))
		send_instruction(data, ADD_SENSOR, GEOMAGNETIC_RAW,
			bufAdc, 8);

	do {
		msleep(60);
		if (check_data_spec(data) == SUCCESS)
			break;
	} while (--iSpecOutRetries);

	if (iSpecOutRetries > 0)
		iResult[3] = 0;

	iADC_X = data->buf[GEOMAGNETIC_RAW].x;
	iADC_Y = data->buf[GEOMAGNETIC_RAW].y;
	iADC_Z = data->buf[GEOMAGNETIC_RAW].z;

	if (!(atomic_read(&data->aSensorEnable) & (1 << GEOMAGNETIC_RAW)))
		send_instruction(data, REMOVE_SENSOR, GEOMAGNETIC_RAW,
			bufAdc, 4);

	ssp_infof("adc, x = %d, y = %d, z = %d, retry = %d",
		iADC_X, iADC_Y, iADC_Z, iSpecOutRetries);

exit:
	ssp_infof("out. Result = %d %d %d %d",
		iResult[0], iResult[1], iResult[2], iResult[3]);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		iResult[0], iResult[1], iSF_X, iSF_Y, iSF_Z,
		iResult[2], iResult[3], iADC_X, iADC_Y, iADC_Z);
}

#ifdef SAVE_MAG_LOG
static ssize_t raw_data_logging_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	if (data->bGeomagneticLogged == false) {
		data->buf[GEOMAGNETIC_SENSOR].x = -1;
		data->buf[GEOMAGNETIC_SENSOR].y = -1;
		data->buf[GEOMAGNETIC_SENSOR].z = -1;
	}

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n",
		data->buf[GEOMAGNETIC_SENSOR].x,
		data->buf[GEOMAGNETIC_SENSOR].y,
		data->buf[GEOMAGNETIC_SENSOR].z);
}

static ssize_t raw_data_logging_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	u8 uBuf[4] = {0, };
	int iRet;
	int64_t dEnable;
	struct ssp_data *data = dev_get_drvdata(dev);
	s32 dMsDelay = 10;
	memcpy(&uBuf[0], &dMsDelay, 4);

	iRet = kstrtoll(buf, 10, &dEnable);
	if (iRet < 0)
		return iRet;

	if (dEnable) {
		ssp_infof("[add %u, New = %dns",
			1 << GEOMAGNETIC_SENSOR, dMsDelay);

		iRet = send_instruction(data, GET_LOGGING, GEOMAGNETIC_SENSOR, uBuf, 4);
		if (iRet == SUCCESS) {
			ssp_infof("success");
			data->bGeomagneticLogged = true;
		} else {
			ssp_errf("failed, %d", iRet);
			data->bGeomagneticLogged = false;
		}
	} else {
		iRet = send_instruction(data, REMOVE_SENSOR, GEOMAGNETIC_SENSOR, uBuf, 4);
		if (iRet == SUCCESS) {
			ssp_infof("success");
			data->bGeomagneticLogged = false;
		}
		ssp_info("remove sensor = %d", (1 << GEOMAGNETIC_SENSOR));
	}

	return size;
}
#endif

static DEVICE_ATTR(name, S_IRUGO, magnetic_name_show, NULL);
static DEVICE_ATTR(vendor, S_IRUGO, magnetic_vendor_show, NULL);
static DEVICE_ATTR(raw_data, S_IRUGO | S_IWUSR | S_IWGRP,
		raw_data_show, raw_data_store);
static DEVICE_ATTR(adc, S_IRUGO, adc_data_read, NULL);
static DEVICE_ATTR(selftest, S_IRUGO, magnetic_get_selftest, NULL);
static DEVICE_ATTR(status, S_IRUGO,  magnetic_get_status, NULL);
static DEVICE_ATTR(dac, S_IRUGO, magnetic_check_cntl, NULL);
static DEVICE_ATTR(mpu9250_asa, S_IRUGO, magnetic_get_asa, NULL);
#ifdef SAVE_MAG_LOG
static DEVICE_ATTR(logging_data, S_IRUGO | S_IWUSR | S_IWGRP,
		raw_data_logging_show, raw_data_logging_store);
#endif

static struct device_attribute *mag_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_adc,
	&dev_attr_dac,
	&dev_attr_raw_data,
	&dev_attr_selftest,
	&dev_attr_status,
	&dev_attr_mpu9250_asa,
#ifdef SAVE_MAG_LOG
	&dev_attr_logging_data,
#endif
	NULL,
};

void initialize_magnetic_factorytest(struct ssp_data *data)
{
	sensors_register(data->mag_device, data,
				mag_attrs, "magnetic_sensor");
}

void remove_magnetic_factorytest(struct ssp_data *data)
{
	sensors_unregister(data->mag_device, mag_attrs);
}
