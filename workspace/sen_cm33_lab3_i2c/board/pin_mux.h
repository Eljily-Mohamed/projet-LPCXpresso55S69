/***********************************************************************************************************************
 * This file was generated by the MCUXpresso Config Tools. Any manual edits made to this file
 * will be overwritten if the respective MCUXpresso Config Tools is used to update this file.
 **********************************************************************************************************************/

#ifndef _PIN_MUX_H_
#define _PIN_MUX_H_

/*!
 * @addtogroup pin_mux
 * @{
 */

/***********************************************************************************************************************
 * API
 **********************************************************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @brief Calls initialization functions.
 *
 */
void BOARD_InitBootPins(void);

/*!
 * @brief Enables digital function */
#define IOCON_PIO_DIGITAL_EN 0x0100u
/*!
 * @brief Selects pin function 1 */
#define IOCON_PIO_FUNC1 0x01u
/*!
 * @brief Input function is not inverted */
#define IOCON_PIO_INV_DI 0x00u
/*!
 * @brief No addition pin function */
#define IOCON_PIO_MODE_INACT 0x00u
/*!
 * @brief Open drain is disabled */
#define IOCON_PIO_OPENDRAIN_DI 0x00u
/*!
 * @brief Standard mode, output slew rate control is enabled */
#define IOCON_PIO_SLEW_STANDARD 0x00u
/*!
 * @brief Select Digital mode.: Enable Digital mode. Digital input is enabled. */
#define PIO1_19_DIGIMODE_DIGITAL 0x01u
/*!
 * @brief Selects pin function.: Alternative connection 0. */
#define PIO1_19_FUNC_ALT0 0x00u
/*!
 * @brief Select Digital mode.: Enable Digital mode. Digital input is enabled. */
#define PIO1_20_DIGIMODE_DIGITAL 0x01u
/*!
 * @brief Selects pin function.: Alternative connection 5. */
#define PIO1_20_FUNC_ALT5 0x05u
/*!
 * @brief Select Digital mode.: Enable Digital mode. Digital input is enabled. */
#define PIO1_21_DIGIMODE_DIGITAL 0x01u
/*!
 * @brief Selects pin function.: Alternative connection 5. */
#define PIO1_21_FUNC_ALT5 0x05u

/*! @name PIO1_20 (number 4), FC4_SCL
  @{ */
#define BOARD_INITPINS_FC4_SCL_PORT 1U                   /*!<@brief PORT peripheral base pointer */
#define BOARD_INITPINS_FC4_SCL_PIN 20U                   /*!<@brief PORT pin number */
#define BOARD_INITPINS_FC4_SCL_PIN_MASK (1U << 20U)      /*!<@brief PORT pin mask */
                                                         /* @} */

/*! @name PIO1_21 (number 30), FC4_SDA
  @{ */
#define BOARD_INITPINS_FC4_SDA_PORT 1U                   /*!<@brief PORT peripheral base pointer */
#define BOARD_INITPINS_FC4_SDA_PIN 21U                   /*!<@brief PORT pin number */
#define BOARD_INITPINS_FC4_SDA_PIN_MASK (1U << 21U)      /*!<@brief PORT pin mask */
                                                         /* @} */

/*! @name PIO1_19 (number 58), MMA_INT
  @{ */
#define BOARD_INITPINS_MMA_INT_PORT 1U                   /*!<@brief PORT peripheral base pointer */
#define BOARD_INITPINS_MMA_INT_PIN 19U                   /*!<@brief PORT pin number */
#define BOARD_INITPINS_MMA_INT_PIN_MASK (1U << 19U)      /*!<@brief PORT pin mask */
                                                         /* @} */

/*!
 * @brief Configures pin routing and optionally pin electrical features.
 *
 */
void BOARD_InitPins(void); /* Function assigned for the Cortex-M33 (Core #0) */

#if defined(__cplusplus)
}
#endif

/*!
 * @}
 */
#endif /* _PIN_MUX_H_ */

/***********************************************************************************************************************
 * EOF
 **********************************************************************************************************************/
