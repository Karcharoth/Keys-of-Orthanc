/**
 * \file cmd-pickup.c
 * \brief Pickup code
 *
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke,
 * Copyright (c) 2007 Leon Marrick
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
#include "cave.h"
#include "combat.h"
#include "cmds.h"
#include "game-event.h"
#include "game-input.h"
#include "generate.h"
#include "init.h"
#include "mon-attack.h"
#include "mon-calcs.h"
#include "mon-desc.h"
#include "mon-lore.h"
#include "mon-move.h"
#include "mon-timed.h"
#include "mon-util.h"
#include "obj-desc.h"
#include "obj-gear.h"
#include "obj-ignore.h"
#include "obj-pile.h"
#include "obj-tval.h"
#include "obj-util.h"
#include "player-attack.h"
#include "player-abilities.h"
#include "player-calcs.h"
#include "player-history.h"
#include "player-quest.h"
#include "player-timed.h"
#include "player-util.h"
#include "trap.h"


/**
 * Find the specified object in the inventory (not equipment)
 */
static const struct object *find_stack_object_in_inventory(const struct object *obj, const struct object *start)
{
	const struct object *gear_obj;
	for (gear_obj = (start) ? start : player->gear; gear_obj; gear_obj = gear_obj->next) {
		if (!object_is_equipped(player->body, gear_obj) &&
				object_similar(gear_obj, obj, OSTACK_PACK)) {
			/* We found the object */
			return gear_obj;
		}
	}

	return NULL;
}


/**
 * Determine if an object can be picked up automatically and return the
 * number to pick up.
 */
static int auto_pickup_okay(const struct object *obj)
{
        /*
	 * Use the following inscriptions to guide pickup with the last one
	 * borrowed from Unangband:
	 *
	 * !g     don't pickup
	 * =g     pickup
	 * =g<n>  (i.e. =g5) pick up if have less than n
	 *
	 * !g takes precedence over any of the others if an object is
	 * inscribed with it and any of the others.  =g with no value takes
	 * precedence over =g<n> if an object is inscribed with both.  In
	 * general, inscriptions on the item on the floor are examined first
	 * and the ones on a matching item in the pack will only come into
	 * consideration if those on the item on the floor do not force or
	 * reject pickup.  When examining inscriptions in the pack, only
	 * use those on the first stack.
	 *
	 * The player option to always pick up overrides all of those
	 * inscriptions.  The player option to pickup if in the inventory
	 * honors those inscriptions.
	 */
	int num = inven_carry_num(player, obj);
	unsigned obj_has_auto, obj_has_maxauto;
	int obj_maxauto;

	if (!num) return 0;

	if (OPT(player, pickup_always)) return num;
	if (obj->notice & OBJ_NOTICE_PICKUP) return num;
	if (check_for_inscrip(obj, "!g")) return 0;

	obj_has_auto = check_for_inscrip(obj, "=g");
	obj_maxauto = INT_MAX;
	obj_has_maxauto = check_for_inscrip_with_int(obj, "=g", &obj_maxauto);
	if (obj_has_auto > obj_has_maxauto) return num;

	if (OPT(player, pickup_inven) || obj_has_maxauto) {
		const struct object *gear_obj = find_stack_object_in_inventory(obj, NULL);
		if (!gear_obj) {
			if (obj_has_maxauto) {
				return (num < obj_maxauto) ? num : obj_maxauto;
			}
			return 0;
		}
		if (!check_for_inscrip(gear_obj, "!g")) {
			unsigned int gear_has_auto = check_for_inscrip(gear_obj, "=g");
			unsigned int gear_has_maxauto;
			int gear_maxauto;

			gear_has_maxauto = check_for_inscrip_with_int(gear_obj, "=g", &gear_maxauto);
			if (gear_has_auto > gear_has_maxauto) {
				return num;
			}
			if (obj_has_maxauto || gear_has_maxauto) {
				/* Use the pack inscription if have both. */
				int max_num = (gear_has_maxauto) ?
					gear_maxauto : obj_maxauto;
				/* Determine the total number in the pack. */
				int pack_num = gear_obj->number;

				while (1) {
					if (!gear_obj->next) {
						break;
					}
					gear_obj = find_stack_object_in_inventory(obj, gear_obj->next);
					if (!gear_obj) {
						break;
					}
					pack_num += gear_obj->number;
				}
				if (pack_num >= max_num) {
					return 0;
				}
				return (num < max_num - pack_num) ?
					num : max_num - pack_num;
			}
			return num;
		}
	}

	return 0;
}


/**
 * Move an object from a floor pile to the player's gear, checking first
 * whether partial pickup is needed
 */
static void player_pickup_aux(struct player *p, struct object *obj,
							  int auto_max, bool domsg)
{
	int max = inven_carry_num(p, obj);
	/* Remember if this is auto-pickup. */
	bool autopick = (obj->notice & OBJ_NOTICE_PICKUP);

	/* Confirm at least some of the object can be picked up */
	if (max == 0)
		quit_fmt("Failed pickup of %s", obj->kind->name);

	/* Set ignore status */
	p->upkeep->notice |= PN_IGNORE;

	/* Allow auto-pickup to limit the number if it wants to */
	if (auto_max && max > auto_max) {
		max = auto_max;
	}

	/* Carry the object, prompting for number if necessary */
	if (max == obj->number) {
		square_excise_object(cave, p->grid, obj);
		delist_object(cave, obj);
		inven_carry(p, obj, true, domsg);
	} else {
		int num;
		bool dummy;
		struct object *picked_up;

		if (auto_max)
			num = auto_max;
		else
			num = get_quantity(NULL, max);
		if (!num) return;
		picked_up = floor_object_for_use(p, obj, num, false, &dummy);
		inven_carry(p, picked_up, true, domsg);
	}

	/* If it's auto-pickup of thrown/fired things we're done */
	if (autopick) return;

	/* Store the action type */
	p->previous_action[0] = ACTION_MISC;

	/* Take a turn */
    /* Hack: if auto_max exists don't spend energy, or you could die horribly to autograbbing
    arrows while being chased*/
    if (!auto_max)	p->upkeep->energy_use = z_info->move_energy;
}

/**
 * Pick up objects and treasure on the floor.  -LM-
 *
 * Scan the list of objects in that floor grid. Pick up gold automatically.
 * Pick up objects automatically until backpack space is full if
 * auto-pickup option is on, otherwise, store objects on
 * floor in an array, and tally both how many there are and can be picked up.
 *
 * If not picking up anything, indicate objects on the floor.  Do the same
 * thing if we don't have room for anything.
 *
 * Pick up multiple objects using Tim Baker's menu system.   Recursively
 * call this function (forcing menus for any number of objects) until
 * objects are gone, backpack is full, or player is satisfied.
 *
 * We keep track of number of objects picked up to calculate time spent.
 * This tally is incremented even for automatic pickup, so we are careful
 * (in "dungeon.c" and elsewhere) to handle pickup as either a separate
 * automated move or a no-cost part of the stay still or 'g'et command.
 *
 * Note the lack of chance for the character to be disturbed by unmarked
 * objects.  They are truly "unknown".
 *
 * \param obj is the object to pick up.
 * \param menu is whether to present a menu to the player
 */
void player_pickup_item(struct player *p, struct object *obj, bool menu)
{
	struct object *current = NULL;

	int floor_max = z_info->floor_size + 1;
	struct object **floor_list = mem_zalloc(floor_max * sizeof(*floor_list));
	int floor_num = 0;

	int i;
	int can_pickup = 0;
	bool call_function_again = false;

	bool domsg = true;

	/* Always know what's on the floor */
	square_know_pile(cave, p->grid);

	/* Nothing else to pick up -- return */
	if (!square_object(cave, p->grid)) {
		mem_free(floor_list);
		return;
	}

	/* We're given an object - pick it up */
	if (obj) {
		mem_free(floor_list);
        if (inven_carry_num(p, obj)) {
		    player_pickup_aux(p, obj, 0, domsg);
        }
		return;
	}

	/* Tally objects that can be at least partially picked up.*/
	floor_num = scan_floor(floor_list, floor_max, p, OFLOOR_VISIBLE, NULL);
	for (i = 0; i < floor_num; i++)
	    if (inven_carry_num(p, floor_list[i]) > 0)
			can_pickup++;

	if (!can_pickup) {
	    event_signal(EVENT_SEEFLOOR);
		mem_free(floor_list);
	    return;
	}

	/* Use a menu interface for multiple objects, or pickup single objects */
	if (!menu && !current) {
		if (floor_num > 1)
			menu = true;
		else
			current = floor_list[0];
	}

	/* Display a list if requested. */
	if (menu && !current) {
		const char *q, *s;
		struct object *obj_local = NULL;

		/* Get an object or exit. */
		q = "Get which item?";
		s = "You see nothing there.";
		if (!get_item(&obj_local, q, s, CMD_PICKUP, inven_carry_okay, USE_FLOOR)) {
			mem_free(floor_list);
			return;
		}

		current = obj_local;
		call_function_again = true;

		/* With a list, we do not need explicit pickup messages */
		domsg = true;
	}

	/* Pick up object, if legal */
	if (current) {
		/* Pick up the object */
		player_pickup_aux(p, current, 0, domsg);
	}

	/*
	 * If requested, call this function recursively.  Count objects picked
	 * up.  Force the display of a menu in all cases.
	 */
	if (call_function_again)
		player_pickup_item(p, NULL, true);

	mem_free(floor_list);
}

/**
 * Pick up everything on the floor that requires no player action
 */
static bool do_autopickup(struct player *p)
{
	struct object *obj, *next;
	bool picked_up = false;

	/* Nothing to pick up -- return */
	if (!square_object(cave, p->grid))
		return false;

	/* Scan the remaining objects */
	obj = square_object(cave, p->grid);
	while (obj) {
		next = obj->next;

		/* Ignore all hidden objects and non-objects */
		if (!ignore_item_ok(p, obj)) {
			int auto_num;

			/* Hack -- disturb */
			disturb(p, false);

			/* Automatically pick up items into the backpack */
			auto_num = auto_pickup_okay(obj);
			if (auto_num) {
				/* Pick up the object (as much as possible) with message */
				player_pickup_aux(p, obj, auto_num, true);
				picked_up = true;
			}
		}
		obj = next;
	}
	return picked_up;
}

/**
 * Pick up objects at the player's request
 */
void do_cmd_pickup(struct command *cmd)
{
	struct object *obj = NULL;

	/* See if we have an item already */
	(void) cmd_get_arg_item(cmd, "item", &obj);

	/* Pick up floor objects with a menu for multiple objects */
	player_pickup_item(player, obj, false);

	/* Redraw the object list using the upkeep flag so that the update can be
	 * somewhat coalesced. Use event_signal(EVENT_ITEMLIST to force update. */
	player->upkeep->redraw |= (PR_ITEMLIST);
}

/**
 * Pick up or look at objects on a square when the player steps onto it
 */
void do_cmd_autopickup(struct command *cmd)
{
	/* Get the obvious things */
	if (do_autopickup(player)) {
		/* Look at or feel what's left */
		event_signal(EVENT_SEEFLOOR);
	}

	/* Redraw the object list using the upkeep flag so that the update can be
	 * somewhat coalesced. Use event_signal(EVENT_ITEMLIST to force update. */
	player->upkeep->redraw |= (PR_ITEMLIST);
}

/**
 * Steal an item from a monster.
 */
void do_cmd_burglary(struct command *cmd)
{
	int dir;
	struct loc grid;
	struct monster *mon;
	char m_name[80];
	char m_pronoun_capitalized[80];
	char m_pronoun_possessive[80];
    int p_stealth;
    int rob_difficulty;
    struct object *obj;
	const struct artifact *keys = lookup_artifact_name("of Orthanc");

	if (!player_active_ability(player, "Burglary")) {
		msg("You need the ability 'burglary' to use this command.");
		return;
	}

	/* Get arguments */
	if (cmd_get_direction(cmd, "direction", &dir, false) != CMD_OK)
		return;

	/* Get location */
	grid = loc_sum(player->grid, ddgrid[dir]);

	/* Check for monster */
	mon = square_monster(cave, grid);
    if (!mon) {
		/* No monster, or invisible */
		msg("You cannot see a monster there to steal from.");
		return;
	}

	/* Take a turn */
	player->upkeep->energy_use = z_info->move_energy;

	/* Store the action type */
	player->previous_action[0] = ACTION_MISC;

	/* Apply confusion */
	if (player_confuse_dir(player, &dir, false)) {
		/* Get location */
		grid = loc_sum(player->grid, ddgrid[dir]);
	}

	/* Re-check for a monster (in case confusion changed the move) */
	mon = square_monster(cave, grid);
	if (!mon) {
		/* Message */
		msg("You cannot see a monster there to steal from.");
		return;
	}

	/* Spend 2 stamina*/
    stamina_hit(player, 2);

	/* Recalculate the monster name (in case confusion changed the move) */
	monster_desc(m_name, sizeof(m_name), mon, MDESC_DEFAULT);
    /* Calculate pronouns for the first time */
	monster_desc(m_pronoun_capitalized, sizeof(m_pronoun_capitalized), 
                 mon, MDESC_PRO_VIS | MDESC_CAPITAL);
	monster_desc(m_pronoun_possessive, sizeof(m_pronoun_possessive), 
                 mon, MDESC_PRO_VIS | MDESC_POSS);


    /* Calculate the player's stealth */
    p_stealth = player->state.skill_use[SKILL_STEALTH];

    /* Calculate the monster's stealth (ability to hide the object) */
    rob_difficulty = monster_skill(mon, SKILL_STEALTH);

    /* If it's aware, add its perception too: does it slap your hand away? */
    if (mon->alertness >= ALERTNESS_ALERT) {
        rob_difficulty += monster_skill(mon, SKILL_PERCEPTION);
    }

    /* Penalize burglary for not seeing the monster right. */
	if (player->timed[TMD_IMAGE] || !monster_is_visible(mon)) {
        p_stealth -=5;
    }

    /* Penalize burglary for confusion. */
	if (player->timed[TMD_CONFUSED]) {
		p_stealth -= 5;
    }

    /* Why is burglary harder the 7th time? Eh, all sorts of reasons.*/
    p_stealth -= mon->times_robbed;

    /* We made it! */
    mon->times_robbed++;
    /* Now for the actual roll... */
    if (skill_check(source_player(), p_stealth, rob_difficulty, source_monster(mon->midx)) > 0) {
        /* Steal Saruman's Keys! And make some noise...*/
        if(mon->race == lookup_monster("Saruman of Many Colours") && !is_artifact_created(keys)) {
            drop_the_keys(mon, "You pilfer the keys from Saruman's belt, and they clank of their own accord.", true);
            p_stealth -= 10;
        } else if (mon->held_obj) { /* Monster is carrying an object picked up*/
            msg("You find some treasure on %s!", m_name);
            obj = mon->held_obj;

		    /* Object no longer held */
		    obj->held_m_idx = 0;
		    pile_excise(&mon->held_obj, obj);

            /* Get it! */
            inven_carry(player, obj, true, true);
            pack_overflow(obj);


        } else if(mon->total_loot) {
            msg("You find some treasure on %s!", m_name);
            obj = mon_create_burgled_loot(cave, mon, false);
            inven_carry(player, obj, true, true);
            pack_overflow(obj);
        } else {
            msg("You don't find any treasure on %s.", m_name);
        }

        /* No matter what, notice if it has anything more. */
        mon->eyed_for_treasure = true;
        if (mon->total_loot) {
            msg("You notice %s has some more treasure.", m_name);
        }
    } else {
        msg("You don't find any treasure on %s.", m_name);
    }

	/* Attack of opportunity */
	if ((mon->alertness >= ALERTNESS_ALERT) && !mon->m_timed[MON_TMD_CONF] &&
		!rf_has(mon->race->flags, RF_MINDLESS)) {
		msg("%s attacks you as you rifle through %s belongings.", m_pronoun_capitalized, m_pronoun_possessive);
		make_attack_normal(mon, player);
	}

    
	/* The hiding check: does anyone hear you? */
    monsters_hear(true, true, p_stealth);

}

