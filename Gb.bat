@echo off

call :7

goto :finish

:1
IF EXIST out\tangram_static_debug_x86 (
echo tangram_static_debug_x86 already EXISTS!
) ELSE (
gn gen --ide=vs2019 out/tangram_static_debug_x86 --args="is_tangram_branded = true is_debug = true is_component_build = false symbol_level = 2 enable_nacl = false target_cpu = \"x86\""
)
goto :end

:2
IF EXIST out\tangram_static_debug_x64 (
echo tangram_static_debug_x64 already EXISTS!
) ELSE (
gn gen --ide=vs2019 out/tangram_static_debug_x64 --args="is_tangram_branded = true is_debug = true is_component_build = false symbol_level = 2 enable_nacl = false target_cpu = \"x64\""
)
goto :end

:3
IF EXIST out\tangram_non_static_debug_x86 (
echo tangram_non_static_debug_x86 already EXISTS!
) ELSE (
gn gen --ide=vs2019 out/tangram_non_static_debug_x86 --args="is_tangram_branded = true is_debug = true is_component_build = true symbol_level = 2 enable_nacl = false target_cpu = \"x86\""
)
goto :end

:4
IF EXIST out\tangram_non_static_debug_x64 (
echo tangram_non_static_debug_x64 already EXISTS!
) ELSE (
gn gen --ide=vs2019 out/tangram_non_static_debug_x64 --args="is_tangram_branded = true is_debug = true is_component_build = true symbol_level = 2 enable_nacl = false target_cpu = \"x64\""
)
goto :end

:5
IF EXIST out\tangram_static_release_x86 (
echo tangram_static_release_x86 already EXISTS!
) ELSE (
gn gen --ide=vs2019 out/tangram_static_release_x86 --args="is_tangram_branded = true is_official_build = true is_debug = false is_component_build = false enable_nacl = false target_cpu = \"x86\""
)
goto :end

:6
IF EXIST out\WebRT4817 (
echo WebRT4817 already EXISTS!
) ELSE (
gn gen --ide=vs2019 out/WebRT4817 --args="is_tangram_branded = true is_official_build = true is_debug = false is_component_build = false enable_nacl = false target_cpu = \"x64\" media_use_ffmpeg = true media_use_libvpx = true proprietary_codecs = true ffmpeg_branding = \"Chrome\""
)
goto :end

:7
IF EXIST out\default (
echo default already EXISTS!
) ELSE (
gn gen --ide=vs2022 out/default --args="is_official_build = true is_debug = false is_component_build = false enable_nacl = false target_cpu = \"x64\" media_use_ffmpeg = true media_use_libvpx = true proprietary_codecs = true ffmpeg_branding = \"Chrome\"" 
)
goto :end

:8
gn gen --ide=vs2022 out/webrt4758 --args="v8_symbol_level = 0 is_tangram_branded = true is_official_build = true is_debug = false is_component_build = false enable_nacl = false target_cpu = \"x64\"" 
goto :end

:finish
echo Finish!

:end