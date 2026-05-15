public namespace core {
public namespace stream {

public interface Stream {

    func writeStr(&self, value : *char, length : ubigint);

    func writeStrNoLen(&self, value : *char);

    func writeSigned(&self, value : bigint);

    func writeUnsigned(&self, value : ubigint);

    func writeChar(&self, value : char);

    func writeUChar(&self, value : uchar);

    func writeFloat(&self, value : float);

    func writeDouble(&self, value : double);

}

}
}
