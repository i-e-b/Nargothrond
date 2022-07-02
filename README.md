# Nargothrond
Experiment with a rogue-like

## Plans

- Generally voxel-like world, with 3d bits (something like Myth)
- Randomly generated terrain, with set-piece areas
- 'Camp' is home base, and is where hero starts
- One type of hero, starts with very basic skills and equipment (no classes)
- Bosses guarding 'captives'
- Captives expand the camp.
- Player death is permanent. All equipment, money, and skills gone. But camp remains
- New player character is started in camp (much like Spelunky or Rogue Legacy)
- New player gets extra skill points and stats based on who they have rescued
- Waypoint teleport system like Diablo 2, but no quick go-home spells.
- Random drops from wandering creatures
- 'captain' creatures with minions
- items and skills compound each other by type
- poison, fire, ice, electric magic types.
- skill tree like all diablo classes together, similar types have synergy to be a bit like classes, but magic fighters should be possible
- limited item carrying, especially health-ups

## Tech todo

- add z buffer like thing
- add screen-to-map lookup buffer during render
  - should have screen-point to map-coord, and on screen depth lookup.
- sprite stuff? Start from top of sprite and scan down until depth is closer than sprite base.