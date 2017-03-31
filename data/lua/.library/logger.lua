-------------------- IMPLEMENTATION OF A BASIC LOGGER --------------------

_logger = {}

_logger.Start = function(Filename)
	_logger._data = {}
	_logger._data.f = io.open(Filename, "a+")
	_logger._data.f:write("\n\n")
	_logger._data.f:write(os.date("--- LOGGER STARTED AT [%y-%m-%d %H:%M:%S] ---\n"))
end

_logger.Log = function(Text)
	if _logger._data.f ~= nil then
		_logger._data.f:write(Text .. "\n")
	else
		error("_logger.Log called before _logger.Start")
	end
end

_logger.Flush = function(Text)
	if _logger._data.f ~= nil then
		_logger._data.f:flush()
	else
		error("_logger.Flush called before _logger.Start")
	end
end

_logger.Stop = function(Text)
	if _logger._data.f ~= nil then
		_logger._data.f:flush()
		_logger._data.f:close()
		_logger._data.f = nil
	else
		error("_logger.Stop called before _logger.Start")
	end
end

