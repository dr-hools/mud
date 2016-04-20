/*************************************************************************
*   File: skills.cpp                                   Part of Bylins    *
*   Skills functions here                                                *
*                                                                        *
*  $Author$                                                       *
*  $Date$                                          *
*  $Revision$                                                     *
************************************************************************ */

#include "skills.h"

#include "obj.hpp"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "spells.h"
#include "screen.h"
#include "dg_scripts.h"
#include "constants.h"
#include "im.h"
#include "features.hpp"
#include "random.hpp"
#include "char.hpp"
#include "room.hpp"
#include "utils.h"
#include "structs.h"
#include "sysdep.h"
#include "conf.h"

/*
 * message for doing damage with a spell or skill
 *  C3.0: Also used for weapon damage on miss and death blows
 */
#define DUMMY_KNIGHT 390
#define DUMMY_SHIELD 391
#define DUMMY_WEAPON 392

// �� ������ :(

extern struct message_list fight_messages[MAX_MESSAGES];

static const char *kick_type[] =
// ���� �����. ��������� ������������� ��������� ����������� �������� �����
{
	"�������� ",		//  1..5
	"������ ",		// 6..11
	"",			// 12..26
	"������ ",		// 27..35
	"����� ������ ",	// 36..45
	"����������� ������ ",	// 46..55
	"������ ",		// 56..96
	"����� ������ ",	// 97..136
	"����������� ������ ",	// 137..176
	"���������� ������ ",	// 177..216
	"������ ",	// 217..256
	"����������� ",	// 257..296
	"���������� " // 297+
};

struct brief_shields
{
	brief_shields(CHAR_DATA* ch_, CHAR_DATA* vict_, OBJ_DATA* weap_, std::string add_)
		: ch(ch_), vict(vict_), weap(weap_), add(add_), reflect(false ) {};

	void act_to_char(const char *msg)
	{
		if (!reflect || (reflect && !PRF_FLAGGED(ch, PRF_BRIEF_SHIELDS)))
		{
			if (!add.empty() && PRF_FLAGGED(ch, PRF_BRIEF_SHIELDS))
			{
				act_add(msg, TO_CHAR);
			}
			else
			{
				act_no_add(msg, TO_CHAR);
			}
		}
	}
	void act_to_vict(const char *msg)
	{
		if (!reflect || (reflect && !PRF_FLAGGED(vict, PRF_BRIEF_SHIELDS)))
		{
			if (!add.empty() && PRF_FLAGGED(vict, PRF_BRIEF_SHIELDS))
			{
				act_add(msg, TO_VICT | TO_SLEEP);
			}
			else
			{
				act_no_add(msg, TO_VICT | TO_SLEEP);
			}
		}
	}
	void act_to_room(const char *msg)
	{
		if (add.empty())
		{
			if (reflect)
			{
				act_no_add(msg, TO_NOTVICT | TO_ARENA_LISTEN | TO_NO_BRIEF_SHIELDS);
			}
			else
			{
				act_no_add(msg, TO_NOTVICT | TO_ARENA_LISTEN);
			}
		}
		else
		{
			act_no_add(msg, TO_NOTVICT | TO_ARENA_LISTEN | TO_NO_BRIEF_SHIELDS);
			if (!reflect)
			{
				act_add(msg, TO_NOTVICT | TO_ARENA_LISTEN | TO_BRIEF_SHIELDS);
			}
		}
	}

	CHAR_DATA* ch;
	CHAR_DATA* vict;
	OBJ_DATA* weap;
	std::string add;
	// ���� ����������� ������, ������� ���� ������� � ������ PRF_BRIEF_SHIELDS
	bool reflect;

private:
	void act_no_add(const char *msg, int type)
	{
		act(msg, FALSE, ch, weap, vict, type);
	}
	void act_add(const char *msg, int type)
	{
		char buf_[MAX_INPUT_LENGTH];
		snprintf(buf_, sizeof(buf_), "%s%s", msg, add.c_str());
		act(buf_, FALSE, ch, weap, vict, type);
	}
};

OBJ_DATA* init_weap(CHAR_DATA *ch, int dam, int attacktype)
{
	OBJ_DATA* weap = 0;
	int weap_i = 0;

	switch (attacktype)
	{
	case SKILL_BACKSTAB + TYPE_HIT:
		if (!(weap = GET_EQ(ch, WEAR_WIELD))
				&& (weap_i = real_object(DUMMY_KNIGHT)) >= 0)
			weap = obj_proto[weap_i];
		break;
	case SKILL_THROW + TYPE_HIT:
		if (!(weap = GET_EQ(ch, WEAR_WIELD))
				&& (weap_i = real_object(DUMMY_KNIGHT)) >= 0)
			weap = obj_proto[weap_i];
		break;
	case SKILL_BASH + TYPE_HIT:
		if (!(weap = GET_EQ(ch, WEAR_SHIELD))
				&& (weap_i = real_object(DUMMY_SHIELD)) >= 0)
			weap = obj_proto[weap_i];
		break;
	case SKILL_KICK + TYPE_HIT:
		// weap - ����� ���� �����
		if (dam <= 5)
			weap = (OBJ_DATA *) kick_type[0];
		else if (dam <= 11)
			weap = (OBJ_DATA *) kick_type[1];
		else if (dam <= 26)
			weap = (OBJ_DATA *) kick_type[2];
		else if (dam <= 35)
			weap = (OBJ_DATA *) kick_type[3];
		else if (dam <= 45)
			weap = (OBJ_DATA *) kick_type[4];
		else if (dam <= 56)
			weap = (OBJ_DATA *) kick_type[5];
		else if (dam <= 96)
			weap = (OBJ_DATA *) kick_type[6];
		else if (dam <= 136)
			weap = (OBJ_DATA *) kick_type[7];
		else if (dam <= 176)
			weap = (OBJ_DATA *) kick_type[8];
		else if (dam <= 216)
			weap = (OBJ_DATA *) kick_type[9];
		else if (dam <= 256)
			weap = (OBJ_DATA *) kick_type[10];
		else if (dam <= 296)
			weap = (OBJ_DATA *) kick_type[11];
		else
			weap = (OBJ_DATA *) kick_type[12];
		break;
	case TYPE_HIT:
		weap = 0;
		break;
	default:
		if (!weap && (weap_i = real_object(DUMMY_WEAPON)) >= 0)
			weap = obj_proto[weap_i];
	}

	return weap;
}

///
/// \param add = "", ������ ��� ���������� ����� ��������� ��������� (������� ����� �����)
///
int skill_message(int dam, CHAR_DATA * ch, CHAR_DATA * vict, int attacktype, std::string add)
{
	int i, j, nr;
	struct message_type *msg;
	OBJ_DATA *weap = GET_EQ(ch, WEAR_WIELD) ?
		GET_EQ(ch, WEAR_WIELD) : GET_EQ(ch, WEAR_BOTHS);

	// log("[SKILL MESSAGE] Message for skill %d",attacktype);
	for (i = 0; i < MAX_MESSAGES; i++)
	{
		if (fight_messages[i].a_type == attacktype)
		{
			nr = dice(1, fight_messages[i].number_of_attacks);
			// log("[SKILL MESSAGE] %d(%d)",fight_messages[i].number_of_attacks,nr);
			for (j = 1, msg = fight_messages[i].msg; (j < nr) && msg; j++)
				msg = msg->next;

			weap = init_weap(ch, dam, attacktype);
			brief_shields brief(ch, vict, weap, add);
			if (attacktype == SPELL_FIRE_SHIELD
				|| attacktype == SPELL_MAGICGLASS)
			{
				brief.reflect = true;
			}

			if (!IS_NPC(vict) && (GET_LEVEL(vict) >= LVL_IMMORT))
			{
				switch (attacktype)
				{
				case SKILL_BACKSTAB + TYPE_HIT:
				case SKILL_THROW + TYPE_HIT:
				case SKILL_BASH + TYPE_HIT:
				case SKILL_KICK + TYPE_HIT:
					send_to_char("&W&q", ch);
					break;
				default:
					send_to_char("&y&q", ch);
					break;
				}
				// ch
				brief.act_to_char(msg->god_msg.attacker_msg);
				send_to_char("&Q&n", ch);
				// victim
				brief.act_to_vict(msg->god_msg.victim_msg);
				// room
				brief.act_to_room(msg->god_msg.room_msg);
			}
			else if (dam != 0)
			{
				if (GET_POS(vict) == POS_DEAD)
				{
					// ch
					send_to_char("&Y&q", ch);
					brief.act_to_char(msg->die_msg.attacker_msg);
					send_to_char("&Q&n", ch);
					// vict
					send_to_char("&R&q", vict);
					brief.act_to_vict(msg->die_msg.victim_msg);
					send_to_char("&Q&n", vict);
					// room
					brief.act_to_room(msg->die_msg.room_msg);
				}
				else
				{
					// ch
					send_to_char("&Y&q", ch);
					brief.act_to_char(msg->hit_msg.attacker_msg);
					send_to_char("&Q&n", ch);
					// vict
					send_to_char("&R&q", vict);
					brief.act_to_vict(msg->hit_msg.victim_msg);
					send_to_char("&Q&n", vict);
					// room
					brief.act_to_room(msg->hit_msg.room_msg);
				}
			}
			else if (ch != vict)  	// Dam == 0
			{
				switch (attacktype)
				{
				case SKILL_BACKSTAB + TYPE_HIT:
				case SKILL_THROW + TYPE_HIT:
				case SKILL_BASH + TYPE_HIT:
				case SKILL_KICK + TYPE_HIT:
					send_to_char("&W&q", ch);
					break;
				default:
					send_to_char("&y&q", ch);
					break;
				}
				//ch
				brief.act_to_char(msg->miss_msg.attacker_msg);
				send_to_char("&Q&n", ch);
				// vict
				send_to_char("&r&q", vict);
				brief.act_to_vict(msg->miss_msg.victim_msg);
				send_to_char("&Q&n", vict);
				// room
				brief.act_to_room(msg->miss_msg.room_msg);
			}
			return (1);
		}
	}
	return (0);
}

// *** This function return chance of skill
int calculate_skill(CHAR_DATA * ch, int skill_no, CHAR_DATA * vict)
{
	int skill_is, percent = 0, victim_sav = SAVING_REFLEX, victim_modi = 0; // ������� �������� ������(�������������) / ����������� �������� ������� / ����� ������,
                                                                    // ������� ������ �� ����������� ����� / ������ ������������, �������� �� �����������
	int morale, max_percent = 200, bonus = 0, size = 0;  // ����� ��������, ����������� ��������� ������� �����, ����� �� �������������� ����������.
	bool pass_mod = 0; // � ������ ������ ��� ���.��������, ����� �������� ��� ��� �����,
	// �� �� ��������� � ���� ����� ������� � ������, ��������� ������ �� ���� �������
	bool try_morale = false;
	bool absolute_fail = false;

	if (skill_no < 1 || skill_no > MAX_SKILL_NUM)  	// log("ERROR: ATTEMPT USING UNKNOWN SKILL <%d>", skill_no);
		return 0;
	if ((skill_is = ch->get_skill(skill_no)) <= 0)
		return 0;                                         // ���� ����� ��� ���������� 0.
	if (!IS_NPC(ch) && ch->affected)
	{
		AFFECT_DATA *aff = ch->affected;
		for (aff = ch->affected; aff; aff = aff->next)
		{
			if (aff->location == APPLY_BONUS_SKILLS) // ������ ������ � ���� �������
			skill_is +=  aff->modifier; 
		}
	}
	skill_is += int_app[GET_REAL_INT(ch)].to_skilluse;
        switch (skill_no)
	{
        if (!IS_NPC(ch))
        size = size_app[GET_POS_SIZE(ch)].interpolate;
        else
        size = size_app[GET_POS_SIZE(ch)].interpolate/2;

	case SKILL_HIDETRACK:          // ������� �����
		bonus = (can_use_feat(ch, STEALTHY_FEAT) ? 5 : 0);
		break;
	case SKILL_BACKSTAB:	//��������
                victim_sav = GET_SAVE(vict, SAVING_REFLEX) - dex_bonus(GET_REAL_DEX(vict));
		bonus = dex_bonus(GET_REAL_DEX(ch)) * 2;
		if (awake_others(ch) || equip_in_metall(ch))
			bonus -= 50;

		if (vict)
		{
			if (!CAN_SEE(vict, ch))
				bonus += 25;
			if (GET_POS(vict) < POS_FIGHTING)
				bonus += (20 * (POS_FIGHTING - GET_POS(vict)));
			else if (AFF_FLAGGED(vict, AFF_AWARNESS))
				victim_modi -= 30;
			victim_modi += size_app[GET_POS_SIZE(vict)].ac;
			victim_modi -= dex_bonus(GET_REAL_DEX(vict));
		}
		break;
	case SKILL_BASH:	//�����
		victim_sav = GET_SAVE(vict, SAVING_REFLEX) - dex_bonus(GET_REAL_DEX(vict));
		bonus = size + dex_bonus(GET_REAL_DEX(ch)) +
				  (GET_EQ(ch, WEAR_SHIELD) ?
				   weapon_app[MIN(35, MAX(0, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_SHIELD))))].
				   bashing : 0);
		if (vict)
		{         if (!IS_NPC(vict)) {
			victim_modi -= size_app[GET_POS_SIZE(vict)].interpolate;
			} else {
                        victim_modi -= (size_app[GET_POS_SIZE(vict)].interpolate)/2;
                        }
                        if (GET_POS(vict) < POS_FIGHTING && GET_POS(vict) > POS_SLEEPING)
				victim_modi -= 20;
			if (GET_AF_BATTLE(vict, EAF_AWAKE))
				victim_modi -= calculate_awake_mod(ch, vict);
			victim_modi -= GET_REAL_CON(vict);
		}
		break;
	case SKILL_HIDE:	//����������
		bonus = dex_bonus(GET_REAL_DEX(ch)) - size_app[GET_POS_SIZE(ch)].ac
			+ (can_use_feat(ch, STEALTHY_FEAT) ? 5 : 0);

		if (awake_others(ch) || equip_in_metall(ch))
			bonus -= 50;

		if (IS_DARK(IN_ROOM(ch)))
			bonus += 25;

		if (SECT(IN_ROOM(ch)) == SECT_INSIDE)
			bonus += 20;
		else if (SECT(IN_ROOM(ch)) == SECT_CITY)
			bonus -= 15;
		else if (SECT(IN_ROOM(ch)) == SECT_FOREST)
			bonus += 20;
		else if (SECT(IN_ROOM(ch)) == SECT_HILLS
				 || SECT(IN_ROOM(ch)) == SECT_MOUNTAIN) bonus += 10;

		if (vict)
		{
			if (AWAKE(vict))
				victim_modi -= int_app[GET_REAL_INT(vict)].observation;
		}
		break;
	case SKILL_KICK:	//�����
		victim_sav = GET_SAVE(vict, SAVING_STABILITY) - dex_bonus(GET_REAL_CON(vict));
		bonus = dex_bonus(GET_REAL_DEX(ch)) +
				  dex_bonus(GET_REAL_STR(ch));
		if (vict)
		{
			victim_modi += size_app[GET_POS_SIZE(vict)].interpolate;
			victim_modi -= GET_REAL_CON(vict);
			if (GET_AF_BATTLE(vict, EAF_AWAKE))
				victim_modi -= calculate_awake_mod(ch, vict);
		}
		break;
	case SKILL_PICK_LOCK:	// �����
		bonus = dex_bonus(GET_REAL_DEX(ch))
				  + (can_use_feat(ch, NIMBLE_FINGERS_FEAT) ? 5 : 0);
	        break;
	case SKILL_PUNCH:	//���� ����� �����
        victim_sav = GET_SAVE(vict, SAVING_REFLEX) - dex_bonus(GET_REAL_DEX(vict));
		break;
	case SKILL_RESCUE:	//������
		bonus = dex_bonus(GET_REAL_DEX(ch));
//		victim_modi = 100;
		break;
	case SKILL_SNEAK:	// �����������
		bonus = dex_bonus(GET_REAL_DEX(ch))
				  + (can_use_feat(ch, STEALTHY_FEAT) ? 10 : 0);

		if (awake_others(ch) || equip_in_metall(ch))
			bonus -= 50;

		if (SECT(IN_ROOM(ch)) == SECT_CITY)
			bonus -= 10;
		if (IS_DARK(IN_ROOM(ch)))
			bonus += 20;

		if (vict)
		{
			if (GET_LEVEL(vict) > 35)
				bonus -= 50;
			if (!CAN_SEE(vict, ch))
				bonus += 25;
			if (AWAKE(vict))
			{
				victim_modi -= int_app[GET_REAL_INT(vict)].observation;
			}
		}
		break;
	case SKILL_STEAL:	// �������
                victim_sav = GET_SAVE(vict, SAVING_REFLEX) - dex_bonus(GET_REAL_DEX(vict));
		bonus = dex_bonus(GET_REAL_DEX(ch))
				  + (can_use_feat(ch, NIMBLE_FINGERS_FEAT) ? 5 : 0);

		if (awake_others(ch) || equip_in_metall(ch))
			bonus -= 50;

		if (IS_DARK(IN_ROOM(ch)))
			bonus += 20;

		if (vict)
		{
			if (!CAN_SEE(vict, ch))
				bonus += 25;
			if (AWAKE(vict))
			{
				victim_modi -= int_app[GET_REAL_INT(vict)].observation;
				if (AFF_FLAGGED(vict, AFF_AWARNESS))
					bonus -= 30;
			}
		}
		break;
        case SKILL_TRACK:       //���������
                percent = skill_is + int_app[GET_REAL_INT(ch)].observation
                                  + (can_use_feat(ch, TRACKER_FEAT) ? 10 : 0);

                if (SECT(IN_ROOM(ch)) == SECT_FOREST || SECT(IN_ROOM(ch)) == SECT_FIELD)
                        percent += 10;

                percent = complex_skill_modifier(ch, SKILL_THAC0, GAPPLY_SKILL_SUCCESS, percent);

                if (SECT(IN_ROOM(ch)) == SECT_WATER_SWIM ||
                                SECT(IN_ROOM(ch)) == SECT_WATER_NOSWIM ||
                                SECT(IN_ROOM(ch)) == SECT_FLYING ||
                                SECT(IN_ROOM(ch)) == SECT_UNDERWATER ||
                                SECT(IN_ROOM(ch)) == SECT_SECRET
                                || ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOTRACK)) percent = 0;


                if (vict)
                {
                        victim_modi += GET_REAL_CON(vict) / 2;
                        if (AFF_FLAGGED(vict, AFF_NOTRACK)
                                        || ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOTRACK)) victim_modi = -100;
                }
                break;
        case SKILL_SENSE: // ������������
                percent = skill_is + int_app[GET_REAL_INT(ch)].observation
                                  + (can_use_feat(ch, TRACKER_FEAT) ? 10 : 0);

                percent =
                        complex_skill_modifier(ch, SKILL_THAC0, GAPPLY_SKILL_SUCCESS, percent);

                if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOTRACK))
                        percent = 0;

                if (vict)
                {
                        victim_modi += GET_REAL_CON(vict) / 2;
                        if (AFF_FLAGGED(vict, AFF_NOTRACK)
                                        || ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOTRACK)) victim_modi = -100;
                }
                break;

	case SKILL_MULTYPARRY:  // ������� ������
	case SKILL_PARRY:	//����������
                victim_sav = dex_bonus(GET_REAL_DEX(vict));
		bonus = dex_bonus(GET_REAL_DEX(ch));
		if (GET_AF_BATTLE(ch, EAF_AWAKE))
			bonus += ch->get_skill(SKILL_AWAKE);

		if (GET_EQ(ch, WEAR_HOLD)
				&& GET_OBJ_TYPE(GET_EQ(ch, WEAR_HOLD)) == ITEM_WEAPON)
		{
			bonus +=
				weapon_app[MAX
						   (0,
							MIN(50,
								GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_HOLD))))].
				parrying;
		}
		victim_modi = 100;
		break;

	case SKILL_BLOCK:	//��������� �����
{		// �� 10 ������� �� ���� (21-30) � ����� (21-50)
		int shield_mod = GET_EQ(ch, WEAR_SHIELD) ? MIN(10, MAX(0, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_SHIELD)) - 20)) : 0;
		int dex_mod = MAX(0, (GET_REAL_DEX(ch) - 20)/3);
		bonus = dex_mod + shield_mod;

	        break;
}

	case SKILL_TOUCH:	//��������� ����������
        victim_sav = dex_bonus(GET_REAL_DEX(vict));
	bonus = dex_bonus(GET_REAL_DEX(ch)) +
			size_app[GET_POS_SIZE(vict)].interpolate;

		if (vict)
		{
			victim_modi -= dex_bonus(GET_REAL_DEX(vict));
			victim_modi -= size_app[GET_POS_SIZE(vict)].interpolate;
		}
		break;

	case SKILL_PROTECT:	//�������� ������
		bonus = dex_bonus(GET_REAL_DEX(ch)) + size;

		victim_modi = 50;
		break;

	case SKILL_BOWS:	//����
		bonus = dex_bonus(GET_REAL_DEX(ch));
		break;
	case SKILL_BOTHHANDS:	//����������
	case SKILL_LONGS:	//������� ������
	case SKILL_SPADES:	//����� � ����
	case SKILL_SHORTS:	//�������� ������
	case SKILL_CLUBS:	//������ � ������
	case SKILL_PICK:	//�����������
	case SKILL_NONSTANDART:	//������������� ������
	case SKILL_AXES:	//������
//        victim_sav = GET_SAVE(vict, SAVING_REFLEX);
	break;
	case SKILL_SATTACK:	//����� ������ �����
        victim_sav = GET_SAVE(vict, SAVING_REFLEX);
	break;
	case SKILL_LOOKING:	//������������
		bonus = int_app[GET_REAL_INT(ch)].observation;
		break;
	case SKILL_HEARING:	//������������
		bonus = int_app[GET_REAL_INT(ch)].observation;
		break;
	case SKILL_DISARM:
		victim_sav = GET_SAVE(vict, SAVING_REFLEX) - dex_bonus(GET_REAL_DEX(vict));
		bonus = dex_bonus(GET_REAL_DEX(ch)) + dex_bonus(GET_REAL_STR(ch));
		if (vict)
		{
			victim_modi -= dex_bonus(GET_REAL_STR(ch));
			if (GET_EQ(vict, WEAR_BOTHS))
				victim_modi -= 10;
			if (GET_AF_BATTLE(vict, EAF_AWAKE))
				victim_modi -= calculate_awake_mod(ch, vict);
		}
		break;
	case SKILL_HEAL:   // ������
		break;
	case SKILL_ADDSHOT:   // �������������� �������
		if (equip_in_metall(ch))
			bonus -= 5;
		pass_mod = 1;
		break;
	case SKILL_NOPARRYHIT:
		bonus = dex_bonus(GET_REAL_DEX(ch));
		break;
	case SKILL_CAMOUFLAGE: // ����������
		bonus = dex_bonus(GET_REAL_DEX(ch)) - size_app[GET_POS_SIZE(ch)].ac
				  + (can_use_feat(ch, STEALTHY_FEAT) ? 5 : 0);

		if (awake_others(ch))
			bonus -= 100;

		if (IS_DARK(IN_ROOM(ch)))
			bonus += 15;

		if (SECT(IN_ROOM(ch)) == SECT_CITY)
			bonus -= 15;
		else if (SECT(IN_ROOM(ch)) == SECT_FOREST)
		        bonus += 10;
		else if (SECT(IN_ROOM(ch)) == SECT_HILLS
				 || SECT(IN_ROOM(ch)) == SECT_MOUNTAIN) bonus += 5;
		if (equip_in_metall(ch))
			bonus -= 30;

		if (vict)
		{
			if (AWAKE(vict))
				victim_modi -= int_app[GET_REAL_INT(vict)].observation;
		}
		break;
	case SKILL_DEVIATE:  // ����������
		bonus = - size_app[GET_POS_SIZE(ch)].ac +
			dex_bonus(GET_REAL_DEX(ch));

		if (equip_in_metall(ch))
			bonus -= 40;

		if (vict)
		{
			victim_modi -= dex_bonus(GET_REAL_DEX(vict));
		}
		break;
	case SKILL_CHOPOFF:  // ��������
		victim_sav = GET_SAVE(vict, SAVING_REFLEX) - dex_bonus(GET_REAL_DEX(vict));
		bonus = dex_bonus(GET_REAL_DEX(ch)) + size_app[GET_POS_SIZE(ch)].ac;

		if (equip_in_metall(ch))
			bonus -= 10;

		if (vict)
		{
			if (!CAN_SEE(vict, ch))
				bonus += 10;
			if (GET_POS(vict) < POS_SITTING)
				bonus -= 50;
			if (AWAKE(vict) || AFF_FLAGGED(vict, AFF_AWARNESS) || MOB_FLAGGED(vict, MOB_AWAKE))
				victim_modi -= 20;
// � ����� ����� �����
//			if (GET_AF_BATTLE(vict, EAF_AWAKE))
//				victim_modi -= calculate_awake_mod(ch, vict);
//			victim_modi -= int_app[GET_REAL_INT(vict)].observation;
		}
		break;
	case SKILL_REPAIR: // �������
		break;
	case SKILL_UPGRADE:
	case SKILL_WARCRY:
		break;
	case SKILL_COURAGE:
		break;
	case SKILL_SHIT:
		break;
	case SKILL_MIGHTHIT: // ����������� �����
		victim_sav = GET_SAVE(vict, SAVING_STABILITY) - dex_bonus(GET_REAL_CON(vict));
		bonus = size + dex_bonus(GET_REAL_STR(ch));

		if (IS_NPC(vict))
		{
			victim_modi -= (size_app[GET_POS_SIZE(vict)].shocking)/2;
		}
		else if (!IS_NPC(vict))
		{
                	victim_modi -= size_app[GET_POS_SIZE(vict)].shocking;
                }
               break;
	case SKILL_STUPOR:  // ��������
		victim_sav = GET_SAVE(vict, SAVING_STABILITY) - dex_bonus(GET_REAL_CON(vict));
		bonus = dex_bonus(GET_REAL_STR(ch));
		if (GET_EQ(ch, WEAR_WIELD))
			bonus +=
				weapon_app[GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD))].shocking;
		else if (GET_EQ(ch, WEAR_BOTHS))
			bonus +=
				weapon_app[GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_BOTHS))].shocking;

		if (vict)
		{
			victim_modi -= GET_REAL_CON(vict);
		}
		break;
	case SKILL_POISONED:  // ����������
		break;
	case SKILL_LEADERSHIP: //���������
		bonus = cha_app[GET_REAL_CHA(ch)].leadership;
		break;
	case SKILL_PUNCTUAL:  // ������ �����
		victim_sav = GET_SAVE(vict, SAVING_CRITICAL) - dex_bonus(GET_REAL_CON(vict));
		bonus = dex_bonus(GET_REAL_INT(ch));
		if (GET_EQ(ch, WEAR_WIELD))
			bonus += MAX(18, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD))) - 18
					   + MAX(25, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD))) - 25
					   + MAX(30, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD))) - 30;
		if (GET_EQ(ch, WEAR_HOLD))
			bonus += MAX(18, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_HOLD))) - 18
					   + MAX(25, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_HOLD))) - 25
					   + MAX(30, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_HOLD))) - 30;
		if (GET_EQ(ch, WEAR_BOTHS))
			bonus += MAX(25, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_BOTHS))) - 25
					   + MAX(30, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_BOTHS))) - 30;
		if (vict)
		{
			victim_modi -= int_app[GET_REAL_INT(vict)].observation;
		}
		break;
	case SKILL_AWAKE:  // ���������� �����
		bonus = int_app[GET_REAL_DEX(ch)].observation;

		if (vict)
		{
			victim_modi -= int_app[GET_REAL_INT(vict)].observation;
		}
		break;

	case SKILL_IDENTIFY:
		bonus = int_app[GET_REAL_INT(ch)].observation
				  + (can_use_feat(ch, CONNOISEUR_FEAT) ? 20 : 0);
		break;

	case SKILL_CREATE_POTION:
	case SKILL_CREATE_SCROLL:
	case SKILL_CREATE_WAND:
		break;
	case SKILL_LOOK_HIDE:
		bonus = cha_app[GET_REAL_CHA(ch)].illusive;
		if (vict)
		{
			if (!CAN_SEE(vict, ch))
				bonus += 50;
			else if (AWAKE(vict))
				victim_modi -= int_app[GET_REAL_INT(ch)].observation;
		}
		break;
	case SKILL_ARMORED:
		break;
	case SKILL_DRUNKOFF:
		bonus = - GET_REAL_CON(ch) / 2
				  + (can_use_feat(ch, DRUNKARD_FEAT) ? 20 : 0);
		break;
	case SKILL_AID:    // ������
		bonus = (can_use_feat(ch, HEALER_FEAT) ? 10 : 0);
		break;
	case SKILL_FIRE:
		if (get_room_sky(IN_ROOM(ch)) == SKY_RAINING)
			bonus -= 50;
		else if (get_room_sky(IN_ROOM(ch)) != SKY_LIGHTNING)
			bonus -= number(10, 25);
	case SKILL_HORSE: // �������� ����
		bonus = cha_app[GET_REAL_CHA(ch)].leadership;
		break;
	case SKILL_TURN_UNDEAD:  // ������� ������
		bonus = (can_use_feat(ch, EXORCIST_FEAT) ? 20 : 0);
		break;
	case SKILL_MORPH:
		break;
	case SKILL_STRANGLE: // �������
                victim_sav = GET_SAVE(vict, SAVING_REFLEX) -dex_bonus(GET_REAL_DEX(vict));
                bonus += MAX(0, dex_bonus(GET_REAL_DEX(ch)) - 25);
                pass_mod = 1;
             if (GET_MOB_HOLD(vict))
                bonus += (skill_is + bonus)/2;
             else {
                if (!CAN_SEE(ch,vict))
                bonus += (skill_is + bonus)/5;
                if (vict->get_fighting() ||
			(MOB_FLAGGED(vict, MOB_AWARE) || AFF_FLAGGED(vict, AFF_AWARNESS) || AWAKE(vict) ))
                        bonus -= (skill_is + bonus)/10;
                if (PRF_FLAGGED (vict, PRF_AWAKE))
			victim_modi = -(vict->get_skill(SKILL_AWAKE)/5);
                  }
                        default:
                        break;
			}
//        if(IS_NPC(ch))
//        bonus = 0;
        if ((skill_no == SKILL_SENSE) || (skill_no == SKILL_TRACK))
            return percent;
        else
            percent = skill_is + bonus + victim_sav + victim_modi/2;   // ���������� �������� ����������� �����

	// �� ��� ������ ���� �������������� ��-�� ������� �������� � ������
	if (!pass_mod)
	{
//		percent = complex_skill_modifier(ch, skill_no, GAPPLY_SKILL_SUCCESS, percent);
		morale = ch->calc_morale();

//		if (vict && percent > skill_info[skill_no].max_percent)
//			victim_modi += percent - skill_info[skill_no].max_percent;

		if (AFF_FLAGGED(ch, AFF_DEAFNESS))
			morale -= 20;	// � ������� ������ �� 20 ������
		// ��������� ����������� "������ ���"
		if (vict && can_use_feat(vict, SPIRIT_WARRIOR_FEAT))
			morale -= 10;

		const int prob = number(0, 999);

		int morale_bonus = morale;
		if (morale < 0) {
			morale_bonus = morale * 10;
		}
		const int bonus_limit = MIN(150, morale * 10);
		int fail_limit = MIN(990, 950 + morale_bonus * 10 / 6);
		// ���� prob �������� � ������������ [0, bonus_limit) - ����� � ���� ����. �������� �
		// ������ ����-�������, ���� � ������� [fail_limit, 999] - ����������� ��������. �����
		// ��� ������ ����-������.
		if (morale >= 50)   // �� 50 ����� ���������� ���� �� ��������
			fail_limit = 999;
		if (prob >= fail_limit) {   // ���������� ���� 4.9 ��������
			percent = 0;
			bool absolute_fail = true;
		} else if (prob < bonus_limit) {
//			percent = skill_info[skill_no].max_percent;
                        percent = max_percent + bonus;
						bool try_morale = true;
		}// else if (vict && general_savingthrow(ch, vict, victim_sav, victim_modi)) {
		//	percent = 0;
//		}
// ����������� �����.

		if ((skill_no == SKILL_CHOPOFF) && !IS_NPC(ch) && !IS_NPC(vict)){
		//send_to_char(vict, "���������� �����, ��������� %s: Skill == %d, Percent == %d,Prob == %d, fail_limit == %d, Morale == %d, Bonus_limit == %d\r\n", GET_NAME(vict), skill_is, percent, prob, fail_limit, morale_bonus, bonus_limit);
		//sprintf(buf, "����� %s. ��������� %s, �����: Skill == %d, Percent == %d,Prob == %d, fail_limit == %d, Morale == %d, Bonus_limit == %d\r\n",GET_NAME (ch), GET_NAME(vict), skill_is, percent, prob,fail_limit, morale_bonus, bonus_limit);
			log("����� %s. ��������� %s, �����: Skill == %d, Percent == %d,Prob == %d, fail_limit == %d, Morale == %d, Bonus_limit == %d\r\n",GET_NAME (ch), GET_NAME(vict), skill_is, percent, prob,fail_limit, morale_bonus, bonus_limit); }
	}

	// ������� ����� � ������ ������ �� ���
	if (IS_IMMORTAL(ch))
		percent = MAX(percent, skill_info[skill_no].max_percent);
	else if (GET_GOD_FLAG(ch, GF_GODSCURSE))
		percent = 0;
	else if (vict && GET_GOD_FLAG(vict, GF_GODSCURSE))
		percent = MAX(percent, max_percent);
	else
		percent = MIN(MAX(0, percent), max_percent);

	if (vict && IS_NPC(vict) && (skill_no == SKILL_BASH || skill_no == SKILL_STRANGLE || skill_no == SKILL_MIGHTHIT
		|| skill_no == SKILL_STUPOR || skill_no == SKILL_CHOPOFF || skill_no == SKILL_BACKSTAB || skill_no == SKILL_KICK
		|| skill_no == SKILL_PUNCTUAL) && GET_GOD_FLAG(ch, GF_TESTER))
	{
		//sprintf(buf, "��������� %s: ����� == %d, �������� == %d,����� == %d, ����� == %d, �����/2 == %d\r\n", GET_NAME(vict), skill_is, percent, bonus, victim_sav, victim_modi);
		//mudlog(buf, LGH, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
		if (absolute_fail)
			send_to_char(ch, "������ � ���������� ����\r\n");
		else if (try_morale)
			send_to_char(ch, "������ � �����. �������� prob = %d, ����� = %d, ����� = %d, ����� = %d, �����/2 = %d, ������ =%d\r\n", percent, skill_is, bonus, victim_sav, victim_modi, morale);
		else
			send_to_char(ch, "�������� prob = %d, ����� = %d, ����� = %d, ����� = %d, �����/2 = %d\r\n", percent, skill_is, bonus, victim_sav, victim_modi);
	}

	return (percent);
}

void improove_skill(CHAR_DATA * ch, int skill_no, int success, CHAR_DATA * victim)
{
	const int trained_skill = ch->get_trained_skill(skill_no);
	if (trained_skill == 0 || trained_skill == 200)
	{
		// ����� ����� � �� ����, ���� ������� ������ �� ������
		return;
	}

	int skill_is, diff = 0, prob, div;

	if (IS_NPC(ch))
		return;

	if (victim && (IS_HORSE(victim) || MOB_FLAGGED(victim, MOB_NOTRAIN)))
		return;

	if (IS_IMMORTAL(ch) ||
			((!victim ||
			  OK_GAIN_EXP(ch, victim)) && IN_ROOM(ch) != NOWHERE
			 && !ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL) &&
// �������
			 !ROOM_FLAGGED(IN_ROOM(ch), ROOM_ARENA) &&
//���������
			 !ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE)
			 && !ROOM_FLAGGED(IN_ROOM(ch), ROOM_ATRIUM) &&
//
			 (diff =
				  wis_bonus(GET_REAL_WIS(ch), WIS_MAX_LEARN_L20) * GET_LEVEL(ch) / 20 -
				  trained_skill) > 0
			 && trained_skill < MAX_EXP_RMRT_PERCENT(ch)))
	{
		// Success - multy by 2
		prob = success ? 20000 : 15000;

		div = int_app[GET_REAL_INT(ch)].improove /* + diff */ ;

		if ((int) GET_CLASS(ch) >= 0 && (int) GET_CLASS(ch) < NUM_PLAYER_CLASSES)
			div += (skill_info[skill_no].k_improove[(int) GET_CLASS(ch)][(int) GET_KIN(ch)] / 100);

		prob /= (MAX(1, div));
		// ������� ������ �������� ��� ������� �� ���-�� ������
		prob -= 5 * wis_bonus(GET_REAL_WIS(ch), WIS_MAX_SKILLS);
		prob += number(1, trained_skill * 5);

		skill_is = number(1, MAX(1, prob));

		// if (!IS_NPC(ch))
//        log("Player %s skill '%d' - need to improove %d(%d-%d)",
//            GET_NAME(ch), skill_no, skill_is, div, prob);
                if ((skill_no == SKILL_STEAL) && (!IS_NPC(victim)))
                   return;
		if (
			(victim
			 && skill_is <= GET_REAL_INT(ch) * GET_LEVEL(victim) / GET_LEVEL(ch))
			|| (!victim && skill_is <= GET_REAL_INT(ch)))
		{
			if (success)
				sprintf(buf, "%s�� �������� ������� ������ \"%s\".%s\r\n",
						CCICYN(ch, C_NRM), skill_name(skill_no), CCNRM(ch,
								C_NRM));
			else
				sprintf(buf,
						"%s����� ���� ������, �� �������� ������� ������ \"%s\".%s\r\n",
						CCICYN(ch, C_NRM), skill_name(skill_no), CCNRM(ch,
								C_NRM));
			send_to_char(buf, ch);
			ch->set_morphed_skill(skill_no, (trained_skill + number(1, 2)));
			if (!IS_IMMORTAL(ch))
				ch->set_morphed_skill(skill_no, (MIN(MAX_EXP_PERCENT + GET_REMORT(ch) * 5, ch->get_trained_skill(skill_no))));
// ����� ����������, ������� ���� (���� �� ����)
			if (victim && IS_NPC(victim))
				SET_BIT(MOB_FLAGS(victim, MOB_NOTRAIN), MOB_NOTRAIN);
		}
	}
}


int train_skill(CHAR_DATA * ch, int skill_no, int max_value, CHAR_DATA * vict)
{
	int percent = 0;

	percent = calculate_skill(ch, skill_no, vict);
	if (!IS_NPC(ch))
	{
		if (skill_no != SKILL_SATTACK &&
				ch->get_trained_skill(skill_no) > 0 && (!vict
												|| (IS_NPC(vict)
													&& !MOB_FLAGGED(vict, MOB_PROTECT)
													&& !MOB_FLAGGED(vict, MOB_NOTRAIN)
													&& !AFF_FLAGGED(vict, AFF_CHARM)
													&& !IS_HORSE(vict))))
			improove_skill(ch, skill_no, percent >= max_value, vict);
	}
	else if (!IS_CHARMICE(ch))
		if (ch->get_skill(skill_no) > 0 &&
				GET_REAL_INT(ch) <= number(0, 1000 - 20 * GET_REAL_WIS(ch)) &&
				ch->get_skill(skill_no) < skill_info[skill_no].max_percent)
			ch->set_skill(skill_no, ch->get_skill(skill_no) + 1);

	return (percent);
}

/**
* ������ ������� ��������� � victim ������ ������ killer.
* � ������ ������ ����������� ������ '���� ������ ������', ��� ��������� ���������
* ��� �����/2.5, �.�. 80% �� ����� �� ��� �� ������/������ ��� ����������� ���������� �������.
* � ����� � �������� �� ���������� ���� ���. ����� �� ��������� ���� 80�� (����� �� ������� �� ���).
*/
int calculate_awake_mod(CHAR_DATA *killer, CHAR_DATA *victim)
{
	int result = 0;
	if (!killer || !victim)
		log("SYSERROR: zero character in calculate_awake_mod.");
	else if (IS_NPC(killer) || IS_NPC(victim))
	{
		// ���� ��� ����, �� �� ����� �������� ������, ��� �����
		if (!can_use_feat(victim, SHADOW_STRIKE_FEAT))
		    result = victim->get_skill(SKILL_AWAKE) / 2;
		else
		    result = victim->get_skill(SKILL_AWAKE);
	}
	else
	{
		// ������ ������ ��� ������ �������� � ������ � �� ? ������� - �� ���������
		result = victim->get_skill(SKILL_AWAKE) / 2;
	}
	return result;
}

/*
 * ������� ����������, ����� �� �������� ������� �����
 * ������������ �� ����� ����� ��������� � ����� "���� � ������ ���������"
 */
int min_skill_level(CHAR_DATA *ch, int skill)
{
	return (skill_info[skill].min_level[ch->get_class()][ch->get_kin()] -
		(MAX(0,ch->get_remort()/skill_info[skill].level_decrement[ch->get_class()][ch->get_kin()])));
};

bool can_get_skill(CHAR_DATA *ch, int skill)
{
	if (ch->get_remort() < skill_info[skill].min_remort[ch->get_class()][ch->get_kin()] ||
		(skill_info[skill].classknow[ch->get_class()][ch->get_kin()] != KNOW_SKILL))
		return FALSE;
	if (ch->get_level() < min_skill_level(ch, skill))
		return FALSE;

	return TRUE;
};


//  ���������� ������ Skill

//��������� ���������� ��������
SkillListType Skill::SkillList;

// �����������
Skill::Skill() : _Name(SKILL_NAME_UNDEFINED), _Number(SKILL_UNDEFINED), _MaxPercent(0)
{
// ������������� �� ����� ��������
};

// ������ ���� �������� ������
void Skill::ParseSkill(pugi::xml_node SkillNode)
{
    std::string SkillID;
    pugi::xml_node CurNode;
    SkillPtr TmpSkill(new Skill);

    // ������� ��������� ������ (� ���� ���� ������ � ���)
    SkillID = SkillNode.attribute("id").value();

    CurNode = SkillNode.child("number");
	TmpSkill->_Number = CurNode.attribute("val").as_int();
    CurNode = SkillNode.child("name");
	TmpSkill->_Name = CurNode.attribute("text").value();
	CurNode = SkillNode.child("max_percent");
	TmpSkill->_MaxPercent = CurNode.attribute("val").as_int();

	// ��������� ����� ����� � ����
    Skill::SkillList.insert(make_pair(SkillID, TmpSkill));
};

// ������� �������
// �������� � ��������� �������, �����, ���� ��� �������� ������ XML ����, �� ���������� ������ �����
void Skill::Load(pugi::xml_node XMLSkillList)
{
    pugi::xml_node CurNode;

	for (CurNode = XMLSkillList.child("skill"); CurNode; CurNode = CurNode.next_sibling("skill"))
	{
		Skill::ParseSkill(CurNode);
	}
};

// �������� ����� ������ �� ��� ���
// ������� ������������� �� ������ �����
int Skill::GetNumByID(std::string ID)
{
    SkillPtr TmpSkill = Skill::SkillList[ID];
    //SkillPtr TmpSkill = Skill::SkillList.find(ID);
    if (TmpSkill)
        return TmpSkill->_Number ;

    return SKILL_UNDEFINED;
};

// ����� (���) ���������� ������ Skill

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
