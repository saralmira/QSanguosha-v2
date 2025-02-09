-- translation for God Package

return {
	["god"] = "神",

	["#shenguanyu"] = "鬼神再临",
	["shenguanyu"] = "神关羽",
	["wushen"] = "武神",
	[":wushen"] = "锁定技。你的红桃手牌视为普通【杀】。你使用红桃【杀】无距离限制。",
	["wuhun"] = "武魂",
	[":wuhun"] = "锁定技。每当你受到伤害扣减体力前，伤害来源获得等于伤害点数的“梦魇”标记。你死亡时，你选择一名存活的“梦魇”标记数最多（不为0）的角色，该角色进行判定：若结果不为【桃】或【桃园结义】，该角色死亡。",
	["@wuhun-revenge"] = "请选择“梦魇”标记最多的一名其他角色",
	["@nightmare"] = "梦魇",
	["$WuhunAnimate"] = "image=image/animate/wuhun.png",
	["#WuhunRevenge"] = "%from 的“%arg2”被触发，拥有最多“梦魇”标记的角色 %to（%arg个）死亡",

	["#shenlvmeng"] = "圣光之国士",
	["shenlvmeng"] = "神吕蒙",
	["shelie"] = "涉猎",
	[":shelie"] = "摸牌阶段开始时，你可以放弃摸牌并亮出牌堆顶的五张牌：若如此做，你获得其中每种花色的牌各一张，然后将其余的牌置入弃牌堆。",
	["gongxin"] = "攻心",
	[":gongxin"] = "阶段技。你可以观看一名其他角色的手牌，然后选择其中一张红桃牌并选择一项：弃置之，或将之置于牌堆顶。",
	["gongxin:discard"] = "弃置",
	["gongxin:put"] = "置于牌堆顶",

	["#shenzhouyu"] = "赤壁的火神",
	["shenzhouyu"] = "神周瑜",
	["qinyin"] = "琴音",
	[":qinyin"] = "弃牌阶段结束时，若你于本阶段内弃置了至少两张你的牌，你可以选择一项：令所有角色各回复1点体力，或令所有角色各失去1点体力。",
	["qinyin:up"] = "所有角色回复1点体力",
	["qinyin:down"] = "所有角色失去1点体力",
	["yeyan"] = "业炎",
	[":yeyan"] = "限定技。出牌阶段，你可以对一至三名角色各造成1点火焰伤害；或你可以弃置四种花色的手牌各一张，失去3点体力并选择一至两名角色：若如此做，你对这些角色造成共计至多3点火焰伤害且对其中一名角色造成至少2点火焰伤害。",
	["greatyeyan"] = "业炎",
	["smallyeyan"] = "业炎",
	["$YeyanAnimate"] = "image=image/animate/yeyan.png",

	["#shenzhugeliang"] = "赤壁的妖术师",
	["shenzhugeliang"] = "神诸葛亮",
	["qixing"] = "七星",
	[":qixing"] = "你的起始手牌数+7。分发起始手牌后，你将其中七张扣置于武将牌旁，称为“星”。摸牌阶段结束时，你可以将至少一张手牌与等量的“星”交换。",
	["stars"] = "星",
	["@qixing-exchange"] = "请选择牌用于交换",
	["~qixing"] = "选择的牌将成为“星”",
	["kuangfeng"] = "狂风",
	[":kuangfeng"] = "结束阶段开始时，你可以将一张“星”置入弃牌堆并选择一名角色：若如此做，直到你的回合开始时，火焰伤害结算开始时，此伤害+1。",
	["@gale"] = "狂风",
	["@kuangfeng-card"] = "你可以发动“狂风”",
	["~kuangfeng"] = "选择一名角色→点击确定→然后在窗口中选择一张牌",
	["dawu"] = "大雾",
	[":dawu"] = "结束阶段开始时，你可以将至少一张“星”置入弃牌堆并选择等量的角色：若如此做，直到你的回合开始时，伤害结算开始时，防止这些角色受到的非雷电属性的伤害。",
	["@fog"] = "大雾",
	["@dawu-card"] = "你可以发动“大雾”",
	["~dawu"] = "选择若干名角色→点击确定→然后在窗口中选择相应数量的牌",
	["#QixingExchange"] = "%from 发动了“%arg2”，交换了 %arg 张手牌",
	["#FogProtect"] = "%from 的“<font color=\"yellow\"><b>大雾</b></font>”效果被触发，防止了 %arg 点伤害[%arg2]",
	["#GalePower"] = "“<font color=\"yellow\"><b>狂风</b></font>”效果被触发，%from 的火焰伤害从 %arg 点增加至 %arg2 点",

	["#shencaocao"] = "超世之英杰",
	["shencaocao"] = "神曹操",
	["guixin"] = "归心",
	[":guixin"] = "每当你受到1点伤害后，你可以依次获得所有其他角色区域内的一张牌，然后将武将牌翻面。",
	["$GuixinAnimate"] = "image=image/animate/guixin.png",
	["feiying"] = "飞影",
	[":feiying"] = "锁定技。其他角色与你的距离+1",

	["#shenlvbu"] = "修罗之道",
	["shenlvbu"] = "神吕布",
	["kuangbao"] = "狂暴",
	[":kuangbao"] = "锁定技。游戏开始时，你获得两枚“暴怒”标记。每当你造成或受到1点伤害后，你获得一枚“暴怒”标记。",
	["@wrath"] = "暴怒",
	["wumou"] = "无谋",
	[":wumou"] = "锁定技。每当你使用一张非延时锦囊牌时，你须选择一项：失去1点体力，或弃一枚“暴怒”标记。",
	["wuqian"] = "无前",
	[":wuqian"] = "出牌阶段，你可以弃两枚“暴怒”标记并选择一名其他角色：若如此做，你拥有“无双”且该角色防具无效，直到回合结束。",
	["shenfen"] = "神愤",
	[":shenfen"] = "阶段技。你可以弃六枚“暴怒”标记：若如此做，所有其他角色受到1点伤害，弃置装备区的所有牌，弃置四张手牌，然后你将武将牌翻面。",
	["$ShenfenAnimate"] = "image=image/animate/shenfen.png",
	["#KuangbaoDamage"] = "%from 的“%arg2”被触发，造成 %arg 点伤害获得 %arg 枚“暴怒”标记",
	["#KuangbaoDamaged"] = "%from 的“%arg2”被触发，受到 %arg 点伤害获得 %arg 枚“暴怒”标记",
	["wumou:discard"] = "弃一枚“暴怒”标记",
	["wumou:losehp"] = "失去1点体力",

	["#shenzhaoyun"] = "神威如龙",
	["shenzhaoyun"] = "神赵云",
	["juejing"] = "绝境",
	[":juejing"] = "锁定技。摸牌阶段，你额外摸X张牌。你的手牌上限+2（X为你已损失的体力值）。一名角色的回合结束时，若你的体力值大于1，你失去1点体力然后摸两张牌。",
	["longhun"] = "龙魂",
	[":longhun"] = "你可以将X张同花色的牌按以下规则使用或打出：红桃当【桃】；方块当火【杀】；黑桃当【无懈可击】；梅花当【闪】。（X为你的体力值且至少为1）",

	["#shensimayi"] = "晋国之祖",
	["shensimayi"] = "神司马懿",
	["renjie"] = "忍戒",
	[":renjie"] = "锁定技。每当你受到1点伤害后或于弃牌阶段因你的弃置而失去一张牌后，你获得一枚“忍”。",
	["@bear"] = "忍",
	["baiyin"] = "拜印",
	[":baiyin"] ="觉醒技。准备阶段开始时，若你拥有四枚或更多的“忍”，你失去1点体力上限，然后获得“极略”（你可以弃一枚“忍”并发动以下技能之一：“鬼才”、“放逐”、“集智”、“制衡”、“完杀”）。",
	["$BaiyinAnimate"] = "image=image/animate/baiyin.png",
	["jilve"] = "极略",
	[":jilve"] = "你可以弃一枚“忍”并发动以下技能之一：“鬼才”、“放逐”、“集智”、“制衡”、“完杀”。",
	["jilve_jizhi"] = "极略（集智）",
	["jilve_guicai"] = "极略（鬼才）",
	["jilve_fangzhu"] = "极略（放逐）",
	["lianpo"] = "连破",
	[":lianpo"] = "每当一名角色的回合结束后，若你于本回合杀死至少一名角色，你可以进行一个额外的回合。",
	["@jilve-zhiheng"] = "请发动“制衡”",
	["~zhiheng"] = "选择需要弃置的牌→点击确定",
	["#BaiyinWake"] = "%from 的“忍”为 %arg 个，触发“<font color=\"yellow\"><b>拜印</b></font>”觉醒",
	["#LianpoCanInvoke"] = "%from 在本回合内杀死了 %arg 名角色，满足“%arg2”的发动条件",
	["#LianpoRecord"] = "%from 杀死了 %to，可在 %arg 回合结束后进行一个额外的回合",
}
