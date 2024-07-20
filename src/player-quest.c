/**
 * \file player-quest.c
 * \brief All throne room-related code
 *
 * Copyright (c) 2013 Angband developers
 *
 * This work is free software; you can redistribute it and/or modify it
 * under the terms of either:
 *
 * a) the GNU General Public License as published by the Free Software
 *    Foundation, version 2, or
 *
 * b) the "Angband licence":
 *    This software may be copied and distributed for educational, research,
 *    and not for profit purposes provided that this copyright and statement
 *    are included in all such copies.  Other copyrights may also apply.
 */
#include "angband.h"
#include "combat.h"
#include "datafile.h"
#include "game-input.h"
#include "game-world.h"
#include "generate.h"
#include "init.h"
#include "mon-calcs.h"
#include "mon-desc.h"
#include "mon-move.h"
#include "mon-util.h"
#include "monster.h"
#include "obj-desc.h"
#include "obj-gear.h"
#include "obj-make.h"
#include "obj-pile.h"
#include "obj-tval.h"
#include "obj-util.h"
#include "player-abilities.h"
#include "player-attack.h"
#include "player-calcs.h"
#include "player-history.h"
#include "player-quest.h"
#include "project.h"

/**
 * Makes Saruman drop the Keys of Orthanc with an appropriate message.
 */
void drop_the_keys(struct monster *mon, const char *message, bool burglary)
{
	int i;
	struct loc grid;
	struct monster_race *race = mon->race;
	const struct artifact *keys = lookup_artifact_name("of Orthanc");
	bool note;

	if (!is_artifact_created(keys)) {
		struct object *obj;
		struct object_kind *kind;
		msg(message);

		/* Choose a nearby location, but not his own square */
		find_nearby_grid(cave, &grid, mon->grid, 1, 1);
		for (i = 0; i < 1000; i++) {
			find_nearby_grid(cave, &grid, mon->grid, 1, 1);
			if (!loc_eq(grid, mon->grid) && square_isfloor(cave, grid)) break;
		}

		/* Allocate by hand, prep, apply magic */
		obj = mem_zalloc(sizeof(*obj));
		kind = lookup_kind(keys->tval, keys->sval);
		object_prep(obj, kind, z_info->dun_depth, RANDOMISE);
		obj->artifact = keys;
		copy_artifact_data(obj, obj->artifact);
		mark_artifact_created(keys, true);

		/* Set origin details */
		obj->origin = ORIGIN_DROP;
		obj->origin_depth = convert_depth_to_origin(cave->depth);
		obj->origin_race = race;
		obj->number = 1;
        
        /* If you stole it, you get it. */
        if(burglary) {
            inven_carry(player, obj, true, true);
            pack_overflow(obj);
        } else {
		/* Otherwise, drop it there */
		floor_carry(cave, grid, obj, &note);
        }

        /* Increase Saruman's will and perception when he loses the keys*/
		race->wil += 2;
		race->per += 4;
	}
}


/**
 * Checks if the player has the Keys
 */
int keys_possessed(struct player *p)
{
	int keys = 0;
	struct object *obj;

	for (obj = p->gear; obj; obj = obj->next) {
		if (streq(obj->kind->name, "& Great Keyring~")) {
			keys += 1;
		}
	}
	return keys;
}


/**
 * Break the truce in Saruman's throne room
 */
void break_truce(struct player *p, bool obvious)
{
	int i;
	struct monster *mon = NULL;
	char m_name[80];

	if (p->truce) {
		/* Scan all other monsters */
		for (i = cave_monster_max(cave) - 1; i >= 1; i--) {
			/* Access the monster */
			mon = cave_monster(cave, i);

			/* Ignore dead monsters */
			if (!mon->race) continue;

			/* Ignore monsters out of line of sight */
			if (!los(cave, mon->grid, p->grid)) continue;

			/* Ignore unalert monsters */
			if (mon->alertness < ALERTNESS_ALERT) continue;

			/* Get the monster name (using 'something' for hidden creatures) */
			monster_desc(m_name, sizeof(m_name), mon, MDESC_STANDARD);

			p->truce = false;
		}

		if (obvious) p->truce = false;

		if (!p->truce) {
			if (!obvious) {
				msg("%s lets out a cry! The tension is broken.", m_name);

				/* Make a lot of noise */
				cave->monster_noise.centre = mon->grid;
				update_flow(cave, &cave->monster_noise, NULL);
				monsters_hear(false, false, -10);
			} else {
				msg("The tension is broken.");
			}

			/* Scan all other monsters */
			for (i = cave_monster_max(cave) - 1; i >= 1; i--) {
				/* Access the monster */
				mon = cave_monster(cave, i);

				/* Ignore dead monsters */
				if (!mon->race) continue;

				/* Mark minimum desired range for recalculation */
				mon->min_range = 0;
			}
		}
	}
}

/**
 * Check whether to break the truce in Saruman's throne room
 */
void check_truce(struct player *p)
{
	int d;

	/* Check around the character */
	for (d = 0; d < 8; d++) {
		struct loc grid = loc_sum(p->grid, ddgrid_ddd[d]);
		struct monster *mon = square_monster(cave, grid);

		if (mon && (mon->race == lookup_monster("Saruman of Many Colours"))
			&& (mon->alertness >= ALERTNESS_ALERT)) {
            if (streq(p->race->name, "High Elf") || streq(p->race->name, "Grey Elf")) {
			    msg("With a voice of injured pride, Saruman of Many Colours speaks:");
			    msg("'You dare invade my stronghold? The Elder Days are gone, lone hero.'");
            } else if (streq(p->race->name, "Dwarf")) {
			    msg("With a voice of tired frustration, Saruman of Many Colours speaks:");
			    msg("'You dare invade my stronghold? Your kind only know two languages, and it's too late for gold. Guards!'");
            } else if (streq(p->race->name, "Hobbit")) {
			    msg("With a voice of condescending mirth, Saruman of Many Colours speaks:");
			    msg("'You dare invade my stronghold? Halfling, you should have stayed to guard your little Shire.'");
            } else if (streq(p->race->name, "Man")) {
			    msg("With a voice of ageless hubris, Saruman of Many Colours speaks:");
			    msg("'You dare invade my stronghold? I am Man's brightest hope, and you will not bow. Die, then.'");
            } else {
                msg("With a voice of boiling perplexity, Saruman of Many Colours speaks:"); 
                msg("'Player's race does not have a message for this situation; please report this bug.");
            }
			/* Break the truce (always) */
			break_truce(p, true);
			return;
		}
	}
}

/**
 * Wake up all monsters, and speed up "los" monsters.
 */
void wake_all_monsters(struct player *p)
{
	int i;

	/* Aggravate everyone */
	for (i = 1; i < cave_monster_max(cave); i++) {
		struct monster *mon = cave_monster(cave, i);
		/* Paranoia -- Skip dead monsters */
		if (!mon->race) continue;

		/* Alert it */
		set_alertness(mon, MAX(mon->alertness, ALERTNESS_VERY_ALERT));

		/* Possibly update the monster health bar*/
		if (p->upkeep->health_who == mon) p->upkeep->redraw |= (PR_HEALTH);
	}
}
