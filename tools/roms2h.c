// The MIT License (MIT)

// Copyright (c) 2020 TotalJustice

//  Permission is hereby granted, free of charge, to any person obtaining a
//  copy of this software and associated documentation files (the "Software"),
//  to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense,
//  and/or sell copies of the Software, and to permit persons to whom the
//  Software is furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
//  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//  DEALINGS IN THE SOFTWARE.

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#define ROM_PATH "../roms/"
#define OUT_PATH "../ch8_ext_roms.h"

struct Stuff {
    char path[1024];
    char name[512];
};

static int write_file(const char *file, char *name, FILE *ofile) {
    unsigned char buffer[1024] = {0};

    FILE *fp = fopen(file, "rb");
    if (!fp) return -1;
    fseek(fp, 0, SEEK_END);
    const long filesize = ftell(fp);
    long size = filesize;
    rewind(fp);
    
    fprintf(ofile, "\tcase %s: {\n\t\tconst unsigned char data[] = {", name);

    while (size > 0) {
        const long read_size = size < sizeof(buffer) ? size : sizeof(buffer);
        fread(buffer, read_size, 1, fp);
        for (long i = 0; i < read_size; i++) {
            fprintf(ofile, "0x%02X, ", buffer[i]);
        }
        size -= read_size;
    }

    fprintf(ofile, "};\n\t\treturn ch8_loadrom(ch8, data, %d);\n\t}\n", filesize);

    fclose(fp);
    return 0;
}

int main() {
    long dir_count = {0};
    struct Stuff *stuff = {0};
    const size_t rom_path_len = strlen(ROM_PATH); 
    char full_path[1024] = ROM_PATH;
    FILE *ofile = {0};
    DIR *dir = {0};
    struct dirent *d = {0};

    ofile = fopen(OUT_PATH, "w");
    dir = opendir(ROM_PATH);

    while ((d = readdir(dir))) {
        if (d->d_name[0] == '.' || strcmp(d->d_name, "..") == 0) continue;
        ++dir_count;
        stuff = realloc(stuff, sizeof(struct Stuff) * dir_count);
        strcat(full_path, d->d_name);
        {
            char *ext = strrchr(d->d_name, '.');
            if (ext) *ext = '\0';
        }
        strcpy(stuff[dir_count - 1].name, d->d_name);
        strcpy(stuff[dir_count - 1].path, full_path);
        full_path[rom_path_len] = '\0';
    }
    closedir(dir);

    // guards and includes
    fprintf(ofile, "#ifndef CH8_EXT_ROMS\n#define CH8_EXT_ROMS\n\n#include \"ch8.h\"\n\n");
    
    // enum
    fprintf(ofile, "typedef enum {\n");
    for (size_t i = 0; i < dir_count; i++) {
        fprintf(ofile, "\t%s,\n", stuff[i].name);
    }
    fprintf(ofile, "} ch8_roms_t;\n\n");

    // function dec
    fprintf(ofile, "bool ch8_loadbuiltinrom(ch8_t *ch8, ch8_roms_t idx);\n\n");

    // imp guard and function def
    fprintf(ofile, "#ifdef CH8_ROMS_IMPLEMENTATION\n\n");
    fprintf(ofile, "bool ch8_loadbuiltinrom(ch8_t *ch8, ch8_roms_t idx) {\n");
    fprintf(ofile, "\tswitch (idx) {\n");
    for (size_t i = 0; i < dir_count; i++) {
        write_file(stuff[i].path, stuff[i].name, ofile);
    }
    fprintf(ofile, "\tdefault: return false;\n\t}\n}\n\n");

    // end guard
    fprintf(ofile, "\n#endif // CH8_ROMS_IMPLEMENTATION\n#endif // CH8_EXT_ROMS\n");

    free(stuff);
    fclose(ofile); 
    
    return 0;
}