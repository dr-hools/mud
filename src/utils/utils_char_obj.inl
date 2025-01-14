#ifndef __CHAR_OBJ_UTILS_HPP__
#define __CHAR_OBJ_UTILS_HPP__

#include "structs/structs.h"
#include "entities/char_data.h"
#include "entities/obj_data.h"
#include "utils.h"

inline bool INVIS_OK_OBJ(const CharData *sub, const ObjData *obj) {
	return !obj->has_flag(EObjFlag::kInvisible)
		|| AFF_FLAGGED(sub, EAffect::kDetectInvisible);
}

inline bool MORT_CAN_SEE_OBJ(const CharData *sub, const ObjData *obj) {
	return INVIS_OK_OBJ(sub, obj)
		&& !AFF_FLAGGED(sub, EAffect::kBlind)
		&& (!is_dark(obj->get_in_room())
			|| obj->has_flag(EObjFlag::kGlow)
			|| (IS_CORPSE(obj)
				&& AFF_FLAGGED(sub, EAffect::kInfravision))
			|| IsAbleToUseFeat(sub, EFeat::kDarkReading));
}

inline bool CAN_SEE_OBJ(const CharData *sub, const ObjData *obj) {
	return (obj->get_worn_by() == sub
		|| obj->get_carried_by() == sub
		|| (obj->get_in_obj()
			&& (obj->get_in_obj()->get_worn_by() == sub
				|| obj->get_in_obj()->get_carried_by() == sub))
		|| MORT_CAN_SEE_OBJ(sub, obj)
		|| (!sub->IsNpc()
			&& PRF_FLAGGED((sub), EPrf::kHolylight)));
}

inline const char *OBJN(const ObjData *obj, const CharData *vict, const size_t pad) {
	return CAN_SEE_OBJ(vict, obj)
		   ? (!obj->get_PName(pad).empty()
			  ? obj->get_PName(pad).c_str()
			  : obj->get_short_description().c_str())
		   : GET_PAD_OBJ(pad);
}

inline const char *OBJS(const ObjData *obj, const CharData *vict) {
	return CAN_SEE_OBJ(vict, obj) ? obj->get_short_description().c_str() : "что-то";
}

inline bool CAN_GET_OBJ(const CharData *ch, const ObjData *obj) {
	return (CAN_WEAR(obj, EWearFlag::kTake)
		&& CAN_CARRY_OBJ(ch, obj)
		&& CAN_SEE_OBJ(ch, obj))
		&& !(ch->IsNpc()
			&& obj->has_flag(EObjFlag::kBloody));
}

#endif // __CHAR_OBJ_UTILS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :