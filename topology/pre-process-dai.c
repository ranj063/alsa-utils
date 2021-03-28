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
