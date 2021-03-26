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
#include "gettext.h"
#include "topology.h"
#include "pre-processor.h"

/* Parse manifest object, create the "SectionManifest" and save it */
int tplg_build_manifest_object(struct tplg_pre_processor *tplg_pp,
			       struct tplg_object *object)
{
	snd_config_t *top, *mtop, *data_config, *child;
	struct tplg_attribute *name;
	int ret;

	tplg_pp_debug("Building manifest object: '%s' ...", object->name);

	ret = snd_config_search(tplg_pp->cfg, "SectionManifest", &top);
	if (ret < 0) {
		ret = snd_config_make_add(&top, "SectionManifest", SND_CONFIG_TYPE_COMPOUND,
					  tplg_pp->cfg);
		if (ret < 0)
			return ret;
	} else {
		SNDERR("Manifest object exists already\n");
		return -EEXIST;
	}

	name = tplg_get_attribute_by_name(&object->attribute_list, "name");

	ret = snd_config_make_add(&mtop, name->value.string, SND_CONFIG_TYPE_COMPOUND, top);
	if (ret < 0) {
		SNDERR("Error creating manifest name for %s\n", object->name);
		return ret;
	}

	ret = snd_config_make_add(&data_config, "data", SND_CONFIG_TYPE_COMPOUND, mtop);
	if (ret < 0) {
		SNDERR("Error creating data config for %s\n", object->name);
		return ret;
	}

	ret = snd_config_make_add(&child, "0", SND_CONFIG_TYPE_STRING,
				  data_config);
	if (ret < 0) {
		SNDERR("Error adding data config for %s\n", object->name);
		return ret;
	}

	ret = snd_config_set_string(child, name->value.string);
	if (ret < 0)
		SNDERR("Error setting data config for manifest '%s'\n", object->name);

	tplg_pp_config_debug(tplg_pp, top);

	return ret;
}

/* Parse data object, create the "SectionData" and save it. Only "bytes" data supported for now */
int tplg_build_data_object(struct tplg_pre_processor *tplg_pp, struct tplg_object *object)
{
	struct tplg_attribute *bytes, *name;
	snd_config_t *top, *data_config, *child;
	int ret;

	tplg_pp_debug("Building data object: '%s' ...", object->name);

	ret = snd_config_search(tplg_pp->cfg, "SectionData", &top);
	if (ret < 0) {
		ret = snd_config_make_add(&top, "SectionData", SND_CONFIG_TYPE_COMPOUND,
					  tplg_pp->cfg);
		if (ret < 0)
			return ret;
	}

	bytes = tplg_get_attribute_by_name(&object->attribute_list, "bytes");
	name = tplg_get_attribute_by_name(&object->attribute_list, "name");

	/* not need to do anything if data node exists already */
	data_config = tplg_find_config(top, name->value.string);
	if (data_config)
		return 0;

	ret = snd_config_make_add(&data_config, name->value.string,
				  SND_CONFIG_TYPE_COMPOUND, top);
	if (ret < 0) {
		SNDERR("Error creating data config for %s\n", name->value.string);
		return ret;
	}

	ret = snd_config_make_add(&child, "bytes", SND_CONFIG_TYPE_STRING, data_config);
	if (ret < 0) {
		SNDERR("Error creating 'bytes' for %s\n", name->value.string);
		return ret;
	}

	ret = snd_config_set_string(child, bytes->value.string);
	if (ret < 0)
		SNDERR("Error setting bytes config for %s\n", name->value.string);

	tplg_pp_config_debug(tplg_pp, top);

	return ret;
}
