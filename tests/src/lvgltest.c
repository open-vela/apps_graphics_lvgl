
#if LV_BUILD_TEST

#include <stdio.h>
#include <string.h>
#include "../build_test_nuttx/test_entries.h"

static void usage(void)
{
    printf("Usage: lvgltest <test_name|all|help>\n");
    printf("The test_name can refer to lvgl/tests/build_test_nuttx/test_entries_list.h.\n");
}


int main(int argc, char * argv[])
{
    char * test_name = NULL;
    if(argc == 1) {
        test_name = "all";
    }
    else if(argc == 2) {
        if(strcmp(argv[1], "help") == 0) {
            usage();
            return 0;
        }
        test_name = argv[1];
    }
    else {
        usage();
        return 0;
    }

#include "../build_test_nuttx/test_entries_list.h"
    return 0;
}

#endif
