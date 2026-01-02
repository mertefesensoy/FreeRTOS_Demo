# FreeRTOS Demo Projects

This repository contains demonstration projects showcasing FreeRTOS features and capabilities on Windows simulator platforms.

## Overview

This repository includes two main demonstration systems:

1. **Periodic Sensor Monitoring System** - A comprehensive FreeRTOS demonstration featuring task synchronization, mutex-based resource protection, and interrupt simulation
2. **FreeRTOS+UDP with CLI** - A networking demonstration featuring UDP communication and command-line interface

## Features

### Periodic Sensor Monitoring System

- **Task Synchronization**: Demonstrates direct task notification for efficient inter-task communication
- **Mutex Protection**: Shows proper use of mutexes to protect shared resources
- **Circular Buffer**: Implements a thread-safe circular buffer for sensor data
- **Multiple Tasks**:
  - `SensorISR`: High-priority task simulating ISR timing for periodic sensor data (100ms)
  - `SensorHandler`: Deferred interrupt handler for processing sensor readings
  - `Logger`: Periodic task computing and displaying sensor statistics (1s)
  - `Heartbeat`: Low-priority background task
  - `Console`: Interactive UART-style command interface
- **Runtime Statistics**: Demonstrates FreeRTOS runtime statistics gathering
- **Interactive Console Commands**:
  - `status`: Display buffer status and statistics
  - `avg`: Calculate and display average sensor value
  - `clear`: Clear the sensor data buffer

### FreeRTOS+UDP with CLI

- UDP networking stack demonstration
- Command-line interface over UDP
- Echo client/server implementations
- Network command processing

## Prerequisites

- **Windows OS**: These demos are designed for Windows platforms
- **Visual Studio**: Required for building the projects (Visual Studio solution files included)
- **FreeRTOS**: The necessary FreeRTOS source files should be included or configured in your build environment

## Project Structure

```
.
├── main.c                                    # Sensor monitoring system main file
├── Run-time-stats-utils.c                    # Runtime statistics utilities
├── DemoTasks/                                # FreeRTOS+UDP demo tasks
│   ├── CLI-commands.c                        # Command-line interface commands
│   ├── SimpleClientAndServer.c               # Basic client/server demo
│   ├── TwoEchoClients.c                      # Echo client implementation
│   ├── UDPCommandServer.c                    # UDP command server
│   └── include/                              # Header files
├── Periodic Sensor Monitoring System (FreeRTOS Win32).sln  # VS solution
├── FreeRTOS_Plus_UDP_with_CLI.sln           # VS solution for UDP demo
└── README_FIRST.txt                          # Original documentation
```

## Building the Projects

### Periodic Sensor Monitoring System

1. Open `Periodic Sensor Monitoring System (FreeRTOS Win32).sln` in Visual Studio
2. Select your desired build configuration (Debug/Release)
3. Build the solution (Ctrl+Shift+B)
4. Run the executable

### FreeRTOS+UDP with CLI

1. Open `FreeRTOS_Plus_UDP_with_CLI.sln` in Visual Studio
2. Select your desired build configuration (Debug/Release)
3. Build the solution (Ctrl+Shift+B)
4. Run the executable

## Usage

### Running the Sensor Monitoring System

After building and running the Sensor Monitoring System, you will see:

1. **Sensor readings** printed every 100ms by the handler task
2. **Statistics** logged every second showing sample count and average
3. **Heartbeat messages** every 2 seconds indicating system is running
4. **Interactive console** accepting commands

#### Console Commands

Type commands directly in the console window:

- `status` - Display buffer capacity, current count, and head position
- `avg` - Show the current average of all sensor readings with precision
- `clear` - Reset the sensor data buffer
- Press Enter to execute the command

#### Configuration Options

Edit `main.c` to customize behavior:

```c
#define SENSOR_PERIOD_MS          100      /* Sensor interrupt frequency */
#define LOGGER_PERIOD_MS         1000      /* Statistics logging frequency */
#define HEARTBEAT_PERIOD_MS      2000      /* Heartbeat interval */
#define CONSOLE_POLL_MS            20      /* Console polling rate */
#define CBUF_CAPACITY              64      /* Circular buffer size */
#define USE_MUTEX                   1      /* Enable/disable mutex (set to 0 to test race conditions) */
```

**Note**: Setting `USE_MUTEX` to 0 demonstrates what happens without proper synchronization - you may observe data corruption or incorrect statistics.

### Running the FreeRTOS+UDP Demo

The FreeRTOS+UDP demonstration showcases networking capabilities. For detailed information about this demo, refer to:

- [Demo Documentation](http://www.FreeRTOS.org/FreeRTOS-Plus/FreeRTOS_Plus_UDP/Embedded_Ethernet_Examples/RTOS_UDP_CLI_Windows_Simulator.html)
- [FreeRTOS+UDP API Documentation](http://www.FreeRTOS.org/FreeRTOS-Plus/FreeRTOS_Plus_UDP/FreeRTOS_UDP_API_Functions.html)
- [FreeRTOS+UDP Portal](http://www.FreeRTOS.org/udp)

## Key Concepts Demonstrated

### Task Priorities

The sensor monitoring system demonstrates proper task priority assignment:

- `SensorISR` (Highest): Priority 4 - Simulates interrupt timing requirements
- `SensorHandler`: Priority 3 - Deferred interrupt processing
- `Logger`: Priority 2 - Periodic statistics
- `Heartbeat` and `Console`: Priority 1 - Background tasks

### Synchronization Mechanisms

1. **Task Notifications**: Used for efficient ISR-to-task communication (zero-copy, minimal overhead)
2. **Mutex**: Protects shared circular buffer from concurrent access
3. **Queues**: Available for UART line buffering (demonstrated in code structure)

### Design Patterns

- **Deferred Interrupt Processing**: ISR shim notifies handler task for heavy processing
- **Circular Buffer**: Efficient fixed-size buffer with O(1) average calculation
- **Periodic Tasks**: Using `vTaskDelayUntil()` for precise timing

## Learning Resources

- [FreeRTOS Official Website](https://www.freertos.org/)
- [FreeRTOS Documentation](https://www.freertos.org/Documentation/RTOS_book.html)
- [FreeRTOS API Reference](https://www.freertos.org/a00106.html)
- [FreeRTOS+UDP Documentation](http://www.FreeRTOS.org/udp)

## Notes

- These demos run on the Windows simulator, making them ideal for learning FreeRTOS concepts without hardware
- The sensor readings are pseudo-random values simulating a 12-bit ADC
- The demos include `configASSERT()` calls to catch configuration errors during development
- Runtime statistics can be viewed if enabled in FreeRTOSConfig.h

## License

Refer to the FreeRTOS license terms. FreeRTOS is distributed under the MIT open source license.

## Contributing

This is a demonstration repository. For issues or improvements related to FreeRTOS itself, please visit the [official FreeRTOS repository](https://github.com/FreeRTOS/FreeRTOS).
