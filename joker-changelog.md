## Joker's Patchpack Changelog

* * *

### v1.24.3 (2018-04-28)
* Set maximum size for left part of build-rail-station window since some newGRFs overdo it with the string length.
* Hotfix: Fix two crashes in trace restrict slot window when selling vehicles

### v1.24.2 (2018-04-26)
* Hotfix: Fix flat owner map screenshot crash
* Hotfix: Certain order lists containing depot orders could cause a crash.

### v1.24.1 (2018-04-24)
* Hotfix: Conditional order comparator dropdown list was broken.

### v1.24.0 (2018-04-24)
* Make it possible to filter the industry directory by accepted and produced cargo.
* Adjust timetable window layout
* Adjust trip history window layout
* Add size definition button to the departure board window and color the entire window brown.
* Change the way timetable automation handles conditional orders
* Show useful warnings in timetable window, e.g. when using auto-separation on conditional or full load orders.
* Add configurable buffer times to timetable automation and make ticks_per_minute a GUI setting.
* Add settings to round up travel and loading times during timetable automation.
* Display durations in hours and minutes everywhere additional to ticks.
* Go back to original values for industry site search to prevent the game from freezing too long.
* Bugfix: Conditional orders checking slot occupancy now react correctly when the referenced slot is deleted.

### v1.23.0 (2018-04-22)
* Add a button to force a resort every 74 ticks (every standard non-scaled day). This is especially useful for very frequently changing properties like timetable delay. reliability or profit-this-year.
* The sorting order toggle button in vehicle and group lists now forces a complete resort.
* The user interface no longer displays vehicles as early when the vehicle is waiting due to timetable separation. They are shown as on-time instead and are also sorted as such.
* Level crossings on the WIRE or PIPELINE track types no longer force vehicles to wait (and obviously don't cause crashes either).
* Bugfix: Time table automation now correctly records loading times.

### v1.22.0 (2018-04-20)
* Bugfix regarding the clearing of reserved tile on and beyond signaled bridges and tunnels.
* Bugfix custom road bridgeheads. The bridgehead now correctly allows removing certain road parts.
* Bridges can now be upgraded without losing rail signals or custom road bridgehead properties.
* Stations now keep a history of waiting cargo which can be displayed as a graph (last 48 days in 2 day increments)
* Multiple rail-type sets should now be more compatible with each other
* Adjusted the way rail types are sorted in the toolbar dropdown.

### v1.21.0 (2018-04-19)
* Bugfix in vehicle route rendering
* CargoDist code adjusted to match JGR Patch Pack

### v1.20.0 (2018-04-18)
* Show red overlay for inputs of the currently programmed logic signal.
* Enable use of full 255 NewGRFs in multiplayer by changing network protocol.
* Add routing restriction slot based conditional orders.
* Show the most relevant information in the vehicle list window, based on what the list is sorted on.
* Make it possible to name plans.
* Allow Ctrl-Click on departure board vehicle type buttons to show type exclusively.
* Gradually slow down for red signals on bridges/tunnels instead of coming to a sudden halt immediately before the signal. (imported)
* Improve pathfinder support for multiple docks. (imported)
* CargoDist performance improvements. (imported)
* Viewport performance improvements. (imported)
* Add cargo filter support to vehicle list and tracerestrict slot windows. (imported)
* Add patchpack info and website to about window.
* Add program append management action to routing restriction signals. (imported)
* Double-clicking order in timetable change wait time. (imported)
* Increase maximum permitted depot and station name lengths. (imported)
* Show routing restriction and/or logic signal windows when Ctrl-clicking signal. (imported)
* Add Ctrl+Click to scroll to plan location. (imported)
* Add a 'Show All' button to plans window. (imported)
* Add routing restriction slots (accessible in company trains list window and usable with routing restriction signals) (imported)
* Add wait at PBS signal routing restriction action. (imported)
* Add chunnels (tunnels under sea) (imported)
* Add instruction move up/down buttons to routing restriction window. (imported)
* Allow shallow-removing conditional blocks by use of ctrl+click in routing restriction window. (imported)
* Add instruction scroll-to for PBS entry signal conditional in routing restriction window. (imported)
* Add custom road bridgeheads (bridges without ramps near tiles with foundations. Build bridge, then add road tiles to the ramp to use this feature. (imported)
* Increase maximum permitted vehicle name length. (imported)
* Fix poor cargo label contrast in CargoDist window. (imported)
* Add 32bpp-sse2-anim and 32bpp-sse4-anim higher performance blitters (activate in OpenTTD.cfg like 'blitter = 32bpp-sse4-anim' in the '[misc]' section. (imported)
* Increase max value of difficulty.max_loan setting by factor of 10. (imported)
* Add train length and group name to vehicle details window. (imported)

### v1.18.18 (2018-04-03)
* Add station departure boards. (include wallclock time in timetables and status bar (purely visual))
* Add four flat screenshot variants (normal, height, industry, owner). Usable just like regular screenshots in the dropdown.
* Add timetable automation. (imported and modified)

### v1.18.17 (2018-04-01)
* Added ability to take a minimap screenshot. (Button in the minimap window) (imported)
* Fixed Passenger and Mail cargo amount (accidental quadratic growth). Now small towns will have more, big cities less. (imported)

### v1.18.16 (2018-04-01)
* Fix generation of town cargo other than pax/mail (e.g. ECS tourists). (imported)

### v1.18.15 (2018-04-01)
* Bug fixes

### v1.18.14 (2018-03-31)
* Bug fixes

### v1.18.13 (2018-03-29)
* Allow grass to grow under trees independent of tree growth speed. And allow bringing a tree to full size with the planting tool even if the tree limit has been reached already.

### v1.18.12 (2017-12-25)
* Fix right-click scrolling after Windows 10 Fall Creators Update. (imported)

### v1.18.11 (2017-11-20)
* Make houses with earliest introduction date available per NEWGRF instead of globally. (this caused issues when multiple town/house NewGRFs were loaded with different introduction dates for their earliest buildings.

### v1.18.10 (2017-11-19)
* Fixes to cargo-specific orders.

### v1.18.9 (2017-11-19)
* Prevent level crossings sound for rail types that are not actually rail (wire and pipeline)
* Changed the way snow appears on rail and road slopes and on tunnels.

### v1.18.8 (2017-10-22)
* Fix crash bug with 'go-to-depot' order when depot in orders list is destroyed.

### v1.18.7 (2017-09-24)
* Switch to speed-optimized compilation

### v1.18.6 (2017-09-20)
* Make sure companies don't get paid for cargo until final delivery, despite the new transfer calculation.
* Separated tree line setting from snow line setting.

### v1.18.5 (2017-09-18)
* Add public road creation to connect towns during map creation

### v1.18.4 (2017-09-17)
* Make minimum river length setting visible in the settings window.

### v1.18.3 (2017-09-14)
* Add setting for maximum height level for cities (no more cities on mountain tops)

### v1.18.2 (2017-09-13)
* Improve river generation (proper creation of longer-wider rivers among other things)

### v1.18.1 (2017-09-13)
* Adjusted tree growth around snow line. Towns will never get built above tree line.

### v1.18.0 (2017-09-12)
* Added cargo-specific load and unload orders. (imported)
* Allow multiple docks per station. (imported and modified)
* Added more viewport map tooltips
* Add 'Minimum distance between towns' world generation setting.
* Allow towns to build bridges over rail. (imported and modified)

### v1.17.8 (2017-08-22)
* Bugfix regarding train speed adaptation with crashed trains.

### v1.17.7 (2017-08-14)
* Bugfix

### v1.17.6 (2017-08-14)
* Add option to sell all vehicles in a group to the vehicle group window

### v1.17.5 (2017-08-13)
* Fix a crash when a non-train vehicle without orders is sent to the depot.

### v1.17.4 (2017-07-31)
* Fix long range reserve route restriction feature

### v1.17.3 (2017-07-29)
* Make sure power info is correctly displayed for train templates

### v1.17.2 (2017-07-08)
* Linux compilation fixes (Pull request)
* Fix crash when attemption to auto-group a train without any cargo capacity

### v1.17.1 (2017-06-11)
* Fix division by zero crash

### v1.17.0 (2017-06-10) (exact version changelog missing before this)
* Add button to auto group of vehicles to the vehicle list window
* Make it possible to replace an old vehicle with the same model in the vehicle replacement window.
* Add the "all railtypes" option to train template replacement window.
* Make area-drag-placing of single-tile NewGRF objects and buying land possible.
* Daylength setting can no longer be changed after game start.
* Change the way "Send to Depot/Servicing" works. Now vehicles will prefer a depot on their order list to just any vehicle. Vehicles will go directly to that depot now and then restart their orders list from that very depot. Trains may have difficulties finding their depot, therefore they'll keep processing their normal orders, skipping any loading however, until they reach it. That makes sure they always find their way.
* Improve support for signals in tunnels and on bridges
* Remove the "start date" button from the timetable GUI and instead add a button that allows to transform all estimated times into timetabled ones at once.
* Reactivate subsidies for CargoDist handled cargoes.
* Removed the wait time as a parameter for the station rating and increased the importance of the units of waiting cargo instead to make large capacity vehicles roughly as efficient as multiple small capacity ones.
* Add a group membership conditional order.
* Add the lifetime profit display to the vehicle group window.
* Increase number of possible rail types from 16 to 32.
* Make sure industry build locations are found better.
* Change the train details window total cargo tab to Performance tab and added additional useful info about the length of the train, its maximum speed when empty and loaded.
* Change the window of template vehicles so they correctly show the weight of the fully loaded train (which is of most interest when creating a template).
* Always allow tunnels to cross. Height is very abstract in OpenTTD and it is also perfectly possible for a junction to be in a tunnel.
* Add station catchment button to the station GUI.
* Make it possible to change the destination of a conditional order with Ctrl-Shift-Click.
* Adjust the cargo income curves. They are more stable now and make a lot more sense.
* Vehicles no longer have a running cost when waiting in a depot.
* Add support for experimental seaplane airports and the corresponding upgraded OpenGFX Airports NewGRF
* Change the visuals of the station rating tooltip and improved the 'next stop feedback' functionality again. Now it should work perfectly.
* Several bugfixes

### v1.12.0 (2016-01-13)
* Add a detailed station rating tool tip for each cargo in the station window.
* Change the way punishments for source stations work. That is now also displayed in the tool tip.
* Do a round robin for all accepting industries instead of only delivering to the closest one.
* Add additional conditional orders.
* Add 'Reverse at waypoint' order
* Add setting to hide track overgrowth.
* Hierarchical groups can now be collapsed
* Add train speed adaptation (trains go slower if a train in front of them is slower)

### v1.9.1 (2016-01-07)
* Improved timetable usability with several Ctrl+Click functionalities to set or clear values on all orders.
* Add 'show/hide vehicles' feature for the 'advanced build train' and 'build train template' windows.

### v1.7.0 (2016-01-07)
* Variable day length setting
* 255 GRFs in single player mode
* Enhanced viewport with higher zoom levels and tool tips
* Template-based train replacement
* Plan drawing on the map
* Routing restriction signals
* Logic signals
* Upgrade airports
* Vehicle group info
* Zoning Toolbar (overlay to mark Routing Restriction Signals, station catchment and other data)
* Town cargo generation factor
* Signals in tunnels and on bridges (with support for path signals)
* Measurement tool in landscaping toolbar
* Rating in town label
* Changed tree growth. Less trees at level 1 and approaching the tree line on mountains.
* Ship collision avoidance
* Better road vehicle overtaking behavior. They now mostly ignore opposing traffic but are now capable of truly using both lanes of one way roads. (Think highways)
* Trains can no longer collide with road vehicles (reasoning behind this is that such crashes cannot be prevented.)
* Vehicle trip history window
* Unused tracks are overgrown by grass (purely visual, can be deactivated in settings)
* Make 'cargo payment rates' graph more useful by basing it on vehicle speed
* Rail fences can be hidden by a setting (In the settings dropdown)
* Taxes (a percentage of the company value) are payed by each company. This punished dead cash and rewards investment.































