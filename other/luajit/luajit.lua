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
                settings.link.libs:Add("lua51")
            end
            if platform == "win64" then
                settings.link.libpath:Add(luajit.basepath .. "/win32")
                settings.link.libs:Add("lua51")
            end
            if family == "unix" then
                if platform == "macosx" then
                    settings.link.libpath:Add(luajit.basepath .. "/macosx") --TODO osx support
                else
                    settings.link.libpath:Add(luajit.basepath .. "/unix")
                   -- AddJob(luajit.basepath .. "/unix/libluajit.a", "Build LuaJIT lirary", "cd " .. luajit.basepath .. "/LuaJIT-2.0.2;make -j2 >NUL;cp src/libluajit.a ../unix/libluajit.a")
                   -- TODO! XXX! HACK! this is run four times ALWAYS, but it does well. So I nevermind.
					ExecuteSilent("cd " .. luajit.basepath .. "/LuaJIT-2.0.2;make -j2 >NUL;rm NUL;cp -l src/libluajit.a ../unix/libluajit.a") -- build dynamically
					settings.link.libs:Add("luajit")
                end
            end
            

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
