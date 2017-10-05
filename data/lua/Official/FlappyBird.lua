g_ScriptTitle = "FlappyBirds"
g_ScriptInfo = "Play FlappyBirds in Teeworlds"



--global vars
Started = false
Score = 0
PipeOffset = 700 --pixel
PipePlace = 250 -- pixel
FlappyBaseSpeed = 5 -- pixels per draw
FlappyPos = vec2f(Engine.Graphics.ScreenWidth/4,Engine.Graphics.ScreenHeight/2)
MinPipeLen = 100+PipePlace/2 -- pixel
MaxPipes = (Engine.Graphics.ScreenWidth/2)/PipeOffset+1
CurPipes = 0
PipeQuad = {}
PipeQuad2 = {}
PipePos = {}
JumpMax = PipePlace/1.5 -- in pixel
JumpHeight = JumpMax
NotJumped = 0
Death = false
BroadCastAwait= false
math.randomseed(Game.Client.Tick)

Running = false

--key debugs
IsPressed = false
WasPressed = false
--images
function OnScriptInit()
	Background = Engine.Graphics:LoadTexture(ScriptPath():sub(1, -ScriptPath():reverse():find("/")-1).."/data/back.png", -1, -1, 1)
	Pipe = Engine.Graphics:LoadTexture(ScriptPath():sub(1, -ScriptPath():reverse():find("/")-1).."/data/pipe.png", -1, -1, 1)
	Flappy = Engine.Graphics:LoadTexture(ScriptPath():sub(1, -ScriptPath():reverse():find("/")-1).."/data/flappy.png", -1, -1, 1)
	Import("broadcast")
	Import("math")
	AddPipe()
	EnterFullscreen()
	return true
end

function OnEnterFullscreen()
	Running = true
end

function OnScriptUnload()
	-- unload all the textures to not leak memory!
	Engine.Graphics:UnloadTexture(Pipe)
	Engine.Graphics:UnloadTexture(Flappy)
	Engine.Graphics:UnloadTexture(Background)
end


function RenderBird()
	if not Running then return end

	local SpeedMultiplier = 1/(Game.Client.FPS/60)
	local FlappySpeed = FlappyBaseSpeed * SpeedMultiplier

	if BroadCastAwait then
		if not (BroadcastDraw()) then
			BroadCastAwait= false
			Restart()
		end
	end
	if Started == false then
		BroadcastReset()
		BroadcastStart("Press [Space] to start the game.",0) -- Make permanent broadcast while the game ist not Startet!
		-- TODO: Add this function in the libary
	else
		if not Death then
			BroadcastReset()
			BroadcastStart("Score: "..Score,0)
			for i = 1,CurPipes do -- World move
				local rand = math.random(MinPipeLen,Engine.Graphics.ScreenHeight-MinPipeLen*2)
				PipePos[i] = vec2f(PipePos[i].x-FlappySpeed,PipePos[i].y)
				PipeQuad[i] = QuadItem(PipePos[i].x,PipePos[i].y+739/2+PipePlace/2 ,138,739)
				PipeQuad2[i] = QuadItem(PipePos[i].x,PipePos[i].y-739/2-PipePlace/2 ,138,739)
			end
		end
		-- Flappy move
		FlappyPos.y = FlappyPos.y + FlappySpeed/2 + (NotJumped/2)*SpeedMultiplier
		--FlappyPos = vec2f(FlappyPos.x, FlappyPos.y+FlappySpeed/2+NotJumped/2)
		NotJumped = NotJumped + 1 * SpeedMultiplier
		print(NotJumped)
		for i = 1, CurPipes do
			if FlappyPos.x > PipePos[i].x+138/2 then
				ResetPipe(i)
				Score = Score+1
				break
			end
			if FlappyPos.x > PipePos[i].x-148/2 and Death == false then -- Collision check
				if (FlappyPos.y < PipePos[i].y-PipePlace/2 or FlappyPos.y > PipePos[i].y+PipePlace/2) then
					Death = true
					EndScreen()
				end
			end
		end
	end
	if JumpHeight <= JumpMax then -- Jump handling
		FlappyPos = vec2f(FlappyPos.x, FlappyPos.y-FlappySpeed*2)
		JumpHeight = JumpHeight+FlappySpeed*2
		NotJumped = 0
	end
	if Engine.Input:KeyIsPressed("space") and WasPressed == false and Death == false then
		if Started == false then
			Started = true
		end
		WasPressed = true
		Jump()
	elseif Engine.Input:KeyIsPressed("space") == false and WasPressed then
		WasPressed = false
	end
	Engine.Graphics:MapScreen(0,0,Engine.Graphics.ScreenWidth,Engine.Graphics.ScreenHeight)
	-- Engine.Graphics:TextureSet(-1)
	Engine.Graphics:TextureSet(Background)
	Engine.Graphics:QuadsBegin()
		-- Engine.Graphics:SetColor(0.3,0.5,1,1) With no Background
		Engine.Graphics:SetColor(1,1,1,1)
		Engine.Graphics:QuadsDraw({QuadItem(Engine.Graphics.ScreenWidth/2,Engine.Graphics.ScreenHeight/2,Engine.Graphics.ScreenWidth,Engine.Graphics.ScreenHeight)}) -- Realy pixeled
	Engine.Graphics:QuadsEnd()
	Engine.Graphics:TextureSet(Pipe)
	Engine.Graphics:QuadsBegin()
		Engine.Graphics:SetColor(1,1,1,1)
		Engine.Graphics:QuadsDraw(PipeQuad)
		Engine.Graphics:SetRotation(math.pi/180*180)
		Engine.Graphics:QuadsDraw(PipeQuad2)
	Engine.Graphics:QuadsEnd()
	Engine.Graphics:TextureSet(Flappy)
	Engine.Graphics:QuadsBegin()
		Engine.Graphics:SetColor(1,1,1,1)
		if Started then
			Engine.Graphics:SetRotation(math.pi/180*(clamp((NotJumped*5+30)-80,-80,80)))
		end
		Engine.Graphics:QuadsDraw({QuadItem(FlappyPos.x,FlappyPos.y,148,125)})
	Engine.Graphics:QuadsEnd()
end

function AddPipe()
	while (CurPipes <= MaxPipes) do -- Pipe adding
		CurPipes = CurPipes+1
		local rand = math.random(MinPipeLen,Engine.Graphics.ScreenHeight-MinPipeLen*2)
		PipeQuad[CurPipes] = QuadItem(Engine.Graphics.ScreenWidth/2+(PipeOffset*(CurPipes-1)),rand+739/2+PipePlace/2 ,138,739)
		PipeQuad2[CurPipes] = QuadItem(Engine.Graphics.ScreenWidth/2+(PipeOffset*(CurPipes-1)),rand-739/2-PipePlace/2 ,138,739)
		PipePos[CurPipes] = vec2f(Engine.Graphics.ScreenWidth/2+(PipeOffset*(CurPipes-1)),rand)
	end
end

function ResetPipe(i)
	local rand = math.random(MinPipeLen,Engine.Graphics.ScreenHeight-MinPipeLen*2)
	PipeQuad[i] = QuadItem(Engine.Graphics.ScreenWidth/2+(PipeOffset*(CurPipes-1)),rand+739/2+PipePlace/2 ,138,739)
	PipeQuad2[i] = QuadItem(Engine.Graphics.ScreenWidth/2+(PipeOffset*(CurPipes-1)),rand-739/2-PipePlace/2 ,138,739)
	PipePos[i] = vec2f(Engine.Graphics.ScreenWidth/2+(PipeOffset*(CurPipes-1)),rand)
end

function Jump()
	JumpHeight = 0
	NotJumped = 0
end

function EndScreen()
	BroadcastReset()
	BroadcastStart("GameOver!\nYour end score is "..Score.."\nThe game restarts in",10,5)
	BroadCastAwait = true
end

function Restart()
	Started = false
	Score = 0
	FlappyPos = vec2f(Engine.Graphics.ScreenWidth/4,Engine.Graphics.ScreenHeight/2)
	CurPipes = 0
	PipeQuad = {}
	PipeQuad2 = {}
	PipePos = {}
	JumpHeight = JumpMax
	NotJumped = 0
	Death = false
	BroadCastAwait= false
	math.randomseed(Game.Client.Tick)
	AddPipe()
end


RegisterEvent("OnRenderLevel14","RenderBird")
