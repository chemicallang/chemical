// Copyright (c) Qinetik 2024.

#include <string>
#include <iosfwd>

void write_escape_encoded(std::ostream& stream, char value);

//range : [min, max]
int random(int min, int max);