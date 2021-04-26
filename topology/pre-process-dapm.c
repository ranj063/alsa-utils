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

/* function to create the widget config node */
int tplg_create_widget_config_template(snd_config_t **wtemplate)
{
	snd_config_t *wtop, *child;
	int ret;

	ret = snd_config_make(&wtop, "template", SND_CONFIG_TYPE_COMPOUND);
	if (ret < 0)
		return ret;

	ret = tplg_config_make_add(&child, "index", SND_CONFIG_TYPE_INTEGER, wtop);

	if (ret >= 0)
	ret = tplg_config_make_add(&child, "type", SND_CONFIG_TYPE_STRING, wtop);

	if (ret >= 0)
	ret = tplg_config_make_add(&child, "stream_name", SND_CONFIG_TYPE_STRING, wtop);

	if (ret >= 0)
	ret = tplg_config_make_add(&child, "no_pm", SND_CONFIG_TYPE_INTEGER, wtop);

	if (ret >= 0)
	ret = tplg_config_make_add(&child, "shift", SND_CONFIG_TYPE_INTEGER, wtop);

	if (ret >= 0)
	ret = tplg_config_make_add(&child, "invert", SND_CONFIG_TYPE_INTEGER, wtop);

	if (ret >= 0)
	ret = tplg_config_make_add(&child, "subseq", SND_CONFIG_TYPE_INTEGER, wtop);

	if (ret >= 0)
	ret = tplg_config_make_add(&child, "event_type", SND_CONFIG_TYPE_INTEGER, wtop);

	if (ret >= 0)
	ret = tplg_config_make_add(&child, "event_flags", SND_CONFIG_TYPE_INTEGER, wtop);

	if (ret >= 0)
		*wtemplate = wtop;
	else
		snd_config_delete(wtop);

	return ret;
}
