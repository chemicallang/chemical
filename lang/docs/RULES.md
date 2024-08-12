### Rules

These are rules that will be enforced once in LSP for syntax. Currently 
not being worked on

### Struct Definition Member Names

1 - All members in struct definition cannot have names equal to members' names present
in the structs inherited

2 - All members in struct definition cannot have names equal to inherited structs names

### Struct Definition Multiple Inheritance

1 - For a struct to be compatible with multiple inheritance, It must never
inherit structs that inherit same structs, so that multiple copies of structs can be avoided
