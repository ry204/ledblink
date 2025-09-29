#include "main.h"
#include "i2c.h"
#include "tim.h"
#include "gpio.h"
#include <string.h>

/* Prototype do CubeMX sinh ra */
void SystemClock_Config(void);
void Error_Handler(void);

/* ---- LCD I2C PCF8574 ---- */
#define LCD_ADDR (0x27 << 1)  // Ð?a ch? I2C LCD (thu?ng 0x27 ho?c 0x3F)
extern I2C_HandleTypeDef hi2c1;

void LCD_Delay(uint32_t Delay) {
    HAL_Delay(Delay);
}

void lcd_send_cmd(char cmd) {
    char data_u, data_l;
    uint8_t data_t[4];
    data_u = (cmd & 0xF0);
    data_l = ((cmd << 4) & 0xF0);
    data_t[0] = data_u | 0x0C;
    data_t[1] = data_u | 0x08;
    data_t[2] = data_l | 0x0C;
    data_t[3] = data_l | 0x08;
    HAL_I2C_Master_Transmit(&hi2c1, LCD_ADDR, data_t, 4, 100);
}

void lcd_send_data(char data) {
    char data_u, data_l;
    uint8_t data_t[4];
    data_u = (data & 0xF0);
    data_l = ((data << 4) & 0xF0);
    data_t[0] = data_u | 0x0D;
    data_t[1] = data_u | 0x09;
    data_t[2] = data_l | 0x0D;
    data_t[3] = data_l | 0x09;
    HAL_I2C_Master_Transmit(&hi2c1, LCD_ADDR, data_t, 4, 100);
}

void lcd_init(void) {
    LCD_Delay(50);
    lcd_send_cmd(0x30);
    LCD_Delay(5);
    lcd_send_cmd(0x30);
    LCD_Delay(1);
    lcd_send_cmd(0x30);
    LCD_Delay(10);
    lcd_send_cmd(0x20); // 4-bit mode
    LCD_Delay(10);

    lcd_send_cmd(0x28); // 2 line, 5x8 font
    lcd_send_cmd(0x08); // display off
    lcd_send_cmd(0x01); // clear
    LCD_Delay(2);
    lcd_send_cmd(0x06); // entry mode
    lcd_send_cmd(0x0C); // display on, cursor off
}

void lcd_send_string(char *str) {
    while (*str) lcd_send_data(*str++);
}

void lcd_put_cur(int row, int col) {
    switch (row) {
        case 0: col |= 0x80; break;
        case 1: col |= 0xC0; break;
    }
    lcd_send_cmd(col);
}

/* ---- Chuy?n s? thành chu?i ---- */
void intToStr(int num, char *buf) {
    if (num < 0) {
        buf[0] = '-';
        num = -num;
        if (num < 10) {
            buf[1] = '0' + num;
            buf[2] = '\0';
        } else {
            buf[1] = '0' + num / 10;
            buf[2] = '0' + num % 10;
            buf[3] = '\0';
        }
    } else {
        if (num < 10) {
            buf[0] = '0' + num;
            buf[1] = '\0';
        } else {
            buf[0] = '0' + num / 10;
            buf[1] = '0' + num % 10;
            buf[2] = '\0';
        }
    }
}

/* ---- Bi?n toàn c?c ---- */
extern TIM_HandleTypeDef htim2;
int countdown = 10;
int tick_count = 0;   // d?m s? l?n ng?t 0.5s

/* ---- Hàm callback Timer ---- */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) {
        // Toggle LED PC13 m?i 0.5s
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

        // C?p nh?t LCD + countdown m?i 1s (sau 2 l?n ng?t)
        tick_count++;
        if (tick_count >= 2) {
            tick_count = 0;

            char buf[16];
            lcd_send_cmd(0x01); // clear
            lcd_put_cur(0,0);
            lcd_send_string("  Dem nguoc: ");
            intToStr(countdown, buf);
            lcd_send_string(buf);

            countdown--;
            if (countdown < 0) countdown = 10;
        }
    }
}

/* ---- MAIN ---- */
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_TIM2_Init();  // hàm này ? tim.c

    lcd_init();
    lcd_put_cur(0,0);
    lcd_send_string("STM32 LCD Test");

    // B?t d?u Timer 2 v?i ng?t
    HAL_TIM_Base_Start_IT(&htim2);

    while (1) {
        // vòng l?p chính r?nh
    }
}

/* ---- C?u hình Clock 72 MHz ---- */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;  // 36 MHz
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;  // 72 MHz

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
    Error_Handler();
  }
}

/* ---- Hàm x? lý l?i ---- */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
    // Blink LED báo l?i (nh?p nháy nhanh)
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    HAL_Delay(200);
  }
}
