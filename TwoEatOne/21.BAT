cl -AM -G2 -Gsw  -c -Zpe -Od twoeat.c
cl -AM -G2 -Gsw  -c -Zpe dirtyrec.c
cl -AM -G2 -Gsw  -c -Zpe board.c
cl -AM -G2 -Gsw  -c -Zpe game.c
cl -AM -G2 -Gsw  -c -Zpe ai.c
cl -AM -G2 -Gsw  -c -Zpe dialogs.c
cl -AM -G2 -Gsw  -c -Zpe save.c
link4 twoeat+dirtyrec+board+game+ai+dialogs+save, twoeat.exe, twoeat.map, mlibw mwinlibc, twoeat.def
rc twoeat.rc
