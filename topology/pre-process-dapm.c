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

	print_pre_processed_config(tplg_pp);

	ret = tplg_pp_add_object_data(tplg_pp, object);
	if (ret < 0)
		SNDERR("Failed to add data section for widget %s\n", widget->name);

err:
	free(widget);
	return ret;
}
