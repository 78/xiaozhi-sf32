#include "rtthread.h"
#include "bf0_hal.h"
#include "drv_io.h"
#include "littlevgl2rtt.h"
#include "lv_tiny_ttf.h"
#include "string.h"


static struct rt_semaphore update_ui_sema;
/*Create style with the new font*/
static lv_style_t style;
extern const unsigned char droid_sans_fallback_font[];
extern const int droid_sans_fallback_font_size;

extern const lv_image_dsc_t color_neutral;
extern const lv_image_dsc_t color_happy;
extern const lv_image_dsc_t color_laughing;
extern const lv_image_dsc_t color_funny;
extern const lv_image_dsc_t color_sad;
extern const lv_image_dsc_t color_angry;
extern const lv_image_dsc_t color_crying;
extern const lv_image_dsc_t color_loving;
extern const lv_image_dsc_t color_embarrassed;
extern const lv_image_dsc_t color_surprised;
extern const lv_image_dsc_t color_shocked;
extern const lv_image_dsc_t color_thinking;
extern const lv_image_dsc_t color_winking;
extern const lv_image_dsc_t color_cool;
extern const lv_image_dsc_t color_relaxed;
extern const lv_image_dsc_t color_delicious;
extern const lv_image_dsc_t color_kissy;
extern const lv_image_dsc_t color_confident;
extern const lv_image_dsc_t color_sleepy;
extern const lv_image_dsc_t color_silly;
extern const lv_image_dsc_t color_confused;


extern const lv_image_dsc_t ble;//ble
extern const lv_image_dsc_t ble_close;

static lv_obj_t *global_label1;
static lv_obj_t *global_label2;

static lv_obj_t *global_img1;//emoji
static lv_obj_t *global_img2;
static lv_obj_t *global_img3;
static lv_obj_t *global_img4;
static lv_obj_t *global_img5;
static lv_obj_t *global_img6;
static lv_obj_t *global_img7;
static lv_obj_t *global_img8;
static lv_obj_t *global_img9;
static lv_obj_t *global_img10;
static lv_obj_t *global_img11; 
static lv_obj_t *global_img12;
static lv_obj_t *global_img13;
static lv_obj_t *global_img14;
static lv_obj_t *global_img15;
static lv_obj_t *global_img16;
static lv_obj_t *global_img17;
static lv_obj_t *global_img18;
static lv_obj_t *global_img19;
static lv_obj_t *global_img20;
static lv_obj_t *global_img21;


static lv_obj_t *global_img_ble;
static lv_obj_t *global_img_ble_close;


rt_err_t xiaozhi_ui_obj_init(void)
{
    global_label1 = lv_label_create(lv_screen_active());//top text

    lv_label_set_long_mode(global_label1, LV_LABEL_LONG_SCROLL_CIRCULAR); 
    lv_obj_add_style(global_label1, &style, 0);
    lv_obj_set_width(global_label1, 150);
    lv_obj_set_style_text_align(global_label1,LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(global_label1, LV_ALIGN_TOP_MID, 0, 0);
    

    global_label2 = lv_label_create(lv_screen_active());//output text

    lv_label_set_long_mode(global_label2, LV_LABEL_LONG_WRAP);  /*Break the long lines*/
    lv_obj_add_style(global_label2, &style, 0);
    lv_obj_set_width(global_label2, LV_HOR_RES_MAX);
    lv_obj_set_style_text_align(global_label2,LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(global_label2, LV_ALIGN_BOTTOM_MID, 0, -55);


    LV_IMAGE_DECLARE(color_neutral);
    LV_IMAGE_DECLARE(color_happy);
    LV_IMAGE_DECLARE(color_laughing);
    LV_IMAGE_DECLARE(color_funny);
    LV_IMAGE_DECLARE(color_sad);
    LV_IMAGE_DECLARE(color_angry);
    LV_IMAGE_DECLARE(color_crying);
    LV_IMAGE_DECLARE(color_loving);
    LV_IMAGE_DECLARE(color_embarrassed);
    LV_IMAGE_DECLARE(color_surprised);
    LV_IMAGE_DECLARE(color_shocked);
    LV_IMAGE_DECLARE(color_thinking);
    LV_IMAGE_DECLARE(color_winking);
    LV_IMAGE_DECLARE(color_cool);
    LV_IMAGE_DECLARE(color_relaxed);
    LV_IMAGE_DECLARE(color_delicious);
    LV_IMAGE_DECLARE(color_kissy);
    LV_IMAGE_DECLARE(color_confident);
    LV_IMAGE_DECLARE(color_sleepy);
    LV_IMAGE_DECLARE(color_silly);
    LV_IMAGE_DECLARE(color_confused);

    LV_IMAGE_DECLARE(ble);
    LV_IMAGE_DECLARE(ble_close);

    global_img1 = lv_img_create(lv_screen_active());
    lv_img_set_src(global_img1, &color_neutral);
    lv_obj_align(global_img1, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(global_img1, LV_OBJ_FLAG_HIDDEN);//hidden

    global_img2 = lv_img_create(lv_screen_active());
    lv_img_set_src(global_img2, &color_happy);
    lv_obj_align(global_img2, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(global_img2, LV_OBJ_FLAG_HIDDEN);//hidden

    global_img3 = lv_img_create(lv_screen_active());
    lv_img_set_src(global_img3, &color_laughing);
    lv_obj_align(global_img3, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(global_img3, LV_OBJ_FLAG_HIDDEN);//hidden

    global_img4 = lv_img_create(lv_screen_active());
    lv_img_set_src(global_img4, &color_funny);
    lv_obj_align(global_img4, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(global_img4, LV_OBJ_FLAG_HIDDEN);//hidden

    global_img5 = lv_img_create(lv_screen_active());
    lv_img_set_src(global_img5, &color_sad);
    lv_obj_align(global_img5, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(global_img5, LV_OBJ_FLAG_HIDDEN);//hidden

    global_img6 = lv_img_create(lv_screen_active());
    lv_img_set_src(global_img6, &color_angry);
    lv_obj_align(global_img6, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(global_img6, LV_OBJ_FLAG_HIDDEN);//hidden

    global_img7 = lv_img_create(lv_screen_active());
    lv_img_set_src(global_img7, &color_crying);
    lv_obj_align(global_img7, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(global_img7, LV_OBJ_FLAG_HIDDEN);//hidden

    global_img8 = lv_img_create(lv_screen_active());
    lv_img_set_src(global_img8, &color_loving);
    lv_obj_align(global_img8, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(global_img8, LV_OBJ_FLAG_HIDDEN);//hidden

    global_img9 = lv_img_create(lv_screen_active());
    lv_img_set_src(global_img9, &color_embarrassed);
    lv_obj_align(global_img9, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(global_img9, LV_OBJ_FLAG_HIDDEN);//hidden

    global_img10 = lv_img_create(lv_screen_active());
    lv_img_set_src(global_img10, &color_surprised);
    lv_obj_align(global_img10, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(global_img10, LV_OBJ_FLAG_HIDDEN);//hidden

    global_img11 = lv_img_create(lv_screen_active());
    lv_img_set_src(global_img11, &color_shocked);
    lv_obj_align(global_img11, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(global_img11, LV_OBJ_FLAG_HIDDEN);//hidden

    global_img12 = lv_img_create(lv_screen_active());
    lv_img_set_src(global_img12, &color_thinking);
    lv_obj_align(global_img12, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(global_img12, LV_OBJ_FLAG_HIDDEN);//hidden

    global_img13 = lv_img_create(lv_screen_active());
    lv_img_set_src(global_img13, &color_winking);
    lv_obj_align(global_img13, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(global_img13, LV_OBJ_FLAG_HIDDEN);//hidden

    global_img14 = lv_img_create(lv_screen_active());
    lv_img_set_src(global_img14, &color_cool);
    lv_obj_align(global_img14, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(global_img14, LV_OBJ_FLAG_HIDDEN);//hidden

    global_img15 = lv_img_create(lv_screen_active());
    lv_img_set_src(global_img15, &color_relaxed);
    lv_obj_align(global_img15, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(global_img15, LV_OBJ_FLAG_HIDDEN);//hidden

    global_img16 = lv_img_create(lv_screen_active());
    lv_img_set_src(global_img16, &color_delicious);
    lv_obj_align(global_img16, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(global_img16, LV_OBJ_FLAG_HIDDEN);//hidden

    global_img17 = lv_img_create(lv_screen_active());
    lv_img_set_src(global_img17, &color_kissy);
    lv_obj_align(global_img17, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(global_img17, LV_OBJ_FLAG_HIDDEN);//hidden

    global_img18 = lv_img_create(lv_screen_active());
    lv_img_set_src(global_img18, &color_confident);
    lv_obj_align(global_img18, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(global_img18, LV_OBJ_FLAG_HIDDEN);//hidden

    global_img19 = lv_img_create(lv_screen_active());
    lv_img_set_src(global_img19, &color_sleepy);
    lv_obj_align(global_img19, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(global_img19, LV_OBJ_FLAG_HIDDEN);//hidden

    global_img20 = lv_img_create(lv_screen_active());
    lv_img_set_src(global_img20, &color_silly);
    lv_obj_align(global_img20, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(global_img20, LV_OBJ_FLAG_HIDDEN);//hidden

    global_img21 = lv_img_create(lv_screen_active());
    lv_img_set_src(global_img21, &color_confused);
    lv_obj_align(global_img21, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(global_img21, LV_OBJ_FLAG_HIDDEN);//hidden

    global_img_ble = lv_img_create(lv_screen_active());
    lv_img_set_src(global_img_ble, &ble);
    lv_obj_align(global_img_ble, LV_ALIGN_TOP_LEFT, 30, 0);
    lv_obj_add_flag(global_img_ble, LV_OBJ_FLAG_HIDDEN);//hidden

    global_img_ble_close = lv_img_create(lv_screen_active());
    lv_img_set_src(global_img_ble_close, &ble_close);
    lv_obj_align(global_img_ble_close, LV_ALIGN_TOP_LEFT, 30, 0);
    lv_obj_add_flag(global_img_ble_close, LV_OBJ_FLAG_HIDDEN);//hidden

    return RT_EOK;
}

void xiaozhi_ui_chat_status(char *string)//top text
{
    rt_sem_take(&update_ui_sema, RT_WAITING_FOREVER);

    if (string)
    {
        lv_label_set_text(global_label1, string);
    }

    rt_sem_release(&update_ui_sema);
}

void xiaozhi_ui_chat_output(char *string)
{
    rt_sem_take(&update_ui_sema, RT_WAITING_FOREVER);

    if (string)
    {
        lv_label_set_text(global_label2, string);
    }

    rt_sem_release(&update_ui_sema);
}


void xiaozhi_ui_update_emoji(char *string)//emoji
{
    rt_sem_take(&update_ui_sema, RT_WAITING_FOREVER);

    lv_obj_add_flag(global_img1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(global_img2, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(global_img3, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(global_img4, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(global_img5, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(global_img6, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(global_img7, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(global_img8, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(global_img9, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(global_img10, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(global_img11, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(global_img12, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(global_img13, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(global_img14, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(global_img15, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(global_img16, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(global_img17, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(global_img18, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(global_img19, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(global_img20, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(global_img21, LV_OBJ_FLAG_HIDDEN);

    if(string)
    {
        if(strcmp(string,"neutral") == 0)
        {
            lv_obj_clear_flag(global_img1, LV_OBJ_FLAG_HIDDEN);
        }
        else if(strcmp(string,"happy") == 0)
        {
            lv_obj_clear_flag(global_img2, LV_OBJ_FLAG_HIDDEN);
        }
        else if(strcmp(string,"laughing") == 0)
        {
            lv_obj_clear_flag(global_img3, LV_OBJ_FLAG_HIDDEN);
        }
        else if(strcmp(string,"funny") == 0)
        {
            lv_obj_clear_flag(global_img4, LV_OBJ_FLAG_HIDDEN);
        }
        else if(strcmp(string,"sad") == 0)
        {
            lv_obj_clear_flag(global_img5, LV_OBJ_FLAG_HIDDEN);
        }
        else if(strcmp(string,"angry") == 0)
        {
            lv_obj_clear_flag(global_img6, LV_OBJ_FLAG_HIDDEN);
        }
        else if(strcmp(string,"crying") == 0)
        {
            lv_obj_clear_flag(global_img7, LV_OBJ_FLAG_HIDDEN);
        }
        else if(strcmp(string,"loving") == 0)
        {
            lv_obj_clear_flag(global_img8, LV_OBJ_FLAG_HIDDEN);
        }
        else if(strcmp(string,"embarrassed") == 0)
        {
            lv_obj_clear_flag(global_img9, LV_OBJ_FLAG_HIDDEN);
        }
        else if(strcmp(string,"surprised") == 0)
        {
            lv_obj_clear_flag(global_img10, LV_OBJ_FLAG_HIDDEN);
        }
        else if(strcmp(string,"shocked") == 0)
        {
            lv_obj_clear_flag(global_img11, LV_OBJ_FLAG_HIDDEN);
        }
        else if(strcmp(string,"thinking") == 0)
        {
            lv_obj_clear_flag(global_img12, LV_OBJ_FLAG_HIDDEN);
        }
        else if(strcmp(string,"winking") == 0)
        {
            lv_obj_clear_flag(global_img13, LV_OBJ_FLAG_HIDDEN);
        }
        else if(strcmp(string,"cool") == 0)
        {
            lv_obj_clear_flag(global_img14, LV_OBJ_FLAG_HIDDEN);
        }
        else if(strcmp(string,"relaxed") == 0)
        {
            lv_obj_clear_flag(global_img15, LV_OBJ_FLAG_HIDDEN);
        }
        else if(strcmp(string,"delicious") == 0)
        {
            lv_obj_clear_flag(global_img16, LV_OBJ_FLAG_HIDDEN);
        }
        else if(strcmp(string,"kissy") == 0)
        {
            lv_obj_clear_flag(global_img17, LV_OBJ_FLAG_HIDDEN);
        }
        else if(strcmp(string,"confident") == 0)
        {
            lv_obj_clear_flag(global_img18, LV_OBJ_FLAG_HIDDEN);
        }
        else if(strcmp(string,"sleepy") == 0)
        {
            lv_obj_clear_flag(global_img19, LV_OBJ_FLAG_HIDDEN);
        }
        else if(strcmp(string,"silly") == 0)
        {
            lv_obj_clear_flag(global_img20, LV_OBJ_FLAG_HIDDEN);
        }
        else if(strcmp(string,"confused") == 0)
        {
            lv_obj_clear_flag(global_img21, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_clear_flag(global_img1, LV_OBJ_FLAG_HIDDEN);//common emoji is neutral
        }
    }

    rt_sem_release(&update_ui_sema);
}

void xiaozhi_ui_update_ble(char *string)//ble
{
    rt_sem_take(&update_ui_sema, RT_WAITING_FOREVER);

    lv_obj_add_flag(global_img_ble, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(global_img_ble_close, LV_OBJ_FLAG_HIDDEN);

    if(string)
    {
        if(strcmp(string,"open") == 0)
        {
            lv_obj_clear_flag(global_img_ble, LV_OBJ_FLAG_HIDDEN);
        }
        else if(strcmp(string,"close") == 0)
        {
            lv_obj_clear_flag(global_img_ble_close, LV_OBJ_FLAG_HIDDEN);
        }
    }

    rt_sem_release(&update_ui_sema);
}




void xiaozhi_ui_task(void *args)
{
    rt_err_t ret = RT_EOK;
    rt_uint32_t ms;

    rt_sem_init(&update_ui_sema, "update_ui", 1, RT_IPC_FLAG_FIFO);

    /* init littlevGL */
    ret = littlevgl2rtt_init("lcd");
    if (ret != RT_EOK)
    {
        return;
    }



    lv_style_init(&style);
    lv_font_t *font = lv_tiny_ttf_create_data(droid_sans_fallback_font, droid_sans_fallback_font_size, 30);
    lv_style_set_text_font(&style, font);
    lv_style_set_text_align(&style, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);//SET BG COLOR

    ret = xiaozhi_ui_obj_init();
    if (ret != RT_EOK)
    {
        return;
    }

    xiaozhi_ui_update_ble("close");
    xiaozhi_ui_chat_status("connecting...");
    xiaozhi_ui_chat_output("Ready...");
    xiaozhi_ui_update_emoji("neutral");

    while (1)
    {
        if (RT_EOK == rt_sem_trytake(&update_ui_sema))
        {
            ms = lv_task_handler();
            rt_thread_mdelay(ms);
            rt_sem_release(&update_ui_sema);
        }

    }

}





