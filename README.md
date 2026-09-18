# cMarble

Marble game on **cSim2D**. Same physics, camera, joints, layers, scenes.

```bash
cd /home/working/cMarble
rm -f *.o cMarble
make
./cMarble
```

Needs `/dev/fb0` and `levels/*.scene` next to the binary (or `/home/working/cMarble/levels/`).

| | |
|--|--|
| WASD / arrows | roll (camera-relative) |
| Space | hop |
| Q E | yaw cam |
| + - | zoom |
| R | retry level |
| N | next level (after clear, or skip) |
| 1 | engine debug |
| X | quit |

Yellow gems (`layer 2`, no collision) — roll near them. Green pad (`layer 4`) only counts when you have every gem. Fall under the world → retry.

Three scenes: open field, crate stack + pendulum, stairs + hinge.
