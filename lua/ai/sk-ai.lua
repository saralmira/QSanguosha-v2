local skziguo_skill = {}
skziguo_skill.name = "skziguo"
table.insert(sgs.ai_skills, skziguo_skill)
skziguo_skill.getTurnUseCard = function(self)
	if not self.player:hasUsed("SKZiguoCard") then
		return sgs.Card_Parse("@SKZiguoCard=.")
	end
end

sgs.ai_skill_use_func.SKZiguoCard = function(card, use, self)
	self:sort(self.friends, "hp", false)
	for _, friend in ipairs(self.friends) do
		if friend:isWounded() then 
			use.card = sgs.Card_Parse("@SKZiguoCard=.")
			if use.to then use.to:append(friend) end
			break
		end
	end
end

sgs.ai_use_value.SKZiguoCard = 9.2
sgs.ai_use_priority.SKZiguoCard = 4.3
sgs.ai_card_intention.SKZiguoCard = -35

function sgs.ai_skill_pindian.skguhuo(minusecard, self, requestor)
	if self.player:objectName() ~= requestor:objectName() and self:isFriend(requestor) then
		return self:getMinCard()
	end
	return self:getMaxCard()
end

--决定蛊惑使用什么牌，self于吉，target用牌者
sgs.ai_skguhuo_usecard = function(self, target, target_table)
	local fakecard
	local target_enemy = nil
	local canslash_enemy = false
	local ban = table.concat(sgs.Sanguosha:getBanPackages(), "|")
	for _, enemy in ipairs(self.enemies) do
		if target ~= enemy then 
			target_enemy = enemy
			break
		end
	end
	if self:isEnemy(target) then
		if #self.enemies > 1 then
			-- 决斗
			if math.random(1, 10) > 3.5 then
				if target_enemy and target_table then table.insert(target_table, target_enemy:objectName()) end
				return "duel"
			end
			
			-- 杀
			for _, enemy in ipairs(self.enemies) do
				if target ~= enemy and target:canSlash(enemy, true) then 
					target_enemy = enemy
					canslash_enemy = true
					break
				end
			end
			
			local slashes = "slash"
			if not ban:match("maneuvering") then slashes = slashes .. "|thunder_slash|fire_slash" end
			local slashes_t = slashes:split("|")
			local slash = slashes_t[math.random(1, #slashes_t)]
			if canslash_enemy then
				if target_enemy and target_table then table.insert(target_table, target_enemy:objectName()) end
				return slash
			end
		end
	else
		local weakenemy = nil
		for _, enemy in ipairs(self.enemies) do
			if target ~= enemy and self:isWeak(enemy) then 
				weakenemy = enemy
				canslash_enemy = target:canSlash(enemy, true)
				break
			end
		end
		if weakenemy then
			if target_table then table.insert(target_table, weakenemy:objectName()) end
			if canslash_enemy and math.random(0, 10) > 5 then
				local slashes = "slash"
				if not ban:match("maneuvering") then slashes = slashes .. "|thunder_slash|fire_slash" end
				local slashes_t = slashes:split("|")
				local slash = slashes_t[math.random(1, #slashes_t)]
				return slash
			else
				return "duel"
			end
		end
		
		-- 桃园结义
		fakecard = sgs.Sanguosha:cloneCard("god_salvation")
		fakecard:deleteLater()
		if self:willUseGodSalvation(fakecard) then
			return "god_salvation"
		end
		
		-- 桃
		if target:getHp() < target:getMaxHp() then
			return "peach"
		end
		
		-- 南蛮入侵
		fakecard = sgs.Sanguosha:cloneCard("savage_assault")
		fakecard:deleteLater()
		if self:willUseAOEAttack(fakecard) then
			return "savage_assault"
		end
		-- 万箭齐发
		fakecard = sgs.Sanguosha:cloneCard("archery_attack")
		fakecard:deleteLater()
		if self:willUseAOEAttack(fakecard) then
			return "archery_attack"
		end
		
		-- 无中生有等
		local goodcards = "ex_nihilo|snatch|dismantlement"
		local guhuos = goodcards:split("|")
		for i = 1, #guhuos do
			local forbidden = guhuos[i]
			fakecard = sgs.Sanguosha:cloneCard(forbidden)
			fakecard:deleteLater()
			if target:isLocked(fakecard) then
				table.remove(guhuos, i)
				i = i - 1
			end
		end
		if #guhuos > 0 and self.player:getHandcardNum() > 1 and target:getHandcardNum() > 1 and math.random(0, 10) > 5 then
			local guhuo_card = guhuos[math.random(1, #guhuos)]
			if guhuo_card ~= "ex_nihilo" and target_enemy and target_table then table.insert(target_table, target_enemy:objectName()) end
			return guhuo_card
		end
	end
	return nil
end

sgs.ai_skill_use["@@skguhuo"] = function(self, prompt)
	local target = self.player:getTag("SKGuhuoTarget"):toPlayer()
	local target_table = {}
	local card_str = sgs.ai_skguhuo_usecard(self, target, target_table)
	if not card_str then return "." end
	
	if #target_table == 0 then
		return "@SKGuhuoCard=.:" .. card_str .. "->."
	else
		return "@SKGuhuoCard=.:" .. card_str .. "->" .. table.concat(target_table, "+")
	end
end

sgs.ai_skill_invoke.skguhuo = function(self, data)
	if self.player:getHp() < 2 then return false end
	local target = data:toPlayer()
	local card_str = sgs.ai_skguhuo_usecard(self, target)
	if not card_str then return false end
	
	local max_card = self:getMaxCard()
	local max_point = max_card:getNumber()
	
	if self:isEnemy(target) then
		if max_point < 11 and math.random(1, self.player:getHandcardNum()) < 2 then return false end
		if self:isWeak() or #self.enemies < 2 then return false end
		
	else
		if (card_str == "god_salvation" or card_str == "peach") and not self:isWeak(target) and math.random(0, 10) > 3 then return true end
		if math.random(0, 10) > 3 then return false end
	end
	
	return true
end

sgs.ai_skill_invoke.skfulu = function(self, data)
	local drqueue = data:toStringList()
	local usevalue = 0
	for i = 1,3 do
		local player = findPlayerByObjectName(self.room, drqueue[i], false)
		if player then
			if self:isFriend(player) then
				usevalue = usevalue - 1
			else
				usevalue = usevalue + 1
			end
		end
	end
	for i = 4,6 do
		local player = findPlayerByObjectName(self.room, drqueue[i], false)
		if player then
			if self:isFriend(player) then
				usevalue = usevalue + 1
			else
				usevalue = usevalue - 1
			end
		end
	end
	
	if usevalue > 0 then return true end
	return false
end

sgs.ai_skill_invoke.skjiaohui = function(self, data)
	return true
end

sgs.ai_skill_discard.skjiaohui = function(self)
	local toDis = {}
	
	if self.player:getHandcardNum() == self.player:getHp() + 1 then 
		local card = self:chooseHandCardByKeepValue()
		if card == nil then return toDis end
		table.insert(toDis, card:getEffectiveId()) 
	end
	if self.player:getHandcardNum() == self.player:getHp() and not self.player:getEquips():isEmpty() then 
		local card = self:chooseEquipByKeepValue()
		if card == nil then return toDis end
		table.insert(toDis, card:getEffectiveId()) 
	end

	return toDis
end

sgs.ai_skill_choice.skjiaohui = function(self, choices)
	if self.player:getHandcardNum() == self.player:getHp() + 1 then return "discard" end
	if self.player:getHandcardNum() == self.player:getHp() and not self.player:getEquips():isEmpty() and math.random(0, 10) > 5 then return "discard" end

	return "draw"
end

function sgs.skwengua_getcard(self, putblack)
	local toDis = {}
	local cards = sgs.QList2Table(self.player:getHandcards())
	self:sortByKeepValue(cards)
	
	for _, card in ipairs(cards) do
		if self.player:getMark("@skfuzhu") == 0 or card:isBlack() then
			table.insert(toDis, card:getEffectiveId())
			if putblack == true then 
				if card:isBlack() then 
					self.player:addMark("SKWenguaAIMarkTimes") 
				else
					self.player:removeMark("SKWenguaAIMarkTimes") 
				end
			end
			break
		end
	end
	if #toDis < 1 and not self:isWeak() then
		for _, equip in sgs.qlist(self.player:getEquips()) do
			if self.player:getMark("@skfuzhu") == 0 or equip:isBlack() then
				table.insert(toDis, equip:getEffectiveId())
				if putblack == true then 
					if equip:isBlack() then 
						self.player:addMark("SKWenguaAIMarkTimes") 
					else
						self.player:removeMark("SKWenguaAIMarkTimes") 
					end
				end
				break
			end
		end
	end
	return toDis
end

sgs.ai_skill_discard.skwengua = function(self)
	local toDis = {}
	local cards = {}
	if self.player:hasFlag("SKWenguaPut") then
		toDis = sgs.skwengua_getcard(self, true)
	else
		local skxushi = self.room:findPlayerBySkillName("skwengua")
		local shouldgive = false
		if skxushi == self.player or self:isFriend(skxushi) then shouldgive = true end
		if shouldgive == true then
			toDis = sgs.skwengua_getcard(self)
		end
	end
	
	return toDis
end

sgs.ai_choicemade_filter.cardChosen.skwengua = function(self, player, promptlist)
	local target = findPlayerByObjectName(self.room, promptlist[#promptlist], false, player) 
	if not target then return end
	sgs.updateIntention(player, target, -20)
end

local skfuzhu_skill = {}
skfuzhu_skill.name = "skfuzhu"
table.insert(sgs.ai_skills, skfuzhu_skill)
skfuzhu_skill.getTurnUseCard = function(self)
	if self.player:getMark("@skfuzhu") > 0 then
		return sgs.Card_Parse("@SKFuzhuCard=.")
	end
end

sgs.ai_skill_use_func.SKFuzhuCard = function(card, use, self)
	self:sort(self.enemies, "threat")
	local marksnum = self.player:getMark("SKWenguaAIMarkTimes")
	for _, enemy in ipairs(self.enemies) do
		if self.player:canSlash(enemy, true) and not enemy:hasSkill("yizhong") and not self:hasEightDiagramEffect(enemy) and not enemy:hasArmorEffect("vine") and not enemy:hasArmorEffect("renwang_shield") and marksnum > enemy:getHp() then 
			use.card = sgs.Card_Parse("@SKFuzhuCard=.")
			if use.to then use.to:append(enemy) end
			break
		end
	end
end

sgs.ai_card_intention.SKFuzhuCard = 100

local sksijian_skill = {}
sksijian_skill.name = "sksijian"
table.insert(sgs.ai_skills, sksijian_skill)
sksijian_skill.getTurnUseCard = function(self)
	if not self.player:hasUsed("SKSijianCard") then
		return sgs.Card_Parse("@SKSijianCard=.")
	end
end

sgs.ai_skill_use_func.SKSijianCard = function(card, use, self)
	self:sort(self.friends_noself, "handcard", true)
	for _, friend in ipairs(self.friends_noself) do
		local num = friend:getHandcardNum()
		if self:getAllPeachNum() > 1 or (num > 4 or (num  > 2 and math.random(0, 10) > 5) or math.random(0, 10) > 8) then 
			use.card = sgs.Card_Parse("@SKSijianCard=.")
			if use.to then use.to:append(friend) end
			break
		end
	end
end

sgs.ai_use_value.SKSijianCard = 9.0
sgs.ai_use_priority.SKSijianCard = 4.0
sgs.ai_card_intention.SKSijianCard = -50

sgs.ai_skill_invoke.skgangzhi = function(self, data)
	if self.player:getHp() <= data:toInt() and self:getAllPeachNum() < 1 then return true end
	if self.player:getHp() == self.player:getMaxHp() then return false end
	if self.player:getHp() < 2 and self:getAllPeachNum() < 1 then return true end
	return false
end


sgs.ai_skill_invoke.skleiji = function(self, data)

	local retrialvalue = 0
	local players = sgs.QList2Table(self.room:getAlivePlayers())
	for _, p in ipairs(players) do
		local finalRetrial, wizard = self:getFinalRetrial(p, "skleiji")
		if finalRetrial == 1 then --己方后判
            retrialvalue = retrialvalue + 1
        elseif finalRetrial == 2 then --对方后判
            retrialvalue = retrialvalue - 1
        end
	end

	if retrialvalue < 0 then return false end
	if retrialvalue == 0 then
		local usevalue = 0
		for _, p in ipairs(players) do
			if not p:hasSkill("skshanxi") then
				if self:isFriend(p) then
					if p:isChained() then 
						usevalue = usevalue - 3
					else
						usevalue = usevalue - 1.5
					end
				else
					if p:isChained() then 
						usevalue = usevalue + 3
					else
						usevalue = usevalue + 1.5
					end
				end
			end
		end
		usevalue = usevalue + #players - 1
		if usevalue <= 0 then return false end
	end
		
	return true
end

sgs.ai_skill_playerchosen.skleiji = function(self, targets)
	local players = sgs.QList2Table(self.room:getAlivePlayers())
	for _, p in ipairs(players) do
		if self:isEnemy(p) then
            return p
        end
	end
	
	return nil
end

sgs.ai_playerchosen_intention.skleiji = 20

sgs.ai_skill_invoke.sktaoluan = function(self, data)
	local use = data:toCardUse()
	if use.to then
		local wounded = 0
		local pp = 0
		local damagevalue = 0
		local include_self = false
		local friend_use = not self:isEnemy(use.from)
		
		for _, p in sgs.qlist(use.to) do
			if self:isFriend(p) then
				if self:isWeak(p) then 
					wounded = wounded + 2
					damagevalue = damagevalue - 2
				elseif p:getHp() < p:getMaxHp() then 
					wounded = wounded + 1 
				end
				pp = pp + 1
				
				if p:hasSkills(sgs.masochism_skill) then damagevalue = damagevalue + 1
				else damagevalue = damagevalue - 1 end
			else
				if self:isWeak(p) then 
					wounded = wounded - 2
					damagevalue = damagevalue + 2
				elseif p:getHp() < p:getMaxHp() then 
					wounded = wounded - 1 
				end
				pp = pp - 1
				
				if p:hasSkills(sgs.masochism_skill) then damagevalue = damagevalue - 1
				else damagevalue = damagevalue + 1 end
			end
			if p == self.player then include_self = true end
		end
		
		local general_use = true
		if not include_self and self.player:getHandcardNum() < 2 then
			general_use = false
		end
		
		if use.card:getTypeId() == sgs.Card_TypeTrick then
			if use.card:isKindOf("ExNihilo") and damagevalue > 0 and general_use and not friend_use then
				return true
			end
			if use.card:isKindOf("Duel") and damagevalue < -1 and not friend_use then
				return true
			end
			if use.card:isKindOf("Dismantlement") and include_self and not friend_use then
				return true
			end
			if use.card:isKindOf("Snatch") and pp > 0 and general_use and not friend_use then
				return true
			end
			if use.card:isKindOf("AOE") and damagevalue < -1 then
				return true
			end
			if (use.card:isKindOf("GlobalEffect") or use.card:isKindOf("IronChain")) and (wounded > 0 or damagevalue > 1) then
				return true
			end
			if use.card:isKindOf("FireAttack") and wounded > 0 and not friend_use then
				return true
			end
		else
			if use.card:isKindOf("Slash") and damagevalue < -1 and not friend_use then
				return true
			elseif use.card:isKindOf("Peach") and wounded < -1 and general_use and not friend_use then
				return true
			end
		end
	end
	
	return false
end

sgs.ai_skill_discard.sktaoluan = function(self)
	local toDis = {}
	
	local cards = sgs.QList2Table(self.player:getHandcards())
	self:sortByCardNeed(cards)
	for _, card in ipairs(cards) do
		if not self.player:isJilei(card) then
			table.insert(toDis, card:getEffectiveId())
			break
		end
	end
	
	return toDis
end

sgs.ai_skill_choice.sktaoluan = function(self, choices, data)
	local choice_table = choices:split("+")
	local use = data:toCardUse()
	if use.to then
		local wounded = 0
		local pp = 0
		local damagevalue = 0
		local include_self = false
		local friends = 0
		
		for _, p in sgs.qlist(use.to) do
			if self:isFriend(p) then
				if self:isWeak(p) then 
					wounded = wounded + 2
					damagevalue = damagevalue - 2
				elseif p:getHp() < p:getMaxHp() then 
					wounded = wounded + 1 
				end
				pp = pp + 1
				friends = friends + 1
				
				if p:hasSkills(sgs.masochism_skill) then damagevalue = damagevalue + 1
				else damagevalue = damagevalue - 1 end
			else
				if self:isWeak(p) then 
					wounded = wounded - 2
					damagevalue = damagevalue + 2
				elseif p:getHp() < p:getMaxHp() then 
					wounded = wounded - 1 
				end
				pp = pp - 1
				
				if p:hasSkills(sgs.masochism_skill) then damagevalue = damagevalue - 1
				else damagevalue = damagevalue + 1 end
			end
			if p == self.player then include_self = true end
		end
		
		local general_use = true
		if not include_self and self.player:getHandcardNum() < 2 then
			general_use = false
		end
		
		local snatch_all = false
		if use.from and self:isFriend(use.from) and use.from:getHandcardNum() < 2 then
			snatch_all = true
		end
		
		local dmgresult = nil
		local healresult = nil
		local genresult = nil
		
		if use.card:getTypeId() == sgs.Card_TypeTrick then
			if damagevalue > 0 then
				if friends == 0 then
					if table.contains(choice_table, "duel") then dmgresult = "duel" end
				end
				if not dmgresult then
					if table.contains(choice_table, "savage_assault") then dmgresult = "savage_assault" 
					elseif table.contains(choice_table, "archery_attack") then dmgresult = "archery_attack"
					end
				end
				if not dmgresult then damagevalue = 0 end
			end
			
			if wounded > 0 then 
				if not healresult then
					if table.contains(choice_table, "god_salvation") then healresult = "god_salvation" end
				end
				if not healresult then wounded = 0 end
			end
			
			if pp > 0 then 
				if table.contains(choice_table, "ex_nihilo") then genresult = "ex_nihilo" 
				elseif table.contains(choice_table, "amazing_grace") then genresult = "amazing_grace" 
				end
			end
			
			if snatch_all and table.contains(choice_table, "snatch") then genresult = "snatch" end
			if not genresult then genresult = "" end
				
			if use.card:isKindOf("ExNihilo") then
				if damagevalue > 0 then
					return dmgresult
				else
					return genresult
				end
			end
			if use.card:isKindOf("Duel") or use.card:isKindOf("Snatch") then
				if wounded > 0 then return healresult end
				return genresult
			end
			if use.card:isKindOf("Dismantlement") then
				return genresult or healresult
			end
			if use.card:isKindOf("AOE") then
				if damagevalue < -1 then
					return healresult
				else
					return genresult
				end
			end
			if use.card:isKindOf("GlobalEffect") or use.card:isKindOf("IronChain") then
				if wounded > 0 then
					return healresult
				elseif damagevalue > 1 then
					return dmgresult
				else
					return genresult
				end
			end
			if use.card:isKindOf("FireAttack") then
				if wounded > 0 then
					return healresult
				else
					return genresult
				end
			end
		else
			if damagevalue > 0 then
				if table.contains(choice_table, "fire_slash") then dmgresult = "fire_slash" 
				elseif table.contains(choice_table, "thunder_slash") then dmgresult = "thunder_slash"
				elseif table.contains(choice_table, "slash") then dmgresult = "slash"
				end
				if not dmgresult then damagevalue = 0 end
			end
			
			if wounded > 0 then 
				if table.contains(choice_table, "peach") then healresult = "peach" end
				elseif table.contains(choice_table, "analeptic") then healresult = "analeptic"
				if not healresult then wounded = 0 end
			end
			
			if use.card:isKindOf("Slash") then
				if damagevalue < 0 then
					return healresult
				end
			elseif use.card:isKindOf("Peach") then
				if damagevalue > 0 then
					return dmgresult
				end
			end
		end
	end
	
	return ""
end

sgs.ai_skshejian_goodtarget = function(self, flag)
	for _, friend in ipairs(self.friends_noself) do
		if friend:getCardCount() > 0 and friend:getMark("skshejian_e") == 0 then
			local num = math.min(friend:getMaxHp(), 5) - friend:getHandcardNum()
			if num > 2 or (num  > 1 and (flag or math.random(0, 10) > 5)) then 
				return friend
			end
		end
	end
	
	self:sort(self.enemies, "allcard", true)
	
	local sn = self.player:getCardCount()
	for _, enemy in ipairs(self.enemies) do
		if enemy:getCardCount() > 0 and enemy:getMark("skshejian_e") == 0 then
			local num = enemy:getCardCount()
			if num - sn > 2 or (num - sn > 1 and (flag or math.random(0, 10) > 5)) then 
				return enemy
			end
		end
	end
	
	return nil
end

local skshejian_skill = {}
skshejian_skill.name = "skshejian"
table.insert(sgs.ai_skills, skshejian_skill)
skshejian_skill.getTurnUseCard = function(self)
	if self:getCardsNum("Jink") > 0 or self:getCardsNum("Peach") > 0 then
		if self.player:isSkillEnabledAtPlay("skshejian") and sgs.ai_skshejian_goodtarget(self, true) then
			return sgs.Card_Parse("@SKShejianCard=.")
		end
	end
end

sgs.ai_skill_use_func.SKShejianCard = function(card, use, self)
	use.card = sgs.Card_Parse("@SKShejianCard=.")
	local target = sgs.ai_skshejian_goodtarget(self, false)
	if target and use.to then 
		use.to:append(target) 
	end
end

sgs.ai_skill_cardchosen.skshejian = function(self, who)
	if self:isFriend(who) then
		return self:getCardRandomly(who, "h")
	end
end

sgs.ai_skill_invoke.skshejian = function(self, data)
	local target = data:toPlayer()
	local num = math.min(self.player:getMaxHp(), 5) - self.player:getHandcardNum()
	local divnum = self.player:getCardCount() - target:getCardCount()
	if self:isFriend(target) and (target:hasSkill("skkuangao") or target:getHp() > 1) and num > 1 then
		return true
	elseif self:isEnemy(target) and (not target:hasSkill("skkuangao") or divnum == 1 or divnum == 2) then
		return true
	end
	return false
end

sgs.ai_skill_choice.skkuangao = function(self, choices, data)
	local target = data:toPlayer()
	if self:isFriend(target) then
		sgs.updateIntention(self.player, target, -50)
		return "draw"
	end
	sgs.updateIntention(self.player, target, 50)
    return "discard"
end

sgs.ai_skill_invoke.skkuangao = function(self, data)
	local target = data:toPlayer()
	if self:isFriend(target) then return true end
	local num1 = self.player:getCardCount()
	local divnum = num1 - target:getCardCount()
	if self:isEnemy(target) and num1 > 0 and (divnum >= 0 or divnum < -1 or math.random(0, 10) > 5) then return true end
	return false
end


