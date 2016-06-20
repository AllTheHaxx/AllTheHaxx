cmake_minimum_required(VERSION 3.5)
project(AllTheHaxx)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(
        other/sdl/include
        other/curl/include
        other/freetype/include
        other/luajit/include
        other/luajit/LuaJIT-2.0.2/src
        other/mysql/include
        other/opus/include
        src
)

set(SOURCE_FILES
#    other/curl/include/curl/curl.h
#    other/curl/include/curl/curlbuild.h
#    other/curl/include/curl/curlrules.h
#    other/curl/include/curl/curlver.h
#    other/curl/include/curl/easy.h
#    other/curl/include/curl/mprintf.h
#    other/curl/include/curl/multi.h
#    other/curl/include/curl/stdcheaders.h
#    other/curl/include/curl/typecheck-gcc.h
#    other/curl/include/config-win32.h
#    other/curl/include/libssh2_config.h
#    other/freetype/include/freetype/config/ftconfig.h
#    other/freetype/include/freetype/config/ftheader.h
#    other/freetype/include/freetype/config/ftmodule.h
#    other/freetype/include/freetype/config/ftoption.h
#    other/freetype/include/freetype/config/ftstdlib.h
#    other/freetype/include/freetype/internal/services/svbdf.h
#    other/freetype/include/freetype/internal/services/svcid.h
#    other/freetype/include/freetype/internal/services/svgldict.h
#    other/freetype/include/freetype/internal/services/svgxval.h
#    other/freetype/include/freetype/internal/services/svkern.h
#    other/freetype/include/freetype/internal/services/svmm.h
#    other/freetype/include/freetype/internal/services/svotval.h
#    other/freetype/include/freetype/internal/services/svpfr.h
#    other/freetype/include/freetype/internal/services/svpostnm.h
#    other/freetype/include/freetype/internal/services/svpscmap.h
#    other/freetype/include/freetype/internal/services/svpsinfo.h
#    other/freetype/include/freetype/internal/services/svsfnt.h
#    other/freetype/include/freetype/internal/services/svttcmap.h
#    other/freetype/include/freetype/internal/services/svtteng.h
#    other/freetype/include/freetype/internal/services/svttglyf.h
#    other/freetype/include/freetype/internal/services/svwinfnt.h
#    other/freetype/include/freetype/internal/services/svxf86nm.h
#    other/freetype/include/freetype/internal/autohint.h
#    other/freetype/include/freetype/internal/ftcalc.h
#    other/freetype/include/freetype/internal/ftdebug.h
#    other/freetype/include/freetype/internal/ftdriver.h
#    other/freetype/include/freetype/internal/ftgloadr.h
#    other/freetype/include/freetype/internal/ftmemory.h
#    other/freetype/include/freetype/internal/ftobjs.h
#    other/freetype/include/freetype/internal/ftpic.h
#    other/freetype/include/freetype/internal/ftrfork.h
#    other/freetype/include/freetype/internal/ftserv.h
#    other/freetype/include/freetype/internal/ftstream.h
#    other/freetype/include/freetype/internal/fttrace.h
#    other/freetype/include/freetype/internal/ftvalid.h
#    other/freetype/include/freetype/internal/internal.h
#    other/freetype/include/freetype/internal/psaux.h
#    other/freetype/include/freetype/internal/pshints.h
#    other/freetype/include/freetype/internal/sfnt.h
#    other/freetype/include/freetype/internal/t1types.h
#    other/freetype/include/freetype/internal/tttypes.h
#    other/freetype/include/freetype/freetype.h
#    other/freetype/include/freetype/ftadvanc.h
#    other/freetype/include/freetype/ftbbox.h
#    other/freetype/include/freetype/ftbdf.h
#    other/freetype/include/freetype/ftbitmap.h
#    other/freetype/include/freetype/ftbzip2.h
#    other/freetype/include/freetype/ftcache.h
#    other/freetype/include/freetype/ftchapters.h
#    other/freetype/include/freetype/ftcid.h
#    other/freetype/include/freetype/fterrdef.h
#    other/freetype/include/freetype/fterrors.h
#    other/freetype/include/freetype/ftgasp.h
#    other/freetype/include/freetype/ftglyph.h
#    other/freetype/include/freetype/ftgxval.h
#    other/freetype/include/freetype/ftgzip.h
#    other/freetype/include/freetype/ftimage.h
#    other/freetype/include/freetype/ftincrem.h
#    other/freetype/include/freetype/ftlcdfil.h
#    other/freetype/include/freetype/ftlist.h
#    other/freetype/include/freetype/ftlzw.h
#    other/freetype/include/freetype/ftmac.h
#    other/freetype/include/freetype/ftmm.h
#    other/freetype/include/freetype/ftmodapi.h
#    other/freetype/include/freetype/ftmoderr.h
#    other/freetype/include/freetype/ftotval.h
#    other/freetype/include/freetype/ftoutln.h
#    other/freetype/include/freetype/ftpfr.h
#    other/freetype/include/freetype/ftrender.h
#    other/freetype/include/freetype/ftsizes.h
#    other/freetype/include/freetype/ftsnames.h
#    other/freetype/include/freetype/ftstroke.h
#    other/freetype/include/freetype/ftsynth.h
#    other/freetype/include/freetype/ftsystem.h
#    other/freetype/include/freetype/fttrigon.h
#    other/freetype/include/freetype/fttypes.h
#    other/freetype/include/freetype/ftwinfnt.h
#    other/freetype/include/freetype/ftxf86.h
#    other/freetype/include/freetype/t1tables.h
#    other/freetype/include/freetype/ttnameid.h
#    other/freetype/include/freetype/tttables.h
#    other/freetype/include/freetype/tttags.h
#    other/freetype/include/freetype/ttunpat.h
#    other/freetype/include/ft2build.h
#    other/luajit/include/lauxlib.h
#    other/luajit/include/lua.h
#    other/luajit/include/lua.hpp
#    other/luajit/include/luaconf.h
#    other/luajit/include/luajit.h
#    other/luajit/include/lualib.h
#    other/luajit/LuaJIT-2.0.2/dynasm/dasm_arm.h
#    other/luajit/LuaJIT-2.0.2/dynasm/dasm_mips.h
#    other/luajit/LuaJIT-2.0.2/dynasm/dasm_ppc.h
#    other/luajit/LuaJIT-2.0.2/dynasm/dasm_proto.h
#    other/luajit/LuaJIT-2.0.2/dynasm/dasm_x86.h
#    other/luajit/LuaJIT-2.0.2/src/host/buildvm.c
#    other/luajit/LuaJIT-2.0.2/src/host/buildvm.h
#    other/luajit/LuaJIT-2.0.2/src/host/buildvm_arch.h
#    other/luajit/LuaJIT-2.0.2/src/host/buildvm_asm.c
#    other/luajit/LuaJIT-2.0.2/src/host/buildvm_fold.c
#    other/luajit/LuaJIT-2.0.2/src/host/buildvm_lib.c
#    other/luajit/LuaJIT-2.0.2/src/host/buildvm_peobj.c
#    other/luajit/LuaJIT-2.0.2/src/host/minilua.c
#    other/luajit/LuaJIT-2.0.2/src/lauxlib.h
#    other/luajit/LuaJIT-2.0.2/src/lib_aux.c
#    other/luajit/LuaJIT-2.0.2/src/lib_base.c
#    other/luajit/LuaJIT-2.0.2/src/lib_bit.c
#    other/luajit/LuaJIT-2.0.2/src/lib_debug.c
#    other/luajit/LuaJIT-2.0.2/src/lib_ffi.c
#    other/luajit/LuaJIT-2.0.2/src/lib_init.c
#    other/luajit/LuaJIT-2.0.2/src/lib_io.c
#    other/luajit/LuaJIT-2.0.2/src/lib_jit.c
#    other/luajit/LuaJIT-2.0.2/src/lib_math.c
#    other/luajit/LuaJIT-2.0.2/src/lib_os.c
#    other/luajit/LuaJIT-2.0.2/src/lib_package.c
#    other/luajit/LuaJIT-2.0.2/src/lib_string.c
#    other/luajit/LuaJIT-2.0.2/src/lib_table.c
#    other/luajit/LuaJIT-2.0.2/src/lj_alloc.c
#    other/luajit/LuaJIT-2.0.2/src/lj_alloc.h
#    other/luajit/LuaJIT-2.0.2/src/lj_api.c
#    other/luajit/LuaJIT-2.0.2/src/lj_arch.h
#    other/luajit/LuaJIT-2.0.2/src/lj_asm.c
#    other/luajit/LuaJIT-2.0.2/src/lj_asm.h
#    other/luajit/LuaJIT-2.0.2/src/lj_asm_arm.h
#    other/luajit/LuaJIT-2.0.2/src/lj_asm_mips.h
#    other/luajit/LuaJIT-2.0.2/src/lj_asm_ppc.h
#    other/luajit/LuaJIT-2.0.2/src/lj_asm_x86.h
#    other/luajit/LuaJIT-2.0.2/src/lj_bc.c
#    other/luajit/LuaJIT-2.0.2/src/lj_bc.h
#    other/luajit/LuaJIT-2.0.2/src/lj_bcdef.h
#    other/luajit/LuaJIT-2.0.2/src/lj_bcdump.h
#    other/luajit/LuaJIT-2.0.2/src/lj_bcread.c
#    other/luajit/LuaJIT-2.0.2/src/lj_bcwrite.c
#    other/luajit/LuaJIT-2.0.2/src/lj_carith.c
#    other/luajit/LuaJIT-2.0.2/src/lj_carith.h
#    other/luajit/LuaJIT-2.0.2/src/lj_ccall.c
#    other/luajit/LuaJIT-2.0.2/src/lj_ccall.h
#    other/luajit/LuaJIT-2.0.2/src/lj_ccallback.c
#    other/luajit/LuaJIT-2.0.2/src/lj_ccallback.h
#    other/luajit/LuaJIT-2.0.2/src/lj_cconv.c
#    other/luajit/LuaJIT-2.0.2/src/lj_cconv.h
#    other/luajit/LuaJIT-2.0.2/src/lj_cdata.c
#    other/luajit/LuaJIT-2.0.2/src/lj_cdata.h
#    other/luajit/LuaJIT-2.0.2/src/lj_char.c
#    other/luajit/LuaJIT-2.0.2/src/lj_char.h
#    other/luajit/LuaJIT-2.0.2/src/lj_clib.c
#    other/luajit/LuaJIT-2.0.2/src/lj_clib.h
#    other/luajit/LuaJIT-2.0.2/src/lj_cparse.c
#    other/luajit/LuaJIT-2.0.2/src/lj_cparse.h
#    other/luajit/LuaJIT-2.0.2/src/lj_crecord.c
#    other/luajit/LuaJIT-2.0.2/src/lj_crecord.h
#    other/luajit/LuaJIT-2.0.2/src/lj_ctype.c
#    other/luajit/LuaJIT-2.0.2/src/lj_ctype.h
#    other/luajit/LuaJIT-2.0.2/src/lj_debug.c
#    other/luajit/LuaJIT-2.0.2/src/lj_debug.h
#    other/luajit/LuaJIT-2.0.2/src/lj_def.h
#    other/luajit/LuaJIT-2.0.2/src/lj_dispatch.c
#    other/luajit/LuaJIT-2.0.2/src/lj_dispatch.h
#    other/luajit/LuaJIT-2.0.2/src/lj_emit_arm.h
#    other/luajit/LuaJIT-2.0.2/src/lj_emit_mips.h
#    other/luajit/LuaJIT-2.0.2/src/lj_emit_ppc.h
#    other/luajit/LuaJIT-2.0.2/src/lj_emit_x86.h
#    other/luajit/LuaJIT-2.0.2/src/lj_err.c
#    other/luajit/LuaJIT-2.0.2/src/lj_err.h
#    other/luajit/LuaJIT-2.0.2/src/lj_errmsg.h
#    other/luajit/LuaJIT-2.0.2/src/lj_ff.h
#    other/luajit/LuaJIT-2.0.2/src/lj_ffdef.h
#    other/luajit/LuaJIT-2.0.2/src/lj_ffrecord.c
#    other/luajit/LuaJIT-2.0.2/src/lj_ffrecord.h
#    other/luajit/LuaJIT-2.0.2/src/lj_folddef.h
#    other/luajit/LuaJIT-2.0.2/src/lj_frame.h
#    other/luajit/LuaJIT-2.0.2/src/lj_func.c
#    other/luajit/LuaJIT-2.0.2/src/lj_func.h
#    other/luajit/LuaJIT-2.0.2/src/lj_gc.c
#    other/luajit/LuaJIT-2.0.2/src/lj_gc.h
#    other/luajit/LuaJIT-2.0.2/src/lj_gdbjit.c
#    other/luajit/LuaJIT-2.0.2/src/lj_gdbjit.h
#    other/luajit/LuaJIT-2.0.2/src/lj_ir.c
#    other/luajit/LuaJIT-2.0.2/src/lj_ir.h
#    other/luajit/LuaJIT-2.0.2/src/lj_ircall.h
#    other/luajit/LuaJIT-2.0.2/src/lj_iropt.h
#    other/luajit/LuaJIT-2.0.2/src/lj_jit.h
#    other/luajit/LuaJIT-2.0.2/src/lj_lex.c
#    other/luajit/LuaJIT-2.0.2/src/lj_lex.h
#    other/luajit/LuaJIT-2.0.2/src/lj_lib.c
#    other/luajit/LuaJIT-2.0.2/src/lj_lib.h
#    other/luajit/LuaJIT-2.0.2/src/lj_libdef.h
#    other/luajit/LuaJIT-2.0.2/src/lj_load.c
#    other/luajit/LuaJIT-2.0.2/src/lj_mcode.c
#    other/luajit/LuaJIT-2.0.2/src/lj_mcode.h
#    other/luajit/LuaJIT-2.0.2/src/lj_meta.c
#    other/luajit/LuaJIT-2.0.2/src/lj_meta.h
#    other/luajit/LuaJIT-2.0.2/src/lj_obj.c
#    other/luajit/LuaJIT-2.0.2/src/lj_obj.h
#    other/luajit/LuaJIT-2.0.2/src/lj_opt_dce.c
#    other/luajit/LuaJIT-2.0.2/src/lj_opt_fold.c
#    other/luajit/LuaJIT-2.0.2/src/lj_opt_loop.c
#    other/luajit/LuaJIT-2.0.2/src/lj_opt_mem.c
#    other/luajit/LuaJIT-2.0.2/src/lj_opt_narrow.c
#    other/luajit/LuaJIT-2.0.2/src/lj_opt_sink.c
#    other/luajit/LuaJIT-2.0.2/src/lj_opt_split.c
#    other/luajit/LuaJIT-2.0.2/src/lj_parse.c
#    other/luajit/LuaJIT-2.0.2/src/lj_parse.h
#    other/luajit/LuaJIT-2.0.2/src/lj_recdef.h
#    other/luajit/LuaJIT-2.0.2/src/lj_record.c
#    other/luajit/LuaJIT-2.0.2/src/lj_record.h
#    other/luajit/LuaJIT-2.0.2/src/lj_snap.c
#    other/luajit/LuaJIT-2.0.2/src/lj_snap.h
#    other/luajit/LuaJIT-2.0.2/src/lj_state.c
#    other/luajit/LuaJIT-2.0.2/src/lj_state.h
#    other/luajit/LuaJIT-2.0.2/src/lj_str.c
#    other/luajit/LuaJIT-2.0.2/src/lj_str.h
#    other/luajit/LuaJIT-2.0.2/src/lj_strscan.c
#    other/luajit/LuaJIT-2.0.2/src/lj_strscan.h
#    other/luajit/LuaJIT-2.0.2/src/lj_tab.c
#    other/luajit/LuaJIT-2.0.2/src/lj_tab.h
#    other/luajit/LuaJIT-2.0.2/src/lj_target.h
#    other/luajit/LuaJIT-2.0.2/src/lj_target_arm.h
#    other/luajit/LuaJIT-2.0.2/src/lj_target_mips.h
#    other/luajit/LuaJIT-2.0.2/src/lj_target_ppc.h
#    other/luajit/LuaJIT-2.0.2/src/lj_target_x86.h
#    other/luajit/LuaJIT-2.0.2/src/lj_trace.c
#    other/luajit/LuaJIT-2.0.2/src/lj_trace.h
#    other/luajit/LuaJIT-2.0.2/src/lj_traceerr.h
#    other/luajit/LuaJIT-2.0.2/src/lj_udata.c
#    other/luajit/LuaJIT-2.0.2/src/lj_udata.h
#    other/luajit/LuaJIT-2.0.2/src/lj_vm.h
#    other/luajit/LuaJIT-2.0.2/src/lj_vmevent.c
#    other/luajit/LuaJIT-2.0.2/src/lj_vmevent.h
#    other/luajit/LuaJIT-2.0.2/src/lj_vmmath.c
#    other/luajit/LuaJIT-2.0.2/src/ljamalg.c
#    other/luajit/LuaJIT-2.0.2/src/lua.h
#    other/luajit/LuaJIT-2.0.2/src/lua.hpp
#    other/luajit/LuaJIT-2.0.2/src/luaconf.h
#    other/luajit/LuaJIT-2.0.2/src/luajit.c
#    other/luajit/LuaJIT-2.0.2/src/luajit.h
#    other/luajit/LuaJIT-2.0.2/src/lualib.h
#    other/mysql/include/cppconn/build_config.h
#    other/mysql/include/cppconn/config.h
#    other/mysql/include/cppconn/connection.h
#    other/mysql/include/cppconn/datatype.h
#    other/mysql/include/cppconn/driver.h
#    other/mysql/include/cppconn/exception.h
#    other/mysql/include/cppconn/metadata.h
#    other/mysql/include/cppconn/parameter_metadata.h
#    other/mysql/include/cppconn/prepared_statement.h
#    other/mysql/include/cppconn/resultset.h
#    other/mysql/include/cppconn/resultset_metadata.h
#    other/mysql/include/cppconn/sqlstring.h
#    other/mysql/include/cppconn/statement.h
#    other/mysql/include/cppconn/warning.h
#    other/mysql/include/mysql_connection.h
#    other/mysql/include/mysql_driver.h
#    other/opus/include/ogg/ogg.h
#    other/opus/include/ogg/os_types.h
#    other/opus/include/opus/opus.h
#    other/opus/include/opus/opus_custom.h
#    other/opus/include/opus/opus_defines.h
#    other/opus/include/opus/opus_multistream.h
#    other/opus/include/opus/opus_types.h
#    other/opus/include/opusfile.h
#    other/sdl/include/begin_code.h
#    other/sdl/include/close_code.h
#    other/sdl/include/SDL.h
#    other/sdl/include/SDL_assert.h
#    other/sdl/include/SDL_atomic.h
#    other/sdl/include/SDL_audio.h
#    other/sdl/include/SDL_bits.h
#    other/sdl/include/SDL_blendmode.h
#    other/sdl/include/SDL_clipboard.h
#    other/sdl/include/SDL_config.h
#    other/sdl/include/SDL_cpuinfo.h
#    other/sdl/include/SDL_endian.h
#    other/sdl/include/SDL_error.h
#    other/sdl/include/SDL_events.h
#    other/sdl/include/SDL_filesystem.h
#    other/sdl/include/SDL_gamecontroller.h
#    other/sdl/include/SDL_gesture.h
#    other/sdl/include/SDL_haptic.h
#    other/sdl/include/SDL_hints.h
#    other/sdl/include/SDL_joystick.h
#    other/sdl/include/SDL_keyboard.h
#    other/sdl/include/SDL_keycode.h
#    other/sdl/include/SDL_loadso.h
#    other/sdl/include/SDL_log.h
#    other/sdl/include/SDL_main.h
#    other/sdl/include/SDL_messagebox.h
#    other/sdl/include/SDL_mouse.h
#    other/sdl/include/SDL_mutex.h
#    other/sdl/include/SDL_name.h
#    other/sdl/include/SDL_opengl.h
#    other/sdl/include/SDL_opengles.h
#    other/sdl/include/SDL_opengles2.h
#    other/sdl/include/SDL_pixels.h
#    other/sdl/include/SDL_platform.h
#    other/sdl/include/SDL_power.h
#    other/sdl/include/SDL_quit.h
#    other/sdl/include/SDL_rect.h
#    other/sdl/include/SDL_render.h
#    other/sdl/include/SDL_revision.h
#    other/sdl/include/SDL_rwops.h
#    other/sdl/include/SDL_scancode.h
#    other/sdl/include/SDL_shape.h
#    other/sdl/include/SDL_stdinc.h
#    other/sdl/include/SDL_surface.h
#    other/sdl/include/SDL_system.h
#    other/sdl/include/SDL_syswm.h
#    other/sdl/include/SDL_test.h
#    other/sdl/include/SDL_test_assert.h
#    other/sdl/include/SDL_test_common.h
#    other/sdl/include/SDL_test_compare.h
#    other/sdl/include/SDL_test_crc32.h
#    other/sdl/include/SDL_test_font.h
#    other/sdl/include/SDL_test_fuzzer.h
#    other/sdl/include/SDL_test_harness.h
#    other/sdl/include/SDL_test_images.h
#    other/sdl/include/SDL_test_log.h
#    other/sdl/include/SDL_test_md5.h
#    other/sdl/include/SDL_test_random.h
#    other/sdl/include/SDL_thread.h
#    other/sdl/include/SDL_timer.h
#    other/sdl/include/SDL_touch.h
#    other/sdl/include/SDL_types.h
#    other/sdl/include/SDL_version.h
#    other/sdl/include/SDL_video.h
    scripts/SDL_scancode.h
    src/base/tl/algorithm.h
    src/base/tl/allocator.h
    src/base/tl/array.h
    src/base/tl/base.h
    src/base/tl/range.h
    src/base/tl/sorted_array.h
    src/base/tl/string.h
    src/base/tl/threading.h
    src/base/color.h
    src/base/confusables.c
    src/base/confusables.h
    src/base/debug.h
    src/base/detect.h
    src/base/math.h
    src/base/system.c
    src/base/system.h
    src/base/vmath.h
    src/engine/client/backend_sdl.cpp
    src/engine/client/backend_sdl.h
    src/engine/client/client.cpp
    src/engine/client/client.h
    src/engine/client/db_sqlite3.cpp
    src/engine/client/db_sqlite3.h
    src/engine/client/fetcher.cpp
    src/engine/client/fetcher.h
    src/engine/client/friends.cpp
    src/engine/client/friends.h
    src/engine/client/graphics_threaded.cpp
    src/engine/client/graphics_threaded.h
    src/engine/client/input.cpp
    src/engine/client/input.h
    src/engine/client/irc.cpp
    src/engine/client/irc.h
    src/engine/client/keynames.h
    src/engine/client/lua.cpp
    src/engine/client/lua.h
    src/engine/client/luabinding.cpp
    src/engine/client/luabinding.h
    src/engine/client/luafile.cpp
    src/engine/client/luafile.h
    src/engine/client/serverbrowser.cpp
    src/engine/client/serverbrowser.h
    src/engine/client/sound.cpp
    src/engine/client/sound.h
    src/engine/client/text.cpp
    src/engine/client/updater.cpp
    src/engine/client/updater.h
    src/engine/external/astar-jps/AStar.cpp
    src/engine/external/astar-jps/AStar.h
    src/engine/external/astar-jps/IndexPriorityQueue.c
    src/engine/external/astar-jps/IndexPriorityQueue.h
    src/engine/external/json-parser/json.c
    src/engine/external/json-parser/json.h
    src/engine/external/libwebsockets/alloc.c
    src/engine/external/libwebsockets/base64-decode.c
    src/engine/external/libwebsockets/config.h
    src/engine/external/libwebsockets/context.c
    src/engine/external/libwebsockets/extension-deflate-frame.c
    src/engine/external/libwebsockets/extension-deflate-frame.h
    src/engine/external/libwebsockets/extension-deflate-stream.c
    src/engine/external/libwebsockets/extension-deflate-stream.h
    src/engine/external/libwebsockets/extension.c
    src/engine/external/libwebsockets/getifaddrs.h
    src/engine/external/libwebsockets/handshake.c
    src/engine/external/libwebsockets/huftable.h
    src/engine/external/libwebsockets/lextable-strings.h
    src/engine/external/libwebsockets/lextable.h
    src/engine/external/libwebsockets/libwebsockets.c
    src/engine/external/libwebsockets/libwebsockets.h
    src/engine/external/libwebsockets/lws-plat-unix.c
    src/engine/external/libwebsockets/output.c
    src/engine/external/libwebsockets/parsers.c
    src/engine/external/libwebsockets/pollfd.c
    src/engine/external/libwebsockets/private-libwebsockets.h
    src/engine/external/libwebsockets/server-handshake.c
    src/engine/external/libwebsockets/server.c
    src/engine/external/libwebsockets/service.c
    src/engine/external/libwebsockets/sha-1.c
    src/engine/external/lua/lapi.c
    src/engine/external/lua/lapi.h
    src/engine/external/lua/lauxlib.c
    src/engine/external/lua/lauxlib.h
    src/engine/external/lua/lbaselib.c
    src/engine/external/lua/lbitlib.c
    src/engine/external/lua/lcode.c
    src/engine/external/lua/lcode.h
    src/engine/external/lua/lcorolib.c
    src/engine/external/lua/lctype.c
    src/engine/external/lua/lctype.h
    src/engine/external/lua/ldblib.c
    src/engine/external/lua/ldebug.c
    src/engine/external/lua/ldebug.h
    src/engine/external/lua/ldo.c
    src/engine/external/lua/ldo.h
    src/engine/external/lua/ldump.c
    src/engine/external/lua/lfunc.c
    src/engine/external/lua/lfunc.h
    src/engine/external/lua/lgc.c
    src/engine/external/lua/lgc.h
    src/engine/external/lua/linit.c
    src/engine/external/lua/liolib.c
    src/engine/external/lua/llex.c
    src/engine/external/lua/llex.h
    src/engine/external/lua/llimits.h
    src/engine/external/lua/lmathlib.c
    src/engine/external/lua/lmem.c
    src/engine/external/lua/lmem.h
    src/engine/external/lua/loadlib.c
    src/engine/external/lua/lobject.c
    src/engine/external/lua/lobject.h
    src/engine/external/lua/lopcodes.c
    src/engine/external/lua/lopcodes.h
    src/engine/external/lua/loslib.c
    src/engine/external/lua/lparser.c
    src/engine/external/lua/lparser.h
    src/engine/external/lua/lprefix.h
    src/engine/external/lua/lstate.c
    src/engine/external/lua/lstate.h
    src/engine/external/lua/lstring.c
    src/engine/external/lua/lstring.h
    src/engine/external/lua/lstrlib.c
    src/engine/external/lua/ltable.c
    src/engine/external/lua/ltable.h
    src/engine/external/lua/ltablib.c
    src/engine/external/lua/ltm.c
    src/engine/external/lua/ltm.h
    src/engine/external/lua/lua.h
    src/engine/external/lua/lua.hpp
    src/engine/external/lua/luaconf.h
    src/engine/external/lua/lualib.h
    src/engine/external/lua/lundump.c
    src/engine/external/lua/lundump.h
    src/engine/external/lua/lutf8lib.c
    src/engine/external/lua/lvm.c
    src/engine/external/lua/lvm.h
    src/engine/external/lua/lzio.c
    src/engine/external/lua/lzio.h
    src/engine/external/luabridge/detail/CFunctions.h
    src/engine/external/luabridge/detail/ClassInfo.h
    src/engine/external/luabridge/detail/Constructor.h
    src/engine/external/luabridge/detail/dump.h
    src/engine/external/luabridge/detail/FuncTraits.h
    src/engine/external/luabridge/detail/Iterator.h
    src/engine/external/luabridge/detail/LuaException.h
    src/engine/external/luabridge/detail/LuaHelpers.h
    src/engine/external/luabridge/detail/LuaRef.h
    src/engine/external/luabridge/detail/Namespace.h
    src/engine/external/luabridge/detail/Stack.h
    src/engine/external/luabridge/detail/TypeList.h
    src/engine/external/luabridge/detail/TypeTraits.h
    src/engine/external/luabridge/detail/Userdata.h
    src/engine/external/luabridge/LuaBridge.h
    src/engine/external/luabridge/RefCountedObject.h
    src/engine/external/luabridge/RefCountedPtr.h
    src/engine/external/md5/md5.c
    src/engine/external/md5/md5.h
    src/engine/external/openssl/aes.h
    src/engine/external/openssl/asn1.h
    src/engine/external/openssl/asn1_mac.h
    src/engine/external/openssl/asn1t.h
    src/engine/external/openssl/bio.h
    src/engine/external/openssl/blowfish.h
    src/engine/external/openssl/bn.h
    src/engine/external/openssl/buffer.h
    src/engine/external/openssl/camellia.h
    src/engine/external/openssl/cast.h
    src/engine/external/openssl/cmac.h
    src/engine/external/openssl/cms.h
    src/engine/external/openssl/comp.h
    src/engine/external/openssl/conf.h
    src/engine/external/openssl/conf_api.h
    src/engine/external/openssl/crypto.h
    src/engine/external/openssl/des.h
    src/engine/external/openssl/des_old.h
    src/engine/external/openssl/dh.h
    src/engine/external/openssl/dsa.h
    src/engine/external/openssl/dso.h
    src/engine/external/openssl/dtls1.h
    src/engine/external/openssl/e_os2.h
    src/engine/external/openssl/ebcdic.h
    src/engine/external/openssl/ec.h
    src/engine/external/openssl/ecdh.h
    src/engine/external/openssl/ecdsa.h
    src/engine/external/openssl/engine.h
    src/engine/external/openssl/err.h
    src/engine/external/openssl/evp.h
    src/engine/external/openssl/hmac.h
    src/engine/external/openssl/idea.h
    src/engine/external/openssl/krb5_asn.h
    src/engine/external/openssl/kssl.h
    src/engine/external/openssl/lhash.h
    src/engine/external/openssl/md4.h
    src/engine/external/openssl/md5.h
    src/engine/external/openssl/mdc2.h
    src/engine/external/openssl/modes.h
    src/engine/external/openssl/obj_mac.h
    src/engine/external/openssl/objects.h
    src/engine/external/openssl/ocsp.h
    src/engine/external/openssl/opensslconf.h
    src/engine/external/openssl/opensslv.h
    src/engine/external/openssl/ossl_typ.h
    src/engine/external/openssl/pem.h
    src/engine/external/openssl/pem2.h
    src/engine/external/openssl/pkcs12.h
    src/engine/external/openssl/pkcs7.h
    src/engine/external/openssl/pqueue.h
    src/engine/external/openssl/rand.h
    src/engine/external/openssl/rc2.h
    src/engine/external/openssl/rc4.h
    src/engine/external/openssl/ripemd.h
    src/engine/external/openssl/rsa.h
    src/engine/external/openssl/safestack.h
    src/engine/external/openssl/seed.h
    src/engine/external/openssl/sha.h
    src/engine/external/openssl/srp.h
    src/engine/external/openssl/srtp.h
    src/engine/external/openssl/ssl.h
    src/engine/external/openssl/ssl2.h
    src/engine/external/openssl/ssl23.h
    src/engine/external/openssl/ssl3.h
    src/engine/external/openssl/stack.h
    src/engine/external/openssl/symhacks.h
    src/engine/external/openssl/tls1.h
    src/engine/external/openssl/ts.h
    src/engine/external/openssl/txt_db.h
    src/engine/external/openssl/ui.h
    src/engine/external/openssl/ui_compat.h
    src/engine/external/openssl/whrlpool.h
    src/engine/external/openssl/x509.h
    src/engine/external/openssl/x509_vfy.h
    src/engine/external/openssl/x509v3.h
    src/engine/external/pnglite/pnglite.c
    src/engine/external/pnglite/pnglite.h
    src/engine/external/sqlite3/sqlite3.c
    src/engine/external/sqlite3/sqlite3.h
    src/engine/external/sqlite3/sqlite3ext.h
    src/engine/external/wavpack/bits.c
    src/engine/external/wavpack/float.c
    src/engine/external/wavpack/metadata.c
    src/engine/external/wavpack/unpack.c
    src/engine/external/wavpack/wavpack.h
    src/engine/external/wavpack/words.c
    src/engine/external/wavpack/wputils.c
    src/engine/external/zlib/adler32.c
    src/engine/external/zlib/compress.c
    src/engine/external/zlib/crc32.c
    src/engine/external/zlib/crc32.h
    src/engine/external/zlib/deflate.c
    src/engine/external/zlib/deflate.h
    src/engine/external/zlib/gzclose.c
    src/engine/external/zlib/gzguts.h
    src/engine/external/zlib/gzlib.c
    src/engine/external/zlib/gzread.c
    src/engine/external/zlib/gzwrite.c
    src/engine/external/zlib/infback.c
    src/engine/external/zlib/inffast.c
    src/engine/external/zlib/inffast.h
    src/engine/external/zlib/inffixed.h
    src/engine/external/zlib/inflate.c
    src/engine/external/zlib/inflate.h
    src/engine/external/zlib/inftrees.c
    src/engine/external/zlib/inftrees.h
    src/engine/external/zlib/trees.c
    src/engine/external/zlib/trees.h
    src/engine/external/zlib/uncompr.c
    src/engine/external/zlib/zconf.h
    src/engine/external/zlib/zlib.h
    src/engine/external/zlib/zutil.c
    src/engine/external/zlib/zutil.h
    src/engine/server/register.cpp
    src/engine/server/register.h
    src/engine/server/server.cpp
    src/engine/server/server.h
    src/engine/server/sql_connector.cpp
    src/engine/server/sql_connector.h
    src/engine/server/sql_server.cpp
    src/engine/server/sql_server.h
    src/engine/server/sql_string_helpers.cpp
    src/engine/server/sql_string_helpers.h
    src/engine/shared/compression.cpp
    src/engine/shared/compression.h
    src/engine/shared/config.cpp
    src/engine/shared/config.h
    src/engine/shared/config_variables.h
    src/engine/shared/console.cpp
    src/engine/shared/console.h
    src/engine/shared/datafile.cpp
    src/engine/shared/datafile.h
    src/engine/shared/demo.cpp
    src/engine/shared/demo.h
    src/engine/shared/econ.cpp
    src/engine/shared/econ.h
    src/engine/shared/engine.cpp
    src/engine/shared/fifo.cpp
    src/engine/shared/fifo.h
    src/engine/shared/filecollection.cpp
    src/engine/shared/filecollection.h
    src/engine/shared/huffman.cpp
    src/engine/shared/huffman.h
    src/engine/shared/jobs.cpp
    src/engine/shared/jobs.h
    src/engine/shared/kernel.cpp
    src/engine/shared/linereader.cpp
    src/engine/shared/linereader.h
    src/engine/shared/map.cpp
    src/engine/shared/mapchecker.cpp
    src/engine/shared/mapchecker.h
    src/engine/shared/masterserver.cpp
    src/engine/shared/memheap.cpp
    src/engine/shared/memheap.h
    src/engine/shared/message.h
    src/engine/shared/netban.cpp
    src/engine/shared/netban.h
    src/engine/shared/network.cpp
    src/engine/shared/network.h
    src/engine/shared/network_client.cpp
    src/engine/shared/network_conn.cpp
    src/engine/shared/network_console.cpp
    src/engine/shared/network_console_conn.cpp
    src/engine/shared/network_server.cpp
    src/engine/shared/packer.cpp
    src/engine/shared/packer.h
    src/engine/shared/protocol.h
    src/engine/shared/ringbuffer.cpp
    src/engine/shared/ringbuffer.h
    src/engine/shared/serverbrowser.cpp
    src/engine/shared/snapshot.cpp
    src/engine/shared/snapshot.h
    src/engine/shared/storage.cpp
    src/engine/shared/storage.h
    src/engine/shared/stream.cpp
    src/engine/shared/stream.h
    src/engine/shared/websockets.cpp
    src/engine/shared/websockets.h
    src/engine/client.h
    src/engine/config.h
    src/engine/console.h
    src/engine/demo.h
    src/engine/editor.h
    src/engine/engine.h
    src/engine/fetcher.h
    src/engine/friends.h
    src/engine/graphics.h
    src/engine/input.h
    src/engine/irc.h
    src/engine/kernel.h
    src/engine/keys.h
    src/engine/map.h
    src/engine/masterserver.h
    src/engine/message.h
    src/engine/server.h
    src/engine/serverbrowser.h
    src/engine/sound.h
    src/engine/storage.h
    src/engine/textrender.h
    src/engine/updater.h
    src/game/client/components/astar.cpp
    src/game/client/components/astar.h
    src/game/client/components/background.cpp
    src/game/client/components/background.h
    src/game/client/components/binds.cpp
    src/game/client/components/binds.h
    src/game/client/components/broadcast.cpp
    src/game/client/components/broadcast.h
    src/game/client/components/camera.cpp
    src/game/client/components/camera.h
    src/game/client/components/chat.cpp
    src/game/client/components/chat.h
    src/game/client/components/console.cpp
    src/game/client/components/console.h
    src/game/client/components/controls.cpp
    src/game/client/components/controls.h
    src/game/client/components/countryflags.cpp
    src/game/client/components/countryflags.h
    src/game/client/components/cskins.cpp
    src/game/client/components/cskins.h
    src/game/client/components/damageind.cpp
    src/game/client/components/damageind.h
    src/game/client/components/debughud.cpp
    src/game/client/components/debughud.h
    src/game/client/components/drawing.cpp
    src/game/client/components/drawing.h
    src/game/client/components/effects.cpp
    src/game/client/components/effects.h
    src/game/client/components/emoticon.cpp
    src/game/client/components/emoticon.h
    src/game/client/components/eskins.cpp
    src/game/client/components/eskins.h
    src/game/client/components/flow.cpp
    src/game/client/components/flow.h
    src/game/client/components/fontmgr.cpp
    src/game/client/components/fontmgr.h
    src/game/client/components/ghost.cpp
    src/game/client/components/ghost.h
    src/game/client/components/gskins.cpp
    src/game/client/components/gskins.h
    src/game/client/components/hud.cpp
    src/game/client/components/hud.h
    src/game/client/components/identity.cpp
    src/game/client/components/identity.h
    src/game/client/components/irc.cpp
    src/game/client/components/irc.h
    src/game/client/components/items.cpp
    src/game/client/components/items.h
    src/game/client/components/killmessages.cpp
    src/game/client/components/killmessages.h
    src/game/client/components/luarender.cpp
    src/game/client/components/luarender.h
    src/game/client/components/mapimages.cpp
    src/game/client/components/mapimages.h
    src/game/client/components/maplayers.cpp
    src/game/client/components/maplayers.h
    src/game/client/components/mapsounds.cpp
    src/game/client/components/mapsounds.h
    src/game/client/components/menus.cpp
    src/game/client/components/menus.h
    src/game/client/components/menus_browser.cpp
    src/game/client/components/menus_demo.cpp
    src/game/client/components/menus_hotbar.cpp
    src/game/client/components/menus_identity.cpp
    src/game/client/components/menus_ingame.cpp
    src/game/client/components/menus_irc.cpp
    src/game/client/components/menus_settings.cpp
    src/game/client/components/menus_texture.cpp
    src/game/client/components/motd.cpp
    src/game/client/components/motd.h
    src/game/client/components/nameplates.cpp
    src/game/client/components/nameplates.h
    src/game/client/components/particles.cpp
    src/game/client/components/particles.h
    src/game/client/components/players.cpp
    src/game/client/components/players.h
    src/game/client/components/pskins.cpp
    src/game/client/components/pskins.h
    src/game/client/components/race_demo.cpp
    src/game/client/components/race_demo.h
    src/game/client/components/scoreboard.cpp
    src/game/client/components/scoreboard.h
    src/game/client/components/skindownload.cpp
    src/game/client/components/skindownload.h
    src/game/client/components/skins.cpp
    src/game/client/components/skins.h
    src/game/client/components/sounds.cpp
    src/game/client/components/sounds.h
    src/game/client/components/spectator.cpp
    src/game/client/components/spectator.h
    src/game/client/components/spoofremote.cpp
    src/game/client/components/spoofremote.h
    src/game/client/components/statboard.cpp
    src/game/client/components/statboard.h
    src/game/client/components/translator.cpp
    src/game/client/components/translator.h
    src/game/client/components/voting.cpp
    src/game/client/components/voting.h
    src/game/client/animstate.cpp
    src/game/client/animstate.h
    src/game/client/component.h
    src/game/client/gameclient.cpp
    src/game/client/gameclient.h
    src/game/client/lineinput.cpp
    src/game/client/lineinput.h
    src/game/client/render.cpp
    src/game/client/render.h
    src/game/client/render_map.cpp
    src/game/client/ui.cpp
    src/game/client/ui.h
    src/game/editor/auto_map.cpp
    src/game/editor/auto_map.h
    src/game/editor/editor.cpp
    src/game/editor/editor.h
    src/game/editor/io.cpp
    src/game/editor/layer_game.cpp
    src/game/editor/layer_quads.cpp
    src/game/editor/layer_sounds.cpp
    src/game/editor/layer_tiles.cpp
    src/game/editor/popups.cpp
    src/game/generated/client_data.cpp
    src/game/generated/client_data.h
    src/game/generated/nethash.cpp
    src/game/generated/protocol.cpp
    src/game/generated/protocol.h
    src/game/generated/server_data.cpp
    src/game/generated/server_data.h
    src/game/server/entities/character.cpp
    src/game/server/entities/character.h
    src/game/server/entities/door.cpp
    src/game/server/entities/door.h
    src/game/server/entities/dragger.cpp
    src/game/server/entities/dragger.h
    src/game/server/entities/flag.cpp
    src/game/server/entities/flag.h
    src/game/server/entities/gun.cpp
    src/game/server/entities/gun.h
    src/game/server/entities/laser.cpp
    src/game/server/entities/laser.h
    src/game/server/entities/light.cpp
    src/game/server/entities/light.h
    src/game/server/entities/pickup.cpp
    src/game/server/entities/pickup.h
    src/game/server/entities/plasma.cpp
    src/game/server/entities/plasma.h
    src/game/server/entities/projectile.cpp
    src/game/server/entities/projectile.h
    src/game/server/gamemodes/DDRace.cpp
    src/game/server/gamemodes/DDRace.h
    src/game/server/gamemodes/gamemode.h
    src/game/server/score/file_score.cpp
    src/game/server/score/file_score.h
    src/game/server/score/sql_score.cpp
    src/game/server/score/sql_score.h
    src/game/server/ddracechat.cpp
    src/game/server/ddracechat.h
    src/game/server/ddracecommands.cpp
    src/game/server/entity.cpp
    src/game/server/entity.h
    src/game/server/eventhandler.cpp
    src/game/server/eventhandler.h
    src/game/server/gamecontext.cpp
    src/game/server/gamecontext.h
    src/game/server/gamecontroller.cpp
    src/game/server/gamecontroller.h
    src/game/server/gameworld.cpp
    src/game/server/gameworld.h
    src/game/server/player.cpp
    src/game/server/player.h
    src/game/server/save.cpp
    src/game/server/save.h
    src/game/server/score.h
    src/game/server/teams.cpp
    src/game/server/teams.h
    src/game/collision.cpp
    src/game/collision.h
    src/game/ddracecommands.h
    src/game/gamecore.cpp
    src/game/gamecore.h
    src/game/layers.cpp
    src/game/layers.h
    src/game/localization.cpp
    src/game/localization.h
    src/game/mapitems.cpp
    src/game/mapitems.h
    src/game/teamscore.cpp
    src/game/teamscore.h
    src/game/tuning.h
    src/game/variables.h
    src/game/version.h
    src/game/voting.h
    src/mastersrv/mastersrv.cpp
    src/mastersrv/mastersrv.h
    src/osx/notification.h
    src/osxlaunch/client.h
    src/tools/config_common.h
    src/tools/config_retrieve.cpp
    src/tools/config_store.cpp
    src/tools/crapnet.cpp
    src/tools/dilate.cpp
    src/tools/fake_server.cpp
    src/tools/map_resave.cpp
    src/tools/map_version.cpp
    src/tools/packetgen.cpp
    src/tools/tileset_borderadd.cpp
    src/tools/tileset_borderfix.cpp
    src/tools/tileset_borderrem.cpp
    src/tools/tileset_borderset.cpp
    src/twping/twping.cpp
    src/versionsrv/mapversions.h
    src/versionsrv/versionsrv.cpp
    src/versionsrv/versionsrv.h)

add_executable(AllTheHaxx ${SOURCE_FILES})