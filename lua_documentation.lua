-- Below you will find a short documentation on the Lua features that come with the AllTheHaxx client,
-- however you will not learn how to program in Lua, so if you can't develope in Lua/Luabridge yet search
-- some tutorials for that online first.
-- IF YOU THINK A NEW FUNCTION/CALLBACK SHOULD BE ADDED PLEASE OPEN A TICKET AT https://github.com/AllTheHaxx/AllTheHaxx/issues


-- Structure:
------ Event callbacks
------ Functions


-- ~~~ Event callbacks ~~~
-- Events are registered the following:
RegisterEvent("EventName", FunctionToExecuteOnEvent) -- Example
-- Now, whenever the event occurs your function will be called.
-- LIST OF EVENTS (among explaination):

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

-- ~~~ Functions ~~~
------ _lua
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

------ _client
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
