@echo off

rem CMake > Show configure command
cmake --no-warn-unused-cli -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -SC:/Programming/gdmods/GatoBot/geode -Bc:/Programming/gdmods/GatoBot/build -G "Visual Studio 17 2022" -T host=x86 -A win32

rem CMake > Show build command
cmake --build c:/Programming/gdmods/GatoBot/build --config Release --target ALL_BUILD -j 14 --
