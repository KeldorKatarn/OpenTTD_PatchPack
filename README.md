## Joker's Patchpack version v1.25.0

This is a collection of patches applied to [OpenTTD](http://www.openttd.org/)
This patch pack is a collection of features created by various developers.
I added my own features to the collection and make the rest work together. 
But the credits for features or improvements not created by me 
goes to the original developers.

* * *

OpenTTD is a transport simulation game based upon the popular game Transport
Tycoon Deluxe, written by Chris Sawyer. It attempts to mimic the original
game as closely as possible while extending it with new features.

OpenTTD is licensed under the GNU General Public License version 2.0,
but includes some 3rd party software under different licenses. See the
section "Licensing" in readme.txt for details.

* * *

See [readme.txt](readme.txt) for the original OpenTTD readme.

The thread for this patchpack can be found [here](https://www.tt-forums.net/viewtopic.php?f=33&t=74365).

See [joker-changelog.md](joker-changelog.md) for changelog.


#### This patchpack contains the following


* Variable day length setting

* Show daytime next to date and use hours and minutes additional to ticks in the GUI (ticks_per_minute adjustable per client)

* 255 GRFs in single AND multiplayer mode

* Increase number of available rail track types from 16 to 32

* Automated timetables and separation with several different separation modes.

* Enhanced viewport with higher zoom levels and tool tips

* Template-based train replacement

* Plan drawing on the map

* Stations now keep a history of waiting cargo which can be displayed as a graph (last 48 days in 2 day increments)

* Filter the industry directory by accepted and produced cargo.

* Useful warnings are shown in timetable window, e.g. when using auto-separation on conditional or full load orders.

* Multiple rail-type sets should now be more compatible with each other.

* Rail types are sorted differently in the toolbar dropdown, first by compatibility, then by max speed.

* Routing restriction signals

* Logic signals

* Upgrade airports

* Vehicle group info

* Zoning Toolbar (overlay to mark Routing Restriction Signals, station catchment and other data)

* Departure boards for stations

* Town cargo generation factor

* Vehicles visible in tunnels (transparency setting)

* Signals in tunnels and on bridges (with support for path signals)

* Measurement tool in landscaping toolbar

* Timetabling waiting time in depots

* Minimap screenshots (extra button in the minimap window)

* Rating in town label

* Changed tree growth. Less trees at level 1 and approaching the tree line on mountains.

* Enable building rivers in game

* More conditional orders

* Include the most relevant information based on the selected sorting in the vehicle group 

* Include the train length and group name in the vehicle details window

* Flat minimap screenshots for traffic, terrain height, owner and industry map views
 
* Pause on savegame load if ctrl key is pressed.

* Reverse at waypoint orders

* Vehicle lifetime profit

* Hierarchical group collapse

* Ship collision avoidance

* Polyline rail track building tool

* Cargo type orders, this allows order load/unload types to be set per cargo type

* Towns can be connected by public roads during map creation

* When building tunnels, open new viewports at the far end of the tunnel when the 'v' key is pressed

* Custom bridge heads for road bridges

* Chunnels (tunnels under bodies of water)

* Improved train purchase window (can be activated in settings). Shows locomotives and wagons separately.

* Minimum distance between towns can be set during world creation

* Maximum height level for towns (no more big cities on mountain tops)

* Transfer payment is payed out once the cargo reaches its destination

* Better road vehicle overtaking behavior. They now mostly ignore opposing traffic but are now capable of truly using both lanes of one way roads. (Think highways)

* Level crossing improvements
  * Adjacent level crossings now close
  * Trains can no longer collide with road vehicles (reasoning behind this is that such crashes cannot be prevented.)

* Towns build bridges over rails

* Vehicles now only service in the depot in their orders list if there is one. Since trains can have a hard time finding their depot they go through their normal orders list (without loading at stations) until they reach it. If no depot is in the orders the default behavior is executed.

* Performance improvements (code by JGR from JGR Patch Pack)

* Multiple docks per station

* Cargo type filter in vehicle list windows

* Vehicle trip history window

* Better algorithm for river creation. Also longer rivers become wider than just one tile. (This works best with a map created with a custom heightmap since standard generated maps have too many small valleys and lakes)

* Station ratings now have helpful tooltips

* Station ratings now ignore time between vehicle arrival, so people playing with fewer longer trains are not punished more than people playing with more, shorter trains.

* Miscellaneous  
  * Unused tracks are overgrown by grass (purely visual, can be deactivated in settings)
  * Make 'cargo payment rates' graph more useful by basing it on vehicle speed
  * Increase maximum setting limits for per-company vehicle-type limits.
  * Increase maximum permitted vehicle name length, vehicle group name length, and depot/station name lengths
  * Various minor fixes
  * Rail fences can be hidden by a setting (In the settings dropdown)
  * Taxes (a percentage of the company value) are payed by each company. This punished dead cash and rewards investment.

#### Compiler 

C++11 support is required.

Up-to-date versions of MSVC work best. I do not personally maintain any other compiler toolchain or other platforms than Windows. If you want to help with that, please create a pull request on GitHub.
