
#include "Project_Conf.h"
#include "MiddSerialComm.h"
/* USER CODE BEGIN 1 */
int _write(int file, char *ptr, int len)
{
    middSerialCommSendData(EN_SERIAL_LINE_2, ptr, len, 0xFFFFFFFFU);
    return len;
}
