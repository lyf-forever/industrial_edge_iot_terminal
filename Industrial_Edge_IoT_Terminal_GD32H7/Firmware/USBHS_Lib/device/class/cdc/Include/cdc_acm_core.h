#ifndef CDC_ACM_CORE_H
#define CDC_ACM_CORE_H

#include "usbd_enum.h"
#include "usb_cdc.h"

#define USB_CDC_RX_LEN      USB_CDC_DATA_PACKET_SIZE          /*< CDC data packet size */
#define USER_USB_RXBUFF_SIZE   200     /* USB串口接收缓冲区最大字节数 */

extern uint8_t user_usb_rx_buffer[USER_USB_RXBUFF_SIZE];
extern uint8_t user_usb_usart_rx_buffer[USER_USB_RXBUFF_SIZE];
extern uint16_t user_usb_usart_rx_state; 

typedef struct {
    __ALIGN_BEGIN uint8_t data[USB_CDC_RX_LEN] __ALIGN_END;                  /*< CDC data transfer buff */
    __ALIGN_BEGIN uint8_t cmd[USB_CDC_CMD_PACKET_SIZE] __ALIGN_END;          /*< CDC command packet buff */

    uint8_t packet_sent;                                                     /*< CDC data packet start send flag */
    uint8_t packet_receive;                                                  /*< CDC data packet start receive flag */
    uint32_t receive_length;                                                 /*< CDC data receive length */

    acm_line line_coding;                                                    /*< CDC line coding structure */
} usb_cdc_handler;

extern usb_desc cdc_desc;
extern usb_class_core cdc_class;

/* function declarations */
/* check CDC ACM is ready for data transfer */
uint8_t cdc_acm_check_ready(usb_dev *udev);
/* send CDC ACM data */
void cdc_acm_data_send(usb_dev *udev);
/* receive CDC ACM data */
void cdc_acm_data_receive(usb_dev *udev);

/* user defined */
void usb_printf(const char* fmt, ...);
uint16_t user_usb_send(uint8_t *buf, uint16_t length);
void usb_cdc_send(const uint8_t *data, uint16_t len);
void usb_receive_handler(void);     // 在主循环中定期调用，检查并接收来自PC的数据


#endif /* CDC_ACM_CORE_H */
