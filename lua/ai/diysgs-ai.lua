sgs.ai_skill_invoke.yaojiazi = function(self)
	if self:needKongcheng(self.player, true) and self.player:getHandcardNum() == 0 then return false end
	return true
end

local yaotaiping_skill = {}
yaotaiping_skill.name = "yaotaiping"
table.insert(sgs.ai_skills, yaotaiping_skill)
yaotaiping_skill.getTurnUseCard = function(self)
	local first_found, second_found = false, false
	local first_card, second_card
	if self.player:getHandcardNum() >= 2 then
		local cards = self.player:getHandcards()
		local same_suit = false
		cards = sgs.QList2Table(cards)
		self:sortByKeepValue(cards)
		for _, fcard in ipairs(cards) do
			if (fcard:getSuit() == sgs.Card_Diamond and self:willUseGodSalvation(fcard)) or 
			   (fcard:getSuit() == sgs.Card_Club and self:willUseAmazingGrace(fcard)) or
			   ((fcard:getSuit() == sgs.Card_Heart or fcard:getSuit() == sgs.Card_Spade) and self:willUseAOEAttack(fcard)) then
				
				local fvalueCard = (fcard:isKindOf("Peach") or fcard:isKindOf("ExNihilo") or fcard:isKindOf("SavageAssault") or fcard:isKindOf("ArcheryAttack"))
				if not fvalueCard then
					first_card = fcard
					first_found = true
					for _, scard in ipairs(cards) do
						local svalueCard = (scard:isKindOf("Peach") or scard:isKindOf("ExNihilo") or scard:isKindOf("SavageAssault") or scard:isKindOf("ArcheryAttack"))
						if first_card ~= scard and scard:getSuit() == first_card:getSuit()
							and not svalueCard then
							second_card = scard
							second_found = true
							break
						end
					end
					if second_card then break end
				end
				
			end
		end
	end

	if first_found and second_found then
		local first_id = first_card:getId()
		local second_id = second_card:getId()
		local card_str
		if first_card:getSuit() == sgs.Card_Heart then
            card_str = ("archery_attack:yaotaiping[%s:%s]=%d+%d"):format("to_be_decided", 0, first_id, second_id)
        end
        if first_card:getSuit() == sgs.Card_Spade then
            card_str = ("savage_assault:yaotaiping[%s:%s]=%d+%d"):format("to_be_decided", 0, first_id, second_id)
        end
        if first_card:getSuit() == sgs.Card_Diamond  then
            card_str = ("amazing_grace:yaotaiping[%s:%s]=%d+%d"):format("to_be_decided", 0, first_id, second_id)
        end
        if first_card:getSuit() == sgs.Card_Club then
            card_str = ("god_salvation:yaotaiping[%s:%s]=%d+%d"):format("to_be_decided", 0, first_id, second_id)
        end
		local aoecard = sgs.Card_Parse(card_str)
		assert(aoecard)
		return aoecard
	end
end

sgs.ai_skill_invoke.yaotuzhong = function(self)
	self:sort(self.friends, "hp")
	local kingdom = self.player:getKingdom()
	for _, friend in ipairs(self.friends) do
		if friend:getHp() < friend:getMaxHp() and friend:getKingdom() == kingdom then
			if friend:getHp() <= 2 then return true end
		end
	end
	return false
end

sgs.ai_skill_playerchosen.yaotuzhong = function(self, targets)
	for _, target in sgs.qlist(targets) do
		if self:isFriend(target) and target:getHp() <= 2 then return target end
	end
	return nil
end
