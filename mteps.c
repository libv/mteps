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
#include <linux/hrtimer.h>
#include <linux/workqueue.h>

#define MTEPS_FREQUENCY 120

#define ABS_X_MIN	0
#define ABS_X_MAX	1024
#define ABS_Y_MIN	0
#define ABS_Y_MAX	1024

#define MAX_CONTACTS 10

static struct input_dev *mteps_dev;

static struct hrtimer mteps_hrtimer[1];
static ktime_t mteps_ktime;
static int mteps_rate = MTEPS_FREQUENCY; /* should be a module option */

static struct workqueue_struct *mteps_wq;
static struct work_struct mteps_work[1];

#define MTEPS_DIRECTION_EAST  0
#define MTEPS_DIRECTION_SOUTH 1
#define MTEPS_DIRECTION_WEST  2
#define MTEPS_DIRECTION_NORTH 3
#define MTEPS_DIRECTION_COUNT 4

static void
mteps_work_func(struct work_struct *work)
{
	static int mteps_count = 0;
	static int mteps_direction = 0;

	pr_info("(%d, %d)\n", mteps_direction, mteps_count);

	mteps_count++;
	if (mteps_count == mteps_rate) {
		mteps_count = 0;
		mteps_direction++;
		if (mteps_direction == MTEPS_DIRECTION_COUNT)
			mteps_direction =  MTEPS_DIRECTION_EAST;
	}
}

static int
mteps_wq_init(void)
{
	mteps_wq = create_singlethread_workqueue("mteps");
	if (!mteps_wq)
		return -ENOMEM;

	INIT_WORK(mteps_work, mteps_work_func);

	return 0;
}

static void
mteps_wq_exit(void)
{
	destroy_workqueue(mteps_wq);
}

static enum hrtimer_restart
mteps_hrtimer_callback(struct hrtimer *hrtimer)
{
	queue_work(mteps_wq, mteps_work);

	hrtimer_forward_now(hrtimer, mteps_ktime);

	return HRTIMER_RESTART;
}

static int
mteps_hrtimer_init(void)
{
	int ret;

	pr_info("%s: rate %d -> %ldnsec\n", __func__,
		mteps_rate, NSEC_PER_SEC / mteps_rate);

	mteps_ktime = ktime_set(0, NSEC_PER_SEC / mteps_rate);

	hrtimer_init(mteps_hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);

	mteps_hrtimer->function = mteps_hrtimer_callback;

	ret = hrtimer_start(mteps_hrtimer, mteps_ktime, HRTIMER_MODE_REL);
	if (ret)
		return ret;

        return 0;
}

static void
mteps_hrtimer_exit(void)
{
	hrtimer_cancel(mteps_hrtimer);
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

static int
mteps_input_init(void)
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

	input_set_abs_params(mteps_dev,
			     ABS_MT_POSITION_X, ABS_X_MIN, ABS_X_MAX, 0, 0);
	input_set_abs_params(mteps_dev,
			     ABS_MT_POSITION_Y, ABS_Y_MIN, ABS_Y_MAX, 0, 0);

	return input_register_device(mteps_dev);
}

static void
mteps_input_exit(void)
{
	input_unregister_device(mteps_dev);
}

static int __init
mteps_init(void)
{
	int ret;

	ret = mteps_input_init();
	if (ret)
		return ret;

	ret = mteps_wq_init();
	if (ret) {
		mteps_input_exit();
		return ret;
	}

	ret = mteps_hrtimer_init();
	if (ret) {
		mteps_wq_exit();
		mteps_input_exit();
		return ret;
	}

	return 0;
}

static void __exit
mteps_exit(void)
{
	mteps_hrtimer_exit();
	mteps_wq_exit();
	mteps_input_exit();
}

#define MODNAME "mteps"

module_init(mteps_init);
module_exit(mteps_exit);

MODULE_AUTHOR("Luc Verhaegen, libv@skynet.be");
MODULE_DESCRIPTION("Multitouch Events Per Second driver");
MODULE_LICENSE("GPL");
