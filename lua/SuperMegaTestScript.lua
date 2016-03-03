local function test()	
	Game.HUD:PushNotification("test", vec4f(1,1,1,1))
end

RegisterEvent("OnRenderScoreboard", test)
