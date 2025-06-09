/**
  ******************************************************************************
  * @file   main.c
  * @author Sifli software development team
  ******************************************************************************
*/
/**
 * @attention
 * Copyright (c) 2024 - 2025,  Sifli Technology
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Sifli integrated circuit
 *    in a product or a software update for such product, must reproduce the above
 *    copyright notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of Sifli nor the names of its contributors may be used to endorse
 *    or promote products derived from this software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Sifli integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY SIFLI TECHNOLOGY "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SIFLI TECHNOLOGY OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */



#include "rtthread.h"
#include "bf0_hal.h"
#include "drv_io.h"
#include "stdio.h"
#include "string.h"
#include "xiaozhi2.h"
#include "button.h"
#include "./iot/iot_c_api.h"
#ifdef BSP_USING_PM
    #include "gui_app_pm.h"
#endif // BSP_USING_PM
 extern void xiaozhi_ui_update_ble(char *string);
 extern void xiaozhi_ui_update_emoji(char *string);
 extern void xiaozhi_ui_chat_status(char *string);
 extern void xiaozhi_ui_chat_output(char *string);
 
 extern void xiaozhi_ui_task(void *args);
 extern void xiaozhi(int argc, char **argv);
 extern void xiaozhi2(int argc, char **argv);
 extern void reconnect_websocket();
 extern xiaozhi_ws_t g_xz_ws;   
 /* Common functions for RT-Thread based platform -----------------------------------------------*/
 /**
   * @brief  Initialize board default configuration.
   * @param  None
   * @retval None
   */
 void HAL_MspInit(void)
 {
     //__asm("B .");        /*For debugging purpose*/
     BSP_IO_Init();
 }
 /* User code start from here --------------------------------------------------------*/
 #include "bts2_app_inc.h"
 #include "ble_connection_manager.h"
 #include "bt_connection_manager.h"
 #include "bt_env.h"
 #include "ulog.h"
 
 #define BT_APP_READY 0
 #define BT_APP_CONNECT_PAN  1
 #define BT_APP_CONNECT_PAN_SUCCESS 2
 #define WEBSOCKET_RECONNECT 3
 #define PAN_RECONNECT 4
 #define KEEP_FIRST_PAN_RECONNECT 5
 #define PAN_TIMER_MS        3000
 



 bt_app_t g_bt_app_env;
 rt_mailbox_t g_bt_app_mb;
BOOL g_pan_connected = FALSE;
BOOL first_pan_connected = FALSE;
int first_reconnect_attempts = 0;
 void bt_app_connect_pan_timeout_handle(void *parameter)
 {
     LOG_I("bt_app_connect_pan_timeout_handle %x, %d", g_bt_app_mb, g_bt_app_env.bt_connected);
     if ((g_bt_app_mb != NULL) && (g_bt_app_env.bt_connected))
         rt_mb_send(g_bt_app_mb, BT_APP_CONNECT_PAN);
     return;
 }
 
 
 #if defined(BSP_USING_SPI_NAND) && defined(RT_USING_DFS)
 #include "dfs_file.h"
 #include "dfs_posix.h"
 #include "drv_flash.h"
 #define NAND_MTD_NAME    "root"
 int mnt_init(void)
 {
     //TODO: how to get base address
     register_nand_device(FS_REGION_START_ADDR & (0xFC000000), FS_REGION_START_ADDR - (FS_REGION_START_ADDR & (0xFC000000)), FS_REGION_SIZE, NAND_MTD_NAME);
     if (dfs_mount(NAND_MTD_NAME, "/", "elm", 0, 0) == 0) // fs exist
     {
         rt_kprintf("mount fs on flash to root success\n");
     }
     else
     {
         // auto mkfs, remove it if you want to mkfs manual
         rt_kprintf("mount fs on flash to root fail\n");
         if (dfs_mkfs("elm", NAND_MTD_NAME) == 0)
         {
             rt_kprintf("make elm fs on flash sucess, mount again\n");
             if (dfs_mount(NAND_MTD_NAME, "/", "elm", 0, 0) == 0)
                 rt_kprintf("mount fs on flash success\n");
             else
                 rt_kprintf("mount to fs on flash fail\n");
         }
         else
             rt_kprintf("dfs_mkfs elm flash fail\n");
     }
     return RT_EOK;
 }
 INIT_ENV_EXPORT(mnt_init);
 #endif
 
static void xz_button_event_handler2(int32_t pin, button_action_t action)
{
    rt_kprintf("button(%d) %d:", pin, action);
    
    static button_action_t last_action=BUTTON_RELEASED;
    if(last_action==action)
    {
        return;
    }
    last_action=action;

    if (action == BUTTON_PRESSED)
    {
#ifdef BSP_USING_PM
        gui_pm_fsm(GUI_PM_ACTION_WAKEUP);
#endif // BSP_USING_PM
        rt_kprintf("pressed\r\n");
        rt_mb_send(g_bt_app_mb, PAN_RECONNECT);//连接pan,如果连接成功就会触发BT_NOTIFY_PAN_PROFILE_CONNECTED事件

    }
    else if (action == BUTTON_RELEASED)
    {
#ifdef BSP_USING_PM
        gui_pm_fsm(GUI_PM_ACTION_WAKEUP);
#endif // BSP_USING_PM
        rt_kprintf("released\r\n");
        
    }
}

static void xz_button_init2(void)
{
    static int initialized = 0;

    if (initialized == 0)
    {
        button_cfg_t cfg;
        cfg.pin = BSP_KEY1_PIN;

        cfg.active_state = BSP_KEY1_ACTIVE_HIGH;
        cfg.mode = PIN_MODE_INPUT;
        cfg.button_handler = xz_button_event_handler2;
        int32_t id = button_init(&cfg);
        RT_ASSERT(id >= 0);
        RT_ASSERT(SF_EOK == button_enable(id));
        initialized = 1;
    }
}
void keep_First_pan_connection()
{
    static int first_reconnect_attempts = 0;
    const int max_reconnect_attempts = 3;
    const int reconnect_interval_ms = 4000; // 4秒

    LOG_I("Keep_first_Attempting to reconnect PAN, attempt %d", first_reconnect_attempts + 1);
    xiaozhi_ui_chat_status("connecting pan...");
    xiaozhi_ui_chat_output("正在重连PAN...");
    if(first_reconnect_attempts < max_reconnect_attempts)
    {
        if (g_bt_app_env.pan_connect_timer)
        {
            rt_timer_stop(g_bt_app_env.pan_connect_timer);
        }
        bt_interface_conn_ext((char *)&g_bt_app_env.bd_addr, BT_PROFILE_HID);
    }
    else{
        LOG_W("Failed to keep_first_reconnect PAN after %d attempts", max_reconnect_attempts);
        xiaozhi_ui_chat_status("无法连接PAN");
        xiaozhi_ui_chat_output("请确保设备开启了共享网络,重新发起连接");
        xiaozhi_ui_update_emoji("thinking");
        
        return;
    }
    first_reconnect_attempts++;
    rt_thread_mdelay(reconnect_interval_ms);
    // 检查是否连接成功
    if (g_pan_connected)
    {
        LOG_I("PAN reconnected successfully%d\n",g_pan_connected);
        return;
    }

   
}


 void pan_reconnect(void)
 {
    static int reconnect_attempts = 0;
    const int max_reconnect_attempts = 3;
    const int reconnect_interval_ms = 4000; // 4秒
    while(reconnect_attempts < max_reconnect_attempts)
    {
        LOG_I("Attempting to reconnect PAN, attempt %d", reconnect_attempts + 1);
        xiaozhi_ui_chat_status("connecting pan...");
        xiaozhi_ui_chat_output("正在重连PAN...");
        if (g_bt_app_env.pan_connect_timer)
        {
            rt_timer_stop(g_bt_app_env.pan_connect_timer);
        }
        bt_interface_conn_ext((char *)&g_bt_app_env.bd_addr, BT_PROFILE_HID);
        reconnect_attempts++;
        rt_thread_mdelay(reconnect_interval_ms);
        // 检查是否连接成功
        if (g_pan_connected)
        {
            LOG_I("PAN reconnected successfully%d\n",g_pan_connected);
            reconnect_attempts = 0; 
            return;
        }
    }
   
        LOG_W("Failed to reconnect PAN after %d attempts", max_reconnect_attempts);
        xiaozhi_ui_chat_status("无法连接PAN");
        xiaozhi_ui_chat_output("请确保设备开启了共享网络,重新发起连接");
        xiaozhi_ui_update_emoji("thinking");
        reconnect_attempts = 0; 
    
}
 static int bt_app_interface_event_handle(uint16_t type, uint16_t event_id, uint8_t *data, uint16_t data_len)
 {
     if (type == BT_NOTIFY_COMMON)
     {
         int pan_conn = 0;
 
         switch (event_id)
         {
         case BT_NOTIFY_COMMON_BT_STACK_READY:
         {
             rt_mb_send(g_bt_app_mb, BT_APP_READY);
         }
             break;
         case BT_NOTIFY_COMMON_ACL_DISCONNECTED:
             {
                 bt_notify_device_base_info_t *info = (bt_notify_device_base_info_t *)data;
                 LOG_I("disconnected(0x%.2x:%.2x:%.2x:%.2x:%.2x:%.2x) res %d", info->mac.addr[5],
                       info->mac.addr[4], info->mac.addr[3], info->mac.addr[2],
                       info->mac.addr[1], info->mac.addr[0], info->res);
                 g_bt_app_env.bt_connected = FALSE;
  
                //  memset(&g_bt_app_env.bd_addr, 0xFF, sizeof(g_bt_app_env.bd_addr));
 
                 if (g_bt_app_env.pan_connect_timer)
                     rt_timer_stop(g_bt_app_env.pan_connect_timer);
             }
             break;
         case BT_NOTIFY_COMMON_ENCRYPTION:
             {
                 bt_notify_device_mac_t *mac = (bt_notify_device_mac_t *)data;
                 LOG_I("Encryption competed");
                 g_bt_app_env.bd_addr = *mac;
                 pan_conn = 1;
             }
             break;
         case BT_NOTIFY_COMMON_PAIR_IND:
             {
                 bt_notify_device_base_info_t *info = (bt_notify_device_base_info_t *)data;
                 LOG_I("Pairing completed %d", info->res);
                 if (info->res == BTS2_SUCC)
                 {
                     g_bt_app_env.bd_addr = info->mac;
                     pan_conn = 1;
                 }
             }
             break;
             case BT_NOTIFY_COMMON_KEY_MISSING:
             {
                 bt_notify_device_base_info_t *info = (bt_notify_device_base_info_t *)data;
                 LOG_I("Key missing %d", info->res);
                 memset(&g_bt_app_env.bd_addr, 0xFF, sizeof(g_bt_app_env.bd_addr));
                 bt_cm_delete_bonded_devs_and_linkkey(info->mac.addr);
       
             }
             break;
         default:
             break;
         }
 
         if (pan_conn)
         {
             LOG_I("bd addr 0x%.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n", g_bt_app_env.bd_addr.addr[5],
                   g_bt_app_env.bd_addr.addr[4], g_bt_app_env.bd_addr.addr[3],
                   g_bt_app_env.bd_addr.addr[2], g_bt_app_env.bd_addr.addr[1],
                   g_bt_app_env.bd_addr.addr[0]);
             g_bt_app_env.bt_connected = TRUE;
             // Trigger PAN connection after PAN_TIMER_MS period to avoid SDP confliction.
             if (!g_bt_app_env.pan_connect_timer)
                 g_bt_app_env.pan_connect_timer = rt_timer_create("connect_pan", bt_app_connect_pan_timeout_handle, (void *)&g_bt_app_env,
                                                  rt_tick_from_millisecond(PAN_TIMER_MS), RT_TIMER_FLAG_SOFT_TIMER);
             else
                 rt_timer_stop(g_bt_app_env.pan_connect_timer);
             rt_timer_start(g_bt_app_env.pan_connect_timer);
         }
     }
     else if (type == BT_NOTIFY_PAN)
     {
         switch (event_id)
         {
         case BT_NOTIFY_PAN_PROFILE_CONNECTED:
             {
                 xiaozhi_ui_chat_output("PAN连接成功");
                 xiaozhi_ui_update_ble("open");
                 LOG_I("pan connect successed \n");
                 if ((g_bt_app_env.pan_connect_timer))
                 {
                     rt_timer_stop(g_bt_app_env.pan_connect_timer);
                 }
                 rt_mb_send(g_bt_app_mb, BT_APP_CONNECT_PAN_SUCCESS);
                 g_pan_connected = TRUE;  // 更新PAN连接状态
 
             }
             break;
         case BT_NOTIFY_PAN_PROFILE_DISCONNECTED:
             {
                 xiaozhi_ui_chat_status("PAN断开...");
                 xiaozhi_ui_chat_output("PAN断开,尝试唤醒键重新连接");
                 xiaozhi_ui_update_ble("close");
                 LOG_I("pan disconnect with remote device\n");
                 g_pan_connected = FALSE;  // 更新PAN连接状态
                 if(first_pan_connected == FALSE)//Check if the pan has ever been connected
                 {                       
                    rt_mb_send(g_bt_app_mb, KEEP_FIRST_PAN_RECONNECT);  
                 }


     
             }
             break;
         default:
             break;
         }
     }
     else if(type == BT_NOTIFY_HID)
    {
        switch (event_id)
        {
        case BT_NOTIFY_HID_PROFILE_CONNECTED:
        {
            LOG_I("HID connected\n");
            if (!g_pan_connected)
            {
                if (g_bt_app_env.pan_connect_timer)
                {
                    rt_timer_stop(g_bt_app_env.pan_connect_timer);
                }
                bt_interface_conn_ext((char *)&g_bt_app_env.bd_addr, BT_PROFILE_PAN);
            }
        }
        break;
        case BT_NOTIFY_HID_PROFILE_DISCONNECTED:
        {
            LOG_I("HID disconnected\n");
        }
        break;
        default:
            break;
        }
    }
 
 
     return 0;
 }
 
 uint32_t bt_get_class_of_device()
 {
    return (uint32_t)BT_SRVCLS_NETWORK | BT_DEVCLS_PERIPHERAL | BT_PERIPHERAL_REMCONTROL;
 }

 
 
 /**
   * @brief  Main program
   * @param  None
   * @retval 0 if success, otherwise failure number
   */
 #ifdef BT_DEVICE_NAME
     static const char *local_name = BT_DEVICE_NAME;
 #else
     static const char *local_name = "sifli-pan";
 #endif


 

 
 int main(void)
 {
    xz_button_init2();
    audio_server_set_private_volume(AUDIO_TYPE_LOCAL_MUSIC, 6);//设置音量
    iot_initialize();//Initialize iot
#ifdef BSP_USING_BOARD_SF32LB52_LCHSPI_ULP
    unsigned int *addr2 = (unsigned int *)0x50003088;   //21
    *addr2 = 0x00000200;
    unsigned int *addr = (unsigned int *)0x500030B0;    //31
    *addr = 0x00000200;

    //senser
    HAL_PIN_Set(PAD_PA30, GPIO_A30, PIN_PULLDOWN, 1);
    BSP_GPIO_Set(30, 0, 1);
    HAL_PIN_Set(PAD_PA39, GPIO_A39, PIN_PULLDOWN, 1);
    HAL_PIN_Set(PAD_PA40, GPIO_A40, PIN_PULLDOWN, 1);
    
    //rt_pm_request(PM_SLEEP_MODE_IDLE);
#endif
    //Create  xiaozhi UI  
    rt_thread_t tid = rt_thread_create("xz_ui", xiaozhi_ui_task, NULL, 4096, 30, 10);
    rt_thread_startup(tid);

    
     //Connect BT PAN
     g_bt_app_mb = rt_mb_create("bt_app", 8, RT_IPC_FLAG_FIFO);
 #ifdef BSP_BT_CONNECTION_MANAGER
     bt_cm_set_profile_target(BT_CM_HID, BT_SLAVE_ROLE, 1);
 #endif // BSP_BT_CONNECTION_MANAGER
 
     bt_interface_register_bt_event_notify_callback(bt_app_interface_event_handle);
 
     sifli_ble_enable();
     while (1)
     {
       
         uint32_t value;
 
         // handle pan connect event
         rt_mb_recv(g_bt_app_mb, (rt_uint32_t *)&value, RT_WAITING_FOREVER);
 
         if (value == BT_APP_CONNECT_PAN)
         {
             if (g_bt_app_env.bt_connected)
             {
                 bt_interface_conn_ext((char *)&g_bt_app_env.bd_addr, BT_PROFILE_PAN);
             }
 
         }
         else if (value == BT_APP_READY)
         {
             LOG_I("BT/BLE stack and profile ready");
 
 
             // Update Bluetooth name
             bt_interface_set_local_name(strlen(local_name), (void *)local_name);
         }
         else if (value == BT_APP_CONNECT_PAN_SUCCESS)
         {
             rt_kputs("BT_APP_CONNECT_PAN_SUCCESS\r\n");
             xiaozhi_ui_chat_output("PAN连接成功,开始连接小智...");
             xiaozhi_ui_update_ble("open");
             xiaozhi_ui_chat_status("正在连接小智...");
             xiaozhi_ui_update_emoji("neutral");
 
             rt_thread_mdelay(2000);
             xiaozhi2(0, NULL); //Start Xiaozhi
         }
         else if (value == KEEP_FIRST_PAN_RECONNECT)
         {
            keep_First_pan_connection();//Ensure that the first pan connection is successful
         }
         
         else if (value == PAN_RECONNECT)//Press to wake up pan reconnection
         {
            rt_kprintf("PAN_RECONNECT\r\n");
            if (g_pan_connected) //If the pan is already connected, there is no need to reconnect the pan
            {
                if (g_xz_ws.is_connected)//Check whether the websocket is connected
                {
                    rt_kprintf("g_xz_ws.is_connected = %d\n", g_xz_ws.is_connected);
                    rt_kprintf("ws_connected\n");
                }
                else
                {
                
                    rt_kputs("PAN_CONNECTED\r\n");
    
                    reconnect_websocket();//重连websocket
                }
            }
            else//pan is not connected. You need to connect pan and then press the button to reconnect websocket
            {
				
                pan_reconnect();//Triple reconnection of pan

            }            
         }
         else{
             rt_kputs("WEBSOCKET_DISCONNECT\r\n");
             xiaozhi_ui_chat_output("请重启");
         }
 
     }
     return 0;
 }
 
 
 static void pan_cmd(int argc, char **argv)
 {
     if (strcmp(argv[1], "del_bond") == 0)
     {
 #ifdef BSP_BT_CONNECTION_MANAGER
         bt_cm_delete_bonded_devs();
         LOG_D("Delete bond");
 #endif // BSP_BT_CONNECTION_MANAGER
     }
     // only valid after connection setup but phone didn't enable pernal hop
     else if (strcmp(argv[1], "conn_pan") == 0)
         bt_app_connect_pan_timeout_handle(NULL);
 }
 MSH_CMD_EXPORT(pan_cmd, Connect PAN to last paired device);
 
 
 