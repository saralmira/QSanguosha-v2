--祖茂
sgs.ai_skill_use["@@yinbing"] = function(self, prompt)
	--手牌
	local otherNum = self.player:getHandcardNum() - self:getCardsNum("BasicCard")
	if otherNum == 0 then return "." end

	local slashNum = self:getCardsNum("Slash")
	local jinkNum = self:getCardsNum("Jink")
	local enemyNum = #self.enemies
	local friendNum = #self.friends

	local value = 0
	if otherNum > 1 then value = value + 0.3 end
	for _,card in sgs.qlist(self.player:getHandcards()) do
		if card:isKindOf("EquipCard") then value = value + 1 end
	end
	if otherNum == 1 and self:getCardsNum("Nullification") == 1 then value = value - 0.2 end

	--已有引兵
	if self.player:getPile("yinbing"):length() > 0 then value = value + 0.2 end

	--双将【空城】
	if self:needKongcheng() and self.player:getHandcardNum() == 1 then value = value + 3 end

	if enemyNum == 1 then value = value + 0.7 end
	if friendNum - enemyNum > 0 then value = value + 0.2 else value = value - 0.3 end
	local slash = sgs.Sanguosha:cloneCard("slash")
	--关于 【杀】和【决斗】
	if slashNum == 0 then value = value - 0.1 end
	if jinkNum == 0 then value = value - 0.5 end
	if jinkNum == 1 then value = value + 0.2 end
	if jinkNum > 1 then value = value + 0.5 end
	if self.player:getArmor() and self.player:getArmor():isKindOf("EightDiagram") then value = value + 0.4 end
	for _,enemy in ipairs(self.enemies) do
		if enemy:canSlash(self.player, slash) and self:slashIsEffective(slash, self.player, enemy) and (enemy:inMyAttackRange(self.player) or enemy:hasSkills("zhuhai|shensu")) then
			if ((enemy:getWeapon() and enemy:getWeapon():isKindOf("Crossbow")) or enemy:hasSkills("paoxiao|tianyi|xianzhen|jiangchi|fuhun|gongqi|longyin|qiangwu")) and enemy:getHandcardNum() > 1 then
				value = value - 0.2
			end
			if enemy:hasSkills("tieqi|wushuang|yijue|liegong|mengjin|qianxi") then
				value = value - 0.2
			end
			value = value - 0.2
		end
		if enemy:hasSkills("lijian|shuangxiong|mingce|mizhao") then
			value = value - 0.2
		end
	end
	--肉盾
	local yuanshu = self.room:findPlayerBySkillName("tongji")
	if yuanshu and yuanshu:getHandcardNum() > yuanshu:getHp() then value = value + 0.4 end
	for _,friend in ipairs(self.friends) do
		if friend:hasSkills("fangquan|zhenwei|kangkai") then value = value + 0.4 end
	end

	if value < 0 then return "." end

	local card_ids = {}
	local nulId
	for _,card in sgs.qlist(self.player:getHandcards()) do
		if not card:isKindOf("BasicCard") then
			if card:isKindOf("Nullification") then
				nulId = card:getEffectiveId()
			else
				table.insert(card_ids, card:getEffectiveId())
			end
		end
	end
	if nulId and #card_ids == 0 then
		table.insert(card_ids, nulId)
	end
	return "@YinbingCard=" .. table.concat(card_ids, "+") .. "->."
end

sgs.yinbing_keep_value = {
	EquipCard = 5,
	TrickCard = 4
}
