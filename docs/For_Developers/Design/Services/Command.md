# Command

Command service occupies one port of the port_handler. It is meant for short lived commands, primarily used during debugging and validation testing. Because of this it prioritizes ease of implementing commands above everything else.

## Command arguments

{
    "cmd": "command name",
    "args": ["arg0", 1.0, "AwQF"],
    "port": 42,
    "id": 512
}

## Command result


