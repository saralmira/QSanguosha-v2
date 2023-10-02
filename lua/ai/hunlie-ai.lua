local skyanmie_skill = {}
skyanmie_skill.name = "skyanmie"
table.insert(sgs.ai_skills, skyanmie_skill)

sgs.ai_skill_invoke.skyanmie = function(self, effect)
	return true
end

sgs.ai_get_skyanmie_card = function(self)
	local cards = sgs.QList2Table(self.player:getHandcards())
    self:sortByKeepValue(cards)
	for _, card in ipairs(cards) do
		if card:getSuit() == sgs.Card_Spade and not self.player:isJilei(card) then
			return card
		end
	end
	for _, equip in sgs.qlist(self.player:getEquips()) do
		if equip:getSuit() == sgs.Card_Spade and not self.player:isJilei(equip) then
			return equip
		end
	end
	return nil
end

sgs.ai_get_skyanmie_target = function(self)
	local arr = {"guixin", "fangzhu"}
	self:sort(self.enemies, "handcard", true)
	for _, enemy in ipairs(self.enemies) do
		if (not enemy:hasSkills(table.concat(arr, "|")) and enemy:getHandcardNum() > 1) or enemy:getHandcardNum() > 2 then
			return enemy
		end
	end
	local arr2 = {"yiji", "jieming", "guixin"}
	self:sort(self.friends_noself, "defense", true)
	for _, friend in ipairs(self.friends_noself) do
		if friend:hasSkills(table.concat(arr2, "|")) and friend:getHandcardNum() < 3 and friend:getHp() > 2 and math.random(0, 10) > 6 then
			return friend
		end
	end
	return nil
end

skyanmie_skill.getTurnUseCard = function(self)
	local card = sgs.ai_get_skyanmie_card(self)
	if card == nil then
		return nil
	end
	
	--local target = sgs.ai_get_skyanmie_target(self)
	--if target == nil then
	--	return nil
	--end
	
	return sgs.Card_Parse("@SKYanmieCard=.")
end

sgs.ai_skill_use_func.SKYanmieCard = function(card, use, self)
	local card_turn = sgs.ai_get_skyanmie_card(self)
	if card_turn == nil then return end
	local target = sgs.ai_get_skyanmie_target(self)
	if target == nil then return end
	use.card = sgs.Card_Parse("@SKYanmieCard=" .. card_turn:getEffectiveId())
	--use.card = sgs.Card_Parse("@SKYanmieCard=%d->%s"):format(card_turn:getEffectiveId(), target:objectName())
	if use.to then use.to:append(target) end
end

sgs.ai_use_value.SKYanmieCard = 8.0
sgs.ai_use_priority.SKYanmieCard = 4.0
sgs.ai_card_intention.SKYanmieCard = 100

sgs.ai_skill_invoke.skshunshi = function(self)
	return true
end

sgs.ai_skill_playerschosen.skshunshi = function(self, targets)
	
	local targets_tb = sgs.QList2Table(targets)
	local targets_choose = {}
	local goodvalue = self.player:getTag("SKShunshiGoodValue"):toBool()
	
	if goodvalue == true then
		for _, p in ipairs(targets_tb) do
			if self:isFriend(p) then
				table.insert(targets_choose, p)
			end
		end
	else
		local arr = {"guixin", "yiji", "fangzhu"}
		local skills_arr = table.concat(arr, "|")
		for _, p in ipairs(targets_tb) do
			if self:isEnemy(p) then
				if not p:hasSkills(skills_arr) or math.random(0, 9) > 5 or self:needKongcheng(p, true) then table.insert(targets_choose, p) end
			elseif self:isFriend(p) then
				if (p:hasSkills(skills_arr) and p:getHp() > 2) or p:getHp() > getBestHp(p) then
					table.insert(targets_choose, p)
				end
			end
		end
	end
	
	return targets_choose
end

sgs.ai_playerschosen_intention.skshunshi = function(self, from, to)
	local goodvalue = self.player:getTag("SKShunshiGoodValue"):toBool()
	if goodvalue == true then
		return -10
	end
	return 0
end

sgs.ai_skfenying_get_targets = function(self)
	local targets = {}
	local player = self.room:findPlayerBySkillName("skfenying")
	if not player then return targets end
	
	local target_org = findPlayerByObjectName(self.room, player:property("SKFenyingTarget"):toString(), false)
	if not target_org then return targets end
	
	local dist = player:property("SKFenyingDistance"):toInt()
	for _, enemy in ipairs(self.enemies) do
		if target_org:distanceTo(enemy) <= dist then table.insert(targets, enemy) end
	end
	return targets
end

sgs.ai_skfenying_should_use = function(self, target)
	if not self:isEnemy(target) then return false end
	
	local cards = sgs.QList2Table(self.player:getHandcards())
    self:sortByKeepValue(cards)
	local dmg = self.player:getTag("SKFenyingDamage"):toInt()
	local red_cards = 0
	for _, card in ipairs(cards) do
		if card:isRed() and not self.player:isJilei(card) then
			red_cards = red_cards + dmg
		end
	end
	for _, equip in sgs.qlist(self.player:getEquips()) do
		if equip:isRed() and not self.player:isJilei(equip) then
			red_cards = red_cards + dmg
		end
	end
	
	if target:getHp() <= red_cards then return true end
	local hand_cards = self.player:getHandcardNum()
	if hand_cards > 2 or dmg > 1 then return true end
	if hand_cards > 1 and math.random(0, 10) > 5 then return true end
	
	return false
end

sgs.ai_skfenying_get_card = function(self)
	local cards = sgs.QList2Table(self.player:getHandcards())
    self:sortByKeepValue(cards)
	
	for _, card in ipairs(cards) do
		if card:isRed() and not self.player:isJilei(card) then
			return card
		end
	end
	for _, equip in sgs.qlist(self.player:getEquips()) do
		if equip:isRed() and not self.player:isJilei(equip) then
			return equip
		end
	end
	return nil
end

sgs.ai_skjieyan_get_card = function(self, to_dis)
	local cards = sgs.QList2Table(self.player:getHandcards())
    self:sortByKeepValue(cards)
	
	for _, card in ipairs(cards) do
		if not table.contains(to_dis, card:getId()) and not card:isRed() and not self.player:isJilei(card) then
			return card
		end
	end
	for _, card in ipairs(cards) do
		if not table.contains(to_dis, card:getId()) and not self.player:isJilei(card) then
			return card
		end
	end
	return nil
end

sgs.ai_skill_invoke.skjieyan = function(self, data)
	local carduse = data:toCardUse()
	if carduse.to:length() ~= 1 then return false end
	local targets_tb = sgs.QList2Table(carduse.to)
	if self:isEnemy(targets_tb[1]) and math.random(0, 10) > 3 then
		return true
	end
	return false
end

sgs.ai_skill_discard.skjieyan = function(self, discard_num, min_num, optional, include_equip)
	local toDis = {}
	while #toDis < min_num do
		local todiscard = sgs.ai_skjieyan_get_card(self, toDis)
		if todiscard then
			table.insert(toDis, todiscard:getId())
		end
	end
	return toDis
end

sgs.ai_skill_use["@@skfenying"] = function(self, prompt, method)
	
	local targets = sgs.ai_skfenying_get_targets(self)
	if #targets < 1 then return "." end
	self:sort(targets, "hp")
	
	for _, target in ipairs(targets) do
		if sgs.ai_skfenying_should_use(self, target) then
			local card_discard = sgs.ai_skfenying_get_card(self)
			if card_discard then
				return "@SKFenyingCard="..card_discard:getEffectiveId().."->"..target:objectName()
			end
			break
		end
	end
	return "."
end

sgs.ai_skill_invoke.skjuechen = function(self, data)
	local target = data:toPlayer()
	return self:isEnemy(target)
end

sgs.skjuechen_choose_equip = function(self)
	local cards = sgs.QList2Table(self.player:getHandcards())
	self:sortByKeepValue(cards)
	
	for _, card in ipairs(cards) do
		if card:isKindOf("EquipCard") and not self.player:isJilei(card) then
			return card
		end
	end
	
	cards = sgs.QList2Table(self.player:getEquips())
	self:sortByKeepValue(cards)
	for _, card in ipairs(cards) do
		return card
	end
	
	return nil
end

local skqianqi_skill = {}
skqianqi_skill.name = "skqianqi"
table.insert(sgs.ai_skills, skqianqi_skill)
skqianqi_skill.getTurnUseCard = function(self)
	if self.player:getMark("@skqianqi") > 0 then
		return sgs.Card_Parse("@SKQianqiCard=.")
	end
end

sgs.ai_skill_use_func.SKQianqiCard = function(card, use, self)
	self:sort(self.enemies, "threat")
	local tlist = self.player:property("skqianqi_targets"):toString()
	
	for _, enemy in ipairs(self.enemies) do
		if self.player:canSlash(enemy, false) and not tlist:contains(enemy:objectName()) then 
			use.card = sgs.Card_Parse("@SKQianqiCard=.")
			if use.to then use.to:append(enemy) end
			break
		end
	end
end

sgs.ai_card_intention.SKQianqiCard = 20

sgs.ai_skill_discard.skjuechen = function(self)
	local toDis = {}
	local equip = sgs.skjuechen_choose_equip(self)
	if equip == nil then return toDis end
	
	if self.player:hasFlag("SKJuechenSecond") then
		if self.player:getHp() > getBestHp(self.player) + 1 then return toDis end
		
	else
		if self.player:getHp() > getBestHp(self.player) then return toDis end
		if self:getCardsNum("Jink") < 1 and not self:hasEightDiagramEffect() and not self.player:hasArmorEffect("vine") then return toDis end
		if self:needToLoseHp(self.player, nil, true, false) then return toDis end
		
	end
	
	table.insert(toDis, equip:getEffectiveId())
	return toDis
end

local skshajue_skill = {}
skshajue_skill.name = "skshajue"
table.insert(sgs.ai_skills, skshajue_skill)
skshajue_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("SKShajueCard") or self.player:getHandcardNum() < 3 then return nil end
	
	if self.player:getHp() == 1 and self:getCardsNum("Peach") + self:getCardsNum("Analeptic") < 1 and not self.player:hasSkill("skguiqu") then
		return nil
	end
	
	return sgs.Card_Parse("@SKShajueCard=.")
end

aiskill_skshajue_target = function(self)
	local others = self.room:getOtherPlayers(self.player)
	local targets = sgs.QList2Table(others)
	self:sort(targets, "threat")
	for _, t in ipairs(targets) do
		if self:isEnemy(t) and self.player:canSlash(t, false) and not t:hasSkill("xiangle") then
            return t
        end
	end
	
	return nil
end

sgs.ai_skill_use_func.SKShajueCard = function(card, use, self)
	local target = aiskill_skshajue_target(self)
	if target then
		use.card = sgs.Card_Parse("@SKShajueCard=.")
		if use.to then use.to:append(target) end
	end
end

sgs.ai_skill_playerchosen.skshajue = function(self, targets)
	return aiskill_skshajue_target(self)
end

sgs.ai_playerchosen_intention.skshajue = 100

sgs.ai_skill_invoke.skguiqu = function(self, data)
	local dying = data:toDying()
	local peaches = 1 - dying.who:getHp()
	local owns = 0
	local peach = sgs.Sanguosha:cloneCard("Peach")
	peach:deleteLater()
	if self.player:isLocked(peach, true) then return false end
	owns = owns + self:getCardsNum("Peach")
	local analeptic = sgs.Sanguosha:cloneCard("Analeptic")
	analeptic:deleteLater()
	if self.player:isLocked(analeptic, true) then return false end
	owns = owns + self:getCardsNum("Analeptic")

	return owns < peaches
end

--sgs.ai_view_as.skguiqu = function(card, player, card_place, class_name)
--	if class_name ~= "Peach" then return nil end
--	local suit = card:getSuitString()
--	local number = card:getNumberString()
--	local card_id = card:getEffectiveId()
--	room:writeToConsole(("[skguiqu] class_name:%s"):format(class_name))
--	
--	if card_place ~= sgs.Player_PlaceSpecial and card:isRed() and player:getPhase() == sgs.Player_NotActive
--		and player:getMark("Global_PreventPeach") == 0 then
--		return ("peach:jijiu[%s:%s]=%d"):format(suit, number, card_id)
--	end
--	
--	
--	local room = player:getRoom()
--	if room:aiSkill_SKGuiqu(player, "skguiqu") then
--		return ("peach:skguiqu[%s:%s]=."):format(6, 0)
--	end
--end

sgs.ai_skill_choice.skguiqu = function(self, choices)
	local choice_table = choices:split("+")
	local second_table = {}
    for index, achoice in ipairs(choice_table) do
        if achoice == "skguiqu" or achoice == "skshajue" or achoice == "skluosha" then 
			table.insert(second_table, achoice)
			table.remove(choice_table, index) 
			break 
		end
    end
	if #choice_table > 0 then
		local r = math.random(1, #choice_table)
		return choice_table[r]
	end
    if #second_table > 0 then
		if table.contains(second_table, "skshajue") then return "skshajue" end
		if table.contains(second_table, "skluosha") then return "skluosha" end
		if table.contains(second_table, "skguiqu") then return "skguiqu" end
		-- fake logic
		local r = math.random(1, #second_table)
		return second_table[r]
	end
end

local skguiyuan_skill = {}
skguiyuan_skill.name = "skguiyuan"
table.insert(sgs.ai_skills, skguiyuan_skill)
skguiyuan_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("SKGuiyuanCard") then return nil end
	
	if self.player:getHp() == 1 and self:getCardsNum("Peach") + self:getCardsNum("Analeptic") < 1 then
		return nil
	end
	
	return sgs.Card_Parse("@SKGuiyuanCard=.")
end

sgs.ai_skill_use_func.SKGuiyuanCard = function(card, use, self)
	use.card = sgs.Card_Parse("@SKGuiyuanCard=.")
end

sgs.ai_skill_invoke.skchongsheng = function(self, data)
	local str = data:toString()
	if str == "ASK_FOR_CHOICE" then
		return true
	else
		local target = data:toPlayer()
		if target and self:isFriend(target) then
			return true
		end
	end
	return false
end

sgs.ai_skill_invoke.skyaozhi = true

sgs.ai_skill_invoke.sklvezhen = function(self, data)
	local target = data:toPlayer()
	return not self:isFriend(target)
end

sgs.ai_skill_invoke.skyoulong = function(self, data)
	return not self:needKongcheng()
end

sgs.ai_skill_use["@@skyoulong"] = function(self, prompt)
	local snatch = sgs.Sanguosha:cloneCard("Snatch")
	snatch:setSkillName("skyoulong")
	snatch:deleteLater()
	local targets = {}
	self:getCardSnatchOrDismantlementTarget(snatch, targets)
	if #targets < 1 then return "." end
	local s = table.concat(targets, "+")
	return "&Snatch=.:.skyoulong->" .. s
end

sgs.ai_skill_invoke.skshenfu = true

sgs.ai_skill_playerchosen.skshenfu = function(self, targets)
	local targets_tb = sgs.QList2Table(targets)
	self:sort(targets_tb, "hp")
	for _, p in ipairs(targets_tb) do
		if self:isEnemy(p) then
			return p
		end
	end
	return nil
end

