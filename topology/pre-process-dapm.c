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

static int tplg_pp_parse_channel(struct snd_soc_tplg_mixer_control *mc,
				 struct tplg_object *object)
{
	struct snd_soc_tplg_channel *channel = mc->channel;
	struct list_head *pos;
	char *channel_name = strchr(object->name, '.') + 1;
	int channel_id = snd_tplg_lookup_channel(channel_name);

	if (channel_id < 0) {
		SNDERR("invalid channel %d for mixer %s", channel_id, mc->hdr.name);
		return -EINVAL;
	}

	channel += mc->num_channels;

	channel->id = channel_id;
	channel->size = sizeof(*channel);
	list_for_each(pos, &object->attribute_list) {
		struct tplg_attribute *attr = list_entry(pos, struct tplg_attribute, list);

		if (!strcmp(attr->name, "reg"))
			channel->reg = attr->value.integer;


		if (!strcmp(attr->name, "shift"))
			channel->shift = attr->value.integer;
	}

	mc->num_channels++;
	if (mc->num_channels >= SND_SOC_TPLG_MAX_CHAN) {
		SNDERR("Max channels exceeded for %s\n", mc->hdr.name);
		return -EINVAL;
	}

	tplg_pp_debug("channel: %s id: %d reg:%d shift %d", channel_name, channel->id,
		      channel->reg, channel->shift);

	return 0;
}

static int tplg_pp_parse_tlv(struct tplg_pre_processor *tplg_pp, struct tplg_object *object)
{
	struct snd_soc_tplg_ctl_tlv *tplg_tlv;
	struct snd_soc_tplg_tlv_dbscale *scale;
	struct tplg_attribute *attr, *name;
	struct list_head *pos;
	int ret;

	name = tplg_get_attribute_by_name(&object->attribute_list, "name");

	tplg_tlv = calloc(1, sizeof(*tplg_tlv));
	if (!tplg_tlv)
		return -ENOMEM;

	tplg_tlv->size = sizeof(*tplg_tlv);
	tplg_tlv->type = SNDRV_CTL_TLVT_DB_SCALE;
	scale = &tplg_tlv->scale;

	list_for_each(pos, &object->object_list) {
		struct tplg_object *child = list_entry(pos, struct tplg_object, list);

		if (!strcmp(child->class_name, "scale")) {
			list_for_each_entry(attr, &child->attribute_list, list) {
				if (!attr->cfg)
					continue;

				ret = snd_soc_tplg_parse_tlv_dbscale_param(attr->cfg, scale);
				if (ret < 0) {
					SNDERR("failed to DBScale for tlv %s", object->name);
					goto err;;
				}
			}
			break;
		}
	}

	tplg_pp_debug("TLV: %s scale min: %d step %d mute %d", object->name, scale->min,
		      scale->step, scale->mute);

	ret = snd_soc_tplg_save_tlv(tplg_tlv, &tplg_pp->buf, name->value.string, "");
	if (ret < 0)
		SNDERR("failed to save %s", object->name);

err:
	free(tplg_tlv);
	return ret;
}

int tplg_pp_build_tlv_object(struct tplg_pre_processor *tplg_pp,
			     struct tplg_object *object)
{
	struct tplg_attribute *name;
	int ret;

	tplg_pp_debug("Building TLV Section for: '%s' ...", object->name);

	name = tplg_get_attribute_by_name(&object->attribute_list, "name");
	/* save the SectionControlMixer and its fields */
	ret = snd_tplg_save_printf(&tplg_pp->buf, "", "SectionTLV.", name->value.string);
	if (ret < 0)
		return ret;

	ret = tplg_pp_parse_tlv(tplg_pp, object);
	if (ret < 0) {
		SNDERR("Error parsing tlv for mixer %s\n", object->name);
		return ret;
	}

	ret = snd_tplg_save_printf(&tplg_pp->buf, "", "\n");
	if (ret < 0)
		return ret;

	print_pre_processed_config(tplg_pp);

	return 0;
}

static int tplg_pp_save_control_mixer(struct tplg_pre_processor *tplg_pp,
				   struct tplg_object *object,
				   struct snd_soc_tplg_mixer_control *mc, int index)
{
	struct tplg_object *child;
	bool channels_saved = false;
	int ret;

	/* save the SectionControlMixer and its fields */
	ret = snd_tplg_save_printf(&tplg_pp->buf, "", "SectionControlMixer.");
	if (ret < 0)
		return ret;

	ret =  snd_soc_tplg_save_control_mixer_params(&tplg_pp->buf, mc, mc->hdr.name, index, "");
	if (ret < 0)
		return ret;

	/* parse the rest from child objects */
	list_for_each_entry(child, &object->object_list, list) {
		if (!child->cfg)
			continue;

		/* save ops */
		if (!strcmp(child->class_name, "ops")) {
			ret = snd_soc_tplg_save_ops(NULL, &mc->hdr, &tplg_pp->buf, "\t");
			if (ret < 0) {
				SNDERR("Error saving ops for mixer %s\n", object->name);
				return ret;
			}
			continue;
		}

		/* save tlv */
		if (!strcmp(child->class_name, "tlv")) {
			struct tplg_attribute *name;

			name = tplg_get_attribute_by_name(&child->attribute_list, "name");
			ret = snd_tplg_save_printf(&tplg_pp->buf, "", "\ttlv\t\"%s\"\n",
						    name->value.string);
			if (ret < 0)
				return ret;
			continue;
		}

		/* save channels */
		if (!strcmp(child->class_name, "channel")) {
			if (channels_saved)
				continue;
			ret = snd_soc_tplg_save_channels(NULL, mc->channel, mc->num_channels,
							 &tplg_pp->buf, "\t");
			if (ret < 0) {
				SNDERR("Error saving channels for mixer %s\n", object->name);
				return ret;
			}
			channels_saved = true;
		}

	}

	ret = snd_soc_tplg_save_access(NULL, &mc->hdr, &tplg_pp->buf, "\t");
	if (ret < 0) {
		SNDERR("Error saving access for mixer %s\n", object->name);
		return ret;
	}

	ret = snd_tplg_save_printf(&tplg_pp->buf, "", "}\n\n");
	if (ret < 0)
		return ret;

	print_pre_processed_config(tplg_pp);

	return 0;
}

int tplg_build_mixer_control(struct tplg_pre_processor *tplg_pp, struct tplg_object *object)
{
	struct snd_soc_tplg_mixer_control *mc;
	struct snd_soc_tplg_ctl_hdr *hdr;
	struct tplg_attribute *attr, *name, *pipeline_id;
	struct tplg_object *child;
	bool access_set = false, tlv_set = false;
	int j, ret;

	name = tplg_get_attribute_by_name(&object->attribute_list, "name");

	/* skip mixers with no names */
	if (!strcmp(name->value.string, ""))
		return 0;

	tplg_pp_debug("Building Mixer Control object: '%s' ...", object->name);

	pipeline_id = tplg_get_attribute_by_name(&object->attribute_list, "pipeline_id");

	/* init new mixer */
	mc = calloc(1, sizeof(*mc));
	if (!mc)
		return -ENOMEM;

	snd_strlcpy(mc->hdr.name, name->value.string, SNDRV_CTL_ELEM_ID_NAME_MAXLEN);
	mc->hdr.type = SND_SOC_TPLG_TYPE_MIXER;
	mc->size = sizeof(*mc);
	hdr = &mc->hdr;

	/* set channel reg to default state */
	for (j = 0; j < SND_SOC_TPLG_MAX_CHAN; j++)
		mc->channel[j].reg = -1;

	/* parse some control params from attributes */
	list_for_each_entry(attr, &object->attribute_list, list) {
		if (!attr->cfg)
			continue;

		ret = snd_soc_tplg_parse_control_mixer_param(attr->cfg, mc);
		if (ret < 0) {
			SNDERR("Error parsing mixer control params for %s\n", object->name);
			return ret;
		}

		if (!strcmp(attr->name, "access")) {
			ret = snd_soc_tplg_parse_access_values(attr->cfg, &mc->hdr);
			if (ret < 0) {
				SNDERR("Error parsing access attribute for %s\n", object->name);
				goto err;
			}
			access_set = true;
		}

	}

	/* parse ops and channel from child objects */
	list_for_each_entry(child, &object->object_list, list) {
		if (!child->cfg)
			continue;

		if (!strcmp(child->class_name, "ops")) {
			ret = snd_soc_tplg_parse_ops(NULL, child->cfg, hdr);
			if (ret < 0) {
				SNDERR("Error parsing ops for mixer %s\n", object->name);
				goto err;
			}
			continue;
		}

		if (!strcmp(child->class_name, "tlv")) {
			tlv_set = true;
			continue;
		}

		if (!strcmp(child->class_name, "channel")) {
			ret = tplg_pp_parse_channel(mc, child);
			if (ret < 0) {
				SNDERR("Error parsing channel %d for mixer %s\n", child->name,
				       object->name);
				goto err;
			}
			continue;
		}

	}
	tplg_pp_debug("Mixer: %s, num_channels: %d", name->value.string, mc->num_channels);
	tplg_pp_debug("Ops info: %d get: %d put: %d max: %d", hdr->ops.info, hdr->ops.get, hdr->ops.put, mc->max);

	/* set CTL access to default values if none provided */
	if (!access_set) {
		mc->hdr.access = SNDRV_CTL_ELEM_ACCESS_READWRITE;
		if (tlv_set)
			mc->hdr.access |= SNDRV_CTL_ELEM_ACCESS_TLV_READ;
	}

	/* Build complete. Now save to tplg_pp->buf */
	ret = tplg_pp_save_control_mixer(tplg_pp, object, mc, pipeline_id->value.integer);
	if (ret < 0)
		SNDERR("Failed to save control mixer %s\n", object->name);

err:
	free(mc);
	return ret;
}

static int tplg_pp_save_control_bytes(struct tplg_pre_processor *tplg_pp,
				      struct tplg_object *object,
				      struct snd_soc_tplg_bytes_control *be, int index)
{
	struct tplg_object *child;
	int ret;

	/* save the SectionControlMixer and its fields */
	ret = snd_tplg_save_printf(&tplg_pp->buf, "", "SectionControlBytes.");
	if (ret < 0)
		return ret;

	ret =  snd_soc_tplg_save_control_bytes_params(&tplg_pp->buf, be, be->hdr.name, index, "");
	if (ret < 0)
		return ret;

	/* parse the rest from child objects */
	list_for_each_entry(child, &object->object_list, list) {
		if (!object->cfg)
			continue;

		/* save ops */
		if (!strcmp(child->class_name, "ops")) {
			ret = snd_soc_tplg_save_ops(NULL, &be->hdr, &tplg_pp->buf, "\t");
			if (ret < 0) {
				SNDERR("Error saving ops for mixer %s\n", object->name);
				return ret;
			}
			continue;
		}

		/* save ext ops */
		if (!strcmp(child->class_name, "ext_ops")) {
			ret = snd_soc_tplg_save_ext_ops(NULL, be, &tplg_pp->buf, "\t");
			if (ret < 0) {
				SNDERR("Error saving ops for mixer %s\n", object->name);
				return ret;
			}
			continue;
		}

		/* save tlv */
		if (!strcmp(child->class_name, "tlv")) {
			struct tplg_attribute *name;

			name = tplg_get_attribute_by_name(&child->attribute_list, "name");
			ret = snd_tplg_save_printf(&tplg_pp->buf, "", "\ttlv\t\"%s\"\n",
						    name->value.string);
			if (ret < 0)
				return ret;
			continue;
		}
	}

	ret = snd_soc_tplg_save_access(NULL, &be->hdr, &tplg_pp->buf, "\t");
	if (ret < 0) {
		SNDERR("Error saving access for bytes control %s\n", object->name);
		return ret;
	}

	/* Add data references */
	ret = snd_tplg_save_printf(&tplg_pp->buf, "", "\tdata[\n");
	if (ret < 0)
		return ret;
	list_for_each_entry(child, &object->object_list, list) {
		if (!object->cfg)
			continue;

		if (!strcmp(child->class_name, "data")) {
			struct tplg_attribute *name;

			name = tplg_get_attribute_by_name(&child->attribute_list, "name");
			ret = snd_tplg_save_printf(&tplg_pp->buf, "", "\t\t\"%s\"\n",
						    name->value.string);
			if (ret < 0)
				return ret;
		}
	}

	ret = snd_tplg_save_printf(&tplg_pp->buf, "", "\t]\n}\n\n");
	if (ret < 0)
		return ret;

	print_pre_processed_config(tplg_pp);

	return 0;
}

int tplg_build_bytes_control(struct tplg_pre_processor *tplg_pp, struct tplg_object *object)
{
	struct snd_soc_tplg_bytes_control *be;
	struct snd_soc_tplg_ctl_hdr *hdr;
	struct tplg_attribute *attr, *name, *pipeline_id;
	struct tplg_object *child;
	bool access_set = false;
	int ret;

	name = tplg_get_attribute_by_name(&object->attribute_list, "name");

	/* skip byte controls with no names */
	if (!strcmp(name->value.string, ""))
		return 0;

	tplg_pp_debug("Building Bytes Control object: '%s' ...", object->name);

	pipeline_id = tplg_get_attribute_by_name(&object->attribute_list, "pipeline_id");

	/* init new byte control */
	be = calloc(1, sizeof(*be));
	if (!be)
		return -ENOMEM;

	snd_strlcpy(be->hdr.name, name->value.string, SNDRV_CTL_ELEM_ID_NAME_MAXLEN);
	be->hdr.type = SND_SOC_TPLG_TYPE_BYTES;
	be->size = sizeof(*be);
	hdr = &be->hdr;

	/* parse some control params from attributes */
	list_for_each_entry(attr, &object->attribute_list, list) {
		if (!attr->cfg)
			continue;

		ret = snd_soc_tplg_parse_control_bytes_param(attr->cfg, be);
		if (ret < 0) {
			SNDERR("Error parsing control bytes params for %s\n", object->name);
			goto err;
		}

		if (!strcmp(attr->name, "access")) {
			ret = snd_soc_tplg_parse_access_values(attr->cfg, &be->hdr);
			if (ret < 0) {
				SNDERR("Error parsing access attribute for %s\n", object->name);
				goto err;
			}
			access_set = true;
		}
	}

	/* parse the rest from child objects */
	list_for_each_entry(child, &object->object_list, list) {
		if (!child->cfg)
			continue;

		if (!strcmp(child->class_name, "ops")) {
			ret = snd_soc_tplg_parse_ops(NULL, child->cfg, hdr);
			if (ret < 0) {
				SNDERR("Error parsing ops for mixer %s\n", object->name);
				goto err;
			}
			continue;
		}

		if (!strcmp(child->class_name, "extops")) {
			ret = snd_soc_tplg_parse_ext_ops(NULL, child->cfg, hdr);
			if (ret < 0) {
				SNDERR("Error parsing ext ops for bytes %s\n", object->name);
				goto err;
			}
			continue;
		}
	}

	tplg_pp_debug("Bytes: %s Ops info: %d get: %d put: %d", hdr->name, hdr->ops.info,
		      hdr->ops.get, hdr->ops.put);
	tplg_pp_debug("Ext Ops info: %d get: %d put: %d", be->ext_ops.info, be->ext_ops.get,
		      be->ext_ops.put);

	/* set CTL access to default values if none provided */
	if (!access_set)
		be->hdr.access = SNDRV_CTL_ELEM_ACCESS_READWRITE;

	/* Build complete. Now save to tplg_pp->buf */
	ret = tplg_pp_save_control_bytes(tplg_pp, object, be, pipeline_id->value.integer);
	if (ret < 0)
		SNDERR("Failed to save control bytes %s\n", be->hdr.name);

err:
	free(be);
	return ret;
}

static int tplg_add_object_controls(struct tplg_pre_processor *tplg_pp,
				    struct tplg_object *object)
{
	struct tplg_object *child;
	int ret;

	/* save the SectionWidget and its fields */
	ret = snd_tplg_save_printf(&tplg_pp->buf, "", "\tmixer [\n");
	if (ret < 0)
		return ret;

	/* add mixer controls */
	list_for_each_entry(child, &object->object_list, list) {
		struct tplg_attribute *name_attr;

		/* skip if no name is provided */
		name_attr = tplg_get_attribute_by_name(&child->attribute_list,
							"name");

		if (!name_attr || !strcmp(name_attr->value.string, ""))
			continue;

		if (!strcmp(child->class_name, "mixer")) {
			ret = snd_tplg_save_printf(&tplg_pp->buf, "", "\t\t\"%s\"\n",
						   name_attr->value.string);
			if (ret < 0)
				return ret;
		}
	}

	ret = snd_tplg_save_printf(&tplg_pp->buf, "", "\t]\n");
	if (ret < 0)
		return ret;
	/* add mixer controls */

	/* save the bytes references */
	ret = snd_tplg_save_printf(&tplg_pp->buf, "", "\tbytes [\n");
	if (ret < 0)
		return ret;
	list_for_each_entry(child, &object->object_list, list) {
		struct tplg_attribute *name_attr;

		/* skip if no name is provided */
		name_attr = tplg_get_attribute_by_name(&child->attribute_list,
							"name");

		if (!name_attr || !strcmp(name_attr->value.string, ""))
			continue;

		if (!strcmp(child->class_name, "bytes")) {
			ret = snd_tplg_save_printf(&tplg_pp->buf, "", "\t\t\"%s\"\n",
						   name_attr->value.string);
			if (ret < 0)
				return ret;
		}
	}

	return snd_tplg_save_printf(&tplg_pp->buf, "", "\t]\n");
}

int tplg_build_widget_object(struct tplg_pre_processor *tplg_pp, struct tplg_object *object)
{
	struct tplg_attribute *pipeline_id, *attr, *name;
	struct snd_soc_tplg_dapm_widget *widget;
	int ret;

	tplg_pp_debug("Building DAPM widget object: '%s' ...", object->name);

	widget = calloc(1, sizeof(*widget));
	if (!widget)
		return -ENOMEM;

	name = tplg_get_attribute_by_name(&object->attribute_list, "name");
	if (name)
		snd_strlcpy(widget->name, name->value.string, SNDRV_CTL_ELEM_ID_NAME_MAXLEN);
	else
		snd_strlcpy(widget->name, object->name, SNDRV_CTL_ELEM_ID_NAME_MAXLEN);

	widget->size = sizeof(*widget);

	pipeline_id = tplg_get_attribute_by_name(&object->attribute_list, "pipeline_id");

	/* parse widget params from attributes */
	list_for_each_entry(attr, &object->attribute_list, list) {
		if (!attr->cfg)
			continue;

		ret = snd_soc_tplg_parse_dapm_widget_param(attr->cfg, widget);
		if (ret < 0) {
			SNDERR("Error parsing widget params for %s\n", object->name);
			goto err;
		}
	}

	/* save the SectionWidget and its fields */
	ret = snd_tplg_save_printf(&tplg_pp->buf, "", "SectionWidget.");
	if (ret < 0)
		goto err;

	ret = snd_soc_tplg_save_dapm_widget_param(widget, &tplg_pp->buf, widget->name,
						  pipeline_id->value.integer, "");
	if (ret < 0)
		SNDERR("Failed to save DAPM widget %s\n", widget->name);

	ret = tplg_add_object_controls(tplg_pp, object);
	if (ret < 0) {
		SNDERR("Failed to add controls section for widget %s\n", widget->name);
		return ret;
	}

	print_pre_processed_config(tplg_pp);

	ret = tplg_pp_add_object_data(tplg_pp, object);
	if (ret < 0)
		SNDERR("Failed to add data section for widget %s\n", widget->name);

err:
	free(widget);
	return ret;
}

/*
 * Widget names for pipeline endpoints can be of the following type:
 * "Object.class.index" which refers to an object of class "class" with index in the
 * parent object_list or the global topology object_list
 */
static char *tplg_pp_get_widget_name(struct tplg_pre_processor *tplg_pp,
				     struct tplg_object *object,
				     char *string)
{
	struct tplg_object *child;
	struct list_head *list;
	char *object_str, *last_dot;
	char class_name[SNDRV_CTL_ELEM_ID_NAME_MAXLEN], *index_str;

	/* strip "Object." from the string */
	object_str = strchr(string, '.');
	if (!object_str) {
		SNDERR("Incomplete widget name '%s'\n", string);
		return NULL;
	}

	/* get last occurence of '.' */
	last_dot = strrchr(string, '.');

	/* get index of object */
	index_str = strchr(object_str + 1, '.');
	if (!index_str) {
		SNDERR("No unique attribute for widget_name %s\n",
		       string);
		return NULL;
	}

	/* get class name */
	snd_strlcpy(class_name, object_str + 1, strlen(object_str) - strlen(index_str));

	/*
	 * look up widget from parent object_list or the global object list if
	 * this is a route object. For all other objects, search for widget in its object_list.
	 */
	if (!strcmp(object->class_name, "route")) {
		if (object->parent)
			list = &object->parent->object_list;
		else
			list = &tplg_pp->object_list;
	} else {
		list = &object->object_list;
	}

	child = tplg_object_lookup_in_list(list, class_name, index_str + 1);
	if (!child) {
		SNDERR("Widget %s%s not found \n", class_name, index_str);
		return NULL;
	}

	/* end of string? */
	if (last_dot != index_str) {
		char *str = strchr(index_str + 1, '.');
		char new_str[SNDRV_CTL_ELEM_ID_NAME_MAXLEN];

		snprintf(new_str, SNDRV_CTL_ELEM_ID_NAME_MAXLEN, "%s%s", "Object", str);

		return tplg_pp_get_widget_name(tplg_pp, child, new_str);
	}

	return child->name;
}

int tplg_build_dapm_route_object(struct tplg_pre_processor *tplg_pp,
				 struct tplg_object *object)
{
	struct snd_soc_tplg_dapm_graph_elem *line;
	struct tplg_attribute *attr, *pipeline_id, *index;
	int ret = 0;

	line = calloc(1, sizeof(*line));
	if (!line)
		return -ENOMEM;

	tplg_pp_debug("Building DAPM route object: '%s' ...", object->name);

	pipeline_id = tplg_get_attribute_by_name(&object->attribute_list, "pipeline_id");
	index = tplg_get_attribute_by_name(&object->attribute_list, "index");

	/* Parse connection object and get widget names for source and sink */
	list_for_each_entry(attr, &object->attribute_list, list) {
		char *dest, *widget_name;

		if (!strcmp(attr->name, "control")) {
			snd_strlcpy(line->control, attr->value.string,
				    SNDRV_CTL_ELEM_ID_NAME_MAXLEN);
			continue;
		}

		if (!strcmp(attr->name, "source"))
			dest = line->source;
		else if (!strcmp(attr->name, "sink"))
			dest = line->sink;
		else
			continue;

		widget_name = tplg_pp_get_widget_name(tplg_pp, object, attr->value.string);
		if (!widget_name) {
			SNDERR("Failed to find widget '%s' for route %s\n", attr->value.string,
			      object->name);
			ret = -EINVAL;
			goto err;
		}

		snd_strlcpy(dest, widget_name, SNDRV_CTL_ELEM_ID_NAME_MAXLEN);
	}

	/* save SectionGraph */
	if (object->parent)
		ret = snd_tplg_save_printf(&tplg_pp->buf, "",
					    "SectionGraph.\"%s.route.%d\" {\n",
					    object->parent->name, index->value.integer);
	else
		ret = snd_tplg_save_printf(&tplg_pp->buf, "",
					    "SectionGraph.\"%s.route.%d\" {\n",
					    "Endpoint", index->value.integer);
	if (ret < 0)
		goto err;

	ret = snd_tplg_save_printf(&tplg_pp->buf, "",
				    "\tindex %d\n\tlines [\n", pipeline_id->value.integer);
	if (ret < 0)
		goto err;

	ret = snd_tplg_save_printf(&tplg_pp->buf, "", "\t\t\"%s, %s, %s\"\n",
				    line->source, line->control, line->sink);
	if (ret < 0)
		goto err;

	ret = snd_tplg_save_printf(&tplg_pp->buf, "", "\t]\n}\n\n");
	if (ret < 0)
		goto err;

	tplg_pp_debug("DAPM route: %s -> %s -> %s", line->source, line->control, line->sink);
	print_pre_processed_config(tplg_pp);

err:
	free(line);
	return ret;
}
