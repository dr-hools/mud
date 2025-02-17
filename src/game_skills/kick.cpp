#include "kick.h"

#include "game_fight/pk.h"
#include "game_fight/fight.h"
#include "game_fight/fight_hit.h"
#include "game_fight/common.h"
#include "protect.h"
#include "structs/global_objects.h"

// ******************  KICK PROCEDURES
void go_kick(CharData *ch, CharData *vict) {
	const char *to_char = nullptr, *to_vict = nullptr, *to_room = nullptr;

	if (IsUnableToAct(ch)) {
		SendMsgToChar("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}
	if (ch->HasCooldown(ESkill::kKick)) {
		SendMsgToChar("Вы уже все ноги себе отбили, отдохните слегка.\r\n", ch);
		return;
	};

	vict = TryToFindProtector(vict, ch);

	bool success = false;
	if (PRF_FLAGGED(ch, EPrf::kTester)) {
		SkillRollResult result = MakeSkillTest(ch, ESkill::kKick, vict);
		success = result.success;
	} else {
		int percent = ((10 - (compute_armor_class(vict) / 10)) * 2) + number(1, MUD::Skills()[ESkill::kKick].difficulty);
		int prob = CalcCurrentSkill(ch, ESkill::kKick, vict);
		if (GET_GOD_FLAG(vict, EGf::kGodscurse) || AFF_FLAGGED(vict, EAffect::kHold)) {
			prob = percent;
		}
		if (GET_GOD_FLAG(ch, EGf::kGodscurse) || (!ch->IsOnHorse() && vict->IsOnHorse())) {
			prob = 0;
		}
		if (check_spell_on_player(ch, kSpellWeb)) {
			prob /= 3;
		}
		success = percent <= prob;
		SendSkillBalanceMsg(ch, MUD::Skills()[ESkill::kKick].name, percent, prob, success);
	}

	TrainSkill(ch, ESkill::kKick, success, vict);
	int cooldown = 2;
	if (!success) {
		Damage dmg(SkillDmg(ESkill::kKick), fight::kZeroDmg, fight::kPhysDmg, nullptr);
		dmg.Process(ch, vict);
		cooldown = 2;
	} else {
		int dam = str_bonus(GET_REAL_STR(ch), STR_TO_DAM) + GetRealDamroll(ch) + GetRealLevel(ch) / 6;
		if (!ch->IsNpc() || (ch->IsNpc() && GET_EQ(ch, EEquipPos::kFeet))) {
			int modi = MAX(0, (ch->get_skill(ESkill::kKick) + 4) / 5);
			dam += number(0, modi * 2);
			modi = 5 * (10 + (GET_EQ(ch, EEquipPos::kFeet) ? GET_OBJ_WEIGHT(GET_EQ(ch, EEquipPos::kFeet)) : 0));
			dam = modi * dam / 100;
		}
		if (ch->IsOnHorse() && (ch->get_skill(ESkill::kRiding) >= 150) && (ch->get_skill(ESkill::kKick) >= 150)) {
			Affect<EApply> af;
			af.location = EApply::kNone;
			af.type = kSpellBattle;
			af.modifier = 0;
			af.battleflag = 0;
			float modi = ((ch->get_skill(ESkill::kKick) + GET_REAL_STR(ch) * 5)
				+ (GET_EQ(ch, EEquipPos::kFeet) ? GET_OBJ_WEIGHT(GET_EQ(ch, EEquipPos::kFeet)) : 0) * 3) / float(GET_SIZE(vict));
			if (number(1, 1000) < modi * 10) {
				switch (number(0, (ch->get_skill(ESkill::kKick) - 150) / 10)) {
					case 0:
					case 1:
						if (!AFF_FLAGGED(vict, EAffect::kStopRight)) {
							to_char = "Каблук вашего сапога надолго запомнится $N2, если конечно он выживет.";
							to_vict = "Мощный удар ноги $n1 изуродовал вам правую руку.";
							to_room = "След сапога $n1 надолго запомнится $N2, если конечно он$Q выживет.";
							af.type = kSpellBattle;
							af.bitvector = to_underlying(EAffect::kStopRight);
							af.duration = CalcDuration(vict, 3 + GET_REAL_REMORT(ch) / 4, 0, 0, 0, 0);
							af.battleflag = kAfBattledec | kAfPulsedec;
						} else if (!AFF_FLAGGED(vict, EAffect::kStopLeft)) {
							to_char = "Каблук вашего сапога надолго запомнится $N2, если конечно он выживет.";
							to_vict = "Мощный удар ноги $n1 изуродовал вам левую руку.";
							to_room = "След сапога $n1 надолго запомнится $N2, если конечно он выживет.";
							af.bitvector = to_underlying(EAffect::kStopLeft);
							af.duration = CalcDuration(vict, 3 + GET_REAL_REMORT(ch) / 4, 0, 0, 0, 0);
							af.battleflag = kAfBattledec | kAfPulsedec;
						} else {
							to_char = "Каблук вашего сапога надолго запомнится $N2, $M теперь даже бить вас нечем.";
							to_vict = "Мощный удар ноги $n1 вывел вас из строя.";
							to_room = "Каблук сапога $n1 надолго запомнится $N2, $M теперь даже биться нечем.";
							af.bitvector = to_underlying(EAffect::kStopFight);
							af.duration = CalcDuration(vict, 3 + GET_REAL_REMORT(ch) / 4, 0, 0, 0, 0);
							af.battleflag = kAfBattledec | kAfPulsedec;
						}
						dam *= 2;
						break;
					case 2:
					case 3:to_char = "Сильно пнув в челюсть, вы заставили $N3 замолчать.";
						to_vict = "Мощный удар ноги $n1 попал вам точно в челюсть, заставив вас замолчать.";
						to_room = "Сильно пнув ногой в челюсть $N3, $n заставил$q $S замолчать.";
						af.type = kSpellBattle;
						af.bitvector = to_underlying(EAffect::kSilence);
						af.duration = CalcDuration(vict, 3 + GET_REAL_REMORT(ch) / 5, 0, 0, 0, 0);
						af.battleflag = kAfBattledec | kAfPulsedec;
						dam *= 2;
						break;
					case 4:
					case 5:SetWaitState(vict, number(2, 5) * kPulseViolence);
						if (GET_POS(vict) > EPosition::kSit) {
							GET_POS(vict) = EPosition::kSit;
						}
						to_char = "Ваш мощный пинок выбил пару зубов $N2, усадив $S на землю!";
						to_vict = "Мощный удар ноги $n1 попал точно в голову, свалив вас с ног.";
						to_room = "Мощный пинок $n1 выбил пару зубов $N2, усадив $S на землю!";
						dam *= 2;
						break;
					default:break;
				}
			} else if (number(1, 1000) < (ch->get_skill(ESkill::kRiding) / 2)) {
				dam *= 2;
				if (!ch->IsNpc())
					SendMsgToChar("Вы привстали на стременах.\r\n", ch);
			}

			if (to_char) {
				if (!ch->IsNpc()) {
					sprintf(buf, "&G&q%s&Q&n", to_char);
					act(buf, false, ch, nullptr, vict, kToChar);
					sprintf(buf, "%s", to_room);
					act(buf, true, ch, nullptr, vict, kToNotVict | kToArenaListen);
				}
			}
			if (to_vict) {
				if (!vict->IsNpc()) {
					sprintf(buf, "&R&q%s&Q&n", to_vict);
					act(buf, false, ch, nullptr, vict, kToVict);
				}
			}
			ImposeAffect(vict, af, true, false, true, false);
		}

		if (GET_AF_BATTLE(vict, kEafAwake)) {
			dam >>= (2 - (ch->IsOnHorse() ? 1 : 0));
		}
		Damage dmg(SkillDmg(ESkill::kKick), dam, fight::kPhysDmg, nullptr);
		dmg.Process(ch, vict);
		cooldown = 2;
	}
	SetSkillCooldownInFight(ch, ESkill::kKick, cooldown);
	SetSkillCooldownInFight(ch, ESkill::kGlobalCooldown, 1);
}

void do_kick(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->get_skill(ESkill::kKick) < 1) {
		SendMsgToChar("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->HasCooldown(ESkill::kKick)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	CharData *vict = FindVictim(ch, argument);
	if (!vict) {
		SendMsgToChar("Кто это так сильно путается под вашими ногами?\r\n", ch);
		return;
	}

	if (vict == ch) {
		SendMsgToChar("Вы БОЛЬНО пнули себя! Поздравляю, вы сошли с ума...\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict, argument))
		return;
	if (!check_pkill(ch, vict, arg))
		return;

	if (IS_IMPL(ch) || !ch->GetEnemy()) {
		go_kick(ch, vict);
	} else if (IsHaveNoExtraAttack(ch)) {
		act("Хорошо. Вы попытаетесь пнуть $N3.", false, ch, nullptr, vict, kToChar);
		ch->SetExtraAttack(kExtraAttackKick, vict);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
