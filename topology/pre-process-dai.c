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

int tplg_create_pcm_caps_template(snd_config_t **pcm_caps_template)
{
	snd_config_t *top, *child;
	int ret;

	ret = snd_config_make(&top, "template", SND_CONFIG_TYPE_COMPOUND);
	if (ret < 0)
		return ret;

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "formats", SND_CONFIG_TYPE_STRING, top);

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "rates", SND_CONFIG_TYPE_STRING, top);

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "rate_min", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "rate_max", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "channels_min", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "channels_max", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "periods_min", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "periods_max", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "period_size_min", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "period_size_max", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "buffer_size_min", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "buffer_size_max", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = tplg_config_make_add(&child, "sig_bits", SND_CONFIG_TYPE_INTEGER, top);

	if (ret < 0)
		snd_config_delete(top);

	*pcm_caps_template = top;
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

static int tplg_update_pcm_object(struct tplg_pre_processor *tplg_pp,
			       snd_config_t *obj_cfg, snd_config_t *parent)
{
	snd_config_t *top, *parent_obj, *obj, *dest, *cfg, *pcm, *child;
	const char *parent_name, *item_name, *direction;
	int ret;

	/* get object name */
	obj = tplg_object_get_instance_config(tplg_pp, obj_cfg);
	item_name = tplg_object_get_name(tplg_pp, obj);
	if (!item_name)
		return -EINVAL;
	
	/* get direction */
	ret = snd_config_search(obj, "direction", &cfg);
	if (ret < 0) {
		SNDERR("no direction attribute in %s\n", item_name);
		return ret;
	}

	ret = snd_config_get_string(cfg, &direction);
	if (ret < 0) {
		SNDERR("Invalid direction attribute in %s\n", item_name);
		return ret;
	}

	/* add to parent section */
	top = tplg_object_get_section(tplg_pp, parent);
	if (!top) {
		SNDERR("Cannot find parent for %s\n", item_name);
		return -EINVAL;
	}

	parent_obj = tplg_object_get_instance_config(tplg_pp, parent);

	/* get parent name. if parent has no name, skip adding config */
	parent_name = tplg_object_get_name(tplg_pp, parent_obj);
	if (!parent_name)
		return 0;

	/* find parent config with name */
	dest = tplg_find_config(top, parent_name);
	if (!dest) {
		SNDERR("Cannot find parent section %s\n", parent_name);
		return -EINVAL;
	}

	ret = snd_config_search(dest, "pcm", &pcm);
	if (ret < 0) {
		ret = tplg_config_make_add(&pcm, "pcm", SND_CONFIG_TYPE_COMPOUND, dest);
		if (ret < 0) {
			SNDERR("Error creating pcm config in %s\n", parent_name);
			return ret;
		}
	}

	ret = snd_config_search(pcm, direction, &cfg);
	if (ret >= 0) {
		SNDERR("pcm.%s exists already in %s\n", direction, parent_name);
		return -EEXIST;
	}

	ret = tplg_config_make_add(&cfg, direction, SND_CONFIG_TYPE_COMPOUND, pcm);

	if (ret >= 0)
	ret = tplg_config_make_add(&child, "capabilities", SND_CONFIG_TYPE_STRING, cfg);

	if (ret >= 0)
	ret = snd_config_set_string(child, item_name);

	return ret;
}

int tplg_build_pcm_caps_object(struct tplg_pre_processor *tplg_pp,
			       snd_config_t *obj_cfg, snd_config_t *parent)
{
	snd_config_t *caps;
	int ret;

	ret = tplg_build_object_from_template(tplg_pp, obj_cfg, &caps, NULL, false);
	if (ret < 0)
		return ret;

	tplg_pp_config_debug(tplg_pp, caps);

	/* add pcm capabilities to parent */
	return tplg_update_pcm_object(tplg_pp, obj_cfg, parent);
}
