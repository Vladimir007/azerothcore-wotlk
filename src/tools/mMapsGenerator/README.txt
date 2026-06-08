Generator command line args

--config   [file.*]  The path to the YAML config file
                     Default: "mMapsConfig.yaml"

--threads  [#]       Max number of threads used by the generator
                     Default: 3

--map      [#]       Build only the map specified by #

           [path.*]  Data directory path. It should contain "Maps" and "vMaps" folders.
                     Default: current script folder.

Examples:

mMapsGenerator
builds maps using the default settings (see above for defaults)

mMapsGenerator --map 0
builds map 0

mMapsGenerator --map 0 ../Data/
builds map 0 with data directory set to "../Data"
