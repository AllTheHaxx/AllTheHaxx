luajit = {
	basepath = PathDir(ModuleFilename()),

	OptFind = function (name, required)
		local check = function(option, settings)
		end

		local apply = function(option, settings)
            --todo: dynamicly build luajit for the platform
            settings.cc.includes:Add(luajit.basepath .. "/include")
            if platform == "win32" then
                settings.link.libpath:Add(luajit.basepath .. "/win32")
            end
            if platform == "win64" then
                settings.link.libpath:Add(luajit.basepath .. "/win32")
            end
            if family == "unix" then
                if platform == "macosx" then
                    settings.link.libpath:Add(luajit.basepath .. "/macosx")
                else
                    settings.link.libpath:Add(luajit.basepath .. "/unix") --meh
                end
            end
            settings.link.libs:Add("luajit-5.1")

		end

		local save = function(option, output)
		end

		local display = function(option)
            return ""
		end

		local o = MakeOption(name, 0, check, save, display)
		o.Apply = apply
		o.include_path = nil
		o.lib_path = nil
		o.required = required
		return o
	end
}
