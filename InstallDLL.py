import os, shutil, subprocess

GAME_DIR = "C:\Games\Geometry Dash - modding" # my dir
MODLDRPATH = GAME_DIR + "/adaf-dll"

print("Installing GatoBot")

# replace DLL in game directory
DLLFILEPATH = "Release/GatoBot.dll"

if os.path.isfile(DLLFILEPATH):
    shutil.move(DLLFILEPATH, MODLDRPATH + "/GatoBot.dll")

else:
    print("Error installing GatoBot.dll: file not found!")
    print(f"Current directory: {os.getcwd()}")

print("GatoBot.dll installed!")

# run SilvrPS
print("Starting GD...")

subprocess.Popen([GAME_DIR + "/GDModding.exe"], cwd=GAME_DIR)

print("Post build script finished.")