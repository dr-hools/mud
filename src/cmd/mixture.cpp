#include "mixture.h"

#include "game_magic/spells.h"
#include "game_magic/magic_utils.h"
#include "handler.h"
#include "administration/privilege.h"
#include "game_magic/spells_info.h"

void do_mixture(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	if (ch->IsNpc())
		return;
	if (IS_IMMORTAL(ch) && !privilege::CheckFlag(ch, privilege::kUseSkills)) {
		SendMsgToChar("Не положено...\r\n", ch);
		return;
	}

	CharData *tch;
	ObjData *tobj;
	RoomData *troom;
	char *s, *t;
	int spellnum, target = 0;

	// get: blank, spell name, target name
	s = strtok(argument, "'*!");
	if (!s) {
		if (subcmd == SCMD_RUNES)
			SendMsgToChar("Что вы хотите сложить?\r\n", ch);
		else if (subcmd == SCMD_ITEMS)
			SendMsgToChar("Что вы хотите смешать?\r\n", ch);
		return;
	}
	s = strtok(nullptr, "'*!");
	if (!s) {
		SendMsgToChar("Название вызываемой магии смеси должно быть заключено в символы : ' или * или !\r\n", ch);
		return;
	}
	t = strtok(nullptr, "\0");

	spellnum = FixNameAndFindSpellNum(s);

	if (spellnum < 1 || spellnum > kSpellCount) {
		SendMsgToChar("И откуда вы набрались рецептов?\r\n", ch);
		return;
	}

	if (((!IS_SET(GET_SPELL_TYPE(ch, spellnum), kSpellItems)
		&& subcmd == SCMD_ITEMS)
		|| (!IS_SET(GET_SPELL_TYPE(ch, spellnum), kSpellRunes)
			&& subcmd == SCMD_RUNES)) && !IS_GOD(ch)) {
		SendMsgToChar("Это блюдо вам явно не понравится.\r\n" "Научитесь его правильно готовить.\r\n", ch);
		return;
	}

	if (!CheckRecipeValues(ch, spellnum, subcmd == SCMD_ITEMS ? kSpellItems : kSpellRunes, false))
		return;

	if (!CheckRecipeItems(ch, spellnum, subcmd == SCMD_ITEMS ? kSpellItems : kSpellRunes, false)) {
		if (subcmd == SCMD_ITEMS)
			SendMsgToChar("У вас нет нужных ингредиентов!\r\n", ch);
		else if (subcmd == SCMD_RUNES)
			SendMsgToChar("У вас нет нужных рун!\r\n", ch);
		return;
	}

	// Find the target
	if (t != nullptr)
		one_argument(t, arg);
	else
		*arg = '\0';

	target = FindCastTarget(spellnum, arg, ch, &tch, &tobj, &troom);

	if (target && (tch == ch) && spell_info[spellnum].violent) {
		SendMsgToChar("Лекари не рекомендуют использовать ЭТО на себя!\r\n", ch);
		return;
	}

	if (!target) {
		SendMsgToChar("Тяжеловато найти цель для вашей магии!\r\n", ch);
		return;
	}

	if (tch != ch && !IS_IMMORTAL(ch) && IS_SET(spell_info[spellnum].targets, kTarSelfOnly)) {
		SendMsgToChar("Вы можете колдовать это только на себя!\r\n", ch);
		return;
	}

	if (IS_MANA_CASTER(ch)) {
		if (GetRealLevel(ch) < CalcRequiredLevel(ch, spellnum)) {
			SendMsgToChar("Вы еще слишком малы, чтобы колдовать такое.\r\n", ch);
			return;
		}

		if (ch->mem_queue.stored < GET_MANA_COST(ch, spellnum)) {
			SendMsgToChar("У вас маловато магической энергии!\r\n", ch);
			return;
		} else {
			ch->mem_queue.stored = ch->mem_queue.stored - GET_MANA_COST(ch, spellnum);
		}
	}

	if (CheckRecipeItems(ch, spellnum, subcmd == SCMD_ITEMS ? kSpellItems : kSpellRunes, true, tch)) {
		if (!CalcCastSuccess(ch, tch, ESaving::kStability, spellnum)) {
			SetWaitState(ch, kPulseViolence);
			if (!tch || !SendSkillMessages(0, ch, tch, spellnum)) {
				if (subcmd == SCMD_ITEMS)
					SendMsgToChar("Вы неправильно смешали ингредиенты!\r\n", ch);
				else if (subcmd == SCMD_RUNES)
					SendMsgToChar("Вы не смогли правильно истолковать значение рун!\r\n", ch);
			}
		} else {
			if (CallMagic(ch, tch, tobj, world[ch->in_room], spellnum, GetRealLevel(ch)) >= 0) {
				if (!(IS_IMMORTAL(ch) || ch->get_wait() > 0 ))
					SetWaitState(ch, kPulseViolence);
			}
		}
	}
}
