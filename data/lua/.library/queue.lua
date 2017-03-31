
local Queue = {}
local Delay = nil

function QueueAdd()
	table.inser(Queue,)
end

function QueueSetInterval(intv)
	if type(intv) ~= "number" then
		error("You must give a number as interval!")
	end
	Delay = intv
end

function QueueWorker()
	if #Queue == 0 then return end -- nothing left
	if Game.Client.LocalTime < LastChat + _ConfigGet("ChatDelay")/10 then return end -- no spam
	Game.Chat:Say(_ConfigGet("UseTeamchat"), AnswerQueue[1]) -- add the message to the queue
	--LastChat = Game.Client.LocalTime -- used above for spamprotection
	table.remove(AnswerQueue, 1) -- remove the message from the table
	AnswerQSize = AnswerQSize - 1 -- substract one from the counter
end
