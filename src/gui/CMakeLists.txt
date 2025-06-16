SET(SOURCES screens/ui_Index.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    images/ui_img_505560643.c
    images/ui_img_303578728.c
    images/ui_img_hatchback_1_png.c
    fonts/ui_font_Arial_Rounded_MT_Bold_24.c
    fonts/ui_font_Kanit20.c
    fonts/ui_font_Kanit40.c)

add_library(ui ${SOURCES})
