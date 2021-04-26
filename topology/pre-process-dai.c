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
