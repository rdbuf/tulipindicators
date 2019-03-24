/*
 * Tulip Indicators
 * https://tulipindicators.org/
 * Copyright (c) 2010-2017 Tulip Charts LLC
 * Lewis Van Winkle (LV@tulipcharts.org)
 *
 * This file is part of Tulip Indicators.
 *
 * Tulip Indicators is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * Tulip Indicators is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Tulip Indicators.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "utils/minctest.h"
#include "indicators.h"
#include "utils/buffer.h"
#include <string.h>
#include <stdlib.h>

/*********** TYPEDEFS ************/

enum {OK, FAILURES_OCCURED, PARSING_ERROR, VERSION_MISMATCH};

/*********** GLOBALS ************/

int tested[TI_INDICATOR_COUNT] = {0};
int failed_cnt = 0;

/************ PARSING PRIMITIVES *************/

char *read_line(FILE *fp) {
    static char buf[65536];
    while (fgets(buf, 65536, fp)) {
        if (buf[0] == '#' || buf[0] == '\n' || buf[0] == '\r') { continue; }
        return buf;
    }
    return 0;
}

int read_array(FILE *fp, TI_REAL *s) {
    char *line = read_line(fp);
    if (!line) {
        printf("seems like an unexpected eof\n");
        exit(PARSING_ERROR);
    }

    if (line[0] != '{') {
        printf("bad input: expected array, got '%s'\n", line);
        exit(PARSING_ERROR);
    }

    TI_REAL *inp = s;
    char *num = strtok(&line[1], ",}\r\n");
    while (num) {
        *inp++ = atof(num);
        num = strtok(0, ",}\r\n");
    }

    return (int)(inp - s);
}

/*********** UTILITIES *************/

int equal_reals(TI_REAL a, TI_REAL b) {
    return fabs(a - b) < 0.001;
}

int equal_arrays(TI_REAL *a, TI_REAL *b, int size_a, int size_b) {
    if (size_a != size_b) { return 0; }

    int i;
    for (i = 0; i < size_a; ++i) {
        if (!equal_reals(a[i], b[i])) { return 0; };
    }
    return 1;
}

void print_array(TI_REAL *a, int size) {
    printf("[%i] = {", size);
    for (int i = 0; i < size-1; ++i) {
        printf("%.3f,", a[i]);
    }
    if (size) { printf("%.3f", a[size-1]); }
    printf("}");
}

/*********** PARSING, TESTING, REPORTING ************/

void run_one(FILE *fp, const char* target_name) {
    char *line = read_line(fp);
    if (!line) { return; }
    if (line[0] < 'a' || line[0] > 'z') {
        printf("expected indicator name, got %s\n", line);
        exit(PARSING_ERROR);
    }
    char *name = strtok(line, " \n\r");
    int skip_this = target_name && strcmp(name, target_name) != 0;
    if (!skip_this) { printf("running \t%-16s... ", name); }

    int any_failures_here = 0;

    const ti_indicator_info *info = ti_find_indicator(name);
    if (!info) {
        printf("unknown indicator %s\n", name);
        failed_cnt += 1;
        any_failures_here = 1;
        goto cleanup;
    }

    tested[(int)(info - ti_indicators)] = 1;

    TI_REAL options[TI_MAXINDPARAMS];
    TI_REAL *o = options;
    const char *s;
    while ((s = strtok(0, " \n\r"))) { *o++ = atof(s); }

    if (o-options != info->options) {
        printf("options number mismatch: expected %lli, got %lli\n", o-options, info->options);
        failed_cnt += 1;
        any_failures_here = 1;
        goto cleanup;
    }

    int i;

    TI_REAL *inputs[TI_MAXINDPARAMS] = {0};
    TI_REAL *answers[TI_MAXINDPARAMS] = {0};
    TI_REAL *outputs[TI_MAXINDPARAMS] = {0};

    int input_size = 0;
    for (i = 0; i < info->inputs; ++i) {
        inputs[i] = malloc(sizeof(TI_REAL) * 4096);
        input_size = read_array(fp, inputs[i]);
    }

    int answer_size = 0;
    for (i = 0; i < info->outputs; ++i) {
        answers[i] = malloc(sizeof(TI_REAL) * 4096);
        outputs[i] = malloc(sizeof(TI_REAL) * 4096);
        answer_size = read_array(fp, answers[i]);
    }

    if (skip_this) { goto cleanup; }

    const clock_t ts_start = clock();
    const int ret = info->indicator(input_size, (const double * const*)inputs, options, outputs);
    const clock_t ts_end = clock();

    if (ret != TI_OKAY) {
        printf("return code %i\n", ret);
        failed_cnt += 1;
        any_failures_here = 1;
        goto cleanup;
    }

    int output_size = input_size - info->start(options);
    if (output_size < 0) output_size = 0;
    for (i = 0; i < info->outputs; ++i) {
        if (!equal_arrays(answers[i], outputs[i], answer_size, output_size)) {
            failed_cnt += 1;
            any_failures_here = 1;
            printf("output '%s' mismatch\n", info->output_names[i]);
            printf("> expected: "); print_array(answers[i], answer_size); printf("\n");
            printf("> got:      "); print_array(outputs[i], output_size); printf("\n");
        }
    }

cleanup:
    for (i = 0; i < info->inputs; ++i) { if (inputs[i]) { free(inputs[i]); } };
    for (i = 0; i < info->outputs; ++i) { if (answers[i]) { free(answers[i]); } };
    for (i = 0; i < info->outputs; ++i) { if (outputs[i]) { free(outputs[i]); } };
    if (!any_failures_here && !skip_this) {
        printf("%4dμs\n", (int)((ts_end - ts_start) / (double)CLOCKS_PER_SEC * 1000000.0));
    }
}

void run_tests(const char *fname, const char* target_name) {
    printf("# test suite %s:\n", fname);
    FILE *fp = fopen(fname, "r");
    if (!fp) {
        printf("failed to open\n");
        exit(PARSING_ERROR);
    }

    while (!feof(fp)) {
        run_one(fp, target_name);
    }

    printf("\n");
    fclose(fp);
}

void test_buffer() {
    ti_buffer *b = ti_buffer_new(3);
    ti_buffer_push(b, 5.0); lfequal(b->sum, 5.0);
    ti_buffer_push(b, 5.0); lfequal(b->sum, 10.0);
    ti_buffer_push(b, 1.0); lfequal(b->sum, 11.0);
    ti_buffer_push(b, 1.0); lfequal(b->sum, 7.0);
    ti_buffer_push(b, 3.0); lfequal(b->sum, 5.0);
    ti_buffer_push(b, 1.0); lfequal(b->sum, 5.0);
    ti_buffer_push(b, 2.0); lfequal(b->sum, 6.0);
    ti_buffer_push(b, 3.0); lfequal(b->sum, 6.0);

    lfequal(ti_buffer_get(b, 0), 3.0);
    lfequal(ti_buffer_get(b, -1), 2.0);
    lfequal(ti_buffer_get(b, -2), 1.0);
    lfequal(ti_buffer_get(b, -3), 3.0);

    ti_buffer_free(b);
}

int main(int argc, const char** argv) {
    if (strcmp(TI_VERSION, ti_version()) != 0) {
        printf("library version mismatch: header %s, binary %s\n", TI_VERSION, ti_version());
        exit(VERSION_MISMATCH);
    }
    if (TI_BUILD != ti_build()) {
        printf("build version mismatch, header %i, binary %i\n", TI_BUILD, ti_build());
        exit(VERSION_MISMATCH);
    }

    const char* target_name = argc > 1 ? argv[1] : 0;

    run_tests("tests/untest.txt", target_name);
    run_tests("tests/atoz.txt", target_name);
    run_tests("tests/extra.txt", target_name);

    int i;
    for (i = 0; i < TI_INDICATOR_COUNT; ++i) {
        if (!tested[i]) { printf("WARNING: no test for %s\n", ti_indicators[i].name); }
    }

    if (failed_cnt == 0) {
        printf("ALL TESTS PASSED\n");
    } else {
        printf(" %d TEST%s FAILED\n", failed_cnt, failed_cnt > 1 ? "s" : "");
    }

    return failed_cnt ? FAILURES_OCCURED : OK;
}
