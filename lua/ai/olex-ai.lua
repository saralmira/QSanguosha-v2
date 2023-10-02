sgs.ai_skill_invoke.olex_jiqiao = true

sgs.ai_skill_askforag.olex_jiqiao = function(self, card_ids)
	local jqvalue = 0
	local red_cards = {}
	local black_cards = {}
	for _, card_id in ipairs(card_ids) do
        local card = sgs.Sanguosha:getCard(card_id)
		if card:isRed() then
			jqvalue = jqvalue + 1
			table.insert(red_cards, card_id)
		else
			jqvalue = jqvalue - 1
			table.insert(black_cards, card_id)
		end
    end
	
	if jqvalue > 0 then
		return red_cards[math.random(1, #red_cards)]
	elseif jqvalue < 0 then
		return black_cards[math.random(1, #black_cards)]
	end
	return nil
end

sgs.ai_skill_invoke.olex_xiongyi = true

sgs.ai_skill_invoke.olex_yuqi = function(self, data)
	local target = data:toPlayer()
	local total = math.max(1, self.player:getTag("yuqi_total"):toInt())
	if not self:isFriend(target) and total < 2 then return false end
	return true
end

sgs.ai_skill_use["@@olex_yuqi"] = function(self, prompt)
	local toget = self.player:property("yuqi_toget"):toString()
	local n = tonumber(toget)
	if n then return string.format("@OLEXYuqiCard=%d->.", n) end
	local correct_cards = toget:split("+")
	self:sortByCardIdNeed(correct_cards)
	if self.player:hasFlag("yuqi_InMovingStage1") then
		return string.format("@OLEXYuqiCard=%d->.", correct_cards[1])
	end
	local yuqi_player = math.max(1, self.player:getTag("yuqi_player"):toInt())
	local minvalue = math.min(yuqi_player, #correct_cards)
	if minvalue == 1 then
		return string.format("@OLEXYuqiCard=%d->.", correct_cards[#correct_cards])
	else
		return string.format("@OLEXYuqiCard=%s->.", table.concat(correct_cards, "+", #correct_cards - minvalue + 1, #correct_cards))
	end
end

sgs.ai_skill_invoke.olex_shanshen = true

olexcaojinyu_choice = function(self, choices)
	local choice_table = choices:split("+")
	if #choice_table < 1 then return "" end
	local total = math.max(1, self.player:getTag("yuqi_total"):toInt())
	if total < 2 then return "total" end
	local dist = math.max(0, self.player:getTag("yuqi_dist"):toInt())
	if dist < 1 then return "dist" end
	if total < 3 then return "total" end
	local yqtarget = math.max(1, self.player:getTag("yuqi_target"):toInt())
	local yqplayer = math.max(1, self.player:getTag("yuqi_player"):toInt())
	
	local ran = math.random(0, 1)
	if ran == 1 and choice_table:contains("total") then return "total" end
	if ran == 0 and choice_table:contains("player") then return "player" end
    return choice_table[math.random(1, #choice_table)]
end

sgs.ai_skill_choice.olex_shanshen = function(self, choices)
	return olexcaojinyu_choice(self, choices)
end

sgs.ai_skill_invoke.olex_xianjing = true

sgs.ai_skill_choice.olex_xianjing = function(self, choices)
	return olexcaojinyu_choice(self, choices)
end
