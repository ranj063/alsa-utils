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

#include <alsa/sound/uapi/tlv.h>
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

static int tplg_add_control_config(struct tplg_object *object, snd_config_t *widget,
				   char *control_type)
{
	struct tplg_object *child;
	snd_config_t *control_top;
	int ret, i = 0;

	/* add control config */
	ret = snd_config_make_add(&control_top, control_type,
				  SND_CONFIG_TYPE_COMPOUND, widget);
	if (ret < 0) {
		SNDERR("Cant add %s config for widget %s\n", control_type, object->name);
		return ret;
	}

	/* add control names */
	list_for_each_entry(child, &object->object_list, list) {
		struct tplg_attribute *name;
		snd_config_t *control;
		char id[SNDRV_CTL_ELEM_ID_NAME_MAXLEN];

		if (strcmp(child->class_name, control_type))
			continue;

		/* skip if no name is provided */
		name = tplg_get_attribute_by_name(&child->attribute_list, "name");
		if (!strcmp(name->value.string, ""))
			continue;

		snprintf(id, SNDRV_CTL_ELEM_ID_NAME_MAXLEN, "%d", i++);

		/* add control name to config */
		ret = snd_config_make_add(&control, id, SND_CONFIG_TYPE_STRING, control_top);
		if (ret < 0)
			return ret;

		ret = snd_config_set_string(control, name->value.string);
		if (ret < 0)
			return ret;
	}

	return 0;
}

/* function to create the widget config node */
static int tplg_create_widget_config(snd_config_t *parent, char *name, int pipeline_id)
{
	snd_config_t *wtop, *child;
	int ret;

	ret = snd_config_make_add(&wtop, name, SND_CONFIG_TYPE_COMPOUND, parent);

	if (ret >= 0)
	ret = snd_config_make_add(&child, "index", SND_CONFIG_TYPE_INTEGER, wtop);

	if (ret >= 0)
	ret = snd_config_set_integer(child, pipeline_id);

	if (ret >= 0)
	ret = snd_config_make_add(&child, "type", SND_CONFIG_TYPE_STRING, wtop);

	if (ret >= 0)
	ret = snd_config_make_add(&child, "stream_name", SND_CONFIG_TYPE_STRING, wtop);

	if (ret >= 0)
	ret = snd_config_make_add(&child, "no_pm", SND_CONFIG_TYPE_INTEGER, wtop);

	if (ret >= 0)
	ret = snd_config_make_add(&child, "shift", SND_CONFIG_TYPE_INTEGER, wtop);

	if (ret >= 0)
	ret = snd_config_make_add(&child, "invert", SND_CONFIG_TYPE_INTEGER, wtop);

	if (ret >= 0)
	ret = snd_config_make_add(&child, "subseq", SND_CONFIG_TYPE_INTEGER, wtop);

	if (ret >= 0)
	ret = snd_config_make_add(&child, "event_type", SND_CONFIG_TYPE_INTEGER, wtop);

	if (ret >= 0)
	ret = snd_config_make_add(&child, "event_flags", SND_CONFIG_TYPE_INTEGER, wtop);

	return ret;
}

int tplg_pp_build_tlv_object(struct tplg_pre_processor *tplg_pp,
			     struct tplg_object *object)
{
	struct tplg_attribute *name;
	struct tplg_object *child;
	snd_config_t *top, *tlv;
	int ret;

	tplg_pp_debug("Building TLV Section for: '%s' ...", object->name);

	name = tplg_get_attribute_by_name(&object->attribute_list, "name");

	ret = snd_config_search(tplg_pp->cfg, "SectionTLV", &top);
	if (ret < 0) {
		ret = snd_config_make_add(&top, "SectionTLV",
					  SND_CONFIG_TYPE_COMPOUND, tplg_pp->cfg);
		if (ret < 0) {
			SNDERR("Error creating SectionTLV config\n");
			return ret;
		}
	}

	/* no need to do anything if TLV node exists already */
	tlv = tplg_find_config(top, name->value.string);
	if (tlv)
		return 0;

	ret = snd_config_make_add(&tlv, name->value.string, SND_CONFIG_TYPE_COMPOUND, top);
	if (ret < 0) {
		SNDERR("Error creating SectionTLV config for '%s\n", object->name);
		return ret;
	}

	/* parse the rest from child objects */
	list_for_each_entry(child, &object->object_list, list) {
		snd_config_t *scale;
		struct tplg_attribute *attr;

		if (strcmp(child->class_name, "scale"))
			continue;

		/* add scale config */
		ret = snd_config_make_add(&scale, "scale", SND_CONFIG_TYPE_COMPOUND, tlv);
		if (ret < 0) {
			SNDERR("Error creating TLV scale config for '%s'\n", object->name);
			return ret;
		}

		/* add config nodes for all scale attributes */
		list_for_each_entry(attr, &child->attribute_list, list) {
			snd_config_t *dst;
			const char *id;

			if (!strcmp(attr->name, "name") || !attr->cfg)
				continue;

			if (snd_config_get_id(attr->cfg, &id) < 0)
				continue;

			/* copy the attribute cfg */
			ret = snd_config_copy(&dst, attr->cfg);
			if (ret < 0) {
				SNDERR("Error copying scale config node %s for '%s'\n",
				       id, object->name);
				return ret;
			}

			ret = snd_config_add(scale, dst);
			if (ret < 0) {
				SNDERR("Error adding scale config node %s for %s\n",
				       id, object->name);
				return ret;
			}
		}
	}

	return 0;
}

/* create new channel config template */
static int tplg_pp_create_channel_config(snd_config_t *parent, char *name)
{
	snd_config_t *ctop, *child;
	int ret;

	ret = snd_config_make_add(&ctop, name, SND_CONFIG_TYPE_COMPOUND, parent);

	if (ret >= 0)
	ret = snd_config_make_add(&child, "reg", SND_CONFIG_TYPE_INTEGER, ctop);

	if (ret >= 0)
	ret = snd_config_make_add(&child, "shift", SND_CONFIG_TYPE_INTEGER, ctop);

	return ret;
}

static int tplg_build_mixer_channels(struct tplg_object *object,
				     snd_config_t *mixer_cfg)
{
	snd_config_t *channel;
	struct tplg_object *child;
	int ret;

	/* add channel config node */
	ret = snd_config_make_add(&channel, "channel", SND_CONFIG_TYPE_COMPOUND,
				  mixer_cfg);
	if (ret < 0) {
		SNDERR("Error creating channel config for %s\n", object->name);
		return ret;
	}

	list_for_each_entry(child, &object->object_list, list) {
		struct tplg_attribute *attr;
		snd_config_t *ctop;
		
		if (!child->cfg || strcmp(child->class_name, "channel"))
			continue;

		/* create new channel config template */
		attr = tplg_get_attribute_by_name(&child->attribute_list, "name");
		ret = tplg_pp_create_channel_config(channel, attr->value.string);
		if (ret < 0) {
			SNDERR("Failed to create channel config %s for %s\n",
			       attr->value.string, object->name);
		}

		ctop = tplg_find_config(channel, attr->value.string);
		if (!ctop) {
			SNDERR("Can't find channel config %s for %s\n",
			       attr->value.string, object->name);
			return -ENOENT;
		}

		/* update the reg/shift values in the channel */
		list_for_each_entry(attr, &child->attribute_list, list) {
			ret = tplg_attribute_config_update(ctop, attr);
			if (ret < 0) {
				SNDERR("failed to add config for attribute %s in channel %s\n",
				       attr->name, object->name);
				return ret;
			}
		}
	}

	return 0;
}

static int tplg_build_mixer_control_child_objects(struct tplg_object *object,
						  snd_config_t *mixer_cfg)
{
	snd_config_t *ops, *dst, *child_cfg;
	struct tplg_object *child;
	int ret;

	/* add ops config node */
	ret = snd_config_make_add(&ops, "ops", SND_CONFIG_TYPE_COMPOUND,
				  mixer_cfg);
	if (ret < 0) {
		SNDERR("Error creating ops config for %s\n", object->name);
		return ret;
	}

	/* parse ops, tlv and channel from child objects */
	list_for_each_entry(child, &object->object_list, list) {
		const char *id;

		if (!child->cfg)
			continue;

		if (snd_config_get_id(child->cfg, &id) < 0)
				continue;

		/* copy ops node */
		if (!strcmp(child->class_name, "ops")) {
			ret = snd_config_copy(&dst, child->cfg);
			if (ret < 0) {
				SNDERR("Error copying ops node %s for '%s'\n", id, object->name);
				return ret;
			}

			ret = snd_config_add(ops, dst);
			if (ret < 0) {
				SNDERR("Error adding ops node %s for %s\n", id, object->name);
				return ret;
			}
			continue;
		}

		/* add and set tlv node */
		if (!strcmp(child->class_name, "tlv")) {
			struct tplg_attribute *child_name;

			child_name = tplg_get_attribute_by_name(&child->attribute_list, "name");
			ret = snd_config_make_add(&child_cfg, "tlv",
						  SND_CONFIG_TYPE_STRING, mixer_cfg);
			if (ret < 0) {
				SNDERR("Error creating tlv config for %s\n",
				       object->name);
				return ret;
			}

			ret = snd_config_set_string(child_cfg, child_name->value.string);
			if (ret < 0) {
				SNDERR("Error setting tlv config for %s\n", object->name);
				return ret;
			}
		}
	}

	return tplg_build_mixer_channels(object, mixer_cfg);
}

/* create new mixer config template */
static int tplg_pp_create_mixer_config(snd_config_t *parent, char *name, int pipeline_id)
{
	snd_config_t *top, *child;
	int ret;

	ret = snd_config_make_add(&top, name, SND_CONFIG_TYPE_COMPOUND, parent);

	if (ret >= 0)
	ret = snd_config_make_add(&child, "index", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
	ret = snd_config_set_integer(child, pipeline_id);

	if (ret >= 0)
	ret = snd_config_make_add(&child, "max", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
	ret = snd_config_make_add(&child, "invert", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
	ret = snd_config_make_add(&child, "access", SND_CONFIG_TYPE_COMPOUND, top);

	return ret;
}


int tplg_build_mixer_control(struct tplg_pre_processor *tplg_pp, struct tplg_object *object)
{
	struct tplg_attribute *attr, *name, *pipeline_id;
	snd_config_t *top, *mixer_cfg;
	int ret;

	name = tplg_get_attribute_by_name(&object->attribute_list, "name");

	/* skip mixers with no names */
	if (!strcmp(name->value.string, ""))
		return 0;

	tplg_pp_debug("Building Mixer Control object: '%s' ...", object->name);

	pipeline_id = tplg_get_attribute_by_name(&object->attribute_list, "pipeline_id");

	/* get top-level SectionControlMixer config */
	ret = snd_config_search(tplg_pp->cfg, "SectionControlMixer", &top);
	if (ret < 0) {
		ret = snd_config_make_add(&top, "SectionControlMixer",
					  SND_CONFIG_TYPE_COMPOUND, tplg_pp->cfg);
		if (ret < 0) {
			SNDERR("Error creating SectionControlMixer config\n");
			return ret;
		}
	}

	/* create mixer config */
	ret = tplg_pp_create_mixer_config(top, name->value.string, pipeline_id->value.integer);
	if (ret < 0) {
		SNDERR("Error creating mixer config for %s\n", object->name);
		return ret;
	}

	mixer_cfg = tplg_find_config(top, name->value.string);
	if (!mixer_cfg) {
		SNDERR("Can't find mixer config %s\n", object->name);
		return -EINVAL;
	}

	/* update mixer config */
	list_for_each_entry(attr, &object->attribute_list, list) {

		/* don't update index */
		if (!strcmp(attr->name, "index"))
			continue;

		ret = tplg_attribute_config_update(mixer_cfg, attr);
		if (ret < 0) {
			SNDERR("failed to add config for attribute %s in mixer %s\n",
			       attr->name, object->name);
			return ret;
		}
	}

	return tplg_build_mixer_control_child_objects(object, mixer_cfg);
}

/* create new mixer config template */
static int tplg_pp_create_bytes_config(snd_config_t *parent, char *name, int pipeline_id)
{
	snd_config_t *top, *child;
	int ret;

	ret = snd_config_make_add(&top, name, SND_CONFIG_TYPE_COMPOUND, parent);

	if (ret >= 0)
	ret = snd_config_make_add(&child, "index", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
	ret = snd_config_set_integer(child, pipeline_id);

	if (ret >= 0)
	ret = snd_config_make_add(&child, "base", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
	ret = snd_config_make_add(&child, "num_regs", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
	ret = snd_config_make_add(&child, "max", SND_CONFIG_TYPE_INTEGER, top);

	if (ret >= 0)
	ret = snd_config_make_add(&child, "mask", SND_CONFIG_TYPE_COMPOUND, top);

	return ret;
}

static int tplg_build_bytes_control_child_objects(struct tplg_object *object,
						  snd_config_t *bytes_cfg)
{
	snd_config_t *ops, *ext_ops, *data, *dst, *child_cfg;
	struct tplg_object *child;
	int ret, i = 0;

	/* add ops config node */
	ret = snd_config_make_add(&ops, "ops", SND_CONFIG_TYPE_COMPOUND,
				  bytes_cfg);
	if (ret < 0) {
		SNDERR("Error creating ops config for %s\n", object->name);
		return ret;
	}

	/* add ext_ops config node */
	ret = snd_config_make_add(&ext_ops, "ext_ops", SND_CONFIG_TYPE_COMPOUND,
				  bytes_cfg);
	if (ret < 0) {
		SNDERR("Error creating ext_ops config for %s\n", object->name);
		return ret;
	}

	/* add data config node */
	ret = snd_config_make_add(&data, "data", SND_CONFIG_TYPE_COMPOUND,
				  bytes_cfg);
	if (ret < 0) {
		SNDERR("Error creating data config for %s\n", object->name);
		return ret;
	}

	/* parse ops, ext_ops and data from child objects */
	list_for_each_entry(child, &object->object_list, list) {
		const char *id;

		if (!child->cfg)
			continue;

		if (snd_config_get_id(child->cfg, &id) < 0)
				continue;

		/* copy ops node */
		if (!strcmp(child->class_name, "ops") ||
		    !strcmp(child->class_name, "ext_ops")) {
			ret = snd_config_copy(&dst, child->cfg);
			if (ret < 0) {
				SNDERR("Error copying ops node %s for '%s'\n", id, object->name);
				return ret;
			}

			if (!strcmp(child->class_name, "ops"))
				ret = snd_config_add(ops, dst);
			else
				ret = snd_config_add(ext_ops, dst);
			if (ret < 0) {
				SNDERR("Error adding ops node %s for %s\n", id, object->name);
				return ret;
			}
			continue;
		}

		/* add and set data references */
		if (!strcmp(child->class_name, "data")) {
			struct tplg_attribute *child_name;
			char data_id[SNDRV_CTL_ELEM_ID_NAME_MAXLEN];

			snprintf(data_id, SNDRV_CTL_ELEM_ID_NAME_MAXLEN, "%d", i++);
			ret = snd_config_make_add(&child_cfg, data_id,
						  SND_CONFIG_TYPE_STRING, bytes_cfg);
			if (ret < 0) {
				SNDERR("Error creating data reference config for %s\n",
				       object->name);
				return ret;
			}

			child_name = tplg_get_attribute_by_name(&child->attribute_list, "name");
			ret = snd_config_set_string(child_cfg, child_name->value.string);
			if (ret < 0) {
				SNDERR("Error setting tlv config for %s\n", object->name);
				return ret;
			}
		}
	}

	return 0;
}

int tplg_build_bytes_control(struct tplg_pre_processor *tplg_pp, struct tplg_object *object)
{
	struct tplg_attribute *attr, *name, *pipeline_id;
	snd_config_t *top, *bytes_cfg;
	int ret;

	name = tplg_get_attribute_by_name(&object->attribute_list, "name");

	/* skip byte controls with no names */
	if (!strcmp(name->value.string, ""))
		return 0;

	tplg_pp_debug("Building Bytes Control object: '%s' ...", object->name);

	pipeline_id = tplg_get_attribute_by_name(&object->attribute_list, "pipeline_id");

	/* get top-level SectionControlMixer config */
	ret = snd_config_search(tplg_pp->cfg, "SectionControlBytes", &top);
	if (ret < 0) {
		ret = snd_config_make_add(&top, "SectionControlBytes",
					  SND_CONFIG_TYPE_COMPOUND, tplg_pp->cfg);
		if (ret < 0) {
			SNDERR("Error creating SectionControlBytes config\n");
			return ret;
		}
	}

	/* create bytes config */
	ret = tplg_pp_create_bytes_config(top, name->value.string, pipeline_id->value.integer);
	if (ret < 0) {
		SNDERR("Error creating bytes config for %s\n", object->name);
		return ret;
	}

	bytes_cfg = tplg_find_config(top, name->value.string);
	if (!bytes_cfg) {
		SNDERR("Can't find bytes config %s\n", object->name);
		return -EINVAL;
	}

	/* update bytes config */
	list_for_each_entry(attr, &object->attribute_list, list) {

		/* don't update index */
		if (!strcmp(attr->name, "index"))
			continue;

		ret = tplg_attribute_config_update(bytes_cfg, attr);
		if (ret < 0) {
			SNDERR("failed to add config for attribute %s in bytes %s\n",
			       attr->name, object->name);
			return ret;
		}
	}
	return tplg_build_bytes_control_child_objects(object, bytes_cfg);
}

int tplg_build_widget_object(struct tplg_pre_processor *tplg_pp, struct tplg_object *object)
{
	struct tplg_attribute *pipeline_id, *attr, *name;
	snd_config_t *top, *wtop;
	char object_name[SNDRV_CTL_ELEM_ID_NAME_MAXLEN];
	int ret;

	tplg_pp_debug("Building DAPM widget object: '%s' ...", object->name);

	ret = snd_config_search(tplg_pp->cfg, "SectionWidget", &top);
	if (ret < 0) {
		ret = snd_config_make_add(&top, "SectionWidget", SND_CONFIG_TYPE_COMPOUND, tplg_pp->cfg);
		if (ret < 0) {
			SNDERR("Error creating 'SectionWidget' config\n");
			return ret;
		}
	}

	name = tplg_get_attribute_by_name(&object->attribute_list, "name");
	pipeline_id = tplg_get_attribute_by_name(&object->attribute_list, "pipeline_id");

	if (name)
		snd_strlcpy(object_name, name->value.string, SNDRV_CTL_ELEM_ID_NAME_MAXLEN);
	else
		snd_strlcpy(object_name, object->name, SNDRV_CTL_ELEM_ID_NAME_MAXLEN);

	/* create widget config */
	ret = tplg_create_widget_config(top, object_name, pipeline_id->value.integer);
	if (ret < 0) {
		SNDERR("Failed to create widget config for %s\n", object->name);
		return ret;
	}

	wtop = tplg_find_config(top, object_name);
	if (!wtop) {
		SNDERR("Can't find widget config for %s\n", object->name);
		return -ENOENT;
	}

	/* update widget params from attributes */
	list_for_each_entry(attr, &object->attribute_list, list) {
		if (!attr->found || !strcmp(attr->name, "index"))
			continue;

		ret = tplg_attribute_config_update(wtop, attr);
		if (ret < 0) {
			SNDERR("failed to add config for attribute %s in widget %s\n",
			       attr->name, object->name);
			return ret;
		}
	}

	/* Add widget control config */
	ret = tplg_add_control_config(object, wtop, "mixer");
	if (ret < 0)
		return ret;

	ret = tplg_add_control_config(object, wtop, "bytes");
	if (ret < 0)
		return ret;

	ret = tplg_add_control_config(object, wtop, "enum");
	if (ret < 0)
		return ret;

	ret = tplg_pp_add_object_data(tplg_pp, object, wtop);
	if (ret < 0)
		SNDERR("Failed to add data section for widget %s\n", object->name);

	tplg_pp_config_debug(tplg_pp, top);

	return ret;
}
