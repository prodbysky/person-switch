# Person-switch
2D infinite fighter game written in C, with raylib.
Currently is **unfinished**
No lore, you are just a person trying to survive many waves

## Controls (shown in-game):
 - `A`           - Move left
 - `D`           - Move right
 - `Space`       - Jump
 - `Left arrow`  - Shoot to the left
 - `Right arrow` - Shoot to the right
 - `P`           - Pause the game
 - `[`           - Decrease volume by 5%
 - `]`           - Increase volume by 5%
 - `/`           - Disable shaders
 - `PrntScr`     - Take a screenshot

## Gameplay screen-shots

Main gameplay (with shaders enabled):

![pswitch_ss_5 84](https://github.com/user-attachments/assets/44827610-430a-4f45-97e3-b2e37f5c7d10)

Main gameplay (without shaders):

![pswitch_ss_16 28](https://github.com/user-attachments/assets/7e6530f8-93c3-4057-a40e-150d32613a8f)

Class selection screen (pops up after finishing a wave):

![pswitch_ss_24 52](https://github.com/user-attachments/assets/bac0d10d-7bae-42f7-8f55-1fabc1212020)


TODO:
 - Game art
 - More enemy types
 - Particles :0
 - Environment hazards (lava pits (as suggested by u/skeeto))
 - Powerups
 - Boses?


## Build
```bash
    mkdir build
    cp -r assets/ build/assets/
    make release 
```
