/*
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __PRE_PROCESSOR_H
#define __PRE_PROCESSOR_H

#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include "topology.h"

#define DEBUG_MAX_LENGTH	256
#define ARRAY_SIZE(a) (sizeof (a) / sizeof (a)[0])

typedef int (*build_func)(struct tplg_pre_processor *tplg_pp, snd_config_t *obj,
			  snd_config_t *parent);
typedef int (*template_func)(snd_config_t **wtemplate);

struct build_function_map {
	char *class_type;
	char *class_name;
	char *section_name;
	build_func builder;
	template_func template_function;
};

extern const struct build_function_map object_build_map[];

/* debug helpers */
void tplg_pp_debug(char *fmt, ...);
void tplg_pp_config_debug(struct tplg_pre_processor *tplg_pp, snd_config_t *cfg);

/* object build helpers */
int tplg_build_object_from_template(struct tplg_pre_processor *tplg_pp, snd_config_t *obj_cfg,
				    snd_config_t **wtop, snd_config_t *top_config,
				    bool skip_name);
int tplg_build_data_object(struct tplg_pre_processor *tplg_pp, snd_config_t *obj,
			   snd_config_t *parent);
int tplg_build_vendor_token_object(struct tplg_pre_processor *tplg_pp,
			       snd_config_t *obj_cfg, snd_config_t *parent);
int tplg_build_tlv_object(struct tplg_pre_processor *tplg_pp, snd_config_t *obj_cfg,
			      snd_config_t *parent);
int tplg_build_scale_object(struct tplg_pre_processor *tplg_pp, snd_config_t *obj_cfg,
			      snd_config_t *parent);
int tplg_build_ops_object(struct tplg_pre_processor *tplg_pp, snd_config_t *obj_cfg,
			      snd_config_t *parent);
int tplg_build_channel_object(struct tplg_pre_processor *tplg_pp, snd_config_t *obj_cfg,
			      snd_config_t *parent);
int tplg_build_mixer_control(struct tplg_pre_processor *tplg_pp, snd_config_t *obj_cfg,
			      snd_config_t *parent);
int tplg_build_bytes_control(struct tplg_pre_processor *tplg_pp, snd_config_t *obj_cfg,
			      snd_config_t *parent);
int tplg_build_dapm_route_object(struct tplg_pre_processor *tplg_pp, snd_config_t *obj_cfg,
			      snd_config_t *parent);
int tplg_parent_update(struct tplg_pre_processor *tplg_pp, snd_config_t *parent,
			  const char *section_name, const char *item_name);

/* object helpers */
int tplg_pre_process_objects(struct tplg_pre_processor *tplg_pp, snd_config_t *cfg,
			     snd_config_t *parent);
snd_config_t *tplg_object_get_instance_config(struct tplg_pre_processor *tplg_pp,
					snd_config_t *class_type);
const struct build_function_map *tplg_object_get_map(struct tplg_pre_processor *tplg_pp,
						     snd_config_t *obj);
const char *tplg_object_get_name(struct tplg_pre_processor *tplg_pp,
				 snd_config_t *object);
snd_config_t *tplg_object_get_section(struct tplg_pre_processor *tplg_pp, snd_config_t *class);

/* class helpers */
snd_config_t *tplg_class_lookup(struct tplg_pre_processor *tplg_pp, snd_config_t *cfg);
snd_config_t *tplg_class_find_attribute_by_name(struct tplg_pre_processor *tplg_pp,
						snd_config_t *class, const char *name);
bool tplg_class_is_attribute_mandatory(const char *attr, snd_config_t *class_cfg);
bool tplg_class_is_attribute_immutable(const char *attr, snd_config_t *class_cfg);
bool tplg_class_is_attribute_unique(const char *attr, snd_config_t *class_cfg);
const char *tplg_class_get_unique_attribute_name(struct tplg_pre_processor *tplg_pp,
						 snd_config_t *class);
snd_config_type_t tplg_class_get_attribute_type(struct tplg_pre_processor *tplg_pp,
						snd_config_t *attr);
const char *tplg_class_get_attribute_token_ref(struct tplg_pre_processor *tplg_pp,
					        snd_config_t *class, const char *attr_name);
long tplg_class_attribute_valid_tuple_value(struct tplg_pre_processor *tplg_pp,
					        snd_config_t *class, snd_config_t *attr);

/* config helpers */
snd_config_t *tplg_find_config(snd_config_t *config, const char *name);
int tplg_config_make_add(snd_config_t **config, const char *id, snd_config_type_t type,
			 snd_config_t *parent);

/* config template functions */
int tplg_create_data_config_template(snd_config_t **dtemplate);
int tplg_create_widget_config_template(snd_config_t **wtemplate);
int tplg_create_scale_template(snd_config_t **scale_template);
int tplg_create_ops_template(snd_config_t **ops_template);
int tplg_create_channel_template(snd_config_t **ctemplate);
int tplg_create_mixer_template(snd_config_t **mixer_template);
int tplg_create_bytes_template(snd_config_t **bytes_template);

char *tplg_snprintf(char *fmt, ...);
#endif
