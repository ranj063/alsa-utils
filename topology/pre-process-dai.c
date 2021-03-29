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
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <alsa/input.h>
#include <alsa/output.h>
#include <alsa/conf.h>
#include <alsa/error.h>
#include <alsa/global.h>
#include <alsa/topology.h>
#include <alsa/soc-topology.h>
#include "gettext.h"
#include "topology.h"
#include "pre-processor.h"
#include "../alsactl/list.h"

static int tplg_pp_create_hwcfg_config(snd_config_t *parent, char *name)
{
	snd_config_t *top, *child;
	int ret;

	ret = snd_config_make_add(&top, name, SND_CONFIG_TYPE_COMPOUND, parent);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "id", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "format", SND_CONFIG_TYPE_STRING, top);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "bclk", SND_CONFIG_TYPE_STRING, top);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "bclk_freq", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "bclk_invert", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "fsync", SND_CONFIG_TYPE_STRING, top);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "fsync_invert", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "fsync_freq", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "mclk", SND_CONFIG_TYPE_STRING, top);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "mclk_freq", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "pm_gate_clocks", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "tdm_slots", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "tdm_slot_width", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "tx_slots", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "rx_slots", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "tx_channels", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "rx_channels", SND_CONFIG_TYPE_INTEGER, top);

	return ret;
}

int tplg_pp_build_hw_cfg_object(struct tplg_pre_processor *tplg_pp, struct tplg_object *object)
{
	struct tplg_attribute *attr, *id;
	snd_config_t *top, *hw_cfg;
	char name[SNDRV_CTL_ELEM_ID_NAME_MAXLEN];
	int ret;

	tplg_pp_debug("Building SectionHWConfig for: '%s' ...", object->name);

	/* get top-level SectionControlMixer config */
	ret = snd_config_search(tplg_pp->cfg, "SectionHWConfig", &top);
	if (ret < 0) {
		ret = snd_config_make_add(&top, "SectionHWConfig",
					  SND_CONFIG_TYPE_COMPOUND, tplg_pp->cfg);
		if (ret < 0) {
			SNDERR("Error creating SectionHWConfig config\n");
			return ret;
		}
	}

	id = tplg_get_attribute_by_name(&object->attribute_list, "id");
	ret = snprintf(name, SNDRV_CTL_ELEM_ID_NAME_MAXLEN, "%s.%ld", object->parent->name,
		       id->value.integer);
	if (ret > SNDRV_CTL_ELEM_ID_NAME_MAXLEN) {
		SNDERR("hwcfg name too long\n");
		return ret;
	}

	/* create hwcfg config */
	ret = tplg_pp_create_hwcfg_config(top, name);
	if (ret < 0) {
		SNDERR("Error creating hw_cfg config for %s\n", object->name);
		return ret;
	}

	hw_cfg = tplg_find_config(top, name);
	if (!hw_cfg) {
		SNDERR("Can't find hwcfg config %s\n", object->name);
		return -EINVAL;
	}

	/* update hwcfg config */
	list_for_each_entry(attr, &object->attribute_list, list) {

		ret = tplg_attribute_config_update(hw_cfg, attr);
		if (ret < 0) {
			SNDERR("failed to add config for attribute %s in hwcfg %s\n",
			       attr->name, object->name);
			return ret;
		}
	}

	return ret;
}

static int tplg_pp_create_be_config(snd_config_t *parent, char *name)
{
	snd_config_t *top, *child;
	int ret;

	ret = snd_config_make_add(&top, name, SND_CONFIG_TYPE_COMPOUND, parent);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "id", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "stream_name", SND_CONFIG_TYPE_STRING, top);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "default_hw_conf_id",
					  SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "symmertic_rates",
					  SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "symmetric_channels",
					  SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "symmetric_sample_bits",
					  SND_CONFIG_TYPE_INTEGER, top);

	return ret;
}

int tplg_build_dai_object(struct tplg_pre_processor *tplg_pp, struct tplg_object *object)
{
	struct tplg_attribute *attr;
	struct tplg_object *child;
	snd_config_t *top, *be_cfg, *hw_cfg;
	int ret, i = 0;

	tplg_pp_debug("Building SectionBE for: '%s' ...", object->name);

	/* get top-level SectionControlMixer config */
	ret = snd_config_search(tplg_pp->cfg, "SectionBE", &top);
	if (ret < 0) {
		ret = snd_config_make_add(&top, "SectionBE",
					  SND_CONFIG_TYPE_COMPOUND, tplg_pp->cfg);
		if (ret < 0) {
			SNDERR("Error creating SectionBE config\n");
			return ret;
		}
	}

	/* create BE config */
	ret = tplg_pp_create_be_config(top, object->name);
	if (ret < 0) {
		SNDERR("Error creating BE config for %s\n", object->name);
		return ret;
	}

	be_cfg = tplg_find_config(top, object->name);
	if (!be_cfg) {
		SNDERR("Can't find BE config %s\n", object->name);
		return -EINVAL;
	}

	/* update BE config */
	list_for_each_entry(attr, &object->attribute_list, list) {

		ret = tplg_attribute_config_update(be_cfg, attr);
		if (ret < 0) {
			SNDERR("failed to add config for attribute %s in BE config %s\n",
			       attr->name, object->name);
			return ret;
		}
	}

	/* add hw_cfg */
	ret = snd_config_make_add(&hw_cfg, "hw_cfg", SND_CONFIG_TYPE_COMPOUND, be_cfg);
	if (ret < 0) {
		SNDERR("Error creating hw_cfg for %s\n", object->name);
		return ret;
	}

	/* add BE hw_cfg */
	list_for_each_entry(child, &object->object_list, list) {
		snd_config_t *childcfg;
		struct tplg_attribute *id;
		char hw_cfg_name[SNDRV_CTL_ELEM_ID_NAME_MAXLEN];
		char hw_cfg_id[SNDRV_CTL_ELEM_ID_NAME_MAXLEN];

		if (!child->cfg || strcmp(child->class_name, "hw_config"))
			continue;

		snprintf(hw_cfg_id, SNDRV_CTL_ELEM_ID_NAME_MAXLEN, "%d", i++);
		id = tplg_get_attribute_by_name(&child->attribute_list, "id");

		ret = snprintf(hw_cfg_name, SNDRV_CTL_ELEM_ID_NAME_MAXLEN, "%s.%ld",
			       object->name, id->value.integer);
		if (ret > SNDRV_CTL_ELEM_ID_NAME_MAXLEN) {
			SNDERR("hwcfg name too long\n");
			return ret;
		}

		/* add hw_cfg */
		ret = snd_config_make_add(&childcfg, hw_cfg_id, SND_CONFIG_TYPE_STRING, hw_cfg);
		if (ret < 0) {
			SNDERR("Error creating hw_cfg item for %s\n", object->name);
			return ret;
		}

		ret = snd_config_set_string(childcfg, hw_cfg_name);
		if (ret < 0) {
			SNDERR("Error creating hw_cfg item for %s\n", object->name);
			return ret;
		}
	}

	ret = tplg_pp_add_object_data(tplg_pp, object, be_cfg);
	if (ret < 0)
		SNDERR("Failed to add data section for be %s\n", object->name);

	return 0;
}

static int tplg_pp_create_pcm_config(snd_config_t *parent, char *name)
{
	snd_config_t *top, *child;
	int ret;

	ret = snd_config_make_add(&top, name, SND_CONFIG_TYPE_COMPOUND, parent);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "id", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "compress", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "symmertic_rates",
					  SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "symmetric_channels",
					  SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "symmetric_sample_bits",
					  SND_CONFIG_TYPE_INTEGER, top);

	return ret;
}

int tplg_build_pcm_object(struct tplg_pre_processor *tplg_pp,
			 struct tplg_object *object)
{
	struct tplg_attribute *name, *attr;
	struct tplg_object *child;
	snd_config_t *top, *pcm_cfg, *pcm;
	int ret;

	tplg_pp_debug("Building SectionPCM for: '%s' ...", object->name);

	/* get top-level SectionControlMixer config */
	ret = snd_config_search(tplg_pp->cfg, "SectionPCM", &top);
	if (ret < 0) {
		ret = snd_config_make_add(&top, "SectionPCM",
					  SND_CONFIG_TYPE_COMPOUND, tplg_pp->cfg);
		if (ret < 0) {
			SNDERR("Error creating SectionPCM config\n");
			return ret;
		}
	}

	name = tplg_get_attribute_by_name(&object->attribute_list, "name");

	/* create BE config */
	ret = tplg_pp_create_pcm_config(top, name->value.string);
	if (ret < 0) {
		SNDERR("Error creating BE config for %s\n", object->name);
		return ret;
	}

	pcm_cfg = tplg_find_config(top, name->value.string);
	if (!pcm_cfg) {
		SNDERR("Can't find BE config %s\n", object->name);
		return -EINVAL;
	}

	/* update BE config */
	list_for_each_entry(attr, &object->attribute_list, list) {

		ret = tplg_attribute_config_update(pcm_cfg, attr);
		if (ret < 0) {
			SNDERR("failed to add config for attribute %s in hwcfg %s\n",
			       attr->name, object->name);
			return ret;
		}
	}

	ret = snd_config_make_add(&pcm, "pcm", SND_CONFIG_TYPE_COMPOUND, pcm_cfg);
	if (ret < 0) {
		SNDERR("Error creating pcm config for %s\n", object->name);
		return ret;
	}

	/* add PCM fe_dai and caps */
	list_for_each_entry(child, &object->object_list, list) {
		if (!child->cfg)
			continue;

		if (!strcmp(child->class_name, "fe_dai")) {
			struct tplg_attribute *id, *name;
			snd_config_t *dai_cfg, *childcfg, *dai_name;

			ret = snd_config_make_add(&dai_cfg, "dai",
				  SND_CONFIG_TYPE_COMPOUND, pcm_cfg);
			if (ret < 0) {
				SNDERR("Error creating fe dai config for %s\n", object->name);
				return ret;
			}

			id = tplg_get_attribute_by_name(&child->attribute_list, "id");
			name = tplg_get_attribute_by_name(&child->attribute_list, "name");

			ret = snd_config_make_add(&dai_name, name->value.string,
						  SND_CONFIG_TYPE_COMPOUND, dai_cfg);
			if (ret < 0) {
				SNDERR("Error creating fe dai name for %s\n", object->name);
				return ret;
			}

			ret = snd_config_make_add(&childcfg, "id",
						  SND_CONFIG_TYPE_INTEGER, dai_name);
			if (ret < 0) {
				SNDERR("Error creating fe dai id for %s\n", object->name);
				return ret;
			}

			ret = snd_config_set_integer(childcfg, id->value.integer);
			if (ret < 0) {
				SNDERR("Error setting fe dai config for %s\n", object->name);
				return ret;
			}

		}

		if (!strcmp(child->class_name, "pcm_caps")) {
			struct tplg_attribute *caps, *dir;
			snd_config_t *dircfg, *childcfg;

			caps = tplg_get_attribute_by_name(&child->attribute_list, "capabilities");
			dir = tplg_get_attribute_by_name(&child->attribute_list, "direction");

			ret = snd_config_make_add(&dircfg, dir->value.string,
						  SND_CONFIG_TYPE_COMPOUND, pcm);
			if (ret < 0) {
				SNDERR("Error creating fe dai id for %s\n", object->name);
				return ret;
			}

			ret = snd_config_make_add(&childcfg, "capabilities",
						  SND_CONFIG_TYPE_STRING, dircfg);
			if (ret < 0) {
				SNDERR("Error creating capabilities cfg id for %s\n",
				       object->name);
				return ret;
			}

			ret = snd_config_set_string(childcfg, caps->value.string);
			if (ret < 0) {
				SNDERR("Error setting fe dai config for %s\n", object->name);
				return ret;
			}
		}
	}

	ret = tplg_pp_add_object_data(tplg_pp, object, pcm_cfg);
	if (ret < 0)
		SNDERR("Failed to add data section for PCM %s\n", object->name);

	return ret;
}

static int tplg_pp_create_pcm_caps_config(snd_config_t *parent, char *name)
{
	snd_config_t *top, *child;
	int ret;

	ret = snd_config_make_add(&top, name, SND_CONFIG_TYPE_COMPOUND, parent);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "formats", SND_CONFIG_TYPE_STRING, top);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "rates", SND_CONFIG_TYPE_STRING, top);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "rate_min", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "rate_max", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "channels_min", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "channels_max", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "periods_min", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "periods_max", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "period_size_min", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "period_size_max", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "buffer_size_min", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "buffer_size_max", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
		ret = snd_config_make_add(&child, "sig_bits", SND_CONFIG_TYPE_INTEGER, top);

	return ret;
}

int tplg_build_pcm_caps_object(struct tplg_pre_processor *tplg_pp,
			       struct tplg_object *object)
{
	struct tplg_attribute *attr, *capabilities;
	snd_config_t *top, *caps;
	int ret;

	tplg_pp_debug("Building SectionPCMCapabilities for: '%s' ...", object->name);

	/* get top-level SectionControlMixer config */
	ret = snd_config_search(tplg_pp->cfg, "SectionPCMCapabilities", &top);
	if (ret < 0) {
		ret = snd_config_make_add(&top, "SectionPCMCapabilities",
					  SND_CONFIG_TYPE_COMPOUND, tplg_pp->cfg);
		if (ret < 0) {
			SNDERR("Error creating SectionPCMCapabilities config\n");
			return ret;
		}
	}

	capabilities = tplg_get_attribute_by_name(&object->attribute_list, "capabilities");
	
	/* create pcm_caps config */
	ret = tplg_pp_create_pcm_caps_config(top, capabilities->value.string);
	if (ret < 0) {
		SNDERR("Error creating hw_cfg config for %s\n", object->name);
		return ret;
	}

	caps = tplg_find_config(top, capabilities->value.string);
	if (!caps) {
		SNDERR("Can't find pcm_caps config %s\n", object->name);
		return -EINVAL;
	}

	/* update pcm_caps config */
	list_for_each_entry(attr, &object->attribute_list, list) {

		ret = tplg_attribute_config_update(caps, attr);
		if (ret < 0) {
			SNDERR("failed to add config for attribute %s in pcm caps %s\n",
			       attr->name, object->name);
			return ret;
		}
	}

	return ret;
}
