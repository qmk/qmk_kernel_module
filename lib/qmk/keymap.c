#include <qmk/types.h>
#include <qmk/utils.h>
#include <qmk/keymap.h>

qmk_keycode keycode_from_keymap(struct qmk_interface *qi, uint8_t layer, 
                                uint8_t row, uint8_t col)
{
    return qi->keymap[  
        layer << get_count_order(qi->rows << get_count_order(qi->cols)) |
        row << get_count_order(qi->cols) |
        col];
}


