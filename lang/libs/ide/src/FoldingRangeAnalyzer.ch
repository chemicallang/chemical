public struct FoldingRangeAnalyzer {

    // puts a single folding range
    func put(&self,
        start : Position,
        end : Position,
        comment : bool
    );

    func stackPush(&self, pos : Position)

    func stackEmpty(&self) : bool

    func stackPop(&self) : Position

}