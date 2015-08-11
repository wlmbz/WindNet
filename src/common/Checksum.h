#pragma once

#include <cstdint>
#include <cstddef>


/**
 * Compute the CRC-32C checksum of a buffer, using a hardware-accelerated
 * implementation if available or a portable software implementation as
 * a default.
 *
 * @note CRC-32C is different from CRC-32; CRC-32C starts with a different
 *       polynomial and thus yields different results for the same input
 *       than a traditional CRC-32.
 */
uint32_t crc32c(const void* data, size_t nbytes, uint32_t init_crc);
