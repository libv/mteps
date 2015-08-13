/*
 * Copyright (c) 2015 Luc Verhaegen <libv@skynet.be>
 * Copyright (c) 2012-2015 Vitaly Shukela <vi0oss@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * MTEPS: Multi Touch Events Per Second: fake multitouch driver to verify
 * system overhead influence on the whole GUI stack.
 *
 * Started life as a copy of the virtual_touchscreen driver by Vitaly
 * Shukela. Original code exists at https://github.com/vi/virtual_touchscreen
 */

#include <linux/input/mt.h>
#include <linux/module.h>

#define ABS_X_MIN	0
#define ABS_X_MAX	1024
#define ABS_Y_MIN	0
#define ABS_Y_MAX	1024

#define MAX_CONTACTS 10

static struct input_dev *mteps_dev;

static int __init
mteps_init(void)
{
	mteps_dev = input_allocate_device();
	if (!mteps_dev)
		return -ENOMEM;

	mteps_dev->evbit[0] = BIT_MASK(EV_ABS) | BIT_MASK(EV_KEY);
	mteps_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);

	input_set_abs_params(mteps_dev, ABS_X, ABS_X_MIN, ABS_X_MAX, 0, 0);
	input_set_abs_params(mteps_dev, ABS_Y, ABS_Y_MIN, ABS_Y_MAX, 0, 0);

	mteps_dev->name = "MTEPS";
	mteps_dev->phys = "mteps/input0";

	input_mt_init_slots(mteps_dev, MAX_CONTACTS, INPUT_MT_DIRECT);

	input_set_abs_params(mteps_dev, ABS_MT_POSITION_X, ABS_X_MIN, ABS_X_MAX, 0, 0);
	input_set_abs_params(mteps_dev, ABS_MT_POSITION_Y, ABS_Y_MIN, ABS_Y_MAX, 0, 0);

	return input_register_device(mteps_dev);
}

#if 0
static void
execute_command(char command, int arg1) {
	switch(command) {
        case 'x':
		input_report_abs(mteps_dev, ABS_X, arg1);
		break;
        case 'y':
		input_report_abs(mteps_dev, ABS_Y, arg1);
		break;
        case 'd':
		input_report_key(mteps_dev, BTN_TOUCH, 1);
		break;
        case 'u':
		input_report_key(mteps_dev, BTN_TOUCH, 0);
		break;
        case 's':
		input_mt_slot(mteps_dev, arg1);
		break;
        case 'a':
		input_mt_report_slot_state(mteps_dev, MT_TOOL_FINGER, arg1);
		break;
        case 'e':
		input_mt_report_pointer_emulation(mteps_dev, true);
		break;
        case 'X':
		input_event(mteps_dev, EV_ABS, ABS_MT_POSITION_X, arg1);
		break;
        case 'Y':
		input_event(mteps_dev, EV_ABS, ABS_MT_POSITION_Y, arg1);
		break;
        case 'S':
	        input_sync(mteps_dev);
		break;
        case 'M':
		input_mt_sync(mteps_dev);
		break;
        case 'T':
		input_event(mteps_dev, EV_ABS, ABS_MT_TRACKING_ID, arg1);
		break;
        default:
		if ((command >= 0x30) && (command <= 0x3b))
			input_event(mteps_dev, EV_ABS, command, arg1);
		else
			pr_err("<4>mteps: Unknown command %c with arg %d\n",
			       command, arg1);
	}
}
#endif

static void __exit
mteps_exit(void)
{
	input_unregister_device(mteps_dev);
}

#define MODNAME "mteps"

module_init(mteps_init);
module_exit(mteps_exit);

MODULE_AUTHOR("Luc Verhaegen, libv@skynet.be");
MODULE_DESCRIPTION("Multitouch Events Per Second driver");
MODULE_LICENSE("GPL");
