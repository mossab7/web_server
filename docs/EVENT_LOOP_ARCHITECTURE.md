# Event Loop Architecture - Implementation Guide

## Overview

This document describes the best practices implemented in the event-driven web server architecture using Linux epoll.

## Architecture Components

### 1. EventLoop (src/server/EventLoop.cpp)

**Responsibilities:**
- Main event processing loop
- Signal handling integration (g_shutdown flag)
- Exception safety and error recovery
- Handler lifecycle management

**Key Features:**
- **Timeout-based polling**: Uses 1-second timeout for better signal responsiveness
- **Error-first handling**: Processes ERROR events before READ/WRITE to prevent cascading issues
- **Handler validation**: Checks if handler still exists before processing multiple events
- **Exception recovery**: Catches and logs exceptions, attempts graceful cleanup

**Best Practices Implemented:**
```cpp
// 1. Timeout for signal handling
events = epoll.wait(1000);  // 1 second timeout

// 2. Error events processed first
if (IS_ERROR_EVENT(events[i].events)) {
    handler->onError();
    continue;  // Skip other events
}

// 3. Validate handler after each event
if (fd_manager.exists(events[i].data.fd) && IS_WRITE_EVENT(...))
```

### 2. Server (src/server/Server.cpp)

**Responsibilities:**
- Listen socket management
- Accept incoming connections
- Create Client handlers
- Socket option configuration

**Key Features:**
- **SO_REUSEADDR**: Allows immediate restart after shutdown
- **Non-blocking mode**: Prevents blocking on accept()
- **Error isolation**: Server continues on accept failures
- **Level-triggered mode**: Reliable event notification (default epoll behavior)

**Best Practices Implemented:**
```cpp
// 1. Socket reuse for fast restart
setsockopt(socket.get_fd(), SOL_SOCKET, SO_REUSEADDR, ...)

// 2. Non-blocking accept
socket.set_non_blocking();

// 3. Try-catch for accept failures
try {
    Socket client_socket = socket.accept();
    // Register client...
} catch (...) {
    // Log but don't crash server
}

// 4. Edge-triggered registration
// Register client with FdManager
_fd_manager.add(client->get_fd(), client, EPOLLIN);
```

### 3. Client (src/server/Client.cpp)

**Responsibilities:**
- HTTP request parsing
- Response generation and sending
- Connection state management
- Keep-alive support

**State Machine:**
```
READING_REQUEST → PROCESSING → SENDING_RESPONSE → [KEEP_ALIVE | CLOSED]
                                                ↓
                                            ERROR_STATE
```

**Key Features:**
- **EAGAIN handling**: Properly handles non-blocking I/O
- **Partial read/write**: Continues on next event
- **Self-deletion**: Manages own lifecycle
- **Keep-alive support**: Resets state for next request

**Best Practices Implemented:**
```cpp
// 1. Handle EAGAIN/EWOULDBLOCK
if (errno == EAGAIN || errno == EWOULDBLOCK) {
    return true;  // Try again later
}

// 2. Self-deletion pattern
_fd_manager.remove(get_fd());
delete this;  // Safe because we return immediately

// 3. Level-triggered mode switching
_fd_manager.modify(this, EPOLLIN);   // Read mode
_fd_manager.modify(this, EPOLLOUT);  // Write mode

// 4. State reset for keep-alive
if (_state == KEEP_ALIVE) {
    reset();
    _fd_manager.modify(this, EPOLLIN);
}
```

### 4. FdManager (include/server/FdManager.hpp)

**Responsibilities:**
- Map file descriptors to handlers
- Epoll registration management
- Handler lifecycle tracking

**Key Features:**
- **Exists check**: Prevents use-after-free
- **Automatic cleanup**: Removes from epoll on deletion
- **Type safety**: Stores EventHandler pointers

## Memory Management

### Handler Lifetime Rules

1. **Server handlers**: Created in main(), deleted on shutdown
2. **Client handlers**: Created by Server, self-delete on close/error
3. **FdManager ownership**: Stores raw pointers but doesn't own handlers

### Self-Deletion Pattern

Clients use self-deletion for automatic cleanup:

```cpp
void Client::onError()
{
    _fd_manager.remove(get_fd());  // Remove from epoll
    delete this;                    // Safe - no code executes after
}
```

**Safety Requirements:**
- MUST return immediately after `delete this`
- MUST remove from FdManager first
- MUST NOT access member variables after deletion

## Error Handling Strategy

### Three-Level Error Handling

1. **Event Loop Level**:
   - Try-catch around all handler calls
   - Cleanup on exception
   - Log and continue

2. **Handler Level**:
   - Try-catch in onReadable/onWritable
   - Convert exceptions to ERROR_STATE
   - Trigger onError()

3. **I/O Level**:
   - Check return values
   - Distinguish EAGAIN from real errors
   - Set appropriate state

### Error Recovery Flow

```
I/O Error → Set ERROR_STATE → Next Event → onError() → Cleanup & Delete
                    ↓
            Can also build error response
```

## Signal Handling

### Graceful Shutdown

```cpp
// Global flag (volatile for signal safety)
extern volatile sig_atomic_t g_shutdown;

// Signal handler
void signal_handler(int signal) {
    g_shutdown = 1;  // Async-signal-safe
}

// Event loop
while (!g_shutdown) {
    events = epoll.wait(1000);  // Timeout ensures check
    // Process events...
}
```

**Best Practices:**
- Use `volatile sig_atomic_t` for signal safety
- Timeout in epoll_wait for responsiveness
- SIGPIPE ignored to handle broken pipes

## Level-Triggered Mode

### Why Level-Triggered?

**Advantages:**
- Simpler to implement correctly
- More forgiving - won't miss events
- Easier to debug
- Default epoll behavior
- No need to check EAGAIN/EWOULDBLOCK

**Characteristics:**
- Events repeat until handled
- Kernel notifies as long as condition is true
- Works well with non-blocking sockets
- Reliable and predictable

### Implementation Details

```cpp
// Server socket - accepts connections
_fd_manager.add(server->get_fd(), server, EPOLLIN);

// Client socket - reads/writes data
_fd_manager.add(client->get_fd(), client, EPOLLIN);

// Switch between read and write
_fd_manager.modify(client, EPOLLOUT);  // Write mode
_fd_manager.modify(client, EPOLLIN);   // Read mode
```

**Simple Read Loop:**
```cpp
ssize_t bytesRead = socket.recv(buffer, size, 0);
if (bytesRead < 0) {
    // Error occurred
    handleError();
}
if (bytesRead == 0) {
    // Connection closed
    cleanup();
}
// Process data - will be called again if more data available
```

## Performance Optimizations

### 1. Buffer Management
- Stack-allocated buffers (BUFF_SIZE = 8192)
- Single allocation per client
- No dynamic allocation in hot path

### 2. Event Batching
- Process all ready events in one epoll_wait()
- Minimize system calls
- Better cache locality

### 3. Connection Reuse
- Keep-alive support reduces overhead
- Connection pooling by HTTP/1.1
- State reset faster than new connection

### 4. Edge-Triggered Mode
- Reduces epoll_wait() calls
- Better scalability
- Lower CPU usage under load

## Threading Considerations

### Current Implementation: Single-Threaded

**Advantages:**
- No synchronization needed
- Simple debugging
- Predictable behavior
- Lower memory overhead

**Limitations:**
- One CPU core utilization
- Blocking operations affect all clients

### Future Multi-Threading Options

1. **Thread Pool for CGI/Blocking Operations**
2. **Multiple Event Loops** (one per core)
3. **Worker Threads for Processing**

## Testing Recommendations

### 1. Error Conditions
- Client disconnect mid-request
- Client disconnect mid-response
- Malformed HTTP requests
- Socket buffer full (large responses)

### 2. Load Testing
- Many simultaneous connections
- Rapid connect/disconnect cycles
- Keep-alive vs new connections
- Large file transfers

### 3. Signal Testing
- SIGINT during various states
- SIGPIPE handling
- Multiple rapid signals

### 4. Resource Limits
- File descriptor exhaustion
- Memory exhaustion
- Port exhaustion (TIME_WAIT)

## Common Pitfalls Avoided

1. ❌ **Dangling pointers**: Server objects on stack
   - ✅ Fixed: Allocate on heap, track lifetime

2. ❌ **Level-triggered starvation**: Not handling all events
   - ✅ Fixed: Process events properly in each handler

3. ❌ **Use-after-free**: Processing events after handler deleted
   - ✅ Fixed: Check exists() before subsequent events

4. ❌ **Signal race**: Infinite epoll_wait() misses shutdown
   - ✅ Fixed: Timeout in epoll_wait()

5. ❌ **Resource leaks**: Clients not cleaned up on error
   - ✅ Fixed: Self-deletion pattern with exception safety

6. ❌ **SIGPIPE crash**: Writing to closed socket
   - ✅ Fixed: Ignore SIGPIPE globally

7. ❌ **Partial writes ignored**: Not tracking send progress
   - ✅ Fixed: Handle partial sends, let LT mode retry

## Monitoring and Debugging

### Logging Strategy

- **INFO**: Normal operations (connections, requests)
- **WARNING**: Recoverable issues (parse errors, unknown events)
- **ERROR**: Serious issues (I/O errors, exceptions)

### Metrics to Track

1. Active connections count
2. Requests per second
3. Response time distribution
4. Error rate by type
5. Keep-alive reuse rate

### Debug Aids

```cpp
// Enable detailed logging
#define DEBUG_EVENTS 1

// Track fd lifecycle
logger.debug("FD " + fd + " state: " + state);

// Log epoll modifications
logger.debug("Modified FD " + fd + " events: " + events);
```

## Conclusion

This implementation follows industry best practices for high-performance network servers:

- **Robust error handling** at multiple levels
- **Proper resource management** with RAII principles
- **Edge-triggered epoll** for efficiency
- **Signal-safe shutdown** mechanism
- **Memory safety** with careful lifetime management
- **Comprehensive logging** for debugging

The architecture is production-ready for moderate loads and provides a solid foundation for future enhancements like multi-threading or HTTP/2 support.
