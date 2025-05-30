#!/usr/bin/env tclsh

# Convert filename to a valid C symbol by replacing '.' and other non-alphanum chars with '_'
proc file_to_symbol {filename} {
    set symbol $filename
    regsub -all {[^a-zA-Z0-9]} $symbol _ symbol
    return $symbol
}

proc escape_char {char} {
    set ascii [scan $char %c]
    switch -- $char {
        "\\" { return "\\\\" }
        "\"" { return "\\\"" }
        "\n" { return "\\n" }
        "\r" { return "\\r" }
        "\t" { return "\\t" }
        default {
            if { $ascii < 32 || $ascii > 126 } {
                return [format "\\x%02x" $ascii]
            } else {
                return $char
            }
        }
    }
}

# Convert the contents of a file into a C-style char array
proc embed {filename} {
    if {![file exists $filename]} {
        puts stderr "File not found: $filename"
        return
    }

    set symbol_name [file_to_symbol [file tail $filename]]
    set content [read [set fd [open $filename rb]]]
    close $fd

    puts -nonewline "char ${symbol_name}\[\] = {\n    \""
    set n 0
    foreach byte [split $content ""] {
        puts -nonewline [escape_char $byte]
        if { $byte == "\n" } {
            puts -nonewline "\"\n    \""
        }
        incr n
    }

    puts "\"\n};\n"
    puts "size_t ${symbol_name}_len = $n;"
    puts ""
}

# Main loop for arguments
if {[llength $argv] == 0} {
    puts stderr "Usage: $argv0 file1 [file2 ...]"
    exit 1
}

foreach arg $argv {
    embed $arg
}
