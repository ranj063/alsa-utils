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
#include <stdlib.h>
#include <sound/asound.h>
#include "topology.h"
#include "../alsactl/list.h"

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
	struct list_head list; /* item in class attribute list */
	char token_ref[SNDRV_CTL_ELEM_ID_NAME_MAXLEN];
	struct attribute_constraint constraint;
	union {
		long integer;
		long long integer64;
		double d;
		char string[SNDRV_CTL_ELEM_ID_NAME_MAXLEN];
	}value;
};

/* class types */
#define SND_TPLG_CLASS_TYPE_BASE		0

struct tplg_class {
	char name[SNDRV_CTL_ELEM_ID_NAME_MAXLEN];
	int num_args;
	struct list_head attribute_list; /* list of attributes */
	struct list_head list; /* item in class list */
	int type;
};

#define DEBUG_MAX_LENGTH	256

void print_pre_processed_config(struct tplg_pre_processor *tplg_pp);
void tplg_pp_debug(char *fmt, ...);
int tplg_define_classes(struct tplg_pre_processor *tplg_pp, snd_config_t *cfg);
#endif
