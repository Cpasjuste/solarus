# Changelog

## Solarus 1.6.5 (2010-04-06)

### Engine changes

* Add support for suspending the simulation on Window Focus events (#1536).
* Reconnect to another audio device after a device disconnection (#1499).
* Fix joystick hot-plug/unplug (#1501, #1387).
* Fix crash when calling command functions before the game is started (#1476).
* Fix shader compilation on OpenGL ES 3.10 and 3.20 (#1537).
* Fix TTF fonts using wrong color for antialiasing (#1390).
* Fix TTF Fonts not rendering correctly (#1389).
* Fix a possible crash when starting a timer on a removed entity (#1469).
* Fix pickables not falling on negative layers.
* Fix string literals in savegames not being properly escaped (#1533).
* Fix Unicode filename support on Windows.

### Lua API changes

This release adds new features but does not introduce any incompatibility.

* Add methods `destructible:get/set_cut_method()` (#1526).

### Solarus launcher GUI changes

* Add option to control suspension on Window Focus events.
* Rename *Force Software* to *Force software rendering*.
* Disable force-software and suspend-unfocused options when quest is running.

## Solarus 1.6.4 (2020-04-12)

No change in the engine package.

## Solarus 1.6.3 (2020-04-11)

### Engine changes

* Fix macOS port not working since 1.6.1 due to OpenGL.
* Fix shaders compilation in some OpenGL ES 3.0 platforms (#1437).
* Fix UTF-8 quest path encoding handling (#1429).
* Fix UTF-8 file names in Lua Open File API for Windows (#1413).
* Fix memory usage when visiting very large maps (#1395).
* Fix crash when removing the shadow of a carried object (#1423).
* Fix crash when stopping the movement of a thrown object (#1452).
* Fix carried object shadow still displayed after removing it (#1436).
* Fix animating hero sprites dynamically created (#1348).
* Fix hero speed when going from deep water to shallow water (#1186).
* Fix trail sprite not taking the hero's direction (#1464).
* Fix ground sprite still displayed while jumping (#1458).
* Fix custom states wrongly affected by ground speeds (#1416).
* Fix custom states not activating side teletransporters (#1448).
* Fix streams continuing to act when the layer has changed.
* Fix `enemy:on_dying()` not called when setting enemies life to zero (#1440).
* Fix talking to NPCs while swimming (#1043).
* Fix creating dynamic sprite with tileset anims (#1461).

### Solarus launcher GUI changes

* Fix UTF-8 quest path encoding handling (#1429).
* Add drag and drop functionality to add new quests (#1420).

## Solarus 1.6.2 (2019-08-15)

### Engine changes

* Fix scrolling teletransporters with a square size (#1412).
* Fix hero displayed above stairs walls.

## Solarus 1.6.1 (2019-08-10)

### Engine changes

* Add `-force-software-rendering` switch to solarus-run (stdgregwar).
* Add argument to set mouse cursor visibility on start (#1263).
* Add option in CMake to support building with OpenGL ES (#1270).
* Add option in CMake to disable logging errors to file (#1276).
* Add simple performance counters to the engine (#1280).
* Add argument to set fullscreen mode on start (#1281).
* Add joypad buttons combo for quitting the engine (#1283).
* Add option in CMake to control building of unit tests (#1288).
* Add root build path to unit tests environment in Windows (#1291).
* Add git revision information to build and main loop startup (#1292).
* Add argument to set the joypad axis deadzone (#1293).
* Add debug warning if a surface is larger than 2048 x 2048 (#1294).
* Add support of carried object sprites with multiple directions (#1392).
* Fix reordering hero sprites (#1243).
* Fix crash when starting a hero movement from `hero:on_state_changed()` (#1354).
* Fix crash when changing the hero state while pulling a block (#1371).
* Fix crash when starting drowning after falling into a teletransporter (#1353).
* Fix crash when stairs are disabled at map start with dynamic tiles (#1366).
* Fix crash when a state finishes from a coroutine (#1374).
* Fix action command lost after showing a dialog in the pause menu (#1408).
* Fix hero being hurt while in spiral stairs (#1038).
* Fix stairs not activating in a custom state with fixed direction (#1364).
* Fix crash when pickable follow streams (#1361).
* Fix camera movement stuck on separators (#1351).
* Fix jumpers when the hero size is other than 16x16 (#1381).
* Fix blocks when the hero size is other than 16x16 (#1383).
* Fix blocks stuck on separators (#1356).
* Fix `entity:overlaps(...)` crashing when given bad collision mode (#1041).
* Fix straight movement progress reinitialized when setting speed/angle.
* Fix `sol.menu.start` allowing to start one menu multiple times (#1044).
* Fix GlRenderer for non-Android GLES 2.0 devices (#1267).
* Fix OpenGL compatibility issues.
* Fix FindOpengl complaining about GLVND vs GLX choice (#1320).
* Fix incorrect mapping from window coords to quest coords (#1340).
* Fix mouse and finger coordinates using a wrong window size (#1268).
* Fix restoring to windowed mode from fullscreen (#1269).
* Fix screen surface not being initialised in 640x480 video mode (#1273).
* Fix initialisation of screen surface without video window (#1274).
* Fix restoring from fullscreen mode in Windows (#1284).
* Fix warning when the default quest size is not supported by the quest (#1376).
* Fix building with mingw-w64 in Linux (#1282).
* Fix detection of LuaJIT version in CMake module (#1277).
* Fix building GUI with mingw-w64 in Linux (#1285).
* Fix running the unit tests from the build root folder (#1289).
* Fix broken unit test that fails in Windows but not Linux (#1290).

### Lua API changes

This release adds new features but does not introduce any incompatibility.

* Add methods `game:get/set_transition_style()` (#1368).
* Add methods `state:get/set_can_use_teletransporter/switch/stream()` (#1363).
* Make `surface`, `text_surface` and `timer` userdata types indexable (#1394).

## Solarus 1.6.0 (2018-12-22)

### Engine changes

* Solarus 1.5 quests are still supported by Solarus 1.6.
* Add support of OpenGL shaders (#394) by stdgregwar, Vlag and Christopho.
* Improve the performance of loading images.
* Maps can now use multiple tilesets (#1174).
* Animated tile patterns can now have any frame number and delay (#683).
* Fix crash when using coroutines (#1269) by stdgregwar.
* Treasure dialogs are now optional (#1076).
* Allow entities to have a non-multiple of 8 size (#1294).
* Don't center the hero on non-blocking streams (#1063).
* Allow pickables to follow streams (#1139).
* Blocks can now be moved multiple times (#967).
* Fix enemies unable to move on non-blocking streams.
* Fix streams stopping when using the sword several times.
* Fix non-blocking stream turns after going south (#648).
* Fix stairs activating when the hero is not exactly aligned.
* Fix `sol.timer.start()` not detecting invalid context parameter.
* Fix `timer:set_suspended_with_map()` for entity timers (#1158, #1236).
* Fix random movement giving the same path for all entities on Mac (#1083).
* Fix `circle_movement:get_direction4()` not working (#1163).
* Fix precision issue with circle_movement angular speed (#1116).
* Fix `path_movement:get_path()` and `pixel_movement:get_trajectory()` (#1136).
* Fix `straight_movement:set_max_distance()` incorrect on tables (#1075).
* Fix `straight_movement:set_speed()` not handling errors correctly.
* Fix possible crash when reloading very old savegame files (#1064).
* Fix crash when loading a map with tiles whose pattern does not exist (#1299).
* Fix crash when a sprite PNG file is missing.
* Fix crash when calling `entity:test_obstacles()` on an unloaded map.
* Fix crash when opening a chest with a missing treasure dialog (#1089).
* Fix crash when scrolling to the same map (#924) by stdgregwar.
* Fix crash when scrolling to an invalid layer (#1015).
* Fix `entity:get_ground_below()` from `map:on_started()` after scrolling (#925).
* Fix crash when calling `map:get_hero()` after the map is finished (#1228).
* Fix `hero:on_movement_changed()` not called (#1095).
* Fix hero shield sprite directions in animation sword (#1185).
* Fix facing entity randomly chosen when the hero faces multiple ones (#1042).
* Fix disabling teletransporters during their `on_activated()` event (#1266).
* Fix blocks not falling into water or lava (#1214).
* Fix `enemy:on_restarted()` called twice (#1179).
* Fix removed entity still drawn during scrolling transition (#1193).
* Fix `item:on_ability_used()` not called for sword knowledge ability (#1171).
* Fix sprite parameters order in custom entity collision callback (#1162).
* Fix `sol.file.exists()` returning false for directories.
* Fix scripts failing to load if a directory exists with the same name (#1100).
* Fix mouse API not working when the mouse is outside the window (#1018).
* Fix joypad deadzone issue (#672) by strycore.
* Fix compilation error with Clang 3.9 in SNES_SPC.
* Fix possible compilation error when `HAVE_UNISTD_H` has no value (#1084).
* Fix loading quests in non UTF-8 filesystems.
* Add support of .otf fonts.
* Improve Lua error messages.
* CLI can accept an archive's path as argument.
* Replaced SDL renderer by a custom GL backend, by stdgregwar.

### Solarus launcher GUI changes

* Show the path and the compatibility format of each quest (#1129).
* Lua console: provide variables game, map, entities and tp function.
* Clear the console when a quest is started.
* FileDialog now expects a file, either quest.dat or an archive (.solarus, .zip)

### Lua API changes

This release adds new features and deprecates a few ones
but does not introduce any incompatibility.

#### New features

* Add a shader API.
* Add a custom hero state API.
* Add an event `sol.video:on_draw(screen)` to draw to the scaled screen (#1216).
* Add a function `sol.main.get_quest_version()` (#1058) by Nate-Devv.
* Add a function `sol.main.get_game()` (#1212).
* Add functions to get and set the resource elements (#959, #630).
* Add methods `game:simulate_key_pressed/released()` by Vlag (#1034).
* Add abilities `push`, `grab` and `pull` to `game:get/set_ability()` (#788).
* Add a method `item:is_being_used()` by alexander-b (#879).
* Add methods `entity:set_size()` to all entities (#121, #528).
* Add methods `entity:set_origin()` to all entities.
* Add methods `entity:create/remove_sprite()` to all entities (#852).
* Add methods `entity:is/set_drawn_in_y_order()` to all entities (#1098, #1173).
* Add methods `entity:get/set_weight()` to all entities to lift them (#1227).
* Add methods `entity:get_property()` and `entity:set_property()` (#1094).
* Add methods `entity:get_properties()` and `entity:set_properties()` (#1144).
* Add methods `entity:get_layer()` and `entity:set_layer()`.
* Add a method `entity:get_controlling_stream()` (#1204).
* Add optional sprite parameters to `entity:overlaps()` (#1159).
* Add methods `entity:get/set_draw_override()` (#1260).
* Add events `entity:on_pre/post_draw(camera)` to all entities (#1260).
* Add parameter camera to event `entity:on_pre/post_draw()` (#1260).
* Add events `entity:on_enabled/disabled()` to all entities (#898).
* Add event `entity:on_suspended()` to all entities (#1261).
* Add a method `hero:get_carried_object()`.
* Add a method `hero:start_grabbing()` (#1303).
* Add a method `hero:start_attack_loading()` (#1291).
* Add an event `hero:on_state_changing()` (#1247).
* Add an event `camera:on_state_changing()`.
* Add a method `camera:get_surface()` (#1265).
* Add an event `camera:on_state_changing()`.
* Add methods `dynamic_tile:get/set_tileset()` (#1175).
* Add methods `door:open()`, `door:close()` and `door:set_open()` (#1007).
* Add methods `stairs:get_direction()` and `stairs:is_inner()` (#1037).
* `destructible:on_lifting()` now gives a carried object parameter.
* Add method `carried_object:get_carrier()`.
* Add methods `carried_object:get/set_damage_on_enemies()`.
* Add methods `carried_object:get/set_destruction_sound()`.
* Add events `carried_object:on_lifted/thrown/breaking()` (#1233).
* Add property max_moves in `map:create_block()` replacing maximum_moves (#967).
* Add methods `block:get/set_max_moves()` replacing `block:get/set_maximum_moves()`.
* Add optional callback parameter to `enemy:set_attack_consequence()` (#1062).
* Add method `enemy:is_immobilized()` (#1092).
* Add methods `enemy:get/set_attacking_collision_mode()` (#1066).
* Add methods `enemy:get/set_dying_sprite_id()` (#955).
* Add methods `custom_entity:is/set_tiled()` (#1105).
* Add methods `custom_entity:get/set_follow_streams()` (#1221).
* Add methods `drawable:get/set_rotation()` and `get/set_scaling()` by stdgregwar.
* Add methods `get/set_opacity()` to sprite and text_surface (#702) by stdgregwar.
* Add methods `sprite:is_animation_started()` and `sprite:stop_animation()` (#1264).
* Add a method `sprite:get_frame_src_xy()` (#1093).
* Add a method `sprite:get_ignore_suspend()`.
* `sprite:get_num_frames()` can now optionally take an animation and direction.
* `sprite:get_frame_delay()` can now optionally take an animation.
* `sprite:get_size()` can now optionally take an animation and direction.
* `sprite:get_origin()` can now optionally take an animation and direction.
* Add a method `surface:get_pixels()` (#452).
* Add a method `surface:set_pixels()` (#466) by stdgregwar.
* Add methods `surface:gl_bind_as_target/texture()` by stdgregwar.
* Add method `movement:is_suspended()`.
* Add methods `movement:get/set_ignore_suspend()` (#858).
* Add method `get_angle()` to more movement types (#1122) by stdgregwar.
* Add method `circle_movement:get_center()` (#1091).
* Add methods `circle_movement:get/set_angle_from_center()` in radians (#1116).
* Add methods `circle_movement:get/set_angular_speed()` in radians (#1116).
* Add methods `sol.menu.bring_to_front/back()` (#1107).
* Repeated timers can now change their next delay by returning a number (#983).
* Automatically set the language when there is only one in the quest (#1006).
* Add functions `sol.file.is_dir()` and `sol.file.list_dir()` (#971).
* Add finger functions to `sol.input` and finger events by Vlag.
* Add methods `get/set_color_modulation()` by stdgregwar.
* Add function `sol.text_surface.get_predicted_size()` by stdgregwar.

#### Deprecated functions

* Built-in video mode functions: use shaders instead.
* Property maximum_moves of `map:create_block()`: use max_moves instead.
* Method `block:get/set_maximum_moves()`: use `get/set_max_moves()` instead.
* Angles in degrees in circle movement: use functions with radians instead.

### Data files format changes

* Maps: add support of custom properties for entities (#1094).
* Maps: add property enabled_at_start to all entities (#1101).
* Maps: add property tileset to tiles and dynamic tiles (#1174).
* Maps: add property `max_moves` to blocks to allow multiple limited moves (#967).
* Maps: property `maximum_moves` of blocks is now deprecated and optional (#967).
* Maps: add properties `origin_x`, `origin_y` to custom entities.
* Maps: add property tiled to custom entities (#1105).
* Tilesets: add support of border sets (autotiles) (#1069).
* Tilesets: add support of custom frame number and delay (#683).
* Make the tileset entities image optional (#884).

## Solarus 1.5.3 (2016-04-01)

* Speed up loading maps by keeping tilesets in a cache (#1019).
* Fix `text_surface:set_horizontal/vertical_alignment()` not working.
* Fix parallax dynamic tiles still displayed when disabled or invisible.
* Improve error message of non-square tile patterns with diagonal obstacles.

## Solarus 1.5.1 (2016-11-29)

### Engine changes

* Add Spanish translation of the launcher GUI (thanks Diarandor!).
* Fix registering quest to the launcher at quest install time (#948).
* Fix crash when a carried bomb explodes (#953).
* Fix crash when a scrolling teletransporter is incorrectly placed (#977).
* Fix crash when an entity has a wrong savegame variable type (#1008).
* Fix memory leak when creating lots of surfaces (#962).
* Fix cleanup of the quest files at exit.
* Fix error in `sol.main.load_settings()` when the file does not exist.
* Fix ground ignored after `hero:unfreeze()` or back to solid ground (#827).
* Fix `entity:get_name()` returning nil after the entity is removed (#954).
* Improve error messages of surface creations and conversions.
* Chests: set an initial value `"entities/chest"` to the sprite field.

### Solarus launcher GUI changes

* Start the selected quest by pressing Return or double-clicking (#949).

### Sample quest changes

* The sample quest is now in a separate repository (#996).

## Solarus 1.5.0 (2016-07-27)

### Engine changes

* Add a launcher GUI to ease chosing a quest and setting options (#693).
* Rename the `solarus_run` executable to `solarus-run`.
* Add version number and symbolic links when building the library.
* Add a `-lua-console` option to run Lua code from the standard input.
* Remove the `-win-console` option, the preferred way is now to use a GUI.
* Add a `-turbo` option to run at full speed.
* Add a `-lag` option to simulate slower systems for debugging.
* Print when the main loop starts and stops.
* Print the Lua version at startup (#692).
* Outputs are now prefixed by `[Solarus]` and the current simulated time.
* Musics: Add support of custom OGG looping (#643).
* Maps: allow more than 3 layers (#445).
* Improve the performance of loading big maps (#854).
* Improve the performance of custom entity collisions.
* Improve the performance of collisions by using a quadtree.
* Entities far from the camera are no longer suspended.
* The hero no longer automatically jumps when arriving on water (#530).
* Destinations can now set to update or not the starting location (#819).
* Teletransporters on the side of the map now work on all layers (#850).
* Streams can now have a speed of zero (#496).
* Fix crash when main.lua has a syntax error.
* Fix crash with missing directions in sprites controlled by the engine (#864).
* Fix `sprite:on_animation_finished()` and others not working sometimes (#799).
* Fix error in `sprite:set_animation()` when the direction is missing (#937).
* Fix straight movement precision.
* Fix freeze when loading a map with tiles outside the limits (#875).
* Fix crash when trying to use a non-saved item (#889).
* Fix sword tapping sound still played when the game is suspended (#797).
* Fix `hero:set_invincible()` not working without duration (#805).
* Fix lifted item walking animation only accepting 3 frames (#645).
* Fix `enemy:set_attack_consequence_sprite()` with thrown items (#834).
* Fix `custom_entity:set_can_traverse()` for doors (#716).
* Fix `custom_entity:set_can_traverse_ground()` for some grounds (#794).
* Fix custom entity collisions missed for entities that do not move (#671, #883).
* Fix `custom_entity:get_modified_ground()` returning nothing.
* Fix `custom_entity:on_ground_below_changed()` not called (#738).
* Fix missing notifications in `custom_entity:set_origin()` (#880).
* Fix creating an entity with the same name as another one just removed (#795).
* Fix parallax scrolling for dynamic tiles (#816).
* Fix crash when a diagonal tile is not square (#837).
* Fix crash when the teletransporter after stairs is missing.
* Fix `text_surface:set_rendering_mode()` not working (#833).
* Fix possible freeze when changing the position of a path finding entity.
* Fix `circle_movement:set_initial_angle()` not working (#721).
* Fix straight movement setting speed to zero when reaching obstacles (#633).
* Fix support of joypads with multiple axes.
* Fix `sol.input.get_mouse_coordinates()` ignoring the zoom factor (#734).

### Lua API changes

#### Changes that introduce incompatibilities

* Fix missing collision detections and entity notifications.
* `chest:on_empty()` is replaced by `chest:on_opened(treasure)` (#483).
* Enemy ranks no longer exist, `set_hurt_style()` needs to be called (#449).
* Items with amount now have a default max amount of 1000 (#688).
* New ability `"jump_over_water"` in `game:get/set_ability()`, off by default (#530).
* Fix hero state name `"freezed"`, renamed it to `"frozen"` (#813).
* Fix `map:get_entities()` not returning the hero (#670).
* Fix `map:create_custom_entity()` not erroring when width/height are missing.
* `map:get_camera_position()` is now deprecated, use `camera:get_bounding_box()`.
* `map:move_camera()` is now deprecated, use a camera movement instead.
* `map:draw_sprite()` is now deprecated, use `map:draw_visual()` instead (#661).
* Fix `entity:set_enabled(true)` delayed while it blocks the hero (#817).
* Fix brandished treasure sprite and shop treasure sprite not animated (#790).
* `circle_movement:get/set_initial_angle()` now use degrees (#721).
* Add ability to hide mouse cursor (#891).

#### Changes that do not introduce incompatibilities

* Add a function `sol.main.get_solarus_version()` (#767).
* Add a function `sol.main.get_quest_format()`.
* Add a function `sol.main.get_type()` (#744).
* Add a method `game:set_suspended()` (#845).
* Add methods `map:get_min_layer()` and `map:get_max_layer()` (#445).
* Add a method `map:get_entities_by_type()` (#796).
* Add a method `map:get_entities_in_rectangle()` (#142).
* Add a method `map:draw_visual()` to draw any drawable object (#661).
* Add a method `map:get_camera()` (the camera is now a map entity).
* Add methods `map:set_world()` and `map:set_floor()` (#656).
* `map:get_entities()` can now be called without parameter to get all entities.
* `map:get_entities*()` functions now give entities sorted in Z order (#779).
* Add an event `entity:on_movement_started()`.
* Add a method `entity:get_max_bounding_box()` considering sprite boxes (#754).
* `entity:get_center_position()` now also returns the layer.
* Add a method `entity:get_facing_position()`.
* Add a method `entity:get_facing_entity()` (#877).
* Add a method `entity:get_ground_position()` (#830).
* Add a method `entity:get_ground_below()` (#830).
* `entity:set_optimization_distance()` is now only a hint for the engine.
* `entity:test_obstacles()` now also works without parameters.
* `entity:overlaps()` now has an optional collision mode parameter (#748).
* Add `entity:get_sprite()` to all entities, with an optional name value (#669).
* Add a method `entity:get_sprites()` (#851).
* Add methods `entity:bring_sprite_to_front/back()` (#809).
* `enemy/custom_entity:create_sprite()` now take an optional name value.
* `hero:save_solid_ground()` can now take a function parameter (#667).
* Add a method `hero:start_attack()` (#821).
* Add methods `npc:is/set_traversable()` (#712).
* Add methods `chest:get/set_treasure()` (#664).
* Add an event `chest:on_opened()` with treasure info parameters (#483).
* Add methods `dynamic_tile:get_pattern_id()` and get_modified_ground() (#755).
* Add methods `destination:get/set_starting_location_mode()` (#819).
* Add a method `switch:is_walkable()` (#729).
* Add a method `switch:is_locked()`.
* Add a method `sprite:get_num_frames()` (#818).
* Add methods `sprite:get_size()` and `sprite:get_origin()` (#823).
* `sprite:set_animation()` now takes an optional callback parameter (#861).
* Add a method `surface:get_opacity()` (#722).
* Add methods `surface/text_surface/sprite:get/set_blending_mode` (#930).

### Data files format changes

* New directory logos to put the logo and icons of your quest, used in the GUI.
* Quest properties: New properties describing the quest, used in the GUI (#838).
* Quest properties: the `title_bar` property no longer exists, use `title` instead.
* Maps: New properties `min_layer` and `max_layer` (#445).
* Maps: Enemies no longer have a rank property (#449).
* Maps: New property `starting_location_mode` on destinations (#819).
* Maps: `width` and `height` of custom entities are now mandatory as documented.
* Dialogs: Allow empty texts.

### Sample quest changes

* Lots of new sprites and sounds from Diarandor.

## Solarus 1.4.5 (2015-11-22)

Bug fixes for the 1.4 release.

* Fix file name not shown when there is an error in dialogs file (#718).
* Fix saving special characters in data files (#719).
* Fix `sol.main.load_file()` returning a string instead of nil on error (#730).
* Fix performance issue when sprites have huge frame delays (#723).
* Fix collisions triggered for removed entities (#710).
* Fix hero disappearing if lifting animation has less than 5 frames (#682).
* Fix collisions with diagonal dynamic tiles larger than 8x8 (#486).
* Fix path finding movement not working with NPCs (#708).
* Fix stuck on non-traversable dynamic tiles covered by traversables (#769).
* Fix collision detection of custom entities that do not move.
* Fix pickables with special movement falling in holes too early.
* Fix blocking streams not working when the hero's speed is greater (#488).

## Solarus 1.4.4 (2015-08-19)

Bug fixes for the 1.4 release.

* Fix pickables falling in holes even when hooked (#740).

## Solarus 1.4.3 (2015-08-12)

Bug fixes for the 1.4 release.

* Fix a compilation error with Mac OS X.
* Fix crash at exit when a surface has a movement with callback (#699).
* Fix crash when removing a custom entity (#690).
* Fix crash when a sprite file is missing or has no animation (#700).
* Fix crash when trying to remove a sprite already removed (#705).
* Fix crash when a custom entity collision or traversable test errors.
* Fix crash when changing hero sprites sometimes.
* Fix crash when sound buffers are full.
* Fix crash in `map:get_ground()` with out of bounds coordinates.
* Fix Lua error message saying "number expected" instead of "string expected".
* Fix `game:set_command_keyboard/joypad_binding` refusing parameters.
* Fix map scrolling not working if quest size is not a multiple of 5 (#701).
* Fix `camera:move()` ignoring separators.
* Fix entities already destroyed when `map:on_finished()` is called (#691).
* Fix `entity:bring_to_front()/back()` ignoring the order of obstacles.
* Fix hero stuck on blocks.
* Fix hero going backwards on ice sometimes.
* Fix `custom_entity:set_can_traverse_ground()` giving opposite result (#668).
* Fix `enemy:immobilize()` having no effect when already immobilized.
* Fix dying animation of flying and swimming enemies.
* Fix the position of the shadow of pickables when they move.
* Fix pickables not reacting to their ground (#655).

## Solarus 1.4.2 (2015-05-09)

No change in the engine (changes in Solarus Quest Editor only).

## Solarus 1.4.1 (2015-05-09)

Bug fixes for the 1.4 release.

### Engine changes

* Fix crash with doors whose opening condition is an item (#686).
* Fix the size of custom entities supposed to be optional (#680).
* Fix the hero's sprite reset to default ones when changing equipment (#681).
* Fix animated tiles freezed when running a quest a second time (#679).
* Fix saving empty files.
* Print an error message when there is no font in the quest.

## Solarus 1.4.0 (2015-05-02)

The new quest editor release!

### Engine changes

* Solarus now compiles with C++11.
* Solarus Quest Editor was rewritten and is now in a separate repository.
* Solarus can now be used as a library in other projects.
* Add a command-line flag `-win-console=yes` to see output on Windows (#550).
* Add unit tests.
* Fix a crash if an entity has a sprite without animation.
* Fix crash when using the `-no-video` command-line option.
* Fix assertion when a crystal block has less than 4 frames.
* Fix hero freeze when a treasure's dialog is missing (#595).
* Fix hero stuck in dynamic tiles just enabled on him.
* Fix hero sometimes moving in wrong directions (#677).
* Fix tunic and sword collision when their sprite is changed (#617).
* Fix slightly incorrect position of carried item sometimes (#660).
* Fix crash when a tileset image is missing (#590).
* Don't die if the animation of a pickable treasure is missing.

### Lua API changes

#### Changes that introduce incompatibilities

* Text surfaces: the size must now be set at runtime instead of in fonts.dat.
* Text surfaces: the default font is now the first one in alphabetical order.

#### Changes that do not introduce incompatibilities

* `sol.text_surface.create()` now accepts a size parameter (default is 11).
* Add a function `sol.main.get_os()`.
* Fix `sprite:on_frame_changed()` called twice on animation/direction change.

### Data files format changes

You can use the script
`editor/resources/tools/data_files_conversion/1.3_to_1.4/update_quest.lua` to
automatically update your data files. Don't forget to make a backup first.

* fonts.dat no longer exists. Fonts are a resource like others now (#611).
* Fonts are now in a "fonts" directory instead of "text".
* Maps: shop treasures have a new property "font".

## Solarus 1.3.1 (2014-08-25)

Bug fixes for the 1.3 release.

### Solarus Quest Editor changes

* Fix opening an empty sprite (#581).
* Sort resources in the quest tree by natural order (#579).

## Solarus 1.3.0 (2014-08-21)

The sprite editor release!

### Engine changes

* Fix a crash when creating a timer from `game:on_started()` (#575).
* Fix `hero:save_solid_ground` having no effect on water/lava/prickles (#567).

### Lua API changes

#### Changes that do not introduce incompatibilities

* Add mouse functions and events.
* Add a method `sprite:get_animation_set_id()` (#552).
* Add a method `sprite:has_animation()` (#525).
* Add a method `sprite:get_num_directions()`.
* Add a method `hero:get_solid_ground_position()` (#572).
* Add a method `switch:get_sprite()`.
* Allow to customize the sprite and sound of switches (#553).
* Add a method `enemy:get_treasure()` (#501).
* Fix the write directory not having priority over the data dir since 1.1.
* Fix `pickable/destructible:get_treasure()` returning wrong types.
* Fix custom entity collision detection when the other is not moving (#551).
* Allow to call map methods even when the map is not running.

### Data files format changes

You can use the script
`editor/resources/tools/data_files_conversion/1.2_to_1.3/update_quest.lua` to
automatically update your data files. Don't forget to make a backup first.

* Maps: New properties sprite, sound for switches (#553).
* Maps: The subtype of switches is now a string.
* Tilesets: The id of a tile pattern is now a string (#559).

### Solarus Quest Editor changes

* Add a sprite editor (#135). By Maxs.
* Add a zoom level of 400%. By Maxs.
* Add keyboard/mouse zoom features to sprites and tilesets. By Maxs.
* Add Lua syntax coloring (#470). By Maxs.
* Add a close button on tabs (#439). By Maxs.
* Rework the quest tree to show the file hierarchy and Lua scripts. By Maxs.
* Add specific icons for each resource type in the quest tree.
* Move the entity checkboxes to the map view settings panel. By Maxs.
* Allow to change the id of a tile pattern in the tileset editor (#559).
* Don't initially maximize the editor window.
* Fix converting quests to several versions in one go.

## Solarus 1.2.1 (2014-08-02)

Bug fixes for the 1.2 release.

### Engine changes

* Fix `entity:is_in_same_region()` giving wrong results (#500).
* Fix `custom_entity:set_can_traverse()` giving opposite results.
* Fix `custom_entity:on_interaction()` not always called.
* Fix custom_entity sprite collision issues with some entities (#536).
* Fix a crash in `enemy:restart()` when the enemy is dying (#558).
* Fix `hero:set_tunic_sprite_id()` resetting the direction to right (#511).
* Fix `timer:get_remaining_time()` always returning `0` (#503).
* Fix declaring global variables from a map script (#507).
* Fix the hero sometimes moving while no keys are pressed (#513). By xethm55.
* Fix `on_joypad` events not always working (#519). By xethm55.
* Add an error when a hero sprite animation is missing (#485). By Nate-Devv.

### Solarus Quest Editor changes

* Fix corrupted image in quest created by **Quest > New quest** (#548).
* Fix tiles created on invisible layer (#508). By Maxs1789.
* Fix crash when an NPC sprite does not have 4 directions (#510). By Maxs1789.

## Solarus 1.2.0 (2014-05-06)

The SDL2 release.

### Engine changes

* Upgrade to SDL 2 (#262). Thanks Vlag.
* Accelerate video operations in GPU if available. Expect huge speedups.
* Add the hq2x, hq3 and hq4x pixel filter algorithms as new video modes.
* Make the window resizable (#338).
* Use LuaJIT if available instead of vanilla Lua for better performance.
* New map entity type: custom entities, fully scripted (#323).
* Conveyor belts are now called streams and can be non-blocking (#288).
* Collision rules of streams (conveyor belts) are now like holes.
* Rewrite the main loop with a constant timestep (#351).
* Show a dialog box in case of fatal error.
* The `"wide"` video modes do not exist anymore. SDL2 does the job now.
* Fix `enemy:on_hurt()` that was wrongly called when immobilized.
* Fix life and money exceeding the max when the max changes (#355).
* Make stairs sounds optional (#364).
* Make more checks in sprite files to avoid crashes (#357).
* Fix RandomMovement speed that was not taken into account (#361).
* Set the default speed of `StraightMovement` to 32 instead of 0 (#343).
* The size of all map entities must be a multiple of 8 (#358).
* Thrown entities (pots, bombs...) can now fall to a lower layer (#349).
* Running into a crystal or a solid switch now activates it (#193).
* The hero can now jump over distant crystal blocks (#42).
* The shield no longer protects while using the sword or carrying (#192).
* Fix collisions detected on disabled entities (#455).
* Fix pixel collisions coordinates when sprites move (#372).
* Fix a slowness when loading maps (#374).
* Fix crash when accessing a map not active anymore (#371).
* Fix crash when changing the movement of the hero (#392).
* Fix crash when calling `hero:start_treasure()` with wrong parameters (#391).
* Fix crash when calling game:has/get/set_ability() with wrong name (#408).
* Fix a crash when creating two entities with the same name (#370).
* Fix issues with unobtainable treasures.
* Fix the starting location wrongly saved with special destinations (#375).
* Fix `map:set_tileset()` sometimes moving the hero near the map border (#400).
* Fix enemies stuck on blocks (#360).
* Fix enemies stuck on crystal blocks (#41).
* Fix human NPCs not automatically showing `"walking"` when moving (#336).
* Fix the hero leaving the grabbing state even while the game is suspended.
* Fix low walls in dynamic tiles behaving like normal walls.
* Fix wrong collisions of right-up and left-down diagonal jumpers.
* Fix jumpers that could be traversed sideways (#481).
* Fix blocks no longer stopping when aligned on the grid since Solarus 0.9.3.
* Fix entities not always shown when they have no optimization distance.
* Call `hero:on_removed()` and stop hero timers when stopping the game (#421).
* Don't die if the map or destination saved no longer exists (#301).
* Don't die if a map has no destination. Show an error and go to 0,0 instead.
* Don't die if `hero:teleport()` attempts to go to a non-existing place.
* Don't die if attempting to start a game without map.
* Don't die if attempting to start a non-existing dialog.

### Data files format changes

You can use the script `tools/data_files_conversion/1.1_to_1.2/update_quest.lua`
to automatically update your data files. Don't forget to make a backup first.

* Languages: New syntax of strings.dat easier to read and parse (#170).
* Maps: The world property is now optional (#128).
* Maps: Destructibles no longer have subtypes, they are customizable (#270).
* Maps: Rename entity `shop_item` to `shop_treasure`.
* Maps: Rename entity `conveyor_belt` to `stream` with new features (#288).
* Maps: Teletransporters transition property is now a string (#402).
* Maps: Walls have a new property `"stops_projectiles"`.
* Sounds: Running into a wall now plays a sound other than "explosion" (#297).

### Lua API changes

#### Changes that introduce incompatibilities

* Video mode names have changed: no more wide, fullscreen ou windowed names.
* `sol.video.switch_mode()` no longer changes the fullscreen flag.
* `surface:set_transparency_color()` no longer exists. Use `surface:clear()`.
* `sol.audio.play_music("none")` is replaced by `sol.audio.play_music(nil)`.
* `on_key_pressed()` and `on_character_pressed()` are now both called (#344).
* Destructible objects no longer show hardcoded dialogs (#270).
* `map:create_destructible()` has no hardcoded subtypes anymore (#270).
* `map:create_teletransporter()`: the transition is now a string (#402).
* `map:create_shop_item()` is replaced by `map:create_shop_treasure()`.
* `map:create_conveyor_belt()` is replaced by `map:create_stream()` (#288).
* The state `"conveyor belt"` no longer exists in `hero:get_state()` (#288).
* The built-in strength of the sword has changed (#426).
* Bosses are not initially disabled anymore (#448).
* Call `enemy:on_hurt()` before `enemy:on_dying()` (#325).
* `enemy:on_hurt()` no longer takes a life_lost parameter (#426).
* The built-in defense of the tunic has changed (#428).
* `enemy:get/set_magic_damage()` no longer exists (#428).
* `hero:start_hurt()` no longer takes a magic parameter (#428).
* `hero:start_hurt()` now hurts the hero even when enemies cannot.
* Enemies have now a default size of 16x16 and origin of 8,13 (#354).
* The size of enemies must be a multiple of 8 (#358).
* `item:on_pickable_movement_changed` replaced by `pickable:on_movement_changed`.
* `pickable:get_treasure()` now returns the item instead of the item's name.
* Timers: returning true in the callback now repeats the timer (#152).
* `sol.timer.start()` now always returns the timer, even if its delay is zero.

#### Changes that do not introduce incompatibilities

* New Lua type and methods for custom map entities.
* New API of destructible objects, fully customizable now (#270).
* Colors now take an optional alpha value as fourth component.
* New functions `sol.video.get/set/reset_window_size()`.
* New method `surface:clear()`.
* Add loop and callback features to `sol.audio.play_music()` (#280).
* New function `sol.main.get_metatable()`, allows to implement shared behaviors.
* The lifetime of a menu can now be another menu.
* New method `menu:is_started()`.
* Attempt to stop a menu already stopped is no longer an error.
* New method `map:get_hero()` (#362).
* `map:get_world()` can now return nil because the world is now optional (#128).
* `map:create_wall()` accepts a new property `"stops_projectiles"`.
* Entity names are now auto-incremented to simplify their creation.
* New method `entity:get_game()` (#363).
* New methods `entity:bring_to_front()` and `entity:bring_to_back()` (#273).
* `entity:test_obstacles()` now takes an optional layer parameter.
* New methods `enemy:get_attack_consequence()`, `get_attack_consequence_sprite()`.
* The event `entity:on_created()` is now called for all types of entities.
* New event `enemy:on_hurt_by_sword()` to customize the sword strength (#426).
* New event `enemy:on_attacking_hero()` to customize attacks (#356).
* New event `enemy:on_hurting_hero()` to customize hurting the hero (#428).
* New event `hero:on_hurt()` to customize the defense of the equipment (#428).
* `hero:start_hurt()` now takes an optional entity and sprite (#417).
* New methods `hero:is/set_invincible()` and `hero:is/set_blinking()` (#418).
* New methods `hero:get/set_animation()` to set custom animations (#155).
* New methods `hero:get/set_sword_sound_id()` to change the sword sound (#155).
* New methods to set custom tunic, sword and shield sprites (#155).
* New functions to get/set the properties of teletransporters (#403).
* New functions to get/set the properties of blocks.
* New methods to simulate game commands, by mrunderhill (#382).
* New event `sensor:on_left()` (#339).
* New event `block:on_moving()` (#334).
* New event `teletransporter:on_activated()` (#312).
* New event `destination:on_activated()` (#312).
* `movement:on_position_changed()` now takes x and y parameters (#342).
* Fix `movement:start()` raising an error if the optional callback is nil.
* Fix `random_movement:get_max_radius()` that was not working.
* Check the parameter sign in `game:add/remove_life/money/magic` (#416).
* Check the parameter sign in `item:add/remove_amount`.
* Fix `timer:is_suspended_with_map()` that was not working.
* Fix crash when calling `timer:set_suspended_with_map()` without game started.
* New methods `timer:get/set_remaining_time`.
* New function `get_elapsed_time` to get the simulated time (#424).
* New function `get_key_modifiers` to get the key modifiers state.

### Solarus Quest Editor changes

* The editor is now built with Maven (#365).
* Multiple entities can now be resized at the same time (#405).
* Copy-pasting entities now pastes them at the cursor (#404).
* The map view can now be dragged using the middle mouse button (#413).
* Zoom in/out in the map view using the middle mouse wheel (rekcah1986).
* Add buttons to edit or refresh the tileset from the map view (#425).
* The tileset view can now be dragged using the middle mouse button (#427).
* Selecting a tile now highlights its pattern in the tileset view (#290).
* Ctrl/Shift+click even on an entity now starts a selection rectangle (#410).
* Tile patterns can now be moved in the tileset editor (#422).
* The id and name of a new resource and now asked in a single dialog (#321).
* Show the old value in the dialog to change an id/name (#468) (rekcah1986).
* The order of resources in the quest tree can now be changed (#319).
* Increase the stepsize when scrolling the map view.
* Center the dialog of editing an entity (#443).
* Add a scroller to dialogs that are too high (#437) (rekcah1986).
* Show the map or tileset name in the "do you want to save" dialog.
* Show quest name and resource ids in the tree view (thanks rekcah1986).
* Don't place new tiles below other entities if there are some (#461).
* Fix hidden entities getting selected when clicked (#460).
* Fix selecting entities losing their order when changing the layer.
* Fix `NullPointerException` when canceling the Open Project dialog.
* Fix freeze if tiles don't exist when changing the tileset of a map.
* Fix the tree not refreshing when deleting a resource (#335).
* Fix the tileset editor not always showing the save dialog on closing.
* Fix the `num_columns` property of sprites wrongly parsed.
* Fix wrong displaying of right-up and left-down diagonal jumpers.
* Switches, crystals and crystal blocks now show their actual sprite (#376).

## Solarus 1.1.1 (2013-12-01)

Bug fixes for the 1.1 release.

### Engine changes

* Fix a libmodplug compilation problem due to wrong sndfile.h (#324).
* Fix teletransporters activated while coming back from falling (#346).
* Fix a crash when changing the hero state in `block:on_moved` (#340).
* Fix enemy death detection when falling into hole, lava or water (#350).

## Solarus 1.1.0 (2013-10-13)

### Engine changes

* Add a very short sample quest with free graphics and musics (#232, #318).
* Allow scripted dialog boxes (#184).
* Allow a scripted game-over menu (#261).
* Replace the old built-in dialog box by a very minimal one.
* Remove the old built-in game-over menu.
* Remove the old built-in dark rooms displaying (#205).
* New entity: separators to visually separate some regions in a map (#177).
* New type of ground: ice (#182).
* New type of ground: low walls (#117).
* Blocks and thrown items can now fall into holes, lava and water (#191).
* Kill enemies that fall into holes, lava and water (#190).
* Allow quest makers and users to set the size of the playing area.
* Allow maps to have a default destination entity (#231).
* A game can now start without specifying an initial map and destination.
* Stairs inside a single floor can now go from any layer to a next one (#178).
* Fix map menus not receiving `on_command_pressed/released()` events.
* Fix camera callbacks never called when already on the target (#308).
* Fix a crash when adding a new menu during a `menu:on_finished()` event.
* Fix a crash when calling `hero:start_victory()` without sword.
* Fix an error when loading sounds (#236). Sounds were working anyway.
* Fix a possible memory error when playing sounds.
* Fix blocks that continue to follow the hero after picking a treasure (#284).
* Fix `on_obtained()` that was not called for non-brandished treasures (#295).
* Jumpers can no longer be activated the opposite way when in water.
* Jumpers are now activated after a slight delay (#253).
* Sensors no longer automatically reset the hero's movement (#292).
* Correctly detect the ground below the hero or any point.
* Don't die if there is a syntax error in dialogs.dat.
* Show a better error message if trying to play a Solarus 0.9 quest (#260).
* Remove built-in debug keys. This can be done from Lua now.
* Remove the preprocessor constant `SOLARUS_DEBUG_KEYS`.
* Call `on_draw()` before drawing menus.
* Fix .it musics looping when they should not.
* Log all errors in error.txt (#287).
* The quest archive can now also be named data.solarus.zip (#293).

### Data files format changes

You can use the script `tools/data_files_conversion/1.0_to_1.1/update_quest.lua`
to automatically update your data files. Don't forget to make a backup first.

* Sprites: New syntax easier to read and parse (#168).
* project_db.dat: New syntax easier to read and parse (#169).
* quest.dat: Allow to specify a range of supported quest sizes.
* Maps: Add the property `"default"` to destinations.
* Maps: Make optional the property `"destination"` of teletransporters.
* Tilesets: The ground value of diagonal walls with water has changed.
* Tilesets: New ground values `"ice"` and `"low_wall"`.
* dialogs.dat: Allow any property in dialogs. dialog_id and text are mandatory.
* languages.dat no longer exists. Languages are in project_db.dat now (#265).

### Lua API changes

#### Changes that introduce incompatibilities

* `map:is_dialog_enabled()` is replaced by `game:is_dialog_enabled()`.
* `map:start_dialog()` is replaced by `game:start_dialog()`.
* Remove `map:draw_dialog_box()`, no longer needed.
* Remove `map:set_dialog_style()`: replace it in your own dialog box system.
* Remove `map:set_dialog_position()`: replace it in your own dialog box system.
* Remove `map:set_dialog_variable()`: use the info param of `game:start_dialog()`.
* Make `map:get_entities()` returns an iterator instead of an array (#249).
* Replace `map:set_pause_enabled()` by `game:set_pause_allowed()`.
* Make the `enemy:create_enemy()` more like `map:create_enemy()` (#215).
* Remove `sol.language.get_default_language()`, useless and misleading (#265).
* Remove `sol.main.is_debug_enabled()`.
* Remove `map:get_light()` and `map:set_light()` (#205).
* In `game:get/set_ability()`, ability `"get_back_from_death"` no longer exists.
* Empty chests no longer show a dialog if there is no `on:empty()` event (#274).

#### Changes that do not introduce incompatibilities

* `game:get/set_starting_location()`: map and destination can now be nil.
* `hero:teleport()`: make destination optional (maps now have a default one).
* `map:create_teletransporter()`: make destination optional.
* Add a function `sol.video.get_quest_size()`.
* Make `map:get_camera_position()` also return the size of the visible area.
* Add a method `entity:is_in_same_region(entity)`.
* Add a method `entity:get_center_position()`.
* Add methods `entity:get_direction4_to()`, `entity:get_direction8_to()` (#150).
* Add a method `game:get_hero()`.
* Add methods `hero:get/set_walking_speed()` (#206).
* Add `hero:get_state()` and `hero:on_state_changed()` (#207).
* Add events `separator:on_activating()` and `separator:on_activated()` (#272).
* Add methods `enemy:is/set_traversable()` (#147).
* Add a method `enemy:immobilize()` (#160).
* Add `on_position_changed()` to all entities, not only enemies (#298).
* Add `on_obstacle_reached()` to all entities, not only enemies (#298).
* Add `on_movement_changed()` to all entities, not only enemies (#298).
* Add `on_movement_finished()` to all entities, not only enemies/NPCs (#298).
* Method `target_movement:set_target(entity)` now accepts an x,y offset (#154).
* Add a method `game:is_pause_allowed()`.
* Add a method `map:get_ground()` (#141).
* Add a method `map:get_music()` (#306).
* Add an optional parameter `on_top` to `sol.menu.start`.
* Add `sprite:on_animation_changed()` and `sprite:on_direction_changed()` (#153).
* Add a function `sol.input.is_key_pressed()`.
* Add a function `sol.input.is_joypad_button_pressed()`.
* Add a function `sol.input.get_joypad_axis_state()`.
* Add a function `sol.input.get_joypad_hat_direction()`.
* Add functions `sol.input.is/set_joypad_enabled()` (#175).
* Add a function `sol.audio.get_music()` (#146).
* Add a function `sol.audio.get_music_format()`.
* Add a function `sol.audio.get_music_num_channels()`.
* Add functions `sol.audio.get/set_music_channel_volume()` for .it files (#250).
* Add functions `sol.audio.get/set_music_tempo()` for .it files (#250).
* Return nil if the string is not found in `sol.language.get_string()`.
* `sol.language.get_dialog()` is now implemented.
* Add a function `game:stop_dialog(status)` to close the scripted dialog box.
* Add an event `game:on_dialog_started(dialog, info)`.
* Add an event `game:on_dialog_finished(dialog)`.
* Add functions `game:start_game_over()` and `game:stop_game_over` (#261).
* Add events `game:on_game_over_started()`, `game:on_game_over_finished` (#261).
* Add `sol.file` functions: `open()`, `exists()`, `remove()` and `mkdir()` (#267).

### Solarus Quest Editor changes

* Add a GUI to upgrade automatically quest files to the latest format (#247).
* Remove the initial prompt dialog to open a quest (#264).
* Replace non-free images by new icons (#245).
* Add tooltips to the add entity toolbar.
* Simplify the add entity toolbar by showing only one icon per entity type.
* Survive when images cannot be found (#256).
* Create more content when creating a new quest (#258, #279).
* Improve error messages.
* Fix a crash when creating a destructible without tileset selected (#283).
* Fix the sprite field disabled in the NPC properties dialog (#303).

## Solarus 1.0.4 (2013-06-26)

Bug fixes for the 1.0 release.

### Engine changes

* Don't die if a script tries so show a missing string (#237).
* Don't die if a treasure has a variant unexistent in the item sprite.
* Fix customization of joypad commands.

## Solarus 1.0.3 (2013-06-25)

Bug fixes for the 1.0 release.

### Engine changes

* Fix blocks not completely moved since Solarus 1.0.2.

## Solarus 1.0.2 (2013-06-22)

Bug fixes for the 1.0 release.

### Engine changes

* Fix a crash when a treasure callback changes the hero's state (#224).
* Fix a crash when a victory callback changes the hero's state.
* Fix a crash due to invalid sprite frame when animation is changed (#26).
* Fix an assertion error with FollowMovement of pickables.
* Fix the fullscreen mode not working on Mac OS X 10.7+ (#213, #220).
* Fix pickable treasures that could be obtained twice sometimes.
* Fix fade-in/fade-out effects on sprites that did not work (#221).
* Fix `sol.audio.play_music()` that failed with "none" or "same" (#201).
* Fix `item:set_sound_when_brandish()` that did not work.
* Fix diagonal movement that could bypass sensors since Solarus 1.0.1.
* Fix circle movement not working after `entity:set_enabled(true)`.
* Fix detection of movement finished for NPCs.
* Fix memory issues with menus (#210).
* Fix handling of `nil` parameter in boolean setters (#225).
* Fix hangling the default language.
* Correctly suspend timers when `set_suspended_with_map` is called.
* When a sprite is suspended, suspend its transitions (#226).
* Don't die if a music or a sound cannot be found.
* Don't die if an image cannot be found.
* Don't die if running a savegame without starting location specified.
* Don't die if a script refers to a non-existing equipment item.
* Don't die if the self parameter is missing when calling a method (#219).
* Fix dangling pointers after removing some kind of entities.

### Solarus Quest Editor changes

* Editor: allow to create map entities from the quest tree (#208).
* Editor: fix a typo in the bomb flower sprite (#214).
* Editor: fix a possible NullPointerException when opening an invalid map.

### Documentation changes

* Documentation: add the syntax specification of maps and tilesets.

## Solarus 1.0.1 (2013-05-12)

Bug fixes for the 1.0 release.

### Engine changes

* Fix the Mac OS X port.
* Fix jump movement accross two maps ending up in a wall (#189).
* Fix a possible crash in TextSurface.
* Fix the hero disappearing a short time after using the sword (#35).
* Fix the boomerang failing to bring back pickables sometimes (#187).
* Fix parallax scrolling tiles not always displayed (#167).
* Fix the setting joypad_enabled that had no effect (#163).
* Fix doors not working when they require equipment items.
* Fix a possible compilation warning in Surface.cpp.
* Fix creating a transition from the callback of a previous one.
* Fix crystal blocks animated late when coming from a teletransporter (#61).
* Fix arrows that got stopped when outside the screen (#73).
* Fix diagonal movement that failed in narrow passages (#39).
* Don't die if a script makes an error with a sprite (#151).
* Don't die if a script makes an error with an enemy attack consequence.
* Allow enemies to lose 0 life points when attacked (#137).
* Pixel-precise collisions can now also be performed on 32-bit images.

### Solarus Quest Editor changes

* Editor: add the possibility to show or hide each type of entity (#60).
* Editor: keep the map coordinates shown when changing the zoom (#183).
* Editor: fix the map view not updated correctly when changing the zoom (#174).
* Editor: show the correct sprite of destructible objects (#77).
* Editor: show an appropriate error message if the LuaJ jar is missing (#173).
* Editor: fix the title bar string (#176).

### Documentation changes

* Split the C++ documentation and the quest data files documentation (#181).
* Add a search feature to the documentation pages.

## Solarus 1.0.0 (2013-05-03)

The "I love Lua" release.

This is a major release. The version number switches from 0.x to 1.x
because there is a brand new Lua scripting API.
I now consider that Solarus can be used to create your own Zelda-like
games in decent conditions.
By "in decent conditions", I essentially mean: with a clean and stable
scripting API. Clean because there are nice datatypes now and the API is much
easier to use, less error-prone and more with the Lua spirit.
Stable because future versions of the scripting API will now try to keep
compatibility with existing scripts. Any API change that breaks compatibility
will now be clearly documented.

Data files and scripts written for solarus 0.x are not compatible with
solarus 1.x.
Which is not a problem since until now, I never said you could create a quest
in decent conditions :)
More seriously, I guess I'm the only one to have a lot of data files and
scripts created for solarus 0.x.
Anyway, conversion scripts are provided to upgrade your existing data files,
but not your scripts (the scripting API, which was working but ugly and
unstable, has totally changed).

### Engine changes

* Rewrite the Lua scripting API from scratch. Cleaner, real datatypes, much
easier to use, less error-prone, much more features, fully documentated.
* All scripts now live in a single Lua world.
* Add support of scripted graphics.
* Add support of scripted menus. Menus like the title screen, the savegames
menu, the pause menu and the HUD are no longer hardcoded into the engine.
* Doors, chests, teletransporters and destinations are now much more flexible
and customizable.
* Except tiles, all map entities can now have a name. The name is now optional.
* Change the format of some data files, including maps and tilesets.
* No more ini data files (removed the dependency to SimpleIni).
* Add conversion scripts to upgrade existing data files (but not scripts).
* Fix infinite explosions of bomb flowers.

This version also include changes from 0.9.3, though 0.9.3 is not released yet:

* The game screen size can now be set at compilation time.
* Change the savegames directory on Mac OS X.
* Improve the support of Mac OS X, Pandora, Caanoo and other platforms.
* Fix the compilation with Visual C++.
* Fix blocks making sometimes only a half move (#33).
* Fix pixel-precise collisions not always correct (#53).
* Fix the end of target movement on slow machines (#34).
* Fix the hero being freezed when using the hookshot on bomb flowers (#119).

### Solarus Quest Editor changes

* First release of a working editor.
* Implement creating a new quest.
* Implement edition of project_db.dat through the quest tree view.
* Allow to show a grid on the map editor.
* Fix a lot of bugs.

## Solarus 0.9.3 (2013-05-08)

This should be the last release of the 0.9 branch. Existing games must now
upgrade to Solarus 1.0.

However, upgrading to Solarus 1.0 represents a lot of work and testing: in the
meantime, this release introduces important fixes that improve the situation
of existing games.

* The game screen size can now be set at compilation time.
* Change the savegames directory on Mac OS X.
* Improve the support of Mac OS X, Pandora, Caanoo and other platforms.
* Images other than 8-bit can now be used for pixel-precise collisions.
* Fix the compilation with Visual C++.
* Fix the compilation of SimpleIni with gcc 4.7.2.
* Fix blocks making sometimes only a half move (#33).
* Fix pixel-precise collisions not always correct (#53).
* Fix the end of target movement on slow machines (#34).
* Fix the hero being freezed when using the hookshot on bomb flowers (#119).

## Solarus 0.9.2 (2012-04-03)

* Fix a crash using a teletransporter to the same map while an enemy is dying
* Immobilized enemies restarted too early when using a teletransporter

## Solarus 0.9.1 (2012-02-12)

* The player can now run with the action key if he has the ability `"run"`
* Fix locked doors and blocks that could consume more than one small key
* Experimental: new syntax of dialog files in Lua that makes parsing, writing
and translating dialogs easier (a conversion script is available)
* Experimental: optimize displaying and collisions far from the visible area
* Experimental: sensors are not obstacles anymore when jumping or using the
hookshot, they are activated instead
* Include Mac OS X packaging changes to the git repository
* HUD: the 11th heart was not displayed correctly when incomplete
* Lua: improve the prototype of `sol.map.destructible_item_create`
* Lua: add a function `sol.enemy.get_angle_to_hero()`
* Lua: add a function `sol.main.get_distance(x1, y1, x2, y2)`
* Allow the ground sprite below the hero to be tileset dependent
* Show a different sprite animation when plunging into lava or water
* Don't stop the super spin attack on shallow water
* Add debug keys to change the hero's layer

## Solarus 0.9.0 (2011-12-23)

* Initial release of Solarus without quest data
