/*
  Copyright(c) 2021 Intel Corporation
  All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
  The full GNU General Public License is included in this distribution
  in the file called LICENSE.GPL.
*/
#include <errno.h>
#include <stdio.h>
#include <alsa/input.h>
#include <alsa/output.h>
#include <alsa/conf.h>
#include <alsa/error.h>
#include "topology.h"
#include "pre-processor.h"

int tplg_pp_create_be_template(snd_config_t **be_template)
{
	snd_config_t *top, *child;
	int ret;

	ret = snd_config_make(&top, "template", SND_CONFIG_TYPE_COMPOUND);
	if (ret < 0)
		return ret;

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "id", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "stream_name", SND_CONFIG_TYPE_STRING, top);

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "default_hw_conf_id",
					  SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "symmertic_rates",
					  SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "symmetric_channels",
					  SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "symmetric_sample_bits",
					  SND_CONFIG_TYPE_INTEGER, top);
	if (ret < 0)
		snd_config_delete(top);

	*be_template = top;
	return ret;
}

int tplg_create_pcm_template(snd_config_t **pcm_template)
{
	snd_config_t *top, *child;
	int ret;

	ret = snd_config_make(&top, "template", SND_CONFIG_TYPE_COMPOUND);
	if (ret < 0)
		return ret;

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "id", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "compress", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "symmertic_rates",
					  SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "symmetric_channels",
					  SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "symmetric_sample_bits",
					  SND_CONFIG_TYPE_INTEGER, top);

	if (ret < 0)
		snd_config_delete(top);

	*pcm_template = top;
	return ret;
}

int tplg_create_hwcfg_template(snd_config_t **template)
{
	snd_config_t *top, *child;
	int ret;

	ret = snd_config_make(&top, "template", SND_CONFIG_TYPE_COMPOUND);
	if (ret < 0)
		return ret;

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "id", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "format", SND_CONFIG_TYPE_STRING, top);

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "bclk", SND_CONFIG_TYPE_STRING, top);

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "bclk_freq", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "bclk_invert", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "fsync", SND_CONFIG_TYPE_STRING, top);

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "fsync_invert", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "fsync_freq", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "mclk", SND_CONFIG_TYPE_STRING, top);

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "mclk_freq", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "pm_gate_clocks", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "tdm_slots", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "tdm_slot_width", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "tx_slots", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "rx_slots", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "tx_channels", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "rx_channels", SND_CONFIG_TYPE_INTEGER, top);

	if (ret < 0)
		snd_config_delete(top);

	*template = top;
	return ret;
}

/* create new fe_dai config template */
int tplg_create_fe_dai_template(snd_config_t **ctemplate)
{
	snd_config_t *top, *child;
	int ret;

	ret = snd_config_make(&top, "template", SND_CONFIG_TYPE_COMPOUND);
	if (ret < 0)
		return ret;

	if (ret >= 0)
	ret = tplg_config_make_add(&child, "id", SND_CONFIG_TYPE_INTEGER, top);

	if (ret < 0)
		snd_config_delete(top);

	*ctemplate = top;

	return ret;
}

int tplg_build_hw_cfg_object(struct tplg_pre_processor *tplg_pp,
			       snd_config_t *obj_cfg, snd_config_t *parent)
{
	snd_config_t *hw_cfg, *obj;
	const char *name;
	int ret;

	obj = tplg_object_get_instance_config(tplg_pp, obj_cfg);

	name = tplg_object_get_name(tplg_pp, obj);
	if (!name)
		return -EINVAL;

	ret = tplg_build_object_from_template(tplg_pp, obj_cfg, &hw_cfg, NULL, false);
	if (ret < 0)
		return ret;

	return tplg_parent_update(tplg_pp, parent, "hw_configs", name);
}

int tplg_build_fe_dai_object(struct tplg_pre_processor *tplg_pp, snd_config_t *obj_cfg,
			      snd_config_t *parent)
{
	return tplg_build_base_object(tplg_pp, obj_cfg, parent, false);
}
