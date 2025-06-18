/** TODO: generated code doesn't work for the following snippet
TODO: it should actually generate errors, lkg_moved1 needs to be loaded
  because its not a compile time constant, however its being assigned to top level var init

var lkg_move0 = [ lkg_moved0 ]

var lkg_moved0 = () => 98987863

var lkg_move1 = lkg_moved1

var lkg_moved1 = () => 3

struct Container {
    var contain : () => int
}

var lkg_move2 = Container { contain : lkg_moved2 }

var lkg_moved2 = () => 4

**/