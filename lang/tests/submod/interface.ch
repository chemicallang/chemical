@static
interface UnusedInterface334354535545345 {

    func give_me(&self) : int

}

@static
public interface ExportedUnusedInterface34233333 {

    func give_me(&self) : int

}

@static
interface UnimplementedInterface99888 {

    func do_stuff(&self) : int

}

func (thing : &mut UnimplementedInterface99888) unimp_delg_998888() : int {

    var x = thing.do_stuff();

    return x + 2;

}