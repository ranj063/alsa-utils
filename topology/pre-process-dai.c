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

int tplg_pp_build_hw_cfg_object(struct tplg_pre_processor *tplg_pp, struct tplg_object *object)
{
	struct snd_soc_tplg_hw_config *hw_cfg;
	struct tplg_attribute *attr, *id;
	char name[SNDRV_CTL_ELEM_ID_NAME_MAXLEN];
	int ret;

	hw_cfg = calloc(1, sizeof(*hw_cfg));
	if (!hw_cfg)
		return -ENOMEM;

	tplg_pp_debug("Building SectionHWConfig for: '%s' ...", object->name);
	id = tplg_get_attribute_by_name(&object->attribute_list, "id");

	/* parse hw_config params from attributes */
	list_for_each_entry(attr, &object->attribute_list, list) {
		if (!attr->cfg)
			continue;

		ret = snd_soc_tplg_set_hw_config_param(attr->cfg, hw_cfg);
		if (ret < 0) {
			SNDERR("Error parsing hw_config params for object %s\n",
			       object->name);
			goto err;
		}
	}

	ret = snprintf(name, SNDRV_CTL_ELEM_ID_NAME_MAXLEN, "%s.hwcfg.%ld",
		       object->parent->name, id->value.integer);
	if (ret > SNDRV_CTL_ELEM_ID_NAME_MAXLEN) {
		SNDERR("hw_cfg name too long %s\n", name);
		ret = -EINVAL;
		goto err;
	}

	/* save hw_params */
	ret = snd_tplg_save_printf(&tplg_pp->buf, "", "SectionHWConfig.");
	if (ret < 0)
		goto err;

	ret = snd_soc_tplg_save_hw_config(hw_cfg, name, &tplg_pp->buf, "");
	if (ret < 0)
		SNDERR("Failed to save hw_cfg %s\n", name);

	print_pre_processed_config(tplg_pp);

err:
	free(hw_cfg);
	return ret;
}

int tplg_build_dai_object(struct tplg_pre_processor *tplg_pp, struct tplg_object *object)
{
	struct snd_soc_tplg_link_config *link;
	struct tplg_attribute *attr;
	struct tplg_object *child;
	int i = 0;
	int ret;

	link = calloc(1, sizeof(*link));
	if (!link)
		return -ENOMEM;

	tplg_pp_debug("Building SectionBE for: '%s' ...", object->name);

	link->size = sizeof(*link);
	snd_strlcpy(link->name, object->name, SNDRV_CTL_ELEM_ID_NAME_MAXLEN);

	/* parse link params from attributes */
	list_for_each_entry(attr, &object->attribute_list, list) {
		if (!attr->cfg)
			continue;

		ret = snd_soc_tplg_parse_link_param(attr->cfg, link);
		if (ret < 0) {
			SNDERR("Error parsing params for BE link %s\n", object->name);
			goto err;
		}
	}

	tplg_pp_debug("Link elem: %s num_hw_configs: %s", link->name, link->stream_name);

	/* save the SectionBE and its fields */
	ret = snd_tplg_save_printf(&tplg_pp->buf, "", "SectionBE.");
	if (ret < 0)
		goto err;

	ret = snd_soc_tplg_save_link_param(link, link->name, 0, &tplg_pp->buf, "");
	if (ret < 0)
		SNDERR("Failed to save link %s\n", link->name);

	/* save hw_config references */
	ret = snd_tplg_save_printf(&tplg_pp->buf, "", "\thw_configs [\n");
	if (ret < 0)
		goto err;

	list_for_each_entry(child, &object->object_list, list) {
		if (!strcmp(child->class_name, "hw_config")) {
			struct tplg_attribute *id;

			id = tplg_get_attribute_by_name(&child->attribute_list, "id");
			ret = snd_tplg_save_printf(&tplg_pp->buf, "", "\t\t\"%s.hwcfg.%d\"\n",
						    object->name, id->value.integer);
			if (ret < 0)
				goto err;
			i++;
		}
	}

	link->num_hw_configs = i;
	tplg_pp_debug("Link elem: %s num_hw_configs: %d", link->name, link->num_hw_configs);

	ret = snd_tplg_save_printf(&tplg_pp->buf, "", "\t]\n");
	if (ret < 0)
		goto err;

	print_pre_processed_config(tplg_pp);

	ret = tplg_pp_add_object_data(tplg_pp, object);
	if (ret < 0)
		SNDERR("Failed to add data section for widget %s\n", link->name);

err:
	free(link);
	return 0;
}

int tplg_build_pcm_object(struct tplg_pre_processor *tplg_pp,
			 struct tplg_object *object)
{
	struct tplg_attribute *name, *attr;
	struct snd_soc_tplg_pcm *pcm;
	struct tplg_object *child;
	unsigned int streams[2];
	int ret;

	pcm = calloc(1, sizeof(*pcm));
	if (!pcm)
		return -ENOMEM;

	pcm->size = sizeof(*pcm);

	name = tplg_get_attribute_by_name(&object->attribute_list, "name");

	/* parse PCM params */
	list_for_each_entry(attr, &object->attribute_list, list) {
		if (!attr->cfg)
			continue;

		ret = snd_soc_tplg_parse_pcm_param(attr->cfg, pcm);
		if (ret < 0) {
			SNDERR("Failed to parse PCM %s\n", object->name);
			goto err;
		}
	}

	/* parse PCM params */
	list_for_each_entry(child, &object->object_list, list) {
		if (!child->cfg)
			continue;

		if (!strcmp(child->class_name, "fe_dai")) {
			ret = snd_soc_tplg_parse_fe_dai(child->cfg, pcm);
			if (ret < 0) {
				SNDERR("Failed to parse FE DAI for PCM %s\n", object->name);
				goto err;
			}
		}

		if (!strcmp(child->class_name, "pcm_caps")) {
			ret = snd_soc_tplg_parse_streams_param(child->cfg, pcm->caps,
							       &pcm->playback, &pcm->capture);
			if (ret < 0) {
				SNDERR("Failed to parse stream caps for PCM %s\n", object->name);
				goto err;
			}
		}
	}

	snd_strlcpy(pcm->pcm_name, name->value.string, SNDRV_CTL_ELEM_ID_NAME_MAXLEN);
	tplg_pp_debug(" PCM: %s ID: %d dai_name: %s", pcm->pcm_name, pcm->dai_id, pcm->dai_name);
	tplg_pp_debug(" PCM: %s playback: %d capture %d", pcm->pcm_name, pcm->playback, pcm->capture);

	/* Save the SectionPCM */
	ret = snd_tplg_save_printf(&tplg_pp->buf, "", "SectionPCM.");
	if (ret < 0)
		goto err;

	/* save PCM params */
	ret = snd_soc_tplg_save_pcm_param(pcm, pcm->pcm_name, 0, &tplg_pp->buf, "");
	if (ret < 0) {
		SNDERR("Failed to save PCM %s\n", pcm->pcm_name);
		goto err;
	}

	/* save the FE DAI */
	ret = snd_soc_tplg_save_fe_dai(pcm, &tplg_pp->buf, "\t");
	if (ret < 0) {
		SNDERR("Failed to save FE DAI for PCM: '%s'\n", pcm->pcm_name);
		goto err;
	}

	/* save the capabilities */
	streams[0] = pcm->playback;
	streams[1] = pcm->capture;
	ret = snd_soc_tplg_save_streams_param(pcm->caps, streams, &tplg_pp->buf, "\t");
	if (ret < 0) {
		SNDERR("Failed to save FE DAI for PCM: '%s'\n", pcm->pcm_name);
		goto err;
	}

	ret = tplg_pp_add_object_data(tplg_pp, object);
	if (ret < 0)
		SNDERR("Failed to add data section for PCM %s\n", pcm->pcm_name);

err:
	free(pcm);
	return ret;
}
