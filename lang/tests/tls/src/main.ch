public func main(argc : int, argv : **char) : int {
    test_ensure_tmp_dir()
    test_runner(argc, argv)
    return 0
}
