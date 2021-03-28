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
#include <stdlib.h>
#include <alsa/input.h>
#include <alsa/output.h>
#include <alsa/conf.h>
#include <sound/asound.h>
#include "topology.h"
#include "../alsactl/list.h"

#define DEBUG_MAX_LENGTH	256
#define ARRAY_SIZE(a) (sizeof (a) / sizeof (a)[0])

/* set of attributes that are part of the same SectionVendorTuples */
struct tplg_attribute_set {
	char token_ref[SNDRV_CTL_ELEM_ID_NAME_MAXLEN];
	struct list_head list; /* item in object attribute_set_list */
	struct list_head attribute_list; /* list of attributes */
};

struct map_elem {
        const char *name;
        int id;
};

#define TPLG_CLASS_ATTRIBUTE_MASK_MANDATORY	1 << 1
#define TPLG_CLASS_ATTRIBUTE_MASK_IMMUTABLE	1 << 2
#define TPLG_CLASS_ATTRIBUTE_MASK_DEPRECATED	1 << 3
#define TPLG_CLASS_ATTRIBUTE_MASK_AUTOMATIC	1 << 4
#define TPLG_CLASS_ATTRIBUTE_MASK_UNIQUE	1 << 5

/*
 * some attribute's have valid string values that translate to integer values. The "string"
 * field stored the human readable value and the "value" field stores the corresponding integer
 * values. "value" is set to -EINVAL by default and is updated based on the value_ref array
 * values.
 */
struct tplg_attribute_ref {
	const char *id;
	const char *string;
	int value;
	struct list_head list; /* item in attribute constraint value_list */
};

struct attribute_constraint {
	struct list_head value_list; /* list of valid values */
	int mask;
	int min;
	int max;
};

enum tplg_class_param_type {
	TPLG_CLASS_PARAM_TYPE_ARGUMENT,
	TPLG_CLASS_PARAM_TYPE_ATTRIBUTE,
};

struct tplg_attribute {
	char name[SNDRV_CTL_ELEM_ID_NAME_MAXLEN];
	snd_config_type_t type;
	enum tplg_class_param_type param_type;
	struct list_head list; /* item in class/object attribute list */
	char token_ref[SNDRV_CTL_ELEM_ID_NAME_MAXLEN];
	struct attribute_constraint constraint;
	bool found;
	snd_config_t *cfg;
	union {
		long integer;
		long long integer64;
		double d;
		char string[SNDRV_CTL_ELEM_ID_NAME_MAXLEN];
	}value;
};

/* class types */
#define SND_TPLG_CLASS_TYPE_BASE		0
#define SND_TPLG_CLASS_TYPE_WIDGET		1
#define SND_TPLG_CLASS_TYPE_PIPELINE		2
#define SND_TPLG_CLASS_TYPE_DAI		3
#define SND_TPLG_CLASS_TYPE_CONTROL		4
#define SND_TPLG_CLASS_TYPE_PCM		5

struct tplg_class {
	char name[SNDRV_CTL_ELEM_ID_NAME_MAXLEN];
	int num_args;
	struct list_head attribute_list; /* list of attributes */
	struct list_head list; /* item in class list */
	struct list_head object_list; /* list of child objects */
	int type;
};

struct tplg_object {
	char name[SNDRV_CTL_ELEM_ID_NAME_MAXLEN];
	char class_name[SNDRV_CTL_ELEM_ID_NAME_MAXLEN];
	int num_args;
	struct list_head attribute_list;
	struct list_head object_list;
	struct tplg_object *parent;
	snd_config_t *cfg;
	int type;
	struct list_head list; /* item in object list */
	struct list_head attribute_set_list; /* list of attribute sets */
};

typedef int (*build_func)(struct tplg_pre_processor *tplg_pp, struct tplg_object *object);

struct build_function_map {
	int class_type;
	char class_name[SNDRV_CTL_ELEM_ID_NAME_MAXLEN];
	build_func builder;
};

void tplg_pp_debug(char *fmt, ...);
void tplg_pp_config_debug(struct tplg_pre_processor *tplg_pp, snd_config_t *cfg);
int tplg_define_classes(struct tplg_pre_processor *tplg_pp, snd_config_t *cfg);
int tplg_create_objects(struct tplg_pre_processor *tplg_pp, snd_config_t *cfg);
struct tplg_class *tplg_class_lookup(struct tplg_pre_processor *tplg_pp, const char *name);
int tplg_parse_attribute_value(snd_config_t *cfg, struct list_head *list, bool override);
struct tplg_object *
tplg_create_object(struct tplg_pre_processor *tplg_pp, snd_config_t *cfg, struct tplg_class *class,
		   struct tplg_object *parent, struct list_head *list);
struct tplg_object *tplg_object_lookup_in_list(struct list_head *list, const char *class_name,
					       char *input);
int tplg_attribute_config_update(snd_config_t *parent, struct tplg_attribute *attr);
snd_config_t *tplg_find_config(snd_config_t *config, char *name);
struct tplg_attribute *tplg_get_attribute_by_name(struct list_head *list, const char *name);
int tplg_build_data_object(struct tplg_pre_processor *tplg_pp, struct tplg_object *object);
int tplg_build_manifest_object(struct tplg_pre_processor *tplg_pp,
			       struct tplg_object *object);
int tplg_build_widget_object(struct tplg_pre_processor *tplg_pp, struct tplg_object *object);
int tplg_build_vendor_token_object(struct tplg_pre_processor *tplg_pp,
				   struct tplg_object *object);
int tplg_pp_add_object_data(struct tplg_pre_processor *tplg_pp, struct tplg_object *object,
			    snd_config_t *top);
int tplg_pp_build_tlv_object(struct tplg_pre_processor *tplg_pp,
			     struct tplg_object *object);
#endif
