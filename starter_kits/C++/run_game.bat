call make.bat
FOR /L %%A IN (1,1,2) DO (
	halite.exe --replay-directory replays/ -vvv --width 32 --height 32 "GuiSamBot.exe" "GuiSamBot.exe"
)
