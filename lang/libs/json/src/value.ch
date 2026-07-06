public variant JsonValue {
    Null()
    Bool(value : bool)
    Number(value : std::string)
    String(value : std::string)
    Object(values : std::unordered_map<std::string, JsonValue>)
    Array(values : std::vector<JsonValue>)
}
