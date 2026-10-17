# Placing genomes in Blender

bicycle's Blender add-on (`tools/blender/bicycle_blender.py`) lets you build a 3D scene's contents by clicking: pick a
genome from a drop-down, put it where it goes, choose its door's target scene and spawn point from drop-downs, draw
a moving platform's path with points, add spawn points, and export. No Lua needed for what's in a level and where.
The engine spawns everything when the scene loads. Tested with Blender 4.5 LTS (any 4.2 or later should do).

The examples use plat3d (`plat3d/blender/plat3d.blend`); a game of your own works the same once its .blend is saved in
`<game>/blender/` next to `<game>/genome/`.

## 1. Install the add-on (once)

1. Start Blender. In the menu bar at the top, click **Edit > Preferences...**.
2. In the Preferences window, click **Add-ons** in the list on the left.
3. At the top right of that page there's a small **down-arrow** (v) button. Click it, then **Install from Disk...**.
4. In the file browser, go to your bicycle checkout, open `tools/blender/`, click `bicycle_blender.py`, and click
   **Install from Disk** (bottom right).
5. The add-on "bicycle: entities and level export" appears in the list. Make sure its **checkbox is ticked**.
6. Close the Preferences window (it saves automatically). Blender remembers the add-on from now on.

To update it later (after pulling bicycle), repeat steps 3 and 4; it replaces the old copy.

## 2. Open the level and the Bicycle panel

1. **File > Open...**, pick `plat3d/blender/plat3d.blend`, **Open**.
2. Move the mouse over the big 3D view and press **N** (or click the tiny `<` arrow at the right edge of the view).
   A sidebar opens with tabs down its right side. Click the **Bicycle** tab.
3. The panel has four boxes:
   * **Game**: the genome folder it found (`.../plat3d/genome`). If it says to save the .blend, save it in
     `<game>/blender/` first, or pick the folder in **Genome folder**. Click **Refresh genomes** after you add or
     change a genome file, scene or spawn point outside Blender.
   * **Level**: which level file this Blender scene exports to (`-> genome/level/courtyard.lua`). The level name
     is the Blender scene's name unless you type another one.
   * **Place**: the **Genome** drop-down (every `genome/<name>.lua` of the game), **Add entity**, and **Add spawn
     point** with its **Name**.
   * The **selected object**: its genome and parameters (see 4.).
4. The Blender scene is picked at the very top right of the window (the field next to the scene icon, e.g.
   `courtyard`). plat3d.blend has `courtyard`, `bonus` and `models` (the genomes' meshes; not a level).

Moving around the 3D view: **middle mouse drag** orbits, **Shift + middle drag** pans, the **mouse wheel** zooms,
**numpad period** (or View > Frame Selected) centers the selected object.

## 3. Place a genome

1. Put the **3D cursor** where it should go: **Shift + right-click** on a spot in the 3D view (the red-white circle
   moves there).
2. In the Bicycle panel's **Place** box, open the **Genome** drop-down and pick one, e.g. `coin`.
3. Click **Add entity**. An Empty named `coin` appears at the cursor showing the genome's model (a preview; the
   game draws the real thing from the genome).
4. Move it exactly: press **G** and move the mouse (click to drop; **G** then **Z** moves only up/down), or type
   numbers in the sidebar's **Item** tab (Location). **R Z** rotates it around the vertical axis: that's the
   entity's facing (yaw) in the game.
5. Its name (double-click it in the Outliner, top right, to rename) is its key: the engine uses it to remember a
   collected coin, so give every entity of a scene its own name (Blender does that anyway: `coin.001`, ...).

Blender's Z is up; the exporter converts to the game's Y-up meters (Blender x, y, z -> game x, z, -y).

## 4. Per-instance parameters (doors, paths, numbers)

Select a placed entity (click it, or click its name in the Outliner). The panel's last box shows **Entity: <name>**,
a **Genome** drop-down (switches what it is, keeping the parameters both genomes have), **Persists** (carried to the
next scene) and the parameters its genome offers. They come from the genome's `params` table:

```lua
-- plat3d/genome/flag.lua
params = { ["door.to"] = "scene", ["door.spawn"] = "spawn:door.to", ["door.on"] = "enum:touch|press" },
```

* **Doors** (the `flag` genome): **door.to** is a drop-down of the game's scenes; pick `bonus`. **door.spawn**
  then lists the spawn points of that scene (its scene file, its level file, and spawn points placed in this
  .blend); pick `entry`. **door.on**: `touch` (walk into it) or `press` (stand in it and press the action key).
* **Paths** (the `mover` genome, `["motion.path"] = "path"`): the panel shows **motion.path: N points** and an **Add
  path point** button. Click it: a small cube appears 1 m above the entity (or above the last point), as a child of
  the entity. Move it where the platform should go (**G**). Click **Add path point** again for more points. The
  path runs from the entity through its points in name order (`mover path 01`, `02`, ...), and the points move with
  the entity. Instead of points you can parent a curve to the entity (select the curve, then Shift-click the
  entity, **Ctrl + P > Object**): its control points are the path.
* **Numbers, checkboxes, text** (`"number"`, `"int"`, `"bool"`, `"string"`): fields to type in, e.g. the mover's
  **motion.speed** and **motion.pingpong**. A value equal to the genome's own isn't written per instance, so
  changing the genome changes every copy that didn't override it.
* **Genome** (`"genome"`): a drop-down of the game's genomes (e.g. what a spawner makes).

## 5. Spawn points

Spawn points are where the player arrives: the scene's start (`default`) and the far end of doors.

1. Put the 3D cursor there (**Shift + right-click**).
2. In the **Place** box, type its name in **Name** (`default`, `entry`, `from_bonus`, ...) and click **Add spawn
   point**. An arrow Empty named `spawn <name>` appears; rotate it (**R Z**) to set which way the player faces.
3. Doors in other scenes can now pick it in their **door.spawn** drop-down (click **Refresh genomes** in that scene
   if it doesn't show up yet).

## 6. Export

* **Export level** writes this Blender scene: `genome/level/<level>.lua` and its meshes. **Export all scenes** does
  every scene of the .blend (and the genome models of a `models` scene, to `genome/body/model/`).
* Blender's status bar (bottom) says what it wrote; errors show there too.
* Then run the game as usual (dev builds read the new files directly: `./o-3d plat3d`). The build's ID generator
  checks every placement: a genome, door target scene or spawn that doesn't exist is an error naming the level file.

Headless (no window), the same export: `blender -b plat3d/blender/plat3d.blend --python scripts/blender/export.py`.

## 7. Making a scene use the level

A scene file names its level and keeps only the scene-wide look:

```lua
-- plat3d/genome/scene/courtyard.lua
return {
  name = "Courtyard",
  world3d = { level = "courtyard", sky = { 0.50, 0.70, 0.95 }, fog = { 0.62, 0.76, 0.95, 25, 70 },
              light = { { -0.45, -1.0, -0.35 }, { 1.0, 0.96, 0.88 }, 0.40, true } },
}
```

The level file it gets (generated; don't edit it, re-export):

```lua
entities = {
  { genome = "coin", key = "coin1", pos = { 4.5, 1.6, -2.5 } },
  { genome = "flag", door = { on = "touch", spawn = "entry", to = "bonus" }, key = "flag_to_bonus", pos = { 7.5, 4.2, -17.5 } },
  { genome = "hero", key = "hero", pos = { 4.5, 0.5, 9 } },
  { genome = "mover", key = "mover", motion = { path = { { 0, 0, 0 }, { 0, 3.25, 0 } } }, pos = { 2, 0.75, -14 } },
},
spawns = { default = { 4.5, 0.5, 9, 0 }, from_bonus = { 7.5, 4.4, -15.2, 0 } },
```

## 8. Making a genome placeable

Any `genome/<name>.lua` shows up in the drop-down (a starter genome that only names a scene doesn't). To be drawn in
3D it has a `model` (a mesh in `genome/body/model/`, exported from a Blender object with the custom property
`bicycle_role` = `character`: Object Properties > Custom Properties > New, name `bicycle_role`, value `character`).
Give it `params` for what each copy may change. See README, "Self-contained genomes".

## Troubleshooting

* **No Bicycle tab**: the add-on isn't enabled (Preferences > Add-ons, tick it), or the sidebar is closed (**N**).
* **The genome drop-down is empty**: the .blend isn't saved in `<game>/blender/`; save it there or set **Genome
  folder**, then **Refresh genomes**.
* **No preview model**: the genome has no `model`, or its mesh isn't exported yet (export the `models` scene, or
  the mesh is in another .blend: the add-on imports `genome/body/model/<model>.obj` when it exists).
* **"No genome 'x' in this game"** on an entity: the genome file was renamed or removed; pick another in its
  Genome drop-down.
