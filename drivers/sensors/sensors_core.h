/*
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#ifndef _SENSORS_CORE_H_
#define _SENSORS_CORE_H_

#ifdef CONFIG_SLEEP_MONITOR
#define SENSORS_ENABLE_ACCEL	0
#define SENSORS_ENABLE_PROXY	1
#define DEVICE_POWER_OFF_MASK	0
#define DEVICE_ON_ACTIVE2_MASK	(1 << SENSORS_ENABLE_ACCEL | 1 << SENSORS_ENABLE_PROXY)

void sensors_set_enable_mask(int enable, int offset_bit);
#endif

#include <linux/input.h>

int sensors_create_symlink(struct kobject *, const char *);
void sensors_remove_symlink(struct kobject *, const char *);
int sensors_register(struct device *, void *,
	struct device_attribute *[], char *);
void sensors_unregister(struct device *, struct device_attribute *[]);
void destroy_sensor_class(void);
void remap_sensor_data(s16 *val, u32 idx);

struct device *sensors_classdev_register(char *sensors_name);
void sensors_classdev_unregister(struct device *dev);

#endif
