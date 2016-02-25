-- ALLTHEHAXX LUA DOCUMENTATION
-- Below you will find a short documentation on the Lua features that come with the AllTheHaxx client,
-- however you will not learn how to program in Lua, so if you can't develope in Lua/Luabridge yet search
-- some tutorials for that online first.
-- IF YOU THINK A NEW FUNCTION/CALLBACK SHOULD BE ADDED PLEASE OPEN A TICKET AT https://github.com/AllTheHaxx/AllTheHaxx/issues

-- If you got problems understanding how those functions work, checkout our example scripts shipped with the client.




-- Structure:
------ Event callbacks
------ Functions




-- ~~~~~~~~~~~~~~~~~~~~~~~
-- ~~~ Event callbacks ~~~
-- ~~~~~~~~~~~~~~~~~~~~~~~
-- Events are registered the following:
RegisterEvent("EventName", FunctionToExecuteOnEvent) -- Example
	-- Now, whenever the event occurs your function will be called.

-- LIST OF EVENTS (among quick explaination):
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


-- ~~~~~~~~~~~~~~~~~
-- ~~~ Functions ~~~
-- ~~~~~~~~~~~~~~~~~
--------- Namespace: _lua
_lua.SetScriptTitle(Title)
	-- Describtion: Set the script title (ingame displaying)
	-- Parameters: String
	-- Return value: none
_lua.SetScriptInfo(Info)
	-- Describtion: Set the script info (ingame displaying)
	-- Parameters: String
	-- Return value: none
_lua.SetScriptHasSettings()
	-- Describtion: Check wether the script uses a config
	-- Parameters: none
	-- Return value: Boolean

--------- Namespace: _client
_client.Connect(Address)
	-- Describtion: Connects to the given serveradress
	-- Parameters: String
	-- Return value: none
_client.GetTick()
	-- Describtion: Returns the current gametick
	-- Parameters: none
	-- Return value: Integer
_client.GetLocalCharacterID()
	-- Describtion: Returns your current ClientID, -1 on failure
	-- Parameters: none
	-- Return value: Integer
_client.GetLocalCharacterWeapon()
	-- Describtion: Return your active weapon, -1 on failure
	-- Parameters: none
	-- Return value: Integer
_client.GetLocalCharacterWeaponAmmo()
	-- Describtion: Returns your ammo, -1 on failure
	-- Parameters: none
	-- Return value: Integer
_client.GetLocalCharacterHealth()
	-- Describtion: Returns your health, -1 on failure
	-- Parameters: none
	-- Return value: Integer
_client.GetLocalCharacterArmor()
	-- Describtion: Returns your armor, -1 on failure
	-- Parameters: none
	-- Return value: Integer
_client.GetFPS)
	-- Describtion: Returns current FPS
	-- Parameters: none
	-- Return value: Integer

--------- Namespace: _ui
_ui.SetUiColor(r, g, b, a)
	-- Describtion: Sets the UI color to the given values (red, green, blue, alpha)
	-- Parameters: Float (0 to 1), Float (0 to 1), Float (0 to 1), Float (0 to 1)
	-- Return value: none
_ui.DrawUiRect(x, y, w, h, corners, rounding)
	-- Describtion: Draws a rect with the given params (X, Y, width, height, cornertype, rounding of the corners)
	-- Parameters: Float, Float, Float, Float, Integer, Float
	-- Return value: none
	
_ui.DoButton_Menu(Text, Checked, x, y, w, h, Tooltip, corners)
	-- Describtion: Draws an interactable button with the given params (Label, IsChecked?, X, Y, width, height, PopupText, cornertype)
	-- Parameters: String, Integer (0 or 1), Float, Float, Float, Float, String, Integer
	-- Return value: Integer ~= 0 if the button is pressed, otherwise zero

--------- Namespace: _game
------------- SubNamespace: chat
	_game.chat.Send(Team, Message)
		-- Describtion: Sends a chatmessage
		-- Parameters: Integer, String
		-- Return value: none
	_game.chat.Active()
		-- Describtion: Checks wether the chat is active
		-- Parameters: none
		-- Return value: Boolean
	_game.chat.AllActive()
		-- Describtion: Checks wether the "All" chat is active
		-- Parameters: none
		-- Return value: Boolean
	_game.chat.TeamActive()
		-- Describtion: Checks wether the "Team" chat is active
		-- Parameters: none
		-- Return value: Boolean
------------- SubNamespace: collision
	_game.collision.GetMapWidth()
		-- Describtion: Returns the map width (in tiles)
		-- Parameters: none
		-- Return value: Integer
	_game.collision.GetMapHeight()
		-- Describtion: Returns the map heught (in tiles)
		-- Parameters: none
		-- Return value: Integer
	_game.collision.GetTile(x, y)
		-- Describtion: Returns the  tile index at the given position
		-- Parameters: Integer, Integer
		-- Return value: Integer
------------- SubNamespace: emote
	_game.emote.Send(Emote)
		-- Describtion: Do an emoticon (Emote is the ID)
		-- Parameters: Integer
		-- Return value: none
------------- SubNamespace: controls
	_game.controls.LockInput()
		-- Describtion: Locks the players' input, call this before changing the input via Lua
		-- Parameters: none
		-- Return value: none
	_game.controls.UnlockInput()
		-- Describtion: Unlocks the players' input, call this to make the player be able to move by himself again
		-- Parameters: none
		-- Return value: none

--------- Namespace: _graphics
_graphics.GetScreenWidth()
	-- Describtion: Returns the screen width
	-- Parameters: none
	-- Return value: Integer
_graphics.GetScreenHeight()
	-- Describtion: Returns the screen height
	-- Parameters: none
	-- Return value: Integer
_graphics.BlendNone()
	-- Describtion: Sets blendmode to "none"
	-- Parameters: none
	-- Return value: Integer
_graphics.BlendNormal()
	-- Describtion: Sets blendmode to "normal"
	-- Parameters: none
	-- Return value: Integer
_graphics.BlendAdditive()
	-- Describtion: Sets blendmode to "additive"
	-- Parameters: none
	-- Return value: Integer
_graphics.SetColor(r, g, b, a)
	-- Describtion: Sets the color for further rendering tasks to the given values (red, blue, green, alpha)
	-- Parameters: Float (0 to 1), Float (0 to 1), Float (0 to 1), Float (0 to 1)
	-- Return value: none
_graphics.LoadTexture(Filename, StorageType, StoreFormat, Flags)
	-- Describtion: Loads a texture to be rendered
	-- Parameters: String, Integer, Integer, Integer
	-- Return value: none
_graphics.RenderTexture(ID, x, y, w, h, Rotation)
	-- Describtion: Renders a loaded texture with the given params (ID, X, Y, width, height, rotation)
	-- Parameters: Integer, Float, Float, Float, Float, Float
	-- Return value: none
