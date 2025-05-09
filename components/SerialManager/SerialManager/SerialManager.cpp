#include "SerialManager.hpp"

#define ECHO_TEST_TXD (4)
#define ECHO_TEST_RXD (5)
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)

#define ECHO_UART_PORT_NUM (1)
#define ECHO_UART_BAUD_RATE (115200)
#define ECHO_TASK_STACK_SIZE (3072)

static const char *TAG = "UART TEST";

#define BUF_SIZE (1024)
uint8_t *data;

SerialManager::SerialManager(std::shared_ptr<CommandManager> commandManager) : commandManager(commandManager) {}

void SerialManager::setup()
{
  usb_serial_jtag_driver_config_t usb_serial_jtag_config;
  usb_serial_jtag_config.rx_buffer_size = BUF_SIZE;
  usb_serial_jtag_config.tx_buffer_size = BUF_SIZE;
  usb_serial_jtag_driver_install(&usb_serial_jtag_config);
  // Configure a temporary buffer for the incoming data
  data = (uint8_t *)malloc(BUF_SIZE);
}

// rewrite this to serial events or something
void SerialManager::try_receive()
{
  int len = usb_serial_jtag_read_bytes(data, (BUF_SIZE - 1), 20 / portTICK_PERIOD_MS);
  if (len)
  {
    auto result = this->commandManager->executeFromJson(std::string_view((const char *)data));
    auto resultMessage = result.getResult();
    usb_serial_jtag_write_bytes(resultMessage.c_str(), resultMessage.length(), 20 / portTICK_PERIOD_MS);
  }
}

void HandleSerialManagerTask(void *pvParameters)
{
  auto serialManager = static_cast<SerialManager *>(pvParameters);

  while (1)
  {
    serialManager->try_receive();
  }
}