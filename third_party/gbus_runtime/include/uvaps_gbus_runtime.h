#ifndef  _UVAPS_GBUS_RUNTIME_H_
#define _UVAPS_GBUS_RUNTIME_H_


#include <cstdint>
#include <string>
#include <vector>

/**
 * @description:    Initialize function to init socket connection to daemon
 * @param host      The Host name of daemon
 * @return          true for success; false for failed to initialize.
 */
bool gbus_initialize(const char* host);

/**
 * @description:    Finalize function to close socket connection to daemon
 * @return          true for success; false for failed to initialize.
 */
bool gbus_finalize();

/**
 * @description:                Send msg data to target port/function.
 * @param prototypingInstance   The target Prototyping ID, range 0-N.
 * @param boardIdx              The target FPGA board ID, range 0-N.
 * @param fpgaIdx               The target DUT FPGA ID in the target FPGA board, range 0-3, which contains the target port/function.
 * @param instance              The message data will be sent to the target port/function.
 * @param offset                The shift size from gbus start address
 * @param count                 The data size of data, 1 count quals 4byte
 * @param value                 The data value to be read
 * @return                      1 for success; 0 for failed to send message data to target port/function.
 */
int gbus_read(uint8_t prototypingInstance, uint8_t boardIdx, uint8_t fpgaIdx, uint8_t instance, 
    uint64_t offset, size_t count, std::vector<uint8_t>& value);

/**
 * @description:                Send msg data to target port/function.
 * @param prototypingInstance   The target Prototyping ID, range 0-N.
 * @param boardIdx              The target FPGA board ID, range 0-N.
 * @param fpgaIdx               The target DUT FPGA ID in the target FPGA board, range 0-3, which contains the target port/function.
 * @param instance              The message data will be sent to the target port/function.
 * @param offset                The shift size from gbus start address
 * @param count                 The data size of data, 1 count quals 4byte
 * @param file                  The file path where data value will to be read to.
 * @param file_offset           The posion of file to be write start with.
 * @return                      1 for success; 0 for failed to send message data to target port/function.
 */
int gbus_read(uint8_t prototypingInstance, uint8_t boardIdx, uint8_t fpgaIdx, uint8_t instance, 
    uint64_t offset, size_t count, const std::string& file, uint64_t file_offset);


/**
 * @description:                Send msg data to target port/function.
 * @param prototypingInstance   The target Prototyping ID, range 0-N.
 * @param boardIdx              The target FPGA board ID, range 0-N.
 * @param fpgaIdx               The target DUT FPGA ID in the target FPGA board, range 0-3, which contains the target port/function.
 * @param instance              The message data will be sent to the target port/function.
 * @param offset                The shift size from gbus start address
 * @param count                 The data size of data, 1 count quals 4byte
 * @param value                 The data value to be write
 * @return                      1 for success; 0 for failed to send message data to target port/function.
 */
int gbus_write(uint8_t prototypingInstance, uint8_t boardIdx, uint8_t fpgaIdx, uint8_t instance, 
    uint64_t offset, size_t count, std::vector<uint8_t>& value);

/**
 * @description:                Send msg data to target port/function.
 * @param prototypingInstance   The target Prototyping ID, range 0-N.
 * @param boardIdx              The target FPGA board ID, range 0-N.
 * @param fpgaIdx               The target DUT FPGA ID in the target FPGA board, range 0-3, which contains the target port/function.
 * @param instance              The message data will be sent to the target port/function.
 * @param offset                The shift size from gbus start address
 * @param count                 The data size of data, 1 count quals 4byte
 * @param file                  The file path where data value will be read.
 * @param file_offset           The posion of file to be read start with.
 * @return                      1 for success; 0 for failed to send message data to target port/function.
 */
int gbus_write(uint8_t prototypingInstance, uint8_t boardIdx, uint8_t fpgaIdx, uint8_t instance, 
    uint64_t offset, size_t count, const std::string& file, uint64_t file_offset);


/**
 * @description:                Send msg data to target port/function.
 * @param prototypingInstance   The target Prototyping ID, range 0-N.
 * @param boardIdx              The target FPGA board ID, range 0-N.
 * @param fpgaIdx               The target DUT FPGA ID in the target FPGA board, range 0-3, which contains the target port/function.
 * @param instance              The message data will be sent to the target port/function.
 * @param offset                The shift size from gbus start address
 * @param size                  The size in bytes of the message data.
 * @param value                 The data value to be read
 * @return                      1 for success; 0 for failed to send message data to target port/function.
 */
int gbus_dma_read(uint8_t prototypingInstance, uint8_t boardIdx, uint8_t fpgaIdx, uint8_t instance, 
    uint64_t offset, size_t size, uint8_t channel, uint8_t port, std::vector<uint8_t>& value);

/**
 * @description:                Send msg data to target port/function.
 * @param prototypingInstance   The target Prototyping ID, range 0-N.
 * @param boardIdx              The target FPGA board ID, range 0-N.
 * @param fpgaIdx               The target DUT FPGA ID in the target FPGA board, range 0-3, which contains the target port/function.
 * @param instance              The message data will be sent to the target port/function.
 * @param offset                The shift size from gbus start address
 * @param size                  The size in bytes of the message data.
 * @param file                  The file path where data value will to be read to.
 * @param file_offset           The posion of file to be write start with.
 * @return                      1 for success; 0 for failed to send message data to target port/function.
 */
int gbus_dma_read(uint8_t prototypingInstance, uint8_t boardIdx, uint8_t fpgaIdx, uint8_t instance, 
    uint64_t offset, size_t size, uint8_t channel, uint8_t port, const std::string& file, uint64_t file_offset);

/**
 * @description:                Send msg data to target port/function.
 * @param prototypingInstance   The target Prototyping ID, range 0-N.
 * @param boardIdx              The target FPGA board ID, range 0-N.
 * @param fpgaIdx               The target DUT FPGA ID in the target FPGA board, range 0-3, which contains the target port/function.
 * @param instance              The message data will be sent to the target port/function.
 * @param offset                The shift size from gbus start address
 * @param size                  The size in bytes of the message data.
 * @param value                 The data value to be write
 * @return                      1 for success; 0 for failed to send message data to target port/function.
 */
int gbus_dma_write(uint8_t prototypingInstance, uint8_t boardIdx, uint8_t fpgaIdx, uint8_t instance, 
    uint64_t offset, size_t size, uint8_t channel, uint8_t port, std::vector<uint8_t>& value);

/**
 * @description:                Send msg data to target port/function.
 * @param prototypingInstance   The target Prototyping ID, range 0-N.
 * @param boardIdx              The target FPGA board ID, range 0-N.
 * @param fpgaIdx               The target DUT FPGA ID in the target FPGA board, range 0-3, which contains the target port/function.
 * @param instance              The message data will be sent to the target port/function.
 * @param offset                The shift size from gbus start address
 * @param size                  The size in bytes of the message data.
 * @param file                  The file path where data value will be read.
 * @param file_offset           The posion of file to be read start with.
 * @return                      1 for success; 0 for failed to send message data to target port/function.
 */
int gbus_dma_write(uint8_t prototypingInstance, uint8_t boardIdx, uint8_t fpgaIdx, uint8_t instance, 
    uint64_t offset, size_t size, uint8_t channel, uint8_t port, const std::string& file, uint64_t file_offset);

#endif /* _UVAPS_GBUS_RUNTIME_H_ */
