#include "ResourceMonitor.h"

#include <mach/mach.h>
QString ResourceMonitor::memoryStatus() {
    vm_statistics64_data_t vmStats{};
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    const auto host = mach_host_self();
    const auto result = host_statistics64(host, HOST_VM_INFO64,
                                          reinterpret_cast<host_info64_t>(&vmStats),
                                          &count);
    vm_size_t pageSize = 0;
    host_page_size(host, &pageSize);
    mach_port_deallocate(mach_task_self(), host);
    if (result != KERN_SUCCESS) return "Memory: unavailable";
    if (pageSize == 0) return "Memory: unavailable";

    const auto freeBytes = static_cast<uint64_t>(vmStats.free_count) * pageSize;
    const auto inactiveBytes = static_cast<uint64_t>(vmStats.inactive_count) * pageSize;
    const auto availableBytes = freeBytes + inactiveBytes;
    const auto availableGb = static_cast<double>(availableBytes) / (1024.0 * 1024.0 * 1024.0);
    return QString("Memory available: %1 GB").arg(availableGb, 0, 'f', 1);
}
