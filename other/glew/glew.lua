glew = {
	basepath = PathDir(ModuleFilename()),

	OptFind = function (name, required)
		local check = function(option, settings)
		end

		local apply = function(option, settings)
            settings.cc.includes:Add(glew.basepath .. "/glew-1.13.0/include")
            if platform == "win32" then
                settings.link.libpath:Add(glew.basepath .. "/Win32")
                settings.link.libs:Add("glew32")
            end
            if platform == "win64" then
                settings.link.libpath:Add(glew.basepath .. "/Win64")
                settings.link.libs:Add("glew32")
            end
            if family == "unix" then
                if platform == "macosx" then
                    settings.link.libpath:Add(glew.basepath .. "/macosx") --TODO osx support
                else
                    settings.link.libpath:Add(glew.basepath .. "/unix")
					ExecuteSilent("cd " .. glew.basepath .. "/glew-1.13.0;make -j2 >NUL;rm NUL;cp -l lib/*.a ../unix") -- build dynamically
					settings.link.libs:Add("GLEW")
					settings.link.libs:Add("GLEWmx")
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
