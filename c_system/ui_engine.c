#include "ui_engine.h"

#include <stdio.h>

void ui_show_menu(void)
{
    printf("\n1. Generate New Cart\n2. Manage Inventory\n3. Restock Products\n"
           "4. Manage Coupons\n5. View Previous Bills\n"
           "0. Exit\nChoose: ");
}

void ui_clear_input(void)
{
    while (getchar() != '\n') {}
}
