set testcases {}

# name opts inputs outputs

lappend testcases [list "avgprice" {} {
    {81.85,81.2,81.55,82.91,83.1,83.41,82.71,82.7,84.2,84.25,84.03,85.45}
    {82.15,81.89,83.03,83.3,83.85,83.9,83.33,84.3,84.84,85,85.9,86.58}
    {81.29,80.64,81.31,82.65,83.07,83.11,82.49,82.3,84.15,84.11,84.03,85.39}
    {81.59,81.06,82.87,83,83.61,83.15,82.84,83.99,84.55,84.36,85.53,86.54}
} {
    {81.720,81.198,82.190,82.965,83.408,83.393,82.843,83.323,84.435,84.430,84.873,85.990}
}]
lappend testcases [list "vwma" {4} {
    {50.25,50.55,52.5,54.5,54.1,54.12,55.5,50.2,50.45,50.24}
    {12412,12458,15874,12354,12456,12542,15421,19510,12521,12041}
} {
    {51.9819,52.8828,53.7204,54.6075,53.1948,52.4340,51.6345}
}]


file delete -force tests
file mkdir tests
foreach testcase $testcases {
	lassign $testcase name options inputs outputs
	set o [open "tests/$name.c" w]
    set indent "    "
	puts $o "#include <stdio.h>"
    puts $o "#include <math.h>"
    puts $o "#include <stdbool.h>"
	puts $o ""
	puts $o "#include \"../indicators.h\""
	puts $o ""
	puts $o "int main() {"
    puts $o "${indent}const ti_indicator_info *info = ti_find_indicator(\"$name\");"
	for {set i 0} {$i < [llength $inputs]} {incr i} {
        puts $o "${indent}TI_REAL const input_$i\[\] = {[lindex $inputs $i]};"
    }
    puts $o "${indent}const TI_REAL* inputs\[\] = {"
    puts -nonewline $o "${indent}${indent}"
    for {set i 0} {$i < [llength $inputs]} {incr i} {
        puts -nonewline $o "input_$i, "
    }
    puts $o ""
    puts $o "${indent}};"
    for {set i 0} {$i < [llength $options]} {incr i} {
        puts $o "${indent}const TI_REAL option_$i = [lindex $options $i];"
    }
    puts $o "${indent}const TI_REAL options\[\] = {"
    puts -nonewline $o "${indent}${indent}"
    for {set i 0} {$i < [llength $options]} {incr i} {
        puts -nonewline $o "option_$i, "
    }
    puts $o ""
    puts $o "${indent}};"
    set out_size [llength [split [lindex $outputs 0] {,}]]
    set size [llength [split [lindex $inputs 0] {,}]]
    puts $o "${indent}const int size = $size;"
    puts $o "${indent}const int out_size = $out_size;"
    for {set i 0} {$i < [llength $outputs]} {incr i} {
        puts $o "${indent}TI_REAL *output_$i = malloc\(out_size * sizeof(TI_REAL)\);"
    }
    puts $o "${indent}TI_REAL *outputs\[\] = {"
    puts -nonewline $o "${indent}${indent}"
    for {set i 0} {$i < [llength $outputs]} {incr i} {
        puts -nonewline $o "output_$i, "
    }
    puts $o ""
    puts $o "${indent}};"

    for {set i 0} {$i < [llength $outputs]} {incr i} {
        puts $o "${indent}TI_REAL expected_output_$i\[\] = {[lindex $outputs $i]};"
    }

    puts $o "${indent}printf(\"running testcase $name...\\n\");"
    puts $o "${indent}ti_$name\(size, inputs, options, outputs\);"

    puts $o "${indent}bool ok;"
    for {set i 0} {$i < [llength $outputs]} {incr i} {
        puts $o "
    ok = true;
    for (int i = 0; i < out_size; ++i) {
        ok = ok && (fabs(output_$i\[i\] - expected_output_$i\[i\]) < 0.0001);
    }
    if (!ok) {
        printf(\"output #%s mismatch: \\n\", info->output_names\[$i\]);
        printf(\"actual \{\\n    \");
        for (int i = 0; i < out_size; ++i) {
            printf(\"%.4f,\", output_$i\[i\]);
        }
        printf(\"\\n\}\\nexpected \{\\n    \");
        for (int i = 0; i < out_size; ++i) {
            printf(\"%.4f,\", expected_output_$i\[i\]);
        }
        printf(\"\\n\}\\n\");
    }
        "
    }
    puts $o "}"
}