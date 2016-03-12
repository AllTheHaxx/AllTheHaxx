
-- ***** THIS IS OUTDATED, PLEASE REFER TO THE LuaManual INSTEAD *****

-- ALLTHEHAXX LUA DOCUMENTATION
-- Below you will find a short documentation on the Lua features that come with the AllTheHaxx client,
-- however you will not learn how to program in Lua, so if you can't develope in Lua/Luabridge yet search
-- some tutorials for that online first.
-- IF YOU THINK A NEW FUNCTION/CALLBACK SHOULD BE ADDED PLEASE OPEN A TICKET AT https://github.com/AllTheHaxx/AllTheHaxx/issues

-- If you got problems understanding how those functions work, checkout our example scripts shipped with the client.




-- Structure:
------ General
------ Specific callbacks
------ Event callbacks
------ Functions
------ Types




-- ~~~~~~~~~~~~~~~
-- ~~~ General ~~~
-- ~~~~~~~~~~~~~~~

--XXX The script header XXX--

--Every script can (and should) start with the following 3 lines:
g_ScriptTitle = "my cool script title"
g_ScriptInfo = "written by me"
config = {}
 -- g_ScriptTitle: sets an individual title for the script, if not given the filename will be used
 -- g_ScriptInfo: sets a subtext below the script title, if not given it stays empty
 -- config = {}: creates an empty table in which you can store your scripts configuration


--XXX How to use a settings page and script configs XXX--

--- XXX §1: Adding a custom settings page

-- In order to let the api know that your script has a settings page, you must define
-- OnScriptRenderSettings and OnScriptSaveSettings as functions.

--Example code to get your settings page working:

function OnScriptRenderSettings(x, y, w, h)
	-- some code to create a nice GUI goes here
end

function OnScriptSaveSettings()
	-- Functions to write the config down go here. The "config" is also a lua script
	-- which holds the contents of your 'config' table. For example:
	file = io.open("lua/myscript.config", "w+")
	for k, v in next, config do -- DON'T use ipairs(config) here since it won't get you all values!
		if(type(k) == "string") then
			file:write("config[\"" .. k .. "\"] = " .. v .. "\n")
		else
			file:write("config[" .. k .. "] = " .. v .. "\n")
		end
	end
	file:flush()
	file:close()
end -- This function should be okay for nearly every script, so you don't have to get too creative ;)


--- XXX §2: Loading the previously saved config

--     ******* !TODO> VERY IMPROTANT <TODO! *******
--
--      If you save your config to a file, the file MUST end with either '.config' or '.lua'
--      If you don't stick to this, you will be UNABLE to "Import()" it again!!
--     *******     *******      *******     *******

-- Since your written down script config is also just a lua file (it really should be!), you
-- don't have to do anything yourself to get it loaded:

-- (the callback function OnScriptInit() has an own explanation below the events)
function OnScriptInit()
	-- the following call will execute a lua file using this file's lua state
	success = _system.Import("myscript.config") -- note that the "lua/" infront of it is gone now! (see notes below the events)
	if success == true then
		print("Settings loaded!")
	else
		print("Failed to load settings")
		return false -- returning false here will abort the execution of your script because you
		             -- indicated that there were errors and your script is thus not runnable
	end
	return true -- DO NEVER EVER FORGET THIS! If the function gives back control WITHOUT any return,
	            -- the client will CRASH in any circumstance! So please be careful!
end



-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
-- ~~~~~~~~~~~~~~~~~~~~~~~
-- ~~~ Event callbacks ~~~
-- ~~~~~~~~~~~~~~~~~~~~~~~
-- Events are registered the following:
RegisterEvent("EventName", FunctionToExecuteOnEvent) -- Example
	-- Now, whenever the event occurs your function will be called.

-- LIST OF EVENTS (among quick explaination):
RegisterEvent("OnTick", YourCallbackFunction) 
	-- Called on every tick (gameclient render)
	-- Callback function parameters: none

RegisterEvent("OnStateChange", YourCallbackFunction) 
	-- Called when the clientstate changes (i.E. from offline to online - while joining a server)
	-- Callback function parameters: YourCallbackFunction(NewState, OldState)

RegisterEvent("OnChat", YourCallbackFunction) 
	-- Called when recieving a chat message (including own ones)
	-- Callback function parameters: YourCallbackFunction(ClientID, Team, Message)

RegisterEvent("OnRenderBackground", YourCallbackFunction) 
	-- Called whenever the background is rendered (these moving sqares in the backgound while being in the menu)
	-- Callback function parameters: none

RegisterEvent("OnRenderScoreboard", YourCallbackFunction) 
	-- Called whenever the scoreboard is rendered
	-- Callback function parameters: none

RegisterEvent("OnKill", YourCallbackFunction) 
	-- Called when a player is killed.
	-- Callback function parameters: YourCallbackFunction(KillerID, VictimID, Weapon)

RegisterEvent("OnRenderLevelX", YourCallbackFunction) -- Replace X with any number from 1 to 27 to render more in the foreground.
	-- Called when the Level X (replace it, see above) is rendered. Higher X = more in the foreground, lower X = more in the background.
	-- Callback function parameters: none

RegisterEvent("OnEnterGame", YourCallbackFunction)
	-- Called when you join a server and are ready to enter the game.
	-- Callback function parameters: none

RegisterEvent("OnKeyPress", YourCallbackFunction)
	-- Called when you press a key
	-- Callback function parameters: YourCallbackFunction(Key)

RegisterEvent("OnKeyRelease", YourCallbackFunction)
	-- Called when you release a key
	-- Callback function parameters: YourCallbackFunction(Key)



-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
-- ~~~~~~~~~~~~~~~~~~~~~~~~~~
-- ~~~ Specific callbacks ~~~
-- ~~~~~~~~~~~~~~~~~~~~~~~~~~
--------- You can define the following functions if you wish to, but they're not required.
function OnScriptInit() return <true/false> end
	-- Description: Will be executed right after loading your script
	-- Parameters: none
	-- Return value: Boolean
	-- Additional Info: If you give back from this function without return value, the client
	--                  will definitely crash. There is nothing we could do against it.

function OnScriptRenderSettings(x, y, w, h) return nil end
	-- Description: Renders a settings gui
	-- Parameters: float, float, float, float (windows bounds, also known as "MainView")
	-- Return value: none
	-- Additional Info: You must define this function if you want the "settings" button to show up

function OnScriptSaveSettings() return nil end
	-- Description: Called when the user closes your settings page. Used to write down the config (see above)
	-- Parameters: none
	-- Return value: none
	-- Additional Info: You must define this function if you want the "settings" button to show up



-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
-- ~~~~~~~~~~~~~~~~~
-- ~~~ Functions ~~~
-- ~~~~~~~~~~~~~~~~~

Import(UID, Filename)
	-- Description: Execute a lua file (mainly used for loading configs)
	-- Parameters: 'g_ScriptUID', String
	-- Return value: Boolean
	-- Additional Info:
	--  -> You MUST pass 'g_ScriptUID' and nothing else as parameter 1 or it will fail
	--  -> 'Filename' is relativ to the 'lua/' folder
KillScript(UID)
	-- Description: Stop execution of the script; deactivates it
	-- Parameters: 'g_ScriptUID'
	-- Return value: Boolean
	-- Additional Info: You MUST pass 'g_ScriptUID' and nothing else as parameter or it will fail

--------- Namespace: _client
_client.GetPlayerScore(ClientID)
	-- Description: Returns the score of the player with the given ClientID
	-- Parameters: Integer
	-- Return value: Integer

--------- Namespace: _ui
_ui.SetUiColor(r, g, b, a)
	-- Description: Sets the UI color to the given values (red, green, blue, alpha)
	-- Parameters: Float (0 to 1), Float (0 to 1), Float (0 to 1), Float (0 to 1)
	-- Return value: none
_ui.DrawUiRect(x, y, w, h, corners, rounding)
	-- Description: Draws a rect with the given params (X, Y, width, height, cornertype, rounding of the corners)
	-- Parameters: Float, Float, Float, Float, Integer, Float
	-- Return value: none
	
_ui.DoButton_Menu(Text, Checked, x, y, w, h, Tooltip, corners)
	-- Description: Draws an interactable button with the given params (Label, IsChecked?, X, Y, width, height, PopupText, cornertype)
	-- Parameters: String, Integer (0 or 1), Float, Float, Float, Float, String, Integer
	-- Return value: Integer ~= 0 if the button is pressed, otherwise zero

--------- Namespace: _graphics
_graphics.GetScreenWidth()
	-- Description: Returns the screen width
	-- Parameters: none
	-- Return value: Integer
_graphics.GetScreenHeight()
	-- Description: Returns the screen height
	-- Parameters: none
	-- Return value: Integer
_graphics.BlendNone()
	-- Description: Sets blendmode to "none"
	-- Parameters: none
	-- Return value: Integer
_graphics.BlendNormal()
	-- Description: Sets blendmode to "normal"
	-- Parameters: none
	-- Return value: Integer
_graphics.BlendAdditive()
	-- Description: Sets blendmode to "additive"
	-- Parameters: none
	-- Return value: Integer
_graphics.SetColor(r, g, b, a)
	-- Description: Sets the color for further rendering tasks to the given values (red, blue, green, alpha)
	-- Parameters: Float (0 to 1), Float (0 to 1), Float (0 to 1), Float (0 to 1)
	-- Return value: none
_graphics.LoadTexture(Filename, StorageType, StoreFormat, Flags)
	-- Description: Loads a texture to be rendered
	-- Parameters: String, Integer, Integer, Integer
	-- Return value: none
_graphics.RenderTexture(ID, x, y, w, h, Rotation)
	-- Description: Renders a loaded texture with the given params (ID, X, Y, width, height, rotation)
	-- Parameters: Integer, Float, Float, Float, Float, Float
	-- Return value: none



-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
-- ~~~~~~~~~~~~~
-- ~~~ Types ~~~
-- ~~~~~~~~~~~~~
vec2 -- holding integers x, y
vec3 -- holding  integersx, y, z
vec4 -- holding  integersr, g, b, a

vec2f -- holding floats x, y
vec3f -- holding floats x, y, z
vec4f -- holding floats r, g, b, a
