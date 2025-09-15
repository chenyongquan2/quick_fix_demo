set_exceptions "cxx"
set_encodings "utf-8"

--- However, by default, the Microsoft Visual C++ compiler does not 
--- always report the correct value for the __cplusplus macro. 
--- eg: C++20: __cplusplus is 202002L
--- To make the compiler report the correct value, you can use 
--- the /Zc:__cplusplus flag.
set_languages "c++20"
add_cxxflags("cl::/Zc:__cplusplus")

--- The /bigobj flag increases the number of sections that an object file 
--- can contain. This is useful when you have very large source files that 
--- result in a large number of sections, which can exceed the default limit.
--- This flag is often necessary when dealing with large templates or
--- heavily templated code in C++.
add_cxxflags("/bigobj")

--- The /FS flag enables file sharing mode, allowing multiple compiler processes
--- to write to the same .PDB file simultaneously.
add_cxxflags("/FS")

add_cxxflags(
	"/W4", --- warning level 4, like -Wall in gcc
	"/analyze",  --- msvc static code analysis support 
	"/permissive-",  --- enforce strict standard conformance
	"/sdl",  --- enable additional security features
	"/external:anglebrackets", --- treat all headers files included in <> as external code
 	"/analyze:external-", --- didn't analyze external code
 {force = true} )
set_policy("build.warning", true)

if is_mode("release") then 
	--- generate PDB
	add_cxxflags("/Zi")      
	add_ldflags("/DEBUG")    
	--add_cxxflags("/O2")
else 
	add_cxxflags("/Zi")      
	add_ldflags("/DEBUG") 
	add_cxxflags("/Od", --- -O0 in gcc
	 "/RTC1" --- enable basic run-time checks, this cannot be set in release mode
	 )
end

set_config("pkg_searchdirs", "$(env PROJECTS_PATH)/../.xmake_pkgs")
set_config("vcpkg", "$(env PROJECTS_PATH)/vcpkg")

add_rules("mode.debug", "mode.release")
add_rules("mode.profile", "mode.coverage", "mode.asan", "mode.tsan", "mode.lsan", "mode.ubsan")
add_rules("plugin.compile_commands.autoupdate", { outputdir = "build", lsp = "clangd" })
add_rules("utils.install.pkgconfig_importfiles")
add_rules("utils.install.cmake_importfiles")
add_runenvs("PATH", "$(projectdir)/bin")


add_requireconfs("*", { debug = is_mode("debug") })

-- add_requires("spdlog", { alias = "spdlog", configs = { header_only = true }, debug = is_mode("debug") })
-- add_requires("fmt", { alias = "fmt", configs = { header_only = true }, debug = is_mode("debug") })
add_requires("conan::quickfix/1.15.1", { alias = "quickfix", configs = { defines = { "HAVE_SSL=ON" } }, debug = is_mode("debug") })
add_requires("conan::spdlog/1.13.0", { alias = "spdlog", configs = { header_only = true }, debug = is_mode("debug") })


add_defines("HAVE_STD_UNIQUE_PTR", --- if not define this quickfix will use std::auto_ptr which has been deprecated in c++17, cannot compile under cpp20
	"HAVE_SSL"                     --- invoke SSL support of quickfix
	, "_UNICODE", "UNICODE", "NOMINMAX", "BOOST_ASIO_HAS_CO_AWAIT", "BOOST_ASIO_HAS_STD_COROUTINE",
	"SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE", "SPDLOG_WCHAR_TO_UTF8_SUPPORT", "WIN32_LEAN_AND_MEAN"
)

-- add_packages("mt5sdk", "mt4sdk", "grpc", "spdlog", "packio", "opentelemetry-cpp",
-- 	"fmt", "quickfix", "boost", "cppkafka", "cppcodec", "hiredis",
--     "libiconv")

add_packages("quickfix", "spdlog")



target("echo_acceptor")
    set_kind("binary")
    add_files("src/echo_acceptor.cpp")
    
    if is_plat("windows") then
        add_defines("WIN32_LEAN_AND_MEAN", "_WIN32_WINNT=0x0601")
        add_syslinks("ws2_32")
    end

target("echo_initiator")
    set_kind("binary")
    add_files("src/echo_initiator.cpp")
    if is_plat("windows") then
        add_defines("WIN32_LEAN_AND_MEAN", "_WIN32_WINNT=0x0601")
        add_syslinks("ws2_32")
    end


after_build(function(target)
	import("core.project.task")
	task.run("uber_pkg", { archive = false, target = target:name() })
end)

task("uber_pkg")
on_run(function()
	import("core.project.project")
	import("core.project.config")
	import("utils.archive")
	import("core.base.option")

	config.load()

	local tgtname = option.get("target")
	local target = tgtname and project.target(tgtname) or nil
	if not target then
		raise("uber_pkg: invalid target")
	end

	local list = { vformat(path.join("$(projectdir)", target:targetdir(), "*")),
		vformat("$(projectdir)/bin/*"),
		
		vformat("$(env WINDIR)/system32/msvcp140_atomic_wait.dll")
	}

	if option.get("archive") then
		local to = string.format("%s/%s-%s.zip", config.buildir(), target:name(), config.arch())
		print("archive files", list, "to", to)
		archive.archive(to, list, { recurse = false, verbose = true })
	else
		local to = path.join(target:targetdir())
		for i, f in ipairs(list) do
			if i > 1 then
				print("begin copy", f, "to", to)
				os.cp(f, to)
				print("end copy", f, "to", to)
			end
		end
	end
end)


